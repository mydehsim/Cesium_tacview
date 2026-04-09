#ifndef VITEPROCESS_H
#define VITEPROCESS_H

#include <QObject>
#include <QProcess>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

/// Manages the Vite dev server lifecycle (start → poll → ready → cleanup)
class ViteProcess : public QObject
{
    Q_OBJECT
public:
    explicit ViteProcess(const QString &webDir, QObject *parent = nullptr);
    ~ViteProcess() override;

    void start();
    void stop();
    bool isReady() const { return m_ready; }
    bool isRunning() const { return m_process && m_process->state() != QProcess::NotRunning; }

signals:
    void ready();
    void errorOccurred(const QString &msg);
    void logMessage(const QString &msg);

private slots:
    void pollServer();
    void onPollReply(QNetworkReply *reply);
    void onProcessError(QProcess::ProcessError error);

private:
    QString m_webDir;
    QProcess *m_process = nullptr;
    QTimer m_pollTimer;
    QNetworkAccessManager *m_nam = nullptr;
    bool m_ready = false;
    int m_pollAttempts = 0;
    static constexpr int MAX_POLL_ATTEMPTS = 180; // 90 seconds max wait
};

#endif // VITEPROCESS_H
