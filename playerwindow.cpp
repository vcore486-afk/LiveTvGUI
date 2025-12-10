// playerwindow.cpp — ФИНАЛЬНАЯ ВЕРСИЯ (декабрь 2025, FreeBSD + Qt6)
#include "playerwindow.h"
#include "ui_playerwindow.h"

#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QFile>
#include <QTimer>
#include <QProcess>
#include <QDebug>

PlayerWindow::PlayerWindow(const QUrl &url, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PlayerWindow)
{
    ui->setupUi(this);
    setWindowTitle("LiveTV Player — Stream Detector Active");
    resize(1200, 800);

    webView = new QWebEngineView(this);

    // === Профиль + кастомная страница ===
    QWebEngineProfile *profile = new QWebEngineProfile(this);
    DebugPage *page = new DebugPage(profile, this);

    profile->setHttpUserAgent(
        "Mozilla/5.0 (X11; FreeBSD amd64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/128.0 Safari/537.36"
    );

    webView->setPage(page);
    setCentralWidget(webView);

    auto *settings = page->settings();
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    // === Внедряем StreamDetector ===
    QFile scriptFile(":/stream-detector.js");
    if (scriptFile.open(QIODevice::ReadOnly)) {
        QString jsCode = scriptFile.readAll();
        scriptFile.close();

        QWebEngineScript script;
        script.setName("StreamDetector");
        script.setSourceCode(jsCode);
        script.setInjectionPoint(QWebEngineScript::DocumentReady);
        script.setRunsOnSubFrames(true);
        script.setWorldId(QWebEngineScript::MainWorld);

        page->scripts().insert(script);
        qDebug() << "StreamDetector внедрён успешно!";
    } else {
        qCritical() << "ОШИБКА: не найден :/stream-detector.js — проверь resources.qrc!";
        return;
    }

    // === Полноэкранный режим ===
    connect(page, &QWebEnginePage::fullScreenRequested, this,
            [this](QWebEngineFullScreenRequest request) {
        request.accept();
        request.toggleOn() ? showFullScreen() : showNormal();
    });

    // === ГЛАВНОЕ: после загрузки — выводим все потоки + авто-mpv ===
    connect(page, &QWebEnginePage::loadFinished, this, [page, this](bool ok) {
        if (!ok) return;

        QTimer::singleShot(4000, page, [page, this]() {
            page->runJavaScript(R"(
                if (window.detectedStreams && window.detectedStreams.length > 0) {
                    console.log("%cНайдено потоков: " + window.detectedStreams.length, "color: lime; font-weight: bold;");

                    window.detectedStreams.forEach((stream, i) => {
                        const color = stream.type === 'HLS' ? 'cyan' : 
                                     stream.type === 'DASH' ? 'magenta' : 'yellow';
                        console.log(`%c[${i+1}] ${stream.type} → ${stream.url}`, 
                                    `color: ${color}; font-weight: bold;`);
                    });

                    const best = window.detectedStreams
                        .filter(s => s.type === 'HLS' && s.url.includes('.m3u8'))
                        .sort((a, b) => b.url.length - a.url.length)[0];

                    if (best) {
                        console.log("%cЛУЧШИЙ ПОТОК: " + best.url, "color: red; font-size: 18px;");
                        window.bestStreamUrl = best.url;
                    }

                    JSON.stringify(window.detectedStreams);
                } else {
                    "[]";
                }
            )", [page, this](const QVariant &result) {
                QString json = result.toString();
                if (json != "[]") {
                    qDebug().noquote() << "Все потоки:" << json;

                    // Автозапуск mpv
                    page->runJavaScript("window.bestStreamUrl || ''", [this](const QVariant &v) {
                        QString bestUrl = v.toString().trimmed();
                        if (!bestUrl.isEmpty() && bestUrl.contains(".m3u8")) {
                            qDebug().noquote() << "ЗАПУСКАЕМ MPV:" << bestUrl;

                            QProcess::startDetached("mpv", QStringList()
                                << "--referrer=https://livetv.sx/"
                                << "--user-agent=Mozilla/5.0 (X11; FreeBSD amd64)"
                                << "--volume=70"
                                << "--hwdec=auto"
                                << "--no-terminal"
                                << bestUrl
                            );

                            this->close();  // Закрываем плеер
                        }
                    });
                }
            });
        });
    });

    if (url.isValid()) {
        qDebug() << "Загрузка:" << url.toString();
        webView->load(url);
    }
}

PlayerWindow::~PlayerWindow()
{
    delete ui;
}