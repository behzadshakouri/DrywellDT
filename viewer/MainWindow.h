#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QUrl>
#include <QVector>
#include <QByteArray>
#include "CsvLoader.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

class QLineEdit;
class QSpinBox;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QSvgWidget;
class QSplitter;
class QChart;
class QChartView;
class QLineSeries;
class QDateTimeAxis;
class QValueAxis;

struct ChartPanel
{
    QChart        *chart    = nullptr;
    QLineSeries   *series   = nullptr;
    QDateTimeAxis *axisX    = nullptr;
    QValueAxis    *axisY    = nullptr;
    QChartView    *view     = nullptr;
    QLineSeries   *boundary = nullptr;   // vertical "now" line
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

    // --- network reply handlers ---
    void onSvgFetched(QNetworkReply *reply);
    void onForecastSvgFetched(QNetworkReply *reply);
    void onVizStateFetched(QNetworkReply *reply);
    void onForecastVizStateFetched(QNetworkReply *reply);

    // --- mode toggle ---
    void onModeToggled();

private:
    void display();                                          // slice m_lastData by m_lastN and render
    void rebuildPanels(const QVector<CsvSeries> &series);
    void updatePanels (const QVector<CsvSeries> &series);
    static void recomputeBounds(CsvSeries &s);

    // Read solver_state.t (OHQ day-serial) from a viz state JSON and
    // store the converted ms-since-epoch value into outMs.
    void parseVizStateBoundary(const QByteArray &data, qint64 &outMs);

    // Refresh the boundary line on every chart panel using m_advanceEndMs.
    void applyBoundaryToPanels();

    // Load the cached SVG for the currently selected mode into the widget.
    void applySvgForCurrentMode();

    // --- top-bar widgets ---
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
    int  m_refreshSeconds = 300;   // default 5 min, editable from UI
    int  m_lastN          = 0;     // 0 = show all points

    // --- config loading ---
    void loadConfig();
    void onConfigReply(QNetworkReply *reply);
    QNetworkAccessManager m_configNam;

    // --- SVG pane ---
    QSvgWidget   *m_svgWidget    = nullptr;
    QPushButton  *m_currentBtn   = nullptr;
    QPushButton  *m_forecastBtn  = nullptr;
    bool          m_showForecast = false;

    // --- URLs (loaded from config.json) ---
    QUrl m_vizUrl;                  // viz.svg
    QUrl m_vizStateUrl;             // viz_state.json
    QUrl m_forecastVizUrl;          // forecast_viz.svg
    QUrl m_forecastVizStateUrl;     // forecast_viz_state.json

    // --- network access for the new fetches ---
    QNetworkAccessManager m_svgNam;
    QNetworkAccessManager m_forecastSvgNam;
    QNetworkAccessManager m_vizStateNam;
    QNetworkAccessManager m_forecastVizStateNam;

    // --- cached SVG payloads (swap on toggle without refetching) ---
    QByteArray m_currentSvgBytes;
    QByteArray m_forecastSvgBytes;

    // --- boundary timestamps in ms-since-epoch (-1 = unknown) ---
    qint64 m_advanceEndMs  = -1;     // "now" line
    qint64 m_forecastEndMs = -1;     // forecast horizon end
};
