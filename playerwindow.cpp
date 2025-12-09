#include "playerwindow.h"
#include "ui_playerwindow.h"

#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QWebEngineProfile>        // Работает
#include <QDebug>

PlayerWindow::PlayerWindow(const QUrl &url, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PlayerWindow)
{
    ui->setupUi(this);
    setWindowTitle("LiveTV Player");
    resize(1024, 720);

    webView = new QWebEngineView(this);

    // Включаем всё нужное
    QWebEngineSettings *settings = webView->settings();
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    settings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);

    // ← САМЫЙ ВАЖНЫЙ User-Agent — без него livetv869.me показывает ошибку!
    webView->page()->profile()->setHttpUserAgent(
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
    );

    setCentralWidget(webView);

    if (url.isValid()) {
        qDebug() << "Загружаем в плеер:" << url.toString();
        webView->load(url);
    }

    // ← ЭТОТ connect УДАЛЯЕМ НА СТАРЫХ ВЕРСИЯХ FreeBSD/Qt5
    // connect(webView, &QWebEngineView::fullScreenRequested, ...); // НЕТ В ПОРТАХ

    // Альтернатива: можно выйти в полноэкранный режим по F11 вручную
    // (или добавить кнопку — но это уже по желанию)
}

PlayerWindow::~PlayerWindow()
{
    delete ui;
}