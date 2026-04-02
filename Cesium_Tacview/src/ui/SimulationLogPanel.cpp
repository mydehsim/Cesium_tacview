#include "SimulationLogPanel.h"
#include <QVBoxLayout>
#include <QDateTime>

SimulationLogPanel::SimulationLogPanel(QWidget *parent)
    : QDockWidget(tr("Simulation Log"), parent)
{
    setAllowedAreas(Qt::AllDockWidgetAreas);

    m_logEdit = new QPlainTextEdit;
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(2000);
    m_logEdit->setFont(QFont(QStringLiteral("Consolas"), 9));
    setWidget(m_logEdit);
}

void SimulationLogPanel::appendLog(const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));
    m_logEdit->appendPlainText(QStringLiteral("[%1] %2").arg(timestamp, msg));
}

void SimulationLogPanel::clear()
{
    m_logEdit->clear();
}
