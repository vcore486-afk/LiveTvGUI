#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QDebug>
#include <QDesktopServices>

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
    if (url.isValid()) {
        QDesktopServices::openUrl(url);
    }
}


void MainWindow::on_pushButton_clearurl_clicked()
{
    ui->lineEdit->clear();                    // вот твоя очистка URL
    ui->lineEdit->setPlaceholderText("Введите URL...");
    ui->lineEdit->setFocus();


}



