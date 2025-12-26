#include <Python.h> // CPython API
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include "pythonmanager.h"   // ← обязательно!
#include <QDebug>
#include <QDesktopServices>
#include "playerwindow.h"    // ← ЭТО САМОЕ ГЛАВНОЕ!
#include <QProcess>
#include <QFile>
#include <QResource>
#include <iostream>
#include <QDir>
#include <QStandardPaths>
#include <QHelpEvent>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QJsonArray>
#include <functional>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <QPointer>
#include <QtConcurrent/QtConcurrent>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , manager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
ui->textBrowserEvents->setOpenLinks(false);
ui->textBrowserEvents->setOpenExternalLinks(false);
    // ВАЖНО: настройка QTextEdit для кликабельных ссылок
    ui->textBrowser->setOpenExternalLinks(false);
    ui->textBrowser->setOpenLinks(false);
  connect(ui->textBrowser, &QTextBrowser::anchorClicked, this, &MainWindow::geturlpushButton);
  connect(ui->textBrowserEvents, &QTextBrowser::anchorClicked, this, &MainWindow::onLinkClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Хранение URL из lineEdit
QString currentUrl;

void MainWindow::on_lineEdit_textChanged(const QString &arg1)
{
    currentUrl = arg1;

    qDebug() << "[LineEdit] URL обновлён:" << currentUrl;
}




void MainWindow::onLinkClicked(const QUrl &url)
{

qDebug() << "исполняется onLinkClicked функция " << currentUrl;

    if (!url.isValid() || url.isEmpty()) {
        return;
    }

    // Создаём и показываем встроенный плеер
    PlayerWindow *player = new PlayerWindow(url, this);
    player->setAttribute(Qt::WA_DeleteOnClose); // автоудаление при закрытии
    player->show();
    connect(player, &PlayerWindow::urlCaptured, this, &MainWindow::on_urlField_textEdited);
    qDebug() << "Открыт встроенный плеер:" << url.toString();
}


void MainWindow::on_pushButton_clearurl_clicked()
{
    ui->lineEdit->clear();                    // вот твоя очистка URL
    ui->lineEdit->setPlaceholderText("Введите URL...");
    ui->lineEdit->setFocus();


}

void MainWindow::on_urlField_textEdited(const QString &arg1)
{

    // Устанавливаем полученный URL в urlField
    ui->urlField->setText(arg1);

    // Можно добавить дополнительную логику, например, обновление интерфейса
    qDebug() << "Установлен URL в urlField:" << arg1;

}

// Обработчик событий (для фильтрации наведения на ссылки)
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->textBrowser && event->type() == QEvent::HoverMove) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);
        QTextCursor cursor = ui->textBrowser->cursorForPosition(helpEvent->pos());
        QTextCharFormat format = cursor.charFormat();

        qDebug() << "Тип формата:" << format.isAnchor(); // дополнительно выведем статус формата

        if (format.isAnchor()) {
            QString link = format.anchorHref();
            qDebug() << "[HOVER] Наведение на ссылку:" << link;
        }
    }
    return QObject::eventFilter(obj, event);
}


//анализатор ссылки ,если eventinfo вызывается функция getplayerurl
// Метод реакции на наведение на ссылку
void MainWindow::onLinkHovered(const QString &link)
{
    if (link.isEmpty())
        return;

    qDebug() << "[HOVER] Наведение на ссылку:" << link;

    if (link.contains("eventinfo", Qt::CaseInsensitive)) {
        qDebug() << "[HOVER] eventinfo обнаружен — вызываем getplayer()";

        // Ограничиваем частоту вызовов
        static QString lastCalled;
        if (lastCalled != link) {
            lastCalled = link;
            getplayerurl(link);
        }
    }
}

