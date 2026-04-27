#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QPointF>
#include <QString>
#include <QUrl>
#include <QVector>
#include <QList>
#include <limits>

// One parsed time series from selected_output.csv.
// Each CSV series is stored as interleaved (time, value) column pairs:
//   column 2*i     -> OHQ day-serial time
//   column 2*i + 1 -> observed/forecast value
//
// points.x : milliseconds since Unix epoch, converted from OHQ day-serial
// points.y : value read from CSV
struct CsvSeries
{
    QString        name;
    QList<QPointF> points;

    double yMin =  std::numeric_limits<double>::infinity();
    double yMax = -std::numeric_limits<double>::infinity();
    qint64 xMin =  std::numeric_limits<qint64>::max();
    qint64 xMax =  std::numeric_limits<qint64>::min();
};

// Fetches selected_output.csv from a URL and parses interleaved
// (t, value) column pairs into CsvSeries objects.
class CsvLoader : public QObject
{
    Q_OBJECT

public:
    explicit CsvLoader(QObject *parent = nullptr);

    // Starts an async GET. A cache-busting query parameter is appended so
    // repeated refreshes do not return stale browser/proxy data.
    void fetch(const QUrl &url);

signals:
    void loaded(const QVector<CsvSeries> &series);
    void failed(const QString &error);

private:
    void parse(const QByteArray &data);

    QNetworkAccessManager m_nam;
};
