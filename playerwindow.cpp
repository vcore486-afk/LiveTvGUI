#include "playerwindow.h"
#include "ui_playerwindow.h"

#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QWebEngineFullScreenRequest>
#include <QDebug>
#include <QWebEnginePage>
#include <QWebEngineFullScreenRequest>

PlayerWindow::PlayerWindow(const QUrl &url, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PlayerWindow)
{
    ui->setupUi(this);
    setWindowTitle("LiveTV Player — Qt6 WebEngine");
    resize(1100, 750);

    webView = new QWebEngineView(this);

    auto *settings = webView->settings();
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);

    // User-Agent (можно даже оставить как в Chrome)
    webView->page()->profile()->setHttpUserAgent(
        "Mozilla/5.0 (X11; FreeBSD amd64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/128.0 Safari/537.36"
    );

    setCentralWidget(webView);

    if (url.isValid()) {
        qDebug() << "Загружается:" << url.toString();
        webView->load(url);
    }

    // ← ЭТО ГЛАВНОЕ ИСПРАВЛЕНИЕ ДЛЯ Qt6
    connect(webView->page(), &QWebEnginePage::fullScreenRequested,
            this, [this](QWebEngineFullScreenRequest request) {
        request.accept();
        if (request.toggleOn()) {
            showFullScreen();
        } else {
            showNormal();
        }
    });
}

PlayerWindow::~PlayerWindow() { delete ui; }