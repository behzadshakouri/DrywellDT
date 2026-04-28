#include "DTConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cctype>
#include <iostream>

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------
static QString requireString(const QJsonObject &obj, const char *key, bool &ok)
{
    if (!obj.contains(key) || !obj[key].isString())
    {
        ok = false;
        return {};
    }
    return obj[key].toString();
}

// Expand ${var} tokens using the selected machine profile.
// Example:
//   ${project_root}/outputs  ->  /mnt/3rd900/Projects/DrywellDT/outputs
static QString expandVars(QString s, const QJsonObject &vars)
{
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it)
        s.replace("${" + it.key() + "}", it.value().toString());
    return s;
}

// Return true if a path-like string still contains an unresolved ${...} token.
static bool hasUnresolvedVar(const QString &s)
{
    return s.contains("${") && s.contains("}");
}

// Machine name compiled from qmake DEFINES.
// If config.json leaves "machine" empty, this value is used automatically.
static QString compiledMachineName()
{
#ifdef PowerEdge
    return "PowerEdge";
#elif defined(Jason)
    return "Jason";
#elif defined(Behzad)
    return "Behzad";
#elif defined(Arash)
    return "Arash";
#elif defined(SligoCreek)
    return "SligoCreek";
#elif defined(WSL)
    return "WSL";
#else
    return {};
#endif
}

// ---------------------------------------------------------------------------
// DTConfig::parseIntervalMs
// Accepts: <number><unit>, where unit is one of: s, min, hr, day
// Examples: 300s, 15min, 4hr, 1day, 0.5day
// ---------------------------------------------------------------------------
qint64 DTConfig::parseIntervalMs(const std::string &s, QString &err)
{
    const QString raw = QString::fromStdString(s).trimmed();
    const std::string trimmed = raw.toStdString();

    if (trimmed.empty())
    {
        err = "interval string is empty";
        return -1;
    }

    size_t i = 0;
    while (i < trimmed.size() &&
           (std::isdigit(static_cast<unsigned char>(trimmed[i])) || trimmed[i] == '.'))
        ++i;

    if (i == 0)
    {
        err = QString("interval '%1' has no leading number").arg(raw);
        return -1;
    }

    double value = 0.0;
    try
    {
        value = std::stod(trimmed.substr(0, i));
    }
    catch (...)
    {
        err = QString("invalid numeric value in interval '%1'").arg(raw);
        return -1;
    }

    const std::string unit = trimmed.substr(i);

    qint64 multiplierMs = 0;
    if      (unit == "s")   multiplierMs = 1000LL;
    else if (unit == "min") multiplierMs = 60LL   * 1000LL;
    else if (unit == "hr")  multiplierMs = 3600LL * 1000LL;
    else if (unit == "day") multiplierMs = 86400LL * 1000LL;
    else
    {
        err = QString("unknown interval unit '%1' (use s, min, hr, day)")
              .arg(QString::fromStdString(unit));
        return -1;
    }

    const qint64 result = static_cast<qint64>(value * static_cast<double>(multiplierMs));
    if (result <= 0)
    {
        err = QString("interval must be > 0, got '%1'").arg(raw);
        return -1;
    }

    return result;
}

