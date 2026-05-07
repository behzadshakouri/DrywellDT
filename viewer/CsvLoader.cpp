#include "CsvLoader.h"
#include "OhqTime.h"

#include <QDateTime>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStringList>
#include <QUrlQuery>
#include <algorithm>
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


static int viewerSeriesPriority(const QString &name)
{
    const QString n = name.toLower();

    // Put drywell context plots first.
    if (n.contains(QStringLiteral("well_c")) || n.contains(QStringLiteral("well c"))) return 0;
    if (n.contains(QStringLiteral("well_g")) || n.contains(QStringLiteral("well g"))) return 1;
    if (n.contains(QStringLiteral("groundwater")) || n.contains(QStringLiteral("ground water")) || n.contains(QStringLiteral("gw_"))) return 2;
    if (n.contains(QStringLiteral("flow")) || n.contains(QStringLiteral("recharge"))) return 3;

    // Keep ERT-like theta profile panels after the main hydraulic context.
    if (n.contains(QStringLiteral("ert5"))) return 90;
    if (n.contains(QStringLiteral("ert3"))) return 91;
    if (n.contains(QStringLiteral("ert")))  return 92;

    return 10;
}

static int trailingDepthIndex(const QString &name)
{
    // Handles labels like "ERT3 theta z10" or "ERT5_theta_z4".
    const QRegularExpression re(QStringLiteral("[zZ]([0-9]+)\\b"));
    const QRegularExpressionMatch m = re.match(name);
    if (!m.hasMatch()) return -1;
    bool ok = false;
    const int z = m.captured(1).toInt(&ok);
    return ok ? z : -1;
}

static bool viewerSeriesLess(const CsvSeries &a, const CsvSeries &b)
{
    const int pa = viewerSeriesPriority(a.name);
    const int pb = viewerSeriesPriority(b.name);
    if (pa != pb) return pa < pb;

    const int za = trailingDepthIndex(a.name);
    const int zb = trailingDepthIndex(b.name);
    if (za >= 0 && zb >= 0 && za != zb) return za < zb;

    return QString::localeAwareCompare(a.name, b.name) < 0;
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

    std::stable_sort(series.begin(), series.end(), viewerSeriesLess);

    emit loaded(series);
}
