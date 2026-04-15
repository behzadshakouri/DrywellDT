#include "noaaweatherfetcher.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <iostream>

// OHQ epoch: day-serial 0 = 1899-12-30 (Excel convention)
static const QDate kNOAAOHQEpoch(1899, 12, 30);

NOAAWeatherFetcher::NOAAWeatherFetcher(QObject *parent)
    : QObject(parent)
    , manager(new QNetworkAccessManager(this))
{}

// ---------------------------------------------------------------------------
// toOHQDaySerial
// ---------------------------------------------------------------------------
double NOAAWeatherFetcher::toOHQDaySerial(const QDateTime &dt)
{
    const qint64 ms = QDateTime(kNOAAOHQEpoch, QTime(0,0,0), Qt::UTC)
    .msecsTo(dt.toUTC());
    return static_cast<double>(ms) / (86400.0 * 1000.0);
}

// ---------------------------------------------------------------------------
// parseDurationSecs
// Parses ISO 8601 duration: P[nD][T[nH][nM][nS]]
// Examples: PT1H → 3600   P1D → 86400   P7DT14H → 655200   PT30M → 1800
// ---------------------------------------------------------------------------
qint64 NOAAWeatherFetcher::parseDurationSecs(const QString &dur)
{
    qint64 secs = 0;

    const int tIdx = dur.indexOf('T');
    const QString datePart = (tIdx == -1) ? dur : dur.left(tIdx);
    const QString timePart = (tIdx == -1) ? QString() : dur.mid(tIdx + 1);

    QRegularExpression dayRx("(\\d+)D");
    QRegularExpression hourRx("(\\d+)H");
    QRegularExpression minRx("(\\d+)M");
    QRegularExpression secRx("(\\d+)S");

    auto match = dayRx.match(datePart);
    if (match.hasMatch())
        secs += match.captured(1).toLongLong() * 86400;

    match = hourRx.match(timePart);
    if (match.hasMatch())
        secs += match.captured(1).toLongLong() * 3600;

    match = minRx.match(timePart);
    if (match.hasMatch())
        secs += match.captured(1).toLongLong() * 60;

    match = secRx.match(timePart);
    if (match.hasMatch())
        secs += match.captured(1).toLongLong();

    return secs;
}

// ---------------------------------------------------------------------------
// getWeatherPrediction  (NOAA gridpoints)
// ---------------------------------------------------------------------------
QVector<WeatherData> NOAAWeatherFetcher::getWeatherPrediction(
    const QString &office, int gridX, int gridY, datatype type)
{
    QVector<WeatherData> result;

    const QString url = QString("https://api.weather.gov/gridpoints/%1/%2,%3")
                            .arg(office).arg(gridX).arg(gridY);

    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "DTRunner/1.0 (openhydroqual@example.com)");

    QString dataTypeKey;
    switch (type) {
    case datatype::PrecipitationAmount:        dataTypeKey = "quantitativePrecipitation"; break;
    case datatype::ProbabilityofPrecipitation: dataTypeKey = "probabilityOfPrecipitation"; break;
    case datatype::RelativeHumidity:           dataTypeKey = "relativeHumidity"; break;
    case datatype::Temperature:                dataTypeKey = "temperature"; break;
    }

    QNetworkReply *reply = manager->get(request);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        std::cerr << "[NOAA] Network error: "
                  << reply->errorString().toStdString() << "\n";
        reply->deleteLater();
        return result;
    }

    const QJsonObject root =
        QJsonDocument::fromJson(reply->readAll()).object();
    reply->deleteLater();

    const QJsonArray values =
        root["properties"].toObject()[dataTypeKey].toObject()["values"].toArray();

    if (values.isEmpty()) {
        std::cerr << "[NOAA] No values found for key: "
                  << dataTypeKey.toStdString() << "\n";
        return result;
    }

    for (const auto &entry : values) {
        const QJsonObject obj = entry.toObject();
        const QString validTime = obj["validTime"].toString();

        const QStringList parts = validTime.split('/');
        if (parts.size() != 2) continue;

        const QDateTime start =
            QDateTime::fromString(parts[0], Qt::ISODate).toUTC();
        if (!start.isValid()) continue;

        const qint64 durSecs = parseDurationSecs(parts[1]);
        if (durSecs <= 0) continue;

        const QDateTime end = start.addSecs(durSecs);
        const double value  = obj["value"].toDouble();

        result.push_back({ start, end, value });
    }

    std::cout << "[NOAA] Fetched " << result.size()
              << " " << dataTypeKey.toStdString() << " records\n";
    return result;
}

