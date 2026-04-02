#include "app/MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // GPU / WebGL acceleration for QWebEngineView + CesiumJS
    // --use-angle=d3d11       → Force hardware D3D11 (not SwiftShader)
    // --disable-software-rasterizer → Never fall back to CPU rendering
    // --enable-gpu-rasterization    → GPU-accelerated rasterization
    // --enable-zero-copy            → Zero-copy GPU memory for tiles 
    // --in-process-gpu              → GPU in main process (less IPC overhead)
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--use-angle=d3d11 "
            "--disable-software-rasterizer "
            "--ignore-gpu-blocklist "
            "--enable-gpu-rasterization "
            "--enable-zero-copy "
            "--in-process-gpu "
            "--enable-native-gpu-memory-buffers "
            "--remote-debugging-port=9222");

    // Required before QApplication for proper GPU init
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, false);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Cesium Tacview"));
    app.setOrganizationName(QStringLiteral("CesiumTacview"));

    MainWindow window;
    window.show();

    return app.exec();
}