// ---------------------------------------------------------------------------
// DTConfig::load
// ---------------------------------------------------------------------------
bool DTConfig::load(QString &errorMessage)
{
    const QString configPath =
        QCoreApplication::applicationDirPath() + "/config.json";

    QFile file(configPath);
    if (!file.exists())
    {
        errorMessage = "config.json not found at: " + configPath;
        return false;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        errorMessage = "Cannot open config.json: " + configPath;
        return false;
    }

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseErr);
    if (doc.isNull())
    {
        errorMessage = "config.json parse error: " + parseErr.errorString();
        return false;
    }
    if (!doc.isObject())
    {
        errorMessage = "config.json root must be a JSON object";
        return false;
    }

    raw = doc.object();
    stateVarExports.clear();

    // -------------------------------------------------------------------
    // Machine-based variable resolution
    // -------------------------------------------------------------------
    // Priority:
    //   1. config.json "machine" value, if non-empty
    //   2. compiled qmake define, e.g. DEFINES += PowerEdge
    QString machine = raw.value("machine").toString().trimmed();
    if (machine.isEmpty())
        machine = compiledMachineName();

    const QJsonObject machines = raw.value("machines").toObject();
    const QJsonObject machineVars = machines.value(machine).toObject();

    if (!machine.isEmpty() && machineVars.isEmpty())
    {
        errorMessage = "config.json machine profile not found: " + machine;
        return false;
    }

    bool ok = true;

    // --- required fields with ${...} expansion ---
    const QString scriptFileQ = expandVars(requireString(raw, "script_file", ok), machineVars);
    const QString stateDirQ   = expandVars(requireString(raw, "state_dir", ok), machineVars);
    const QString outputDirQ  = expandVars(requireString(raw, "output_dir", ok), machineVars);
    const QString snapDirQ    = expandVars(requireString(raw, "model_snapshot_dir", ok), machineVars);

    if (!ok)
    {
        errorMessage = "config.json is missing one or more required string fields: "
                       "script_file, state_dir, output_dir, model_snapshot_dir";
        return false;
    }

    if (hasUnresolvedVar(scriptFileQ) || hasUnresolvedVar(stateDirQ) ||
        hasUnresolvedVar(outputDirQ)  || hasUnresolvedVar(snapDirQ))
    {
        errorMessage = "config.json contains unresolved path variable(s). "
                       "Check machine/machines/project_root.";
        return false;
    }

    scriptFile       = scriptFileQ.toStdString();
    stateDir         = stateDirQ.toStdString();
    outputDir        = outputDirQ.toStdString();
    modelSnapshotDir = snapDirQ.toStdString();

    // --- optional path fields with ${...} expansion ---
    loadModelJson = expandVars(raw.value("load_model_json").toString(), machineVars).toStdString();
    weatherFile   = expandVars(raw.value("weather_file").toString(), machineVars).toStdString();

    // Model-specific visualization spec. This lets VN/R/HQ/Drywell each use
    // a different viz JSON without renaming files to viz.json or recompiling.
    QString vizFileQ = expandVars(raw.value("viz_file").toString(), machineVars).trimmed();
    if (vizFileQ.isEmpty())
        vizFileQ = QCoreApplication::applicationDirPath() + "/viz.json"; // legacy fallback

    if (hasUnresolvedVar(vizFileQ))
    {
        errorMessage = "config.json viz_file contains unresolved path variable(s). "
                       "Check machine/machines/project_root.";
        return false;
    }

    vizFile = vizFileQ.toStdString();

    // --- weather ---
    noaaOffice    = raw.value("noaa_office").toString("LWX").toStdString();
    noaaGridX     = raw.value("noaa_grid_x").toInt(0);
    noaaGridY     = raw.value("noaa_grid_y").toInt(0);
    weatherSource = raw.value("weather_source").toString("openmeteo").trimmed().toStdString();
    latitude      = raw.value("latitude").toDouble(0.0);
    longitude     = raw.value("longitude").toDouble(0.0);

    // --- timing ---
    startDatetime      = raw.value("start_datetime").toString().trimmed().toStdString();
    intervalStr        = raw.value("interval").toString("1day").trimmed().toStdString();
    forecastHorizonStr = raw.value("forecast_horizon").toString().trimmed().toStdString();

    QString intervalErr;
    intervalMs = parseIntervalMs(intervalStr, intervalErr);
    if (intervalMs < 0)
    {
        errorMessage = "config.json interval error: " + intervalErr;
        return false;
    }

    if (!forecastHorizonStr.empty())
    {
        QString horizonErr;
        forecastHorizonMs = parseIntervalMs(forecastHorizonStr, horizonErr);
        if (forecastHorizonMs < 0)
        {
            errorMessage = "config.json forecast_horizon error: " + horizonErr;
            return false;
        }
    }
    else
    {
        forecastHorizonMs = 0;
    }

    // --- state_variables array ---
    if (raw.contains("state_variables") && raw["state_variables"].isArray())
    {
        const QJsonArray arr = raw["state_variables"].toArray();
        for (const auto &entry : arr)
        {
            if (!entry.isObject()) continue;

            const QJsonObject obj = entry.toObject();
            StateVarExport exp;
            exp.variable = obj.value("variable").toString().toStdString();

            const QString expandedOutputPath =
                expandVars(obj.value("output_path").toString(), machineVars);

            if (hasUnresolvedVar(expandedOutputPath))
            {
                errorMessage = "config.json state_variables output_path contains "
                               "unresolved path variable(s).";
                return false;
            }

            exp.outputPath = expandedOutputPath.toStdString();

            if (!exp.variable.empty() && !exp.outputPath.empty())
                stateVarExports.push_back(exp);
        }
    }

    // --- ensure directories exist ---
    for (const auto &dir : { stateDir, outputDir, modelSnapshotDir })
    {
        if (!dir.empty())
            QDir().mkpath(QString::fromStdString(dir));
    }

    // --- logging ---
    std::cout << "[Config] config.json       : " << configPath.toStdString() << "\n"
              << "[Config] machine           : " << machine.toStdString() << "\n"
              << "[Config] script_file       : " << scriptFile << "\n";

    if (!loadModelJson.empty())
        std::cout << "[Config] load_model_json   : " << loadModelJson << "\n";

    std::cout << "[Config] state_dir         : " << stateDir << "\n"
              << "[Config] output_dir        : " << outputDir << "\n"
              << "[Config] model_snapshot_dir: " << modelSnapshotDir << "\n"
              << "[Config] viz_file          : " << vizFile << "\n"
              << "[Config] weather_source    : " << weatherSource << "\n"
              << "[Config] latitude          : " << latitude << "\n"
              << "[Config] longitude         : " << longitude << "\n"
              << "[Config] noaa_office       : " << noaaOffice << "\n"
              << "[Config] noaa_grid_x       : " << noaaGridX << "\n"
              << "[Config] noaa_grid_y       : " << noaaGridY << "\n"
              << "[Config] interval          : " << intervalStr
              << " (" << intervalMs << " ms)\n";

    if (forecastHorizonMs > 0)
        std::cout << "[Config] forecast_horizon  : " << forecastHorizonStr
                  << " (" << forecastHorizonMs << " ms)\n";
    else
        std::cout << "[Config] forecast_horizon  : disabled\n";

    if (!startDatetime.empty())
        std::cout << "[Config] start_datetime    : " << startDatetime << "\n";

    return true;
}
