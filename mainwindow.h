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
    void callPythonScript(const QString &resourcePath);

private slots:

    void on_lineEdit_textChanged(const QString &arg1);

    void on_pushButton_clearurl_clicked();
    void onLinkClicked(const QUrl &url);
    void onLinkHovered(const QString &link);
    void geturlpushButton(const QUrl &url); // Аргумент QUrl
    void on_urlField_textEdited(const QString &arg1);

    bool eventFilter(QObject *obj, QEvent *event) override;
    void on_playurl_clicked();
    void getplayerurl(const QString &url);

    void on_pushButton_2_clicked();

    void on_geturlpushButton_clicked();
    void processEvents(const QString &tournamentName, int pageNumber);
    void loadTopMatches(int pageNumber);
    void on_parserel_clicked();

    void on_parseruefa_clicked();

    void on_parsernhl_clicked();

    void on_parserbundesliga_clicked();

    void on_turkishliga_clicked();

    void on_matchday_clicked();

    void on_parserhockey_clicked();

    void on_parsebasketball_clicked();

private:
    Ui::MainWindow *ui;
    QString currentUrl;

    QString readLivetvDomainFromConfig();  // ← добавьте сюда
    QNetworkAccessManager *manager;
    int rpcId = 1;

    void sendJsonRpc(
        const QJsonObject &json,
        const QString &desc,
        std::function<void(const QJsonObject&)> onSuccess = nullptr
    );

};

#endif // MAINWINDOW_H
