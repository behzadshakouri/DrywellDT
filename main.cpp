/*
 * OpenHydroQual Digital Twin — generic OHQ model runner
 *
 * main.cpp is intentionally thin:
 *   - Ensure config.json is available next to the binary
 *   - Load config
 *   - Initialise runner
 *   - Fire an immediate first run, then arm QTimer for subsequent intervals
 *   - (Future) Start Crow HTTP API
 */

#include "DTConfig.h"
#include "DTRunner.h"
#include "RuntimeFiles.h"
#include "System.h"

#include <QCoreApplication>
#include <QDir>
#include <QStringList>
#include <QTimer>

#include <algorithm>
#include <climits>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // ------------------------------------------------------------------
    // 1. Ensure config.json is available next to the binary
    // ------------------------------------------------------------------
    // With DESTDIR = build-qmake-<host>/bin, ../../ points back to the
    // project root. The runtime/config folders are optional extra locations.
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString projectRoot = QDir(appDir).absoluteFilePath("../../");

    QStringList runtimeSearchDirs;
    runtimeSearchDirs << appDir;
    runtimeSearchDirs << projectRoot;
    runtimeSearchDirs << QDir(projectRoot).absoluteFilePath("runtime");
    runtimeSearchDirs << QDir(projectRoot).absoluteFilePath("config");

    if (!ensureRuntimeFile("config.json", runtimeSearchDirs))
        return 1;

    // Visualization files are now model-specific and selected by config.json
    // through "viz_file" (e.g. ${project_root}/viz_drywell.json). The legacy
    // appDir/viz.json fallback still exists inside DTConfig for older configs.

    // ------------------------------------------------------------------
    // 2. Load config.json from next to the binary
    // ------------------------------------------------------------------
    DTConfig config;
    QString configError;
    if (!config.load(configError))
    {
        std::cerr << "[Main] Failed to load config: "
                  << configError.toStdString() << "\n";
        return 1;
    }

    // ------------------------------------------------------------------
    // 3. Initialise runner
    // ------------------------------------------------------------------
    DTRunner runner(config);
    QString initError;
    if (!runner.init(initError))
    {
        std::cerr << "[Main] Runner init failed: "
                  << initError.toStdString() << "\n";
        return 2;
    }

    // ------------------------------------------------------------------
    // 4. Run the first interval immediately at start-up
    // ------------------------------------------------------------------
    // Use a single-shot zero-delay timer so the event loop is running before
    // the first solve begins. This keeps the door open for signals/slots and
    // the future Crow API to be wired in before any blocking work.
    QTimer::singleShot(0, &runner, [&runner]() {
        if (!runner.runOnce())
        {
            std::cerr << "[Main] Initial run failed.\n";
            QCoreApplication::exit(3);
        }
    });

    // ------------------------------------------------------------------
    // 5. Arm the recurring timer for subsequent intervals
    // ------------------------------------------------------------------
    QTimer intervalTimer;

    const qint64 safeIntervalMs =
        std::min(config.intervalMs, static_cast<qint64>(INT_MAX));

    intervalTimer.setInterval(static_cast<int>(safeIntervalMs));

    QObject::connect(&intervalTimer, &QTimer::timeout,
                     &runner, [&runner]() {
                         if (!runner.runOnce())
                         {
                             std::cerr << "[Main] Periodic run failed. Continuing to next interval.\n";
                             // Non-fatal: log and wait for the next tick.
                         }
                     });

    intervalTimer.start();

    std::cout << "[Main] OHQ Digital Twin running. Interval: "
              << config.intervalStr << "  (" << config.intervalMs << " ms)\n"
              << "[Main] Press Ctrl+C to stop.\n";

    // ------------------------------------------------------------------
    // 6. TODO: wire in Crow HTTP API here before app.exec()
    // ------------------------------------------------------------------

    return app.exec();
}
