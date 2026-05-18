// Bootstrapper.h
//
// Loads ./config.json (HTTP fetch on WASM, local file on desktop),
// then creates the appropriate top-level window based on the "mode"
// field. The choice is:
//
//   mode == "forward"      -> MainWindow
//   mode == "assimilation" -> AssimViewer
//
// Either window receives both the root config object (for keys shared
// across modes such as refresh_seconds) and its own sub-object.
//
// On desktop, the file is read from <applicationDirPath>/config.json.
// On WASM, the URL "config.json" is fetched (relative to the page).
//
// A legacy flat config (no "mode" key, no "forward" wrapper, all keys
// at top level) is accepted and treated as mode == "forward".

#pragma once

#include <QObject>
#include <QByteArray>

class QNetworkAccessManager;
class QNetworkReply;

class Bootstrapper : public QObject
{
    Q_OBJECT

public:
    explicit Bootstrapper(QObject *parent = nullptr);

    // Desktop only: override the config file path. Empty means use the
    // default (<applicationDirPath>/config.json). Ignored on WASM, where
    // config.json is always fetched relative to the page URL.
    void setConfigPath(const QString &path) { m_configPathOverride = path; }

    // Kicks off config loading and (eventually) window creation.
    void start();

private slots:
    void onConfigReply(QNetworkReply *reply);

private:
    // Shared path for desktop (read-sync) and WASM (read-async).
    void handleConfigData(const QByteArray &data, const QUrl &configBaseUrl);
    void fatal(const QString &msg);

    QNetworkAccessManager *m_nam = nullptr;   // WASM only
    QString                m_configPathOverride;   // desktop only; empty = default
};
