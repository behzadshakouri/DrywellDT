#pragma once
#include "Precipitation.h"
#include <QDateTime>
#include <QObject>
#include <QVector>
class QNetworkAccessManager;
enum class datatype {
    ProbabilityofPrecipitation,
    Temperature,
    PrecipitationAmount,
    RelativeHumidity
};
struct WeatherData {
    QDateTime startTime;   // bin start (was: timestamp)
    QDateTime endTime;     // bin end (startTime + duration)
    double    value;
};
class NOAAWeatherFetcher : public QObject {
    Q_OBJECT
public:
    explicit NOAAWeatherFetcher(QObject *parent = nullptr);

    // NOAA gridpoints forecast (6-hour bins)
    // office: e.g. "LWX",  gridX/gridY: integer grid coords
    QVector<WeatherData> getWeatherPrediction(
        const QString &office, int gridX, int gridY, datatype type);

    // Open-Meteo hourly precipitation forecast (1-hour bins, no API key needed)
    // Returns CPrecipitation in OHQ day-serial units, filtered to [intervalStart, intervalEnd]
    CPrecipitation getOpenMeteoPrecipitation(
        double latitude, double longitude,
        const QDateTime &intervalStart,
        const QDateTime &intervalEnd);

    // Open-Meteo hourly precipitation **archive** (historical) — same units
    // and bin convention as getOpenMeteoPrecipitation, but pulls from the
    // ERA5-backed archive endpoint. Suitable for Truth-Twin / historical
    // replay runs. Note: archive data is delayed ~5 days from real time.
    CPrecipitation getOpenMeteoHistoricalPrecipitation(
        double latitude, double longitude,
        const QDateTime &intervalStart,
        const QDateTime &intervalEnd);

    // Last error message from getOpenMeteoPrecipitation
    QString lastError() const { return m_lastError; }

private:
    static qint64 parseDurationSecs(const QString &duration);
    static double toOHQDaySerial(const QDateTime &dt);
    QNetworkAccessManager *manager;
    QString m_lastError;
};
