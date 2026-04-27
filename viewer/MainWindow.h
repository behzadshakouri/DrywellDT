#pragma once

#include "CsvLoader.h"

#include <QByteArray>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>
#include <QVector>

#include <QtCharts/QChartGlobal>

class QLabel;
class QPushButton;
class QSpinBox;
class QSvgWidget;
class QVBoxLayout;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_BEGIN_NAMESPACE
class QChart;
class QChartView;
class QDateTimeAxis;
class QLineSeries;
class QValueAxis;
QT_CHARTS_END_NAMESPACE
QT_CHARTS_USE_NAMESPACE
#else
class QChart;
class QChartView;
class QDateTimeAxis;
class QLineSeries;
class QValueAxis;
#endif

struct ChartPanel
{
    QChart        *chart    = nullptr;
    QLineSeries   *series   = nullptr;
    QDateTimeAxis *axisX    = nullptr;
    QValueAxis    *axisY    = nullptr;
    QChartView    *view     = nullptr;
    QLineSeries   *boundary = nullptr;   // vertical current-time / advance-end line
    QString        name;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onRefreshClicked();
    void onIntervalChanged(int seconds);
    void onLastNChanged(int n);
    void onLoaded(const QVector<CsvSeries> &series);
    void onFailed(const QString &err);
    void onSeriesHovered(const QPointF &point, bool state);

    // Network reply handlers.
    void onSvgFetched(QNetworkReply *reply);
    void onForecastSvgFetched(QNetworkReply *reply);
    void onVizStateFetched(QNetworkReply *reply);
    void onForecastVizStateFetched(QNetworkReply *reply);

    // Current / Forecast SVG toggle.
    void onModeToggled();

private:
    // Re-slice m_lastData by m_lastN and render/update chart panels.
    void display();

    void rebuildPanels(const QVector<CsvSeries> &series);
    void updatePanels (const QVector<CsvSeries> &series);
    static void recomputeBounds(CsvSeries &s);

    // Read solver_state.t (OHQ day-serial) from a viz state JSON and store
    // the converted milliseconds-since-epoch value in outMs.
    void parseVizStateBoundary(const QByteArray &data, qint64 &outMs);

    // Refresh the vertical boundary line on every chart panel.
    void applyBoundaryToPanels();

    // Load the cached SVG for the currently selected mode into the widget.
    void applySvgForCurrentMode();

    // Config loading.
    void loadConfig();
    void onConfigReply(QNetworkReply *reply);

    // Top-bar widgets.
    QSpinBox    *m_intervalSpin  = nullptr;
    QSpinBox    *m_lastNSpin     = nullptr;
    QPushButton *m_refreshBtn    = nullptr;
    QLabel      *m_statusLabel   = nullptr;
    QVBoxLayout *m_chartsLayout  = nullptr;

    QTimer    m_timer;
    CsvLoader m_loader;

    QVector<ChartPanel> m_panels;
    QVector<CsvSeries>  m_lastData;   // full dataset from most recent fetch

    QUrl m_url;
    int  m_refreshSeconds = 300;      // default 5 minutes, editable from UI
    int  m_lastN          = 0;        // 0 = show all points

    QNetworkAccessManager m_configNam;

    // SVG pane.
    QSvgWidget  *m_svgWidget    = nullptr;
    QPushButton *m_currentBtn   = nullptr;
    QPushButton *m_forecastBtn  = nullptr;
    bool         m_showForecast = false;

    // URLs loaded from viewer config.json.
    QUrl m_vizUrl;                  // viz.svg
    QUrl m_vizStateUrl;             // viz_state.json
    QUrl m_forecastVizUrl;          // forecast_viz.svg
    QUrl m_forecastVizStateUrl;     // forecast_viz_state.json

    // Network managers for independent fetches.
    QNetworkAccessManager m_svgNam;
    QNetworkAccessManager m_forecastSvgNam;
    QNetworkAccessManager m_vizStateNam;
    QNetworkAccessManager m_forecastVizStateNam;

    // Cached SVG payloads. The toggle swaps these without refetching.
    QByteArray m_currentSvgBytes;
    QByteArray m_forecastSvgBytes;

    // Boundary timestamps in milliseconds since Unix epoch (-1 = unknown).
    qint64 m_advanceEndMs  = -1;    // current/advance end line
    qint64 m_forecastEndMs = -1;    // parsed for future use
};
