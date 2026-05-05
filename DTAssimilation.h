#pragma once

#include "DTObservationBuffer.h"

#include <QObject>
#include <QString>
#include <QTimer>

class DTConfig;
class System;   // forward-declared; not held as a member

// ---------------------------------------------------------------------------
// DTAssimilation
//
// Orchestrates the data-assimilation side of a forward digital twin:
//
//   - Owns the DTObservationBuffer that mirrors the Truth Twin's observation
//     stream.
//   - Drives a single QTimer that, on each tick, refreshes the buffer and
//     immediately runs a calibration cycle against the just-refreshed data.
//     There is intentionally no separate poll cadence — calibrating against
//     observations that have already been used is wasteful, so refresh and
//     calibrate are coupled.
//   - On each calibration cycle, builds a fresh System from the latest
//     forward-twin state snapshot (path pushed in by DTRunner via
//     setLatestSnapshot()), pushes buffered observations into it, runs the
//     GA inverse problem, archives the GA output, and writes an updated
//     state snapshot containing the calibrated parameters for the next
//     forward run to pick up.
//
// Lifetime: owned by DTRunner via unique_ptr, instantiated only if
// DTConfig::assimilation.enabled is true.
//
// Threading: single-threaded. All work happens on the Qt main event-loop
// thread. Calibration blocks the loop for as long as it takes — typically
// seconds to minutes per cycle. This is acceptable because calibration
// cadence is on the order of minutes, not seconds.
// ---------------------------------------------------------------------------
class DTAssimilation : public QObject
{
    Q_OBJECT

public:
    explicit DTAssimilation(const DTConfig &config, QObject *parent = nullptr);

    // Configure endpoints and start the calibration timer. Call once after
    // construction (in DTRunner::init()). Returns false if essential config
    // is missing.
    bool start(QString &errorMessage);

    // Stop the calibration timer. Idempotent.
    void stop();

    // Push the path to the most recent forward-twin state snapshot. Called
    // by DTRunner after each Advance stage completes successfully. The path
    // is the input to the next calibration cycle's System load. Empty path
    // means "no snapshot yet" — calibration will skip.
    void setLatestSnapshot(const QString &path) { m_latestSnapshotPath = path; }

    // Read-only access to the buffer.
    const DTObservationBuffer &buffer() const { return m_buffer; }

    // Force a refresh+calibrate cycle outside the normal cadence (useful
    // for testing). Returns true on success.
    bool refreshAndCalibrateNow(QString &errorMessage);

    void setLatestSnapshot(const QString &path) { m_latestSnapshotPath = path; }


signals:
    // Emitted after each successful buffer refresh.
    void buffered(qint64 totalPoints);

    // Emitted on buffer-refresh failure. The cycle is then skipped (no
    // calibration on stale data).
    void pollFailed(QString errorMessage);

    // Emitted after a successful calibration cycle. Carries the path to
    // the new state snapshot containing the calibrated parameters.
    void calibrationCompleted(QString newSnapshotPath);

    // Emitted when calibration fails for any reason (no snapshot to load,
    // System verification errors, GA failure, etc.). The forward run is
    // not affected; the next cycle will try again.
    void calibrationFailed(QString errorMessage);

private slots:
    void onCycleTick();

private:
    // The full refresh-and-calibrate cycle, broken out so it can be invoked
    // both from the timer slot and from refreshAndCalibrateNow().
    bool doCycle(QString &errorMessage);

    // The calibration step, assuming the buffer has already been refreshed.
    // Loads System from latest snapshot, pushes observations, runs GA,
    // archives output, and writes a calibrated snapshot.
    bool runCalibration(QString &errorMessage);

    // Append the contents of ga_output.txt to ga_output_merged.txt with a
    // cycle-delimiter header line. Idempotent against missing input file
    // (logs and continues).
    bool archiveGAOutput(int cycleIndex);

    const DTConfig       &m_config;
    DTObservationBuffer   m_buffer;
    QTimer                m_cycleTimer;
    QString               m_latestSnapshotPath;   // pushed by DTRunner
    int                   m_cyclesCompleted = 0;
    bool                  m_started = false;
    int m_cyclesCompleted = 0;
    bool runCalibration(QString &errorMessage);
    bool archiveGAOutput(int cycleIndex);
    QString m_latestSnapshotPath;
};
