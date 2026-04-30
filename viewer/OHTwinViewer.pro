QT += core gui widgets charts network svg svgwidgets
CONFIG += c++17

TARGET   = DrywellDTViewer
TEMPLATE = app

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    CsvLoader.cpp

HEADERS += \
    MainWindow.h \
    CsvLoader.h \
    OhqTime.h

DISTFILES += \
    config.json

# Copy viewer config next to the binary when present in the project folder.
# Safe for desktop builds; for WASM deployment, keep config.json beside the
# generated .html/.js/.wasm files on the web server.
exists($$PWD/config.json) {
    copy_viewer_config.commands = $$QMAKE_COPY $$shell_quote($$PWD/config.json) $$shell_quote($$OUT_PWD/config.json)
    QMAKE_EXTRA_TARGETS += copy_viewer_config
    PRE_TARGETDEPS += copy_viewer_config
}

# Qt 6 wasm: QApplication + widgets are supported.
# Build with:
#   /path/to/qt-wasm/bin/qmake DrywellDTViewer.pro
#   make
# Output: DrywellDTViewer.html + .js + .wasm.
# Serve those files with config.json beside them, and point config.json to
# selected_output.csv, viz.svg, forecast_viz.svg, and the state JSON files.
