/*
 * OpenHydroTwin
 * Copyright (C) 2026  Arash Massoudieh
 *
 * This file is part of OpenHydroTwin.
 *
 * OpenHydroTwin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenHydroTwin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "DTAssimilation.h"
#include "DTConfig.h"
#include "System.h"
#include "GA.h"
#include "Object.h"
#include "ErrorHandler.h"
#include "observation.h"
#include "Quan.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>
#include "DTWeather.h"
#include <QDateTime>
#include <iostream>

DTAssimilation::DTAssimilation(const DTConfig &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
{
    m_pollTimer.setSingleShot(false);
    connect(&m_pollTimer, &QTimer::timeout,
            this,         &DTAssimilation::onPollTick);
}

bool DTAssimilation::start(QString &errorMessage)
{
    if (!m_config.assimilation.enabled)
    {
        errorMessage = "DTAssimilation::start(): assimilation block is "
                       "disabled in config (this is a programmer error — "
                       "DTRunner shouldn't construct DTAssimilation when "
                       "the block is absent).";
        return false;
    }
    if (m_config.assimilation.truthCsvUrl.empty())
    {
        errorMessage = "DTAssimilation::start(): truth_csv_url is empty";
        return false;
    }
    if (m_config.assimilation.pollIntervalMs <= 0)
    {
        errorMessage = "DTAssimilation::start(): poll_interval must be > 0";
        return false;
    }

    m_buffer.setEndpoints(
        QString::fromStdString(m_config.assimilation.truthCsvUrl),
        QString::fromStdString(m_config.assimilation.truthMetaUrl));

    // Kick off an immediate first refresh so the buffer is populated
    // before the first timer tick. Failure is logged but not fatal —
    // the timer keeps polling and may succeed on a later tick.
    if (!m_buffer.refresh())
    {
        std::cerr << "[Assim] initial refresh failed: "
                  << m_buffer.lastError().toStdString()
                  << " (will retry on poll timer)\n";
    }
    else
    {
        std::cout << "[Assim] initial refresh OK — "
                  << m_buffer.pointCount() << " points across "
                  << m_buffer.variableCount() << " variables\n";
    }

    // Apply time_acceleration the same way the forward loop does, so that
    // poll_interval is interpreted in simulated time. With acceleration=1
    // this is plain wall-clock; with acceleration>1 the calibration timer
    // fires proportionally faster.
    const double wallClockIntervalMsD =
        static_cast<double>(m_config.assimilation.pollIntervalMs)
        / m_config.timeAcceleration;
    const qint64 wallClockIntervalMs =
        static_cast<qint64>(std::max(1.0, wallClockIntervalMsD));
    const qint64 safeIntervalMs =
        std::min(wallClockIntervalMs, static_cast<qint64>(INT_MAX));

    m_pollTimer.start(static_cast<int>(safeIntervalMs));
    m_started = true;

    std::cout << "[Assim] poll timer started — "
              << m_config.assimilation.pollIntervalMs << " ms simulated, "
              << safeIntervalMs << " ms wall-clock"
              << " (acceleration " << m_config.timeAcceleration << "x)\n";
    return true;

}

void DTAssimilation::stop()
{
    if (m_pollTimer.isActive()) m_pollTimer.stop();
    m_started = false;
}

bool DTAssimilation::refreshNow()
{
    return m_buffer.refresh();
}

void DTAssimilation::onPollTick()
{
    if (!m_buffer.refresh())
    {
        std::cerr << "[Assim] poll failed: "
                  << m_buffer.lastError().toStdString() << "\n";
        emit pollFailed(m_buffer.lastError());
        return;   // skip calibration on stale data
    }

    std::cout << "[Assim] poll OK — " << m_buffer.pointCount()
              << " points buffered, starting calibration\n";
    emit buffered(static_cast<qint64>(m_buffer.pointCount()));

    QString calErr;
    if (!runCalibration(calErr))
    {
        std::cerr << "[Assim] calibration failed: "
                  << calErr.toStdString() << "\n";
        emit calibrationFailed(calErr);
        return;
    }
}

// ---------------------------------------------------------------------------
// runCalibration
// Loads a System from the latest snapshot, pushes buffered observations into
// matching Observations, runs the GA inverse solver, and writes a calibrated
// snapshot. Mirrors the structure of MainWindow::oninverserun() in OHQ.
// ---------------------------------------------------------------------------
bool DTAssimilation::runCalibration(QString &errorMessage)
{
    const QDateTime calStart = QDateTime::currentDateTimeUtc();
    std::cout << "[Assim] [" << calStart.toString(Qt::ISODate).toStdString() << "] "
              << "calibration cycle " << (m_cyclesCompleted + 1) << " starting\n";

    // Guard: need a snapshot to calibrate against.
    if (m_latestSnapshotPath.isEmpty())
    {
        errorMessage = "no forward snapshot available yet — calibration skipped";
        return false;
    }
    if (!QFileInfo::exists(m_latestSnapshotPath))
    {
        errorMessage = "snapshot path does not exist: " + m_latestSnapshotPath;
        return false;
    }

    std::cout << "[Assim] loading snapshot: "
              << m_latestSnapshotPath.toStdString() << "\n";

    // 1. Load System from the latest snapshot.
    System sys;

    // Settings template must be loaded before LoadfromJson, otherwise the
    // Settings vector is empty and named Settings objects (Optimizer, MCMC,
    // Solver Settings, General Settings) won't be available via sys.object().
    // Mirrors what DTRunner does on the forward simulation path.
    const std::string defaultTemplatePath =
        QCoreApplication::applicationDirPath().toStdString() + "/../../resources/";
    const std::string settingsFile = defaultTemplatePath + "settings.json";
    sys.SetDefaultTemplatePath(defaultTemplatePath);
    if (!sys.ReadSystemSettingsTemplate(settingsFile))
    {
        errorMessage = "failed to load settings template from " + QString::fromStdString(settingsFile);
        return false;
    }

    if (!sys.LoadfromJson(m_latestSnapshotPath))
    {
        errorMessage = "System::LoadfromJson failed for " + m_latestSnapshotPath;
        return false;
    }

    // 2. Pre-flight checks (mirrors MainWindow::oninverserun).
    if (sys.ParametersCount() == 0)
    {
        errorMessage = "no Parameters defined in model — calibration skipped";
        return false;
    }
    ErrorHandler errs = sys.VerifyAllQuantities();
    if (errs.Count() != 0)
    {
        errorMessage = "model has verification errors";
        return false;
    }

    // 3. Push buffered observations into matching System Observations.
    int matched = 0;
    for (unsigned int i = 0; i < sys.ObservationsCount(); ++i)
    {
        Observation *obs = sys.observation(i);
        const std::string name = obs->GetName();
        TimeSeries<double> series = m_buffer.series(name);
        if (series.size() == 0) continue;
        obs->Variable("observed_data")->SetTimeSeries(series);
        ++matched;
    }
    if (matched == 0)
    {
        errorMessage = "no buffered observations matched any System Observation by name";
        return false;
    }
    std::cout << "[Assim] pushed observed_data into " << matched
              << " observation(s)\n";

    // 4. Standard inverse-run prep (mirrors oninverserun).
    // ===== ADD THIS BLOCK HERE =====
    // Adjust the simulation window to span all buffered observations,
    // so the GA evaluates each candidate over the full data window.
    // The temporal kernel handles down-weighting of older observations.
    if (m_buffer.pointCount() == 0)
    {
        errorMessage = "buffer is empty — nothing to calibrate against";
        return false;
    }
    const double tStart = m_buffer.tMin();
    const double tEnd   = m_buffer.tMax();

    // Read snapshot's window *before* we override it, for diagnostic
    double snapshotStart = -1.0, snapshotEnd = -1.0;
    if (Object *gs = sys.object("General Settings"))
    {
        try {
            snapshotStart = std::stod(gs->Variable("simulation_start_time")->GetProperty());
            snapshotEnd   = std::stod(gs->Variable("simulation_end_time")->GetProperty());
        } catch (...) {}
    }

    Object *generalSettings = sys.object("General Settings");
    if (!generalSettings)
    {
        errorMessage = "no 'General Settings' object found in System";
        return false;
    }
    generalSettings->Variable("simulation_start_time")
        ->SetProperty(std::to_string(tStart));
    generalSettings->Variable("simulation_end_time")
        ->SetProperty(std::to_string(tEnd));

    // Belt-and-suspenders: also set on System directly. SetSystemSettings
    // will then propagate from the (now updated) Settings vector.
    sys.SetProp("simulation_start_time", tStart);
    sys.SetProp("simulation_end_time",   tEnd);
    sys.SetSystemSettings();
    sys.SetSilent(true);
    // Inject precipitation covering the calibration window so the GA's
    // forward solves see the same forcing the truth twin saw.
    {
        // Convert OHQ day-serial to QDateTime.
        // ⚠ NEEDS YOUR EXISTING HELPER — see note below
        const QDateTime windowStart =
            QDateTime::fromMSecsSinceEpoch(
                static_cast<qint64>((tStart - 25569.0) * 86400000.0),
                Qt::UTC);
        const QDateTime windowEnd =
            QDateTime::fromMSecsSinceEpoch(
                static_cast<qint64>((tEnd - 25569.0) * 86400000.0),
                Qt::UTC);

        std::cout << "[Assim] fetching precipitation for "
                  << windowStart.toString(Qt::ISODate).toStdString()
                  << " → "
                  << windowEnd.toString(Qt::ISODate).toStdString() << "\n";

        CPrecipitation precip = DTWeather::fetchPrecipitation(
            m_config.weatherSource,
            m_config.latitude,
            m_config.longitude,
            windowStart, windowEnd);

        DTWeather::injectPrecipitation(&sys, precip);
    }

    std::cout << "[Assim] ===== Calibration window =====\n"
              << "[Assim]   buffer span:        " << tStart << " → " << tEnd
              << "  (" << (tEnd - tStart) << " days, "
              << m_buffer.pointCount() << " buffered points across "
              << m_buffer.variableCount() << " series)\n"
              << "[Assim]   snapshot was:       " << snapshotStart
              << " → " << snapshotEnd
              << "  (" << (snapshotEnd - snapshotStart) << " days)\n"
              << "[Assim]   GA window now:      " << tStart << " → " << tEnd
              << "  (" << (tEnd - tStart) << " days)\n";

    // Verify the override stuck after SetSystemSettings()
    if (Object *gs = sys.object("General Settings"))
    {
        try {
            const double finalStart = std::stod(gs->Variable("simulation_start_time")->GetProperty());
            const double finalEnd   = std::stod(gs->Variable("simulation_end_time")->GetProperty());
            std::cout << "[Assim]   verified post-set:  "
                      << finalStart << " → " << finalEnd << "\n";
            if (std::abs(finalStart - tStart) > 1e-6 ||
                std::abs(finalEnd   - tEnd  ) > 1e-6)
                std::cerr << "[Assim] WARNING: simulation window was overwritten\n";
        } catch (...) {}
    }
    std::cout << "[Assim] ==============================\n";


    // 5. GA setup.
    CGA<System> ga(&sys);
    Object *settings = sys.object("Optimizer");
    if (!settings)
    {
        errorMessage = "no 'Optimizer' object found in System "
                       "(this should not happen — Settings template loaded successfully)";
        return false;
    }
    ga.SetParameters(settings);

    sys.SetSystemSettings();



    std::cout << "[Assim] simulation window for GA: "
              << tStart << " → " << tEnd
              << " (" << (tEnd - tStart) << " days, "
              << m_buffer.pointCount() << " buffered points)\n";

    std::cout << "[Assim] GA configured from Settings 'Optimizer' ("
              << settings->GetVars()->size() << " quans)\n";

    const QString calibDir =
        QString::fromStdString(m_config.assimilation.calibrationOutputDir);
    QDir().mkpath(calibDir);
    ga.filenames.pathname       = calibDir.toStdString() + "/";
    ga.filenames.outputfilename = "ga_output.txt";

    // 6. Warm-start from previous cycle's terminal population, if available.
    const QString prevOutput = calibDir + "/ga_output.txt";
    if (m_cyclesCompleted > 0 && QFileInfo::exists(prevOutput))
    {
        ga.filenames.getfromfilename = prevOutput.toStdString();
        ga.getinifromoutput(prevOutput.toStdString());
        ga.getinitialpop(prevOutput.toStdString());
        std::cout << "[Assim] warm-starting from "
                  << prevOutput.toStdString() << "\n";
    }
    else
    {
        std::cout << "[Assim] cold-start GA (first calibration cycle)\n";
    }

    sys.SetParameterEstimationMode(parameter_estimation_options::inverse_model);

    // 7. Run.
    std::cout << "[Assim] running GA...\n";
    ga.optimize();

    sys.SetParameterEstimationMode();   // reset to default

    // 8. Transfer results onto sys (for snapshot writing).
    sys.TransferResultsFrom(&ga.Model_out);
    sys.Parameters() = ga.Model_out.Parameters();
    sys.SetOutputItems();

    // 9. Archive GA output to the merged file.
    if (!archiveGAOutput(m_cyclesCompleted))
    {
        std::cerr << "[Assim] failed to archive GA output for cycle "
                  << m_cyclesCompleted << "\n";
    }

    // 10. Write a new state snapshot reflecting the calibrated parameters.
    const QString stamp = QDateTime::currentDateTimeUtc()
                              .toString("yyyyMMdd_HHmmss");
    const QString newSnapshotPath = calibDir
                                    + "/state_calibrated_" + stamp + ".json";
    if (!sys.SavetoJson(newSnapshotPath.toStdString(), {}, true, true))
    {
        errorMessage = "failed to write calibrated snapshot: " + newSnapshotPath;
        return false;
    }

    ++m_cyclesCompleted;
    std::cout << "[Assim] calibration cycle " << m_cyclesCompleted
              << " completed: " << newSnapshotPath.toStdString() << "\n";

    const QDateTime calEnd = QDateTime::currentDateTimeUtc();
    const qint64 elapsedMs = calStart.msecsTo(calEnd);
    std::cout << "[Assim] [" << calEnd.toString(Qt::ISODate).toStdString() << "] "
              << "calibration cycle " << m_cyclesCompleted
              << " finished in " << (elapsedMs / 1000.0) << " sec\n";

    writeParameterLog(sys, m_cyclesCompleted);

    emit calibrationCompleted(newSnapshotPath);

    return true;
}

// ---------------------------------------------------------------------------
// archiveGAOutput
// Append ga_output.txt to ga_output_merged.txt with a cycle-delimiter header.
// ---------------------------------------------------------------------------
bool DTAssimilation::archiveGAOutput(int cycleIndex)
{
    const QString calibDir =
        QString::fromStdString(m_config.assimilation.calibrationOutputDir);
    const QString srcPath  = calibDir + "/ga_output.txt";
    const QString destPath = calibDir + "/ga_output_merged.txt";

    QFile src(srcPath);
    if (!src.exists()) return false;
    if (!src.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QFile dest(destPath);
    if (!dest.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return false;

    QTextStream out(&dest);
    const QString stamp = QDateTime::currentDateTimeUtc()
                              .toString("yyyy-MM-dd HH:mm:ss");
    const QString tNow  = (m_buffer.pointCount() > 0)
                             ? QString::number(m_buffer.tMax(), 'f', 6)
                             : QStringLiteral("n/a");

    out << "=== Cycle " << cycleIndex
        << " | timestamp " << stamp
        << " | t_now=" << tNow
        << " ===\n";
    out << src.readAll();
    out << "\n";
    return true;
}

// ---------------------------------------------------------------------------
// writeParameterLog
// Append a row to outputs/calibration/parameter_history.csv recording the
// best parameter values found in this calibration cycle. Header is written
// on the first cycle (when the file doesn't exist yet).
// ---------------------------------------------------------------------------
bool DTAssimilation::writeParameterLog(const System &sys, int cycleIndex)
{
    const QString calibDir =
        QString::fromStdString(m_config.assimilation.calibrationOutputDir);
    const QString filePath = calibDir + "/parameter_history.csv";

    const bool fileExists = QFileInfo::exists(filePath);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        std::cerr << "[Assim] failed to open " << filePath.toStdString()
        << " for parameter log\n";
        return false;
    }

    QTextStream out(&file);
    const QString stamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    const double tMax = (m_buffer.pointCount() > 0) ? m_buffer.tMax() : 0.0;

    auto &params = const_cast<System &>(sys).Parameters();
    const int nParams = static_cast<int>(params.size());

    if (!fileExists)
    {
        out << "cycle,timestamp,t_now";
        for (int i = 0; i < nParams; ++i)
            out << "," << QString::fromStdString(params[i]->GetName());
        out << "\n";
    }

    out << cycleIndex << "," << stamp << "," << QString::number(tMax, 'f', 6);
    for (int i = 0; i < nParams; ++i)
    {
        const std::string val = params[i]->Variable("value")->GetProperty();
        out << "," << QString::fromStdString(val);
    }
    out << "\n";
    file.close();

    std::cout << "[Assim] parameter log updated: cycle " << cycleIndex
              << " → " << filePath.toStdString() << "\n";
    return true;
}
