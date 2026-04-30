#pragma once

#include "DTObservationBuffer.h"

#include <QObject>
#include <QString>
#include <QTimer>

class DTConfig;

// ---------------------------------------------------------------------------
// DTAssimilation
//
// Orchestrates the data-assimilation side of a forward digital twin:
//
//   - Owns the DTObservationBuffer that mirrors the Truth Twin's
//     observation stream.
//   - Drives an independent QTimer that periodically refresh()es the
//     buffer at the configured poll cadence.
//   - (Future) Owns a DTCalibrator and triggers calibration cycles
//     against the buffer's contents.
//
// Lifetime: owned by DTRunner, instantiated only if
// DTConfig::assimilation.enabled is true.
//
// The class is a QObject so the timer slot connection is straightforward
// and so the (future) calibrator can emit signals back to the runner.
// ---------------------------------------------------------------------------
class DTAssimilation : public QObject
{
    Q_OBJECT

public:
    explicit DTAssimilation(const DTConfig &config, QObject *parent = nullptr);

    // Configure endpoints and start the polling timer. Call this once
    // after construction (in DTRunner::init() once basic setup is done).
    // Returns false if essential config is missing.
    bool start(QString &errorMessage);

    // Stop the polling timer. Idempotent.
    void stop();

    // Read-only access to the buffer for callers that need observations
    // (e.g. a future calibration trigger).
    const DTObservationBuffer &buffer() const { return m_buffer; }

    // Force a refresh outside the normal polling cadence (e.g. just
    // before kicking off a calibration cycle). Returns the buffer's
    // refresh() result.
    bool refreshNow();

signals:
    // Emitted after each successful poll. Carries the number of points
    // currently held in the buffer for quick logging/diagnostics.
    void buffered(qint64 totalPoints);

    // Emitted after a polling failure (network/parse). Receivers may
    // log; the timer keeps running.
    void pollFailed(QString errorMessage);

private slots:
    void onPollTick();

private:
    const DTConfig       &m_config;
    DTObservationBuffer   m_buffer;
    QTimer                m_pollTimer;
    bool                  m_started = false;
};
