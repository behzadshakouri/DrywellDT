#include "DTAssimilation.h"
#include "DTConfig.h"

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

    m_pollTimer.start(static_cast<int>(m_config.assimilation.pollIntervalMs));
    m_started = true;

    std::cout << "[Assim] poll timer started — interval "
              << m_config.assimilation.pollIntervalMs << " ms\n";
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
    if (m_buffer.refresh())
    {
        emit buffered(static_cast<qint64>(m_buffer.pointCount()));
    }
    else
    {
        emit pollFailed(m_buffer.lastError());
    }
}
