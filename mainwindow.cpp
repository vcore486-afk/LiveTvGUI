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


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , manager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    // ВАЖНО: настройка QTextEdit для кликабельных ссылок
    ui->textEdit->setOpenExternalLinks(false);
    ui->textEdit->setOpenLinks(false);
connect(ui->textEdit, &QTextBrowser::anchorClicked, this, &MainWindow::onLinkClicked);
//                    ^^^^^^^^^^^^^^^^^^^^^^^^^^^ — исправлено!
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

void MainWindow::on_pushButton_clicked()
{
    if (currentUrl.isEmpty()) {
        ui->textEdit->setHtml("<p><b>URL пустой!</b></p>");
        return;
    }

    QNetworkRequest request{QUrl(currentUrl)};
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->textEdit->setHtml(QString("<p><b>Ошибка загрузки:</b> %1</p>").arg(reply->errorString()));
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
            ui->textEdit->setHtml("<p>Ссылок <code>cdn.livetv869.me/webplayer.php</code> и <code>webplayer2.php</code> не найдено.</p>");
        } else {
            QString htmlOutput;
            for (const QString &link : results) {
                QString fullUrl = link.startsWith("http") ? link : "https://" + link;
                htmlOutput += QString("<p><a href=\"%1\">🔗 %1</a></p>").arg(fullUrl);
            }
            ui->textEdit->setHtml(htmlOutput);
        }
    });
}


void MainWindow::onLinkClicked(const QUrl &url)
{
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


void MainWindow::on_pushButton_2_clicked()
{
    // Сначала вызываем Python-функцию
    PythonManager::instance().callFunction("find_events", "main");

    // Читаем результат из файла events.txt
    QFile file("events.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Файл не открыт!";
        return;
    }

    QTextStream stream(&file);
    QString htmlContent;

    // Читаем файл построчно и формируем HTML-разметку
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QStringList parts = line.split('\t');  // Разделение по табуляции

        if (parts.size() == 2) {
            QString title = parts[0].trimmed();
            QString href = parts[1].trimmed();

            // Добавляем строку в HTML с кликабельной ссылкой
            htmlContent += QString("<p><strong>%1</strong> (<a href='%2'>перейти</a>)</p>\n").arg(title, href);
        }
    }

    file.close();

    // Устанавливаем HTML-разметку в виджет
    ui->textEdit->setHtml(htmlContent);
}
