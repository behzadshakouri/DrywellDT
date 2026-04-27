#include "CsvLoader.h"
#include "OhqTime.h"

#include <QDateTime>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStringList>
#include <QUrlQuery>
#include <QtGlobal>

CsvLoader::CsvLoader(QObject *parent)
    : QObject(parent)
{}

static QNetworkRequest noCacheRequest(QUrl url)
{
    QUrlQuery q(url);
    q.addQueryItem(QStringLiteral("_t"),
                   QString::number(QDateTime::currentMSecsSinceEpoch()));
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                     QNetworkRequest::AlwaysNetwork);
    return req;
}

static void normalizeBounds(CsvSeries &s)
{
    if (s.points.isEmpty()) {
        s.yMin = 0.0;
        s.yMax = 1.0;
        s.xMin = 0;
        s.xMax = 1;
        return;
    }

    // Pad flat y-ranges so the axis is visible instead of a hairline.
    if (s.yMin == s.yMax) {
        s.yMin -= 1.0;
        s.yMax += 1.0;
    }
}

void CsvLoader::fetch(const QUrl &url)
{
    if (!url.isValid() || url.isEmpty()) {
        emit failed(QStringLiteral("CSV URL is empty or invalid"));
        return;
    }

    QNetworkReply *reply = m_nam.get(noCacheRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit failed(reply->errorString());
        } else {
            parse(reply->readAll());
        }
        reply->deleteLater();
    });
}

void CsvLoader::parse(const QByteArray &data)
{
    const QString text = QString::fromUtf8(data);
    const QStringList lines = text.split(QRegularExpression("[\r\n]+"),
                                         Qt::SkipEmptyParts);

    if (lines.size() < 2) {
        emit failed(QStringLiteral("CSV has fewer than 2 lines"));
        return;
    }

    const QStringList headerCols = lines[0].split(',');
    if (headerCols.size() < 2 || headerCols.size() % 2 != 0) {
        emit failed(QString("CSV header has %1 columns; expected even count "
                            "(t, value pairs)").arg(headerCols.size()));
        return;
    }

    const int seriesCount = headerCols.size() / 2;
    QVector<CsvSeries> series(seriesCount);

    for (int i = 0; i < seriesCount; ++i) {
        const QString name = headerCols[2 * i + 1].trimmed();
        series[i].name = name.isEmpty()
                         ? QStringLiteral("Series %1").arg(i + 1)
                         : name;
    }

    for (int row = 1; row < lines.size(); ++row) {
        const QStringList cols = lines[row].split(',');
        if (cols.size() < seriesCount * 2)
            continue;

        for (int i = 0; i < seriesCount; ++i) {
            bool okT = false;
            bool okV = false;

            const double t = cols[2 * i].trimmed().toDouble(&okT);
            const double v = cols[2 * i + 1].trimmed().toDouble(&okV);

            if (!okT || !okV)
                continue;

            const qint64 xMs = ohqSerialToMsEpoch(t);
            series[i].points.append(QPointF(static_cast<qreal>(xMs), v));
            series[i].yMin = qMin(series[i].yMin, v);
            series[i].yMax = qMax(series[i].yMax, v);
            series[i].xMin = qMin(series[i].xMin, xMs);
            series[i].xMax = qMax(series[i].xMax, xMs);
        }
    }

    for (auto &s : series)
        normalizeBounds(s);

    emit loaded(series);
}
