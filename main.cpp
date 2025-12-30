#include "mainwindow.h"
#include "pythonmanager.h"  // ← обязательно!
#include <QApplication>

int main(int argc, char *argv[])
{

    qputenv("QTWEBENGINE_DISABLE_GPU", "1");
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--disable-gpu "
            "--disable-gpu-compositing "
            "--disable-software-rasterizer "
            "--disable-features=UseOzonePlatform");

    qputenv("QT_QPA_PLATFORM", "xcb");      // fallback с Wayland
    qputenv("QT_DISABLE_VULKAN", "1");      // на всякий случай
    QApplication a(argc, argv);
  // Инициализируем Python ОДИН РАЗ
    PythonManager::instance();
    MainWindow w;
    w.callPythonScript(":/config.txt");
    w.callPythonScript(":/proxy.txt");
    w.show();
    return a.exec();
}