void MainWindow::sendJsonRpc(
    const QJsonObject &json,
    const QString &desc,
    std::function<void(const QJsonObject&)> onSuccess)
{
    QNetworkRequest request(QUrl("http://192.168.8.45:8081/jsonrpc"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(
        request,
        QJsonDocument(json).toJson()
        );

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << desc << "failed:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        QJsonObject response =
            QJsonDocument::fromJson(reply->readAll()).object();

        qDebug() << desc << "OK";

        if (onSuccess)
            onSuccess(response);

        reply->deleteLater();
    });
}

void MainWindow::on_playurl_clicked()
{
    QString input = ui->urlField->text();
    QString filePath = "/tmp/list.m3u";

    // 1️⃣ Создаём M3U
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Cannot create M3U file";
        return;
    }
    QTextStream(&file)
        << "#EXTM3U\n"
        << "#EXTINF:-1,My Channel Name\n"
        << input << "\n";
    file.close();

    qDebug() << "Temporary M3U created at" << filePath;

    QPointer<MainWindow> safeThis(this);

    // 2️⃣ Останавливаем плеер
    QJsonObject stop;
    stop["jsonrpc"] = "2.0";
    stop["method"]  = "Player.Stop";
    stop["params"]  = QJsonObject{{"playerid", 1}};
    stop["id"]      = rpcId++;

    sendJsonRpc(stop, "Player.Stop",
                [safeThis, filePath](const QJsonObject &) {
                    if (!safeThis) return;

                    // 3️⃣ Асинхронный SFTP upload
                    QtConcurrent::run([safeThis, filePath]() {

                        const QString host = "192.168.8.45";
                        const int port = 22;
                        const QString user = "pi";
                        const QString password = "639639";
                        const QString remoteFile = "/var/www/html/list.m3u";

                        int sock = socket(AF_INET, SOCK_STREAM, 0);
                        if (sock < 0) return;

                        struct sockaddr_in sin{};
                        sin.sin_family = AF_INET;
                        sin.sin_port   = htons(port);

                        struct hostent* he = gethostbyname(host.toUtf8().constData());
                        if (!he) { ::close(sock); return; }

                        sin.sin_addr = *(struct in_addr*)he->h_addr;

                        if (::connect(sock, (struct sockaddr*)&sin, sizeof(sin)) != 0) {
                            ::close(sock);
                            return;
                        }

                        LIBSSH2_SESSION* session = libssh2_session_init();
                        if (!session) { ::close(sock); return; }

                        if (libssh2_session_handshake(session, sock)) {
                            libssh2_session_free(session);
                            ::close(sock);
                            return;
                        }

                        if (libssh2_userauth_password(
                                session,
                                user.toUtf8().constData(),
                                password.toUtf8().constData())) {

                            libssh2_session_disconnect(session, "Bye");
                            libssh2_session_free(session);
                            ::close(sock);
                            return;
                        }

                        LIBSSH2_SFTP* sftp = libssh2_sftp_init(session);
                        if (!sftp) {
                            libssh2_session_disconnect(session, "Bye");
                            libssh2_session_free(session);
                            ::close(sock);
                            return;
                        }

                        LIBSSH2_SFTP_HANDLE* handle =
                            libssh2_sftp_open(
                                sftp,
                                remoteFile.toUtf8().constData(),
                                LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
                                LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
                                    LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);

                        if (!handle) {
                            libssh2_sftp_shutdown(sftp);
                            libssh2_session_disconnect(session, "Bye");
                            libssh2_session_free(session);
                            ::close(sock);
                            return;
                        }

                        QFile file(filePath);
                        if (file.open(QIODevice::ReadOnly)) {
                            QByteArray data = file.readAll();
                            libssh2_sftp_write(handle, data.constData(), data.size());
                        }

                        libssh2_sftp_close(handle);
                        libssh2_sftp_shutdown(sftp);
                        libssh2_session_disconnect(session, "Bye");
                        libssh2_session_free(session);
                        ::close(sock);

                        qDebug() << "File uploaded successfully:" << remoteFile;

                        // 4️⃣ Возврат в GUI-поток
                        if (!safeThis) return;
                        QMetaObject::invokeMethod(
                            safeThis,
                            [safeThis]() {
                                if (!safeThis) return;

                                // Enable PVR
                                QJsonObject enable;
                                enable["jsonrpc"] = "2.0";
                                enable["method"]  = "Addons.SetAddonEnabled";
                                enable["params"]  = QJsonObject{
                                    {"addonid", "pvr.iptvsimple"},
                                    {"enabled", true}
                                };
                                enable["id"] = safeThis->rpcId++;

                                safeThis->sendJsonRpc(
                                    enable,
                                    "Enable PVR",
                                    [safeThis](const QJsonObject &) {
                                        if (!safeThis) return;

                                        // PVR.Scan
                                        QJsonObject scan;
                                        scan["jsonrpc"] = "2.0";
                                        scan["method"]  = "PVR.Scan";
                                        scan["id"]      = safeThis->rpcId++;

                                        safeThis->sendJsonRpc(
                                            scan,
                                            "PVR.Scan",
                                            [safeThis](const QJsonObject &) {
                                                if (!safeThis) return;

                                                // ⏱ Пауза 1 секунда после Scan
                                                QTimer::singleShot(1000, safeThis, [safeThis]() {
                                                    if (!safeThis) return;

                                                    // ===== Disable → Enable → Play =====

                                                    QJsonObject disable;
                                                    disable["jsonrpc"] = "2.0";
                                                    disable["method"]  = "Addons.SetAddonEnabled";
                                                    disable["params"]  = QJsonObject{
                                                        {"addonid", "pvr.iptvsimple"},
                                                        {"enabled", false}
                                                    };
                                                    disable["id"] = safeThis->rpcId++;

                                                    safeThis->sendJsonRpc(
                                                        disable,
                                                        "Disable pvr.iptvsimple",
                                                        [safeThis](const QJsonObject &) {
                                                            if (!safeThis) return;

                                                            QTimer::singleShot(3000, safeThis, [safeThis]() {
                                                                if (!safeThis) return;

                                                                QJsonObject enableAgain;
                                                                enableAgain["jsonrpc"] = "2.0";
                                                                enableAgain["method"]  = "Addons.SetAddonEnabled";
                                                                enableAgain["params"]  = QJsonObject{
                                                                    {"addonid", "pvr.iptvsimple"},
                                                                    {"enabled", true}
                                                                };
                                                                enableAgain["id"] = safeThis->rpcId++;

                                                                safeThis->sendJsonRpc(
                                                                    enableAgain,
                                                                    "Enable pvr.iptvsimple",
                                                                    [safeThis](const QJsonObject &) {
                                                                        if (!safeThis) return;

                                                                        QTimer::singleShot(1000, safeThis, [safeThis]() {
                                                                            if (!safeThis) return;

                                                                            QJsonObject play;
                                                                            play["jsonrpc"] = "2.0";
                                                                            play["method"]  = "Player.Open";
                                                                            play["params"]  = QJsonObject{
                                                                                {"item", QJsonObject{
                                                                                             {"channelid", 1}
                                                                                         }}
                                                                            };
                                                                            play["id"] = safeThis->rpcId++;

                                                                            safeThis->sendJsonRpc(
                                                                                play,
                                                                                "Play channel",
                                                                                [safeThis](const QJsonObject &) {
                                                                                    if (!safeThis) return;
                                                                                    qDebug() << "Channel 1 playback started";
                                                                                }
                                                                                );
                                                                        });
                                                                    }
                                                                    );
                                                            });
                                                        }
                                                        );
                                                });
                                            }
                                            );
                                    }
                                    );
                            },
                            Qt::QueuedConnection
                            );
                    });
                }
                );
}



void MainWindow::callPythonScript(const QString &resourcePath) {
    // Загрузка Python-скрипта из ресурсов
    QResource resource(resourcePath);
    if (!resource.isValid()) {
        std::cerr << "Resource not found: " << resourcePath.toStdString() << std::endl;
        return;
    }

    // Чтение содержимого скрипта
    QByteArray scriptContent = QByteArray::fromRawData(reinterpret_cast<const char*>(resource.data()), resource.size());

    // Получение пути к домашнему каталогу и создание пути к папке livetv
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString dirPath = homePath + "/.livetv";

    // Создание папки livetv, если она не существует
    QDir().mkpath(dirPath);

    // Определение имени файла из пути ресурса
    QString fileName = QFileInfo(resourcePath).fileName();
    QString filePath = dirPath + "/" + fileName;

    // Проверка, существует ли файл
    QFile file(filePath);
    if (file.exists()) {
        std::cout << "File already exists, skipping overwrite: " << filePath.toStdString() << std::endl;
        return; // Если файл существует, выходим из функции
    }

    // Сохранение скрипта на диск
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(scriptContent);
        file.close();
        std::cout << "Script saved to: " << filePath.toStdString() << std::endl;
    } else {
        std::cerr << "Failed to save script to file: " << filePath.toStdString() << std::endl;
    }
}




//получение ссылок на плееры 
void MainWindow::getplayerurl(const QString &currentUrl)
{
    QString domain = readLivetvDomainFromConfig();
    if (domain.isEmpty()) {
        ui->textBrowser->setHtml(
            "<p><b>Домен не задан в ~/.livetv/config.txt</b></p>"
        );
        return;
    }

    qDebug() << "исполняется getplayerurl функция:" << currentUrl;

    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(currentUrl)));

    connect(reply, &QNetworkReply::finished, this, [this, reply, domain]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->textBrowser->setHtml(
                QString("<p><b>Ошибка загрузки:</b> %1</p>").arg(reply->errorString())
            );
            reply->deleteLater();
            return;
        }

        QString html = QString::fromUtf8(reply->readAll());
        reply->deleteLater();

        // Ищем webplayer / webplayer2 для домена из конфига
        QString escapedDomain = QRegularExpression::escape(domain);
        QRegularExpression re(
            QString(R"((?:https?:)?\/\/(?:cdn\.)?%1\/webplayer(?:2)?\.php[^"'\\s]*)")
                .arg(escapedDomain),
            QRegularExpression::CaseInsensitiveOption
        );

        QRegularExpressionMatchIterator it = re.globalMatch(html);

        QStringList results;
        while (it.hasNext())
            results << it.next().captured(0);

        results.removeDuplicates();

        if (results.isEmpty()) {
            ui->textBrowser->setHtml(
                QString(
                    "<p>Ссылок <code>%1/webplayer.php</code> и "
                    "<code>webplayer2.php</code> не найдено.</p>"
                ).arg(domain)
            );
        } else {
            QString htmlOutput;
            for (QString link : results) {
                if (link.startsWith("//"))
                    link.prepend("https:");
                else if (!link.startsWith("http"))
                    link.prepend("https://");

                htmlOutput += QString("<p><a href=\"%1\">🔗 %1</a></p>").arg(link);
            }
            ui->textBrowser->setHtml(htmlOutput);
        }
    });
}


QString MainWindow::readLivetvDomainFromConfig()
{
    const QString configFilePath =
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        + "/.livetv/config.txt";

    QFile configFile(configFilePath);

    if (!configFile.exists()) {
        qWarning() << "Файл конфигурации не найден:" << configFilePath;
        return QString();
    }

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Не удалось открыть файл конфигурации:" << configFilePath;
        return QString();
    }

    QTextStream in(&configFile);
    QString line = in.readLine().trimmed();   // берём первую строку
    configFile.close();

    if (line.isEmpty())
        return QString();

    // Убираем протокол (http:// или https://)
    line.remove(QRegularExpression(R"(^https?://)", QRegularExpression::CaseInsensitiveOption));

    // Убираем всё после первого слэша (на случай путей)
    int slashPos = line.indexOf('/');
    if (slashPos != -1)
        line = line.left(slashPos);

    return line;
}




//функция получения m3u8 ссылок
void MainWindow::on_geturlpushButton_clicked()
{
    if (currentUrl.isEmpty()) {
        ui->textBrowser->setHtml("<p><b>URL пустой!</b></p>");
        return;
    }

    // Берём домен ТОЛЬКО из config.txt
    QString domain = readLivetvDomainFromConfig();
    if (domain.isEmpty()) {
        ui->textBrowser->setHtml(
            "<p><b>Домен не задан в ~/.livetv/config.txt</b></p>"
        );
        return;
    }

    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(currentUrl)));

    connect(reply, &QNetworkReply::finished, this, [this, reply, domain]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->textBrowser->setHtml(
                QString("<p><b>Ошибка загрузки:</b> %1</p>").arg(reply->errorString())
            );
            reply->deleteLater();
            return;
        }

        QString html = QString::fromUtf8(reply->readAll());
        reply->deleteLater();

        // Ищем webplayer / webplayer2 для домена из конфига
        QString escapedDomain = QRegularExpression::escape(domain);
        QRegularExpression re(
            QString(R"((?:https?:)?\/\/(?:cdn\.)?%1\/webplayer(?:2)?\.php[^"'\\s]*)")
                .arg(escapedDomain),
            QRegularExpression::CaseInsensitiveOption
        );

        QRegularExpressionMatchIterator it = re.globalMatch(html);

        QStringList results;
        while (it.hasNext())
            results << it.next().captured(0);

        results.removeDuplicates();

        if (results.isEmpty()) {
            ui->textBrowser->setHtml(
                QString(
                    "<p>Ссылок <code>%1/webplayer.php</code> и "
                    "<code>webplayer2.php</code> не найдено.</p>"
                ).arg(domain)
            );
        } else {
            QString htmlOutput;
            for (QString link : results) {
                if (link.startsWith("//"))
                    link.prepend("https:");
                else if (!link.startsWith("http"))
                    link.prepend("https://");

                htmlOutput += QString("<p><a href=\"%1\">🔗 %1</a></p>").arg(link);
            }
            ui->textBrowser->setHtml(htmlOutput);
        }
    });
}



//функция получения m3u8 ссылок через ссылки в поле тектсбраузера
void MainWindow::geturlpushButton(const QUrl &currentUrl)
{
    qDebug() << "Навели на ссылку:" << currentUrl.toString();

    if (currentUrl.isEmpty()) {
        ui->textBrowserEvents->setHtml("<p><b>URL пустой!</b></p>");
        return;
    }

    QString host = readLivetvDomainFromConfig();
    if (host.isEmpty()) {
        ui->textBrowserEvents->setHtml(
            "<p><b>Домен не задан в ~/.livetv/config.txt</b></p>"
        );
        return;
    }

    QNetworkReply *reply = manager->get(QNetworkRequest(currentUrl));

    connect(reply, &QNetworkReply::finished, this, [this, reply, host]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->textBrowserEvents->setHtml(
                QString("<p><b>Ошибка загрузки:</b> %1</p>").arg(reply->errorString())
            );
            reply->deleteLater();
            return;
        }

        QString html = QString::fromUtf8(reply->readAll());
        reply->deleteLater();

        QString escapedHost = QRegularExpression::escape("cdn." + host);
        QRegularExpression re(
            QString(R"(%1\/webplayer(?:2)?\.php[^"'\\s]*)").arg(escapedHost),
            QRegularExpression::CaseInsensitiveOption
        );

        QRegularExpressionMatchIterator it = re.globalMatch(html);

        QStringList results;
        while (it.hasNext())
            results << it.next().captured(0);

        results.removeDuplicates();

        if (results.isEmpty()) {
            ui->textBrowserEvents->setHtml(
                QString(
                    "<p>Ссылок <code>cdn.%1/webplayer.php</code> и "
                    "<code>webplayer2.php</code> не найдено.</p>"
                ).arg(host)
            );
        } else {
            QString htmlOutput;
            for (QString link : results) {
                if (link.startsWith("//"))
                    link.prepend("https:");
                else if (!link.startsWith("http"))
                    link.prepend("https://");

                htmlOutput += QString("<p><a href=\"%1\">🔗 %1</a></p>").arg(link);
            }
            ui->textBrowserEvents->setHtml(htmlOutput);
        }
    });
}



// Объявление общей функции processEvents
void MainWindow::processEvents(const QString &tournamentName, int pageNumber)
{
    callPythonScript(":/find_events.py");

    // Объединяем название турнира и номер страницы в одну строку
    QString combinedArgument = tournamentName + "|" + QString::number(pageNumber);
    qDebug() << "Переданная строка: " << combinedArgument; // Диагностика

    // Вызов Python-функции с объединённым аргументом
    PythonManager::instance().callFunction("find_events", "main", combinedArgument.toStdString().c_str());

    // Получение пути к файлу events.txt
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString filePath = homePath + "/.livetv/events.txt";

    // Открытие файла и чтение строк
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Ошибка открытия файла" << std::endl;
        return;
    }

    QTextStream stream(&file);
    QString htmlContent;

    // Формирование HTML-контента
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QStringList parts = line.split('\t');

        // Проверяем, что есть 3 колонки: название, время, ссылка
        if (parts.size() >= 3) {
            QString title = parts[0].trimmed();
            QString time = parts[1].trimmed();
            QString href = parts[2].trimmed();

            // Формируем HTML с временем и ссылкой
            htmlContent += QString("<p><strong>%1</strong> — <em>%2</em> (<a href='%3'>перейти</a>)</p>\n")
                               .arg(title, time, href);
        }
    }

    file.close();

    // Установка сформированного HTML в textBrowser
    ui->textBrowser->setHtml(htmlContent);
}


//парсер страницы для получения событий лиги чемпионов
void MainWindow::on_pushButton_2_clicked()
{

 processEvents("Лига Чемпионов",1); // Аналогично передаем нужный турнир
}

//парсер страницы для получения событий лиги европы
void MainWindow::on_parserel_clicked()
{
 processEvents("Лига Европы",1); // Передача нужного турнира
}


void MainWindow::on_parseruefa_clicked()
{
   processEvents("Лига Конференций",1);
}


void MainWindow::on_parsernhl_clicked()
{
     processEvents("НХЛ",2);
}


void MainWindow::on_parserbundesliga_clicked()
{
      processEvents("Футбол. Германия. Бундеслига",1);
}


void MainWindow::on_turkishliga_clicked()
{
     processEvents("Турция. Суперлига",1);
}


void  MainWindow::loadTopMatches(int pageNumber)
{
    // 1. Сохраняем скрипт из ресурсов
    callPythonScript(":/parser_matchday.py");


    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString scriptPath = homePath + "/.livetv/parser_matchday.py";
    QString filePath = homePath + "/.livetv/matchday_events.txt";  // Обычное имя файла без номеров

    QString htmlContent;

    // 2. Проверяем, что скрипт существует
    if (!QFile::exists(scriptPath)) {
        htmlContent += "<p style='color:red;'><b>Ошибка:</b> Скрипт parser_matchday.py не найден</p>";
        ui->textBrowser->setHtml(htmlContent);
        return;
    }

    // 3. Запускаем скрипт, передаем номер страницы как параметр
    QProcess process;
    process.start("python3", QStringList() << scriptPath << QString::number(pageNumber));

    if (!process.waitForFinished(10000)) { // ждём до 10 секунд
        htmlContent += "<p style='color:red;'><b>Ошибка:</b> Скрипт не завершился вовремя</p>";
        ui->textBrowser->setHtml(htmlContent);
        return;
    }

    // 4. Получаем вывод скрипта
    QString stdOutput = process.readAllStandardOutput();
    QString stdError  = process.readAllStandardError();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        htmlContent += "<p style='color:red;'><b>Ошибка выполнения скрипта Python:</b></p>";
        htmlContent += "<pre>" + stdError.toHtmlEscaped() + "</pre>";
        ui->textBrowser->setHtml(htmlContent);
        return;
    }

    // 5. Читаем файл с результатом
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        htmlContent += "<p style='color:red;'><b>Ошибка:</b> Не удалось открыть файл matchday_events.txt</p>";
        ui->textBrowser->setHtml(htmlContent);
        return;
    }

    QTextStream stream(&file);

    // Пропускаем заголовок
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.trimmed().isEmpty())
            break;
    }

    // Читаем блоки по 4 строки
    while (!stream.atEnd()) {
        QString firstLine = stream.readLine().trimmed();
        if (firstLine.isEmpty())
            continue;

        QString dateTime = stream.readLine().trimmed();
        stream.readLine(); // (Лига) — пропускаем
        QString link = stream.readLine().trimmed();

        QStringList parts = firstLine.split('\t');
        if (parts.size() != 2)
            continue;

        QString league = parts[0];
        QString match = parts[1];

        htmlContent += QString(
                           "<p>"
                           "<b>%1</b><br>"
                           "%2<br>"
                           "<a href='%3'>Перейти к матчу</a>"
                           "</p><hr>"
                           ).arg(match, dateTime, link);
    }

    file.close();

    // 6. Выводим в textBrowser
    if (!stdError.isEmpty()) {
        htmlContent += "<p style='color:red;'><b>Python stderr:</b></p>";
        htmlContent += "<pre>" + stdError.toHtmlEscaped() + "</pre>";
    }

    if (!stdOutput.isEmpty()) {
        htmlContent += "<p style='color:green;'><b>Python stdout:</b></p>";
        htmlContent += "<pre>" + stdOutput.toHtmlEscaped() + "</pre>";
    }

    ui->textBrowser->setOpenExternalLinks(true);
    ui->textBrowser->setHtml(htmlContent);
}



void MainWindow::on_matchday_clicked()
{
    loadTopMatches(1);
}

void MainWindow::on_parserhockey_clicked()
{
    loadTopMatches(2);
}


void MainWindow::on_parsebasketball_clicked()
{
    loadTopMatches(3);
}
void postkodi(int value) {



    QNetworkAccessManager *mgr = new QNetworkAccessManager();

    const QUrl url(QStringLiteral("http://192.168.8.45:8081/jsonrpc"));

    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");


    QJsonObject obj;

    obj["jsonrpc"] = "2.0";

    obj["id"] = "1";

    obj["method"] = "Application.SetVolume";

    obj["params"] = QJsonObject({{"volume", value}});

    QJsonDocument doc(obj);

    QByteArray data = doc.toJson();

    // or

    // QByteArray data("{\"key1\":\"value1\",\"key2\":\"value2\"}");

    //curl -X POST -H "Content-Type: application/json" -d '{"jsonrpc":"2.0","id":1,"method":"Application.SetVolume","params":{"volume":80}}' http://192.168.8.45:8081/jsonrpc

    //QByteArray data("{\"jsonrpc\":\"2.0\",\"id\":\"1\",\"method\":\"Application.SetVolume\",\"params\":\{\"volume\":  50}}");

    QNetworkReply *reply = mgr->post(request, data);


    QObject::connect(reply, &QNetworkReply::finished, [=](){

        if(reply->error() == QNetworkReply::NoError){

            QString contents = QString::fromUtf8(reply->readAll());

            qDebug() << contents;

        }

        else{

            QString err = reply->errorString();

            qDebug() << err;

        }

        reply->deleteLater();

    });



}


void MainWindow::on_horizontalSlider_valueChanged(int value)

{

    postkodi(value);

    qDebug() << "Значение горизонтального слайдера изменилось:" << value;


}

