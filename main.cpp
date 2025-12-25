#include "mainwindow.h"
#include "pythonmanager.h"  // ← обязательно!
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
  // Инициализируем Python ОДИН РАЗ
    PythonManager::instance();
    MainWindow w;
    w.callPythonScript(":/config.txt");
    w.show();
    return a.exec();
}
