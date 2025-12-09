#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , manager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
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
        ui->textEdit->setPlainText("URL пустой!");
        return;
    }

    // Создаём запрос
    QNetworkRequest request{QUrl(currentUrl)};

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->textEdit->setPlainText("Ошибка загрузки страницы: " + reply->errorString());
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

        // Выводим результат или сообщение, если ничего не найдено
        if (results.isEmpty()) {
            ui->textEdit->setPlainText("Ссылок cdn.livetv869.me/webplayer.php и webplayer2.php не найдено.");
        } else {
            ui->textEdit->setPlainText(results.join("\n"));
        }
    });
}





void MainWindow::on_pushButton_clearurl_clicked()
{
    ui->lineEdit->clear();                    // вот твоя очистка URL
    ui->lineEdit->setPlaceholderText("Введите URL...");
    ui->lineEdit->setFocus();


}



