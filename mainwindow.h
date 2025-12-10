#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QNetworkAccessManager>  // <-- обязательно для QNetworkAccessManager
#include <QNetworkReply>
#include <QNetworkRequest>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_lineEdit_textChanged(const QString &arg1);

    void on_pushButton_clearurl_clicked();
    void onLinkClicked(const QUrl &url);


    void on_urlField_textEdited(const QString &arg1);



    void on_playurl_clicked();

    void on_pushButton_2_clicked();

    void on_geturlpushButton_clicked();

private:
    Ui::MainWindow *ui;
    QString currentUrl;
    QNetworkAccessManager *manager;  // <-- поле класса
};

#endif // MAINWINDOW_H
