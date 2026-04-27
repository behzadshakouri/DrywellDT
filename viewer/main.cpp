#include "MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QStyleFactory>

// Modern light theme — Fusion base + QSS overlay.
// Kept inline here so the viewer's visual style stays in one place.
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

    // Fusion renders consistently across desktop and Qt/WASM builds.
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    app.setStyleSheet(QString::fromUtf8(kAppStyleSheet));

    MainWindow w;
    w.show();

    return app.exec();
}
