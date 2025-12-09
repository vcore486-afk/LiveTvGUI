#pragma once
#include <QMainWindow>
#include <QUrl>

QT_BEGIN_NAMESPACE
namespace Ui { class PlayerWindow; }
class QWebEngineView;
QT_END_NAMESPACE

class PlayerWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit PlayerWindow(const QUrl &url, QWidget *parent = nullptr);
    ~PlayerWindow();

private:
    Ui::PlayerWindow *ui;
    QWebEngineView *webView;
};