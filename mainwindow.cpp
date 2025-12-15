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


void MainWindow::on_playurl_clicked()
{
  QString input = ui->urlField->text();

    

    QProcess process;

    QStringList arguments;

    arguments << input;

    QStringList anotherList = {input};

    QString program = "echoplaylist";

    process.setProgram(program);

    process.setArguments(anotherList);

    process.start();

    process.waitForFinished();
}

void callPythonScript(const QString &resourcePath) {
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

    // Сохранение скрипта на диск
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(scriptContent);
        file.close();
    } else {
        std::cerr << "Failed to save script to file: " << filePath.toStdString() << std::endl;
    }
}



//получение ссылок на плееры 
void MainWindow::getplayerurl(const QString &currentUrl)
{

qDebug() << "исполняется getplayerurl функция " << currentUrl;
   QNetworkRequest request{QUrl(currentUrl)};
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->textBrowser->setHtml(QString("<p><b>Ошибка загрузки:</b> %1</p>").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        QString html = reply->readAll();
        reply->deleteLater();

        // Регулярное выражение для поиска ссылок webplayer и webplayer2
        QRegularExpression re(R"(cdn\.livetv869\.me\/webplayer(?:2)?\.php[^"\s]*)");
        QRegularExpressionMatchIterator it = re.globalMatch(html);

        QStringList results;
        while (it.hasNext()) {
            results << it.next().captured(0);
        }
        results.removeDuplicates();

        if (results.isEmpty()) {
            ui->textBrowser->setHtml("<p>Ссылок <code>cdn.livetv869.me/webplayer.php</code> и <code>webplayer2.php</code> не найдено.</p>");
        } else {
            QString htmlOutput;
            for (const QString &link : results) {
                QString fullUrl = link.startsWith("http") ? link : "https://" + link;
                htmlOutput += QString("<p><a href=\"%1\">🔗 %1</a></p>").arg(fullUrl);
            }
            ui->textBrowser->setHtml(htmlOutput);
        }
    });
}


//функция получения m3u8 ссылок
void MainWindow::on_geturlpushButton_clicked()
{

    if (currentUrl.isEmpty()) {
        ui->textBrowser->setHtml("<p><b>URL пустой!</b></p>");
        return;
    }

    QNetworkRequest request{QUrl(currentUrl)};
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->textBrowser->setHtml(QString("<p><b>Ошибка загрузки:</b> %1</p>").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        QString html = reply->readAll();
        reply->deleteLater();

        // Регулярное выражение для поиска ссылок webplayer и webplayer2
        QRegularExpression re(R"(cdn\.livetv869\.me\/webplayer(?:2)?\.php[^"\s]*)");
        QRegularExpressionMatchIterator it = re.globalMatch(html);

        QStringList results;
        while (it.hasNext()) {
            results << it.next().captured(0);
        }
        results.removeDuplicates();

        if (results.isEmpty()) {
            ui->textBrowser->setHtml("<p>Ссылок <code>cdn.livetv869.me/webplayer.php</code> и <code>webplayer2.php</code> не найдено.</p>");
        } else {
            QString htmlOutput;
            for (const QString &link : results) {
                QString fullUrl = link.startsWith("http") ? link : "https://" + link;
                htmlOutput += QString("<p><a href=\"%1\">🔗 %1</a></p>").arg(fullUrl);
            }
            ui->textBrowser->setHtml(htmlOutput);
        }
    });

}

//функция получения m3u8 ссылок через ссылки в поле тектсбраузера
void MainWindow::geturlpushButton(const QUrl &currentUrl)
{
    qDebug() << "Навели на ссылку:" << currentUrl.toString(); // Используем toString(), чтобы получить строку
    if (currentUrl.isEmpty()) {
        ui->textBrowserEvents->setHtml("<p><b>URL пустой!</b></p>");
        return;
    }

    QNetworkRequest request{QUrl(currentUrl)};
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->textBrowserEvents->setHtml(QString("<p><b>Ошибка загрузки:</b> %1</p>").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        QString html = reply->readAll();
        reply->deleteLater();

        // Регулярное выражение для поиска ссылок webplayer и webplayer2
        QRegularExpression re(R"(cdn\.livetv869\.me\/webplayer(?:2)?\.php[^"\s]*)");
        QRegularExpressionMatchIterator it = re.globalMatch(html);

        QStringList results;
        while (it.hasNext()) {
            results << it.next().captured(0);
        }
        results.removeDuplicates();

        if (results.isEmpty()) {
            ui->textBrowserEvents->setHtml("<p>Ссылок <code>cdn.livetv869.me/webplayer.php</code> и <code>webplayer2.php</code> не найдено.</p>");
        } else {
            QString htmlOutput;
            for (const QString &link : results) {
                QString fullUrl = link.startsWith("http") ? link : "https://" + link;
                htmlOutput += QString("<p><a href=\"%1\">🔗 %1</a></p>").arg(fullUrl);
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


void MainWindow::on_matchday_clicked()
{
    // 1. Сохраняем скрипт из ресурсов
   callPythonScript(":/parser_matchday.py");

    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString scriptPath = homePath + "/.livetv/parser_matchday.py";
    QString filePath = homePath + "/.livetv/matchday_events.txt";

    QString htmlContent;

    // 2. Проверяем, что скрипт существует
    if (!QFile::exists(scriptPath)) {
        htmlContent += "<p style='color:red;'><b>Ошибка:</b> Скрипт parser_matchday.py не найден</p>";
        ui->textBrowser->setHtml(htmlContent);
        return;
    }

    // 3. Запускаем скрипт
    QProcess process;
    process.start("python3", QStringList() << scriptPath);

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
