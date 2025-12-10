// playerwindow.h
#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include <QMainWindow>
#include <QUrl>

// Обязательные заголовки для QWebEngine в Qt6
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineFullScreenRequest>

QT_BEGIN_NAMESPACE
namespace Ui { class PlayerWindow; }
QT_END_NAMESPACE

// ===================================================================
// КАСТОМНАЯ СТРАНИЦА — ОБЯЗАТЕЛЬНО В .h, чтобы moc её обработал!
// ===================================================================
class DebugPage : public QWebEnginePage
{
    Q_OBJECT

public:
    explicit DebugPage(QWebEngineProfile *profile, QObject *parent = nullptr)
        : QWebEnginePage(profile, parent)
    {}

protected:
    // Перехват console.log от StreamDetector (работает в Qt6!)
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString &message,
                                  int lineNumber,
                                  const QString &sourceID) override
    {
        Q_UNUSED(level);
        Q_UNUSED(lineNumber);
        Q_UNUSED(sourceID);

        // Фильтруем только нужные сообщения от детектора
        if (message.contains("StreamDetector", Qt::CaseInsensitive) ||
            message.contains("m3u8", Qt::CaseInsensitive) ||
            message.contains("mpd", Qt::CaseInsensitive) ||
            message.contains("Detected", Qt::CaseInsensitive)) {
            qDebug().noquote() << "StreamDetector →" << message;
        }
    }
};

// ===================================================================
// Основное окно плеера
// ===================================================================
class PlayerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PlayerWindow(const QUrl &url, QWidget *parent = nullptr);
    ~PlayerWindow() override;

private:
    Ui::PlayerWindow *ui{nullptr};
    class QWebEngineView *webView{nullptr};  // будет создан в конструкторе
};

#endif // PLAYERWINDOW_H