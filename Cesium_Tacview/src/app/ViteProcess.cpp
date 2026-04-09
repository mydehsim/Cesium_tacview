#include "ViteProcess.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDir>
#include <QDebug>

ViteProcess::ViteProcess(const QString &webDir, QObject *parent)
    : QObject(parent), m_webDir(webDir)
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished, this, &ViteProcess::onPollReply);
    connect(&m_pollTimer, &QTimer::timeout, this, &ViteProcess::pollServer);
}

ViteProcess::~ViteProcess()
{
    stop();
}

void ViteProcess::start()
{
    if (m_ready)
        return;

    // First check if Vite is already running (user may have started it manually)
    m_pollAttempts = 0;
    pollServer();

    // If already running, pollServer will detect it. Otherwise, start the process.
    QTimer::singleShot(800, this, [this]()
                       {
        if (m_ready) return; // Already detected running server

        if (m_process) return; // Already starting

        m_process = new QProcess(this);
        m_process->setWorkingDirectory(m_webDir);
        m_process->setProcessChannelMode(QProcess::MergedChannels);

        connect(m_process, &QProcess::errorOccurred, this, &ViteProcess::onProcessError);

        connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
            QString output = QString::fromUtf8(m_process->readAll());
            for (const QString &line : output.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
                emit logMessage(QStringLiteral("[Vite] %1").arg(line.trimmed()));
            }
        });

        // Start: npm run dev
#ifdef Q_OS_WIN
        m_process->start(QStringLiteral("cmd.exe"),
                         {QStringLiteral("/c"), QStringLiteral("npm"), QStringLiteral("run"), QStringLiteral("dev")});
#else
        m_process->start(QStringLiteral("npm"), {QStringLiteral("run"), QStringLiteral("dev")});
#endif

        emit logMessage(QStringLiteral("[ViteProcess] Starting npm run dev in %1").arg(m_webDir));

        // Start polling every 500ms
        m_pollTimer.start(500); });
}

void ViteProcess::stop()
{
    m_pollTimer.stop();

    if (m_process)
    {
        if (m_process->state() != QProcess::NotRunning)
        {
#ifdef Q_OS_WIN
            // On Windows, QProcess::kill won't kill child node processes.
            // Use taskkill /T to terminate the whole process tree.
            QProcess::execute(QStringLiteral("taskkill"),
                              {QStringLiteral("/F"), QStringLiteral("/T"),
                               QStringLiteral("/PID"), QString::number(m_process->processId())});
#else
            m_process->terminate();
            if (!m_process->waitForFinished(3000))
                m_process->kill();
#endif
        }
        m_process->deleteLater();
        m_process = nullptr;
        emit logMessage(QStringLiteral("[ViteProcess] Stopped"));
    }

    m_ready = false;
}

void ViteProcess::pollServer()
{
    m_pollAttempts++;

    if (m_pollAttempts > MAX_POLL_ATTEMPTS)
    {
        m_pollTimer.stop();
        emit errorOccurred(QStringLiteral("Vite dev server did not start within %1 seconds")
                               .arg(MAX_POLL_ATTEMPTS / 2));
        return;
    }

    QNetworkRequest req(QUrl(QStringLiteral("http://localhost:5173/index-qt.html")));
    req.setTransferTimeout(2000);
    m_nam->get(req);
}

void ViteProcess::onPollReply(QNetworkReply *reply)
{
    reply->deleteLater();

    if (m_ready)
        return;

    if (reply->error() == QNetworkReply::NoError)
    {
        m_ready = true;
        m_pollTimer.stop();
        emit logMessage(QStringLiteral("[ViteProcess] Server ready at localhost:5173"));
        emit ready();
    }
}

void ViteProcess::onProcessError(QProcess::ProcessError error)
{
    QString msg;
    switch (error)
    {
    case QProcess::FailedToStart:
        msg = QStringLiteral("Failed to start npm — is Node.js installed and in PATH?");
        break;
    case QProcess::Crashed:
        msg = QStringLiteral("Vite process crashed");
        break;
    default:
        msg = QStringLiteral("Vite process error: %1").arg(error);
        break;
    }
    emit errorOccurred(msg);
    emit logMessage(QStringLiteral("[ViteProcess] ERROR: %1").arg(msg));
}