// ---------------------------------------------------------------------------
// getOpenMeteoPrecipitation
// Fetches hourly precipitation from Open-Meteo (free, no API key).
// Returns CPrecipitation bins in OHQ day-serial units filtered to interval.
// ---------------------------------------------------------------------------
CPrecipitation NOAAWeatherFetcher::getOpenMeteoPrecipitation(
    double latitude, double longitude,
    const QDateTime &intervalStart,
    const QDateTime &intervalEnd)
{
    CPrecipitation precip;
    m_lastError.clear();

    // Build URL
    QUrl url("https://api.open-meteo.com/v1/forecast");
    QUrlQuery query;
    query.addQueryItem("latitude",      QString::number(latitude,  'f', 5));
    query.addQueryItem("longitude",     QString::number(longitude, 'f', 5));
    query.addQueryItem("hourly",        "precipitation");
    query.addQueryItem("forecast_days", "7");
    query.addQueryItem("timezone",      "GMT");
    url.setQuery(query);

    std::cout << "[OpenMeteo] Fetching: " << url.toString().toStdString() << "\n";

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent",
                         "DTRunner/1.0 (openhydroqual@example.com)");

    QNetworkReply *reply = manager->get(request);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = "[OpenMeteo] Network error: " + reply->errorString();
        std::cerr << m_lastError.toStdString() << "\n";
        reply->deleteLater();
        return precip;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    reply->deleteLater();

    if (doc.isNull()) {
        m_lastError = "[OpenMeteo] JSON parse error: " + parseError.errorString();
        std::cerr << m_lastError.toStdString() << "\n";
        return precip;
    }

    const QJsonObject root   = doc.object();
    const QJsonObject hourly = root.value("hourly").toObject();
    const QJsonArray  times  = hourly.value("time").toArray();
    const QJsonArray  values = hourly.value("precipitation").toArray();

    if (times.isEmpty() || times.size() != values.size()) {
        m_lastError = "[OpenMeteo] Unexpected response structure";
        std::cerr << m_lastError.toStdString() << "\n";
        return precip;
    }

    int loaded  = 0;
    int skipped = 0;

    for (int i = 0; i < times.size(); ++i)
    {
        // Open-Meteo returns "2026-04-15T00:00" — always UTC (timezone=GMT)
        QDateTime binStart = QDateTime::fromString(
            times[i].toString(), "yyyy-MM-ddTHH:mm");
        binStart.setTimeSpec(Qt::UTC);

        if (!binStart.isValid())
            continue;

        const QDateTime binEnd = binStart.addSecs(3600); // 1-hour bins

        // Skip bins entirely outside the interval window
        if (binEnd <= intervalStart || binStart >= intervalEnd) {
            ++skipped;
            continue;
        }

        // Clamp bin edges to interval window
        const QDateTime clampedStart = qMax(binStart, intervalStart);
        const QDateTime clampedEnd   = qMin(binEnd,   intervalEnd);

        const double s  = toOHQDaySerial(clampedStart);
        const double e  = toOHQDaySerial(clampedEnd);
        const double mm = values[i].toDouble();

        // Convert mm → metres (OHQ uses SI units)
        precip.append(s, e, mm / 1000.0);
        ++loaded;
    }

    std::cout << "[OpenMeteo] Precipitation bins loaded: " << loaded
              << " (skipped " << skipped << " outside window)\n";

    return precip;
}
