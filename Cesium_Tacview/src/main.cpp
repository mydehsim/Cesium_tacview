#include "app/MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // =====================================================================
    // GPU / Chromium flags for QWebEngineView + CesiumJS
    // =====================================================================
    // CRITICAL: Do NOT use --in-process-gpu → GPU crash = app crash = BSOD
    //
    // --use-angle=d3d11               → D3D11 backend (not SwiftShader software)
    // --ignore-gpu-blocklist          → Allow GPU even if driver is "blocklisted"
    // --enable-gpu-rasterization      → GPU-accelerated 2D rasterization
    // --enable-zero-copy              → Zero-copy GPU memory for Cesium tiles
    // --enable-native-gpu-memory-buffers → Native GPU buffers (perf)
    // --disable-software-rasterizer   → Never fall back to CPU rendering
    // --dns-prefetch-disable          → Stop "Failed to read DnsConfig" spam
    // --disable-background-timer-throttling → Keep timers at full speed
    // --disable-renderer-backgrounding     → Don't throttle when unfocused
    // --autoplay-policy=no-user-gesture-required → Allow Cesium animations
    // --remote-debugging-port=9222    → DevTools for JS debugging
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--use-angle=d3d11 "
            "--ignore-gpu-blocklist "
            "--enable-gpu-rasterization "
            "--enable-zero-copy "
            "--enable-native-gpu-memory-buffers "
            "--disable-software-rasterizer "
            "--dns-prefetch-disable "
            "--disable-background-timer-throttling "
            "--disable-renderer-backgrounding "
            "--autoplay-policy=no-user-gesture-required "
            "--remote-debugging-port=9222");

    // Required BEFORE QApplication for proper OpenGL context sharing
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Cesium Tacview"));
    app.setOrganizationName(QStringLiteral("CesiumTacview"));

    MainWindow window;
    window.show();

    return app.exec();
}
