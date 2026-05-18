// main.cpp
//
// Entry point for DrywellDTViewer. Loads config.json via Bootstrapper,
// which then creates either MainWindow (forward mode) or AssimViewer
// (assimilation mode).
//
// Desktop only: an alternative config file can be supplied via
//   DrywellDTViewer --config /path/to/myconfig.json
// On WASM the argument is ignored; the config is always fetched
// relative to the page URL.

#include "Bootstrapper.h"

#include <QApplication>
#include <QStyleFactory>

#ifndef Q_OS_WASM
#  include <QCommandLineOption>
#  include <QCommandLineParser>
#  include <QCoreApplication>
#endif

static const char *kAppStyleSheet = R"QSS(
    QWidget {
        font-family: "Inter", "SF Pro Text", "Segoe UI", "Helvetica Neue", Arial, sans-serif;
        font-size: 13px;
        color: #1E2A38;
    }
    QMainWindow, QScrollArea, QScrollArea > QWidget > QWidget {
        background: #F5F7FA;
    }
    QScrollArea { border: none; }

    QLabel#HeaderTitle {
        color: #1E2A38;
        font-size: 20px;
        font-weight: 600;
    }
    QLabel#HeaderSubtitle {
        color: #6B7684;
        font-size: 12px;
    }

    QFrame#TopBarCard {
        background: #FFFFFF;
        border: 1px solid #E1E5EB;
        border-radius: 10px;
    }
    QFrame#TopBarCard QLabel {
        color: #6B7684;
        font-weight: 500;
    }

    QLineEdit, QSpinBox {
        background: #FFFFFF;
        border: 1px solid #D7DDE4;
        border-radius: 6px;
        padding: 6px 10px;
        selection-background-color: #2D7FF9;
        selection-color: #FFFFFF;
        min-height: 22px;
    }
    QLineEdit:focus, QSpinBox:focus {
        border: 1px solid #2D7FF9;
    }
    QSpinBox::up-button, QSpinBox::down-button {
        width: 16px;
    }

    QPushButton {
        background: #2D7FF9;
        color: #FFFFFF;
        border: none;
        border-radius: 6px;
        padding: 8px 18px;
        font-weight: 600;
    }
    QPushButton:hover {
        background: #1E6CE0;
    }
    QPushButton:pressed {
        background: #1758BC;
    }
    QPushButton:disabled {
        background: #B8C3D0;
        color: #FFFFFF;
    }

    QLabel#StatusLabel {
        color: #6B7684;
        font-size: 12px;
        padding: 2px 4px;
    }
)QSS";

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("DrywellDTViewer");

    QApplication::setStyle(QStyleFactory::create("Fusion"));
    app.setStyleSheet(QString::fromUtf8(kAppStyleSheet));

    Bootstrapper bs;

#ifndef Q_OS_WASM
    // Desktop only: --config /path/to/file.json overrides the default
    // location (<applicationDirPath>/config.json).
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "OHTwin viewer. Reads config.json next to the binary by default; "
        "use --config to point at a different file.");
    parser.addHelpOption();
    QCommandLineOption configOpt(
        QStringList{ "c", "config" },
        "Path to a config.json file.", "path");
    parser.addOption(configOpt);
    parser.process(app);

    if (parser.isSet(configOpt))
        bs.setConfigPath(parser.value(configOpt));
#endif

    bs.start();

    return app.exec();
}
