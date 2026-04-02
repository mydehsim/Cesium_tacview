#include "CommandHistoryPanel.h"
#include <QVBoxLayout>
#include <QDateTime>

CommandHistoryPanel::CommandHistoryPanel(QWidget *parent)
    : QDockWidget(tr("Command History"), parent)
{
    setAllowedAreas(Qt::AllDockWidgetAreas);

    m_list = new QListWidget;
    m_list->setFont(QFont(QStringLiteral("Consolas"), 9));
    setWidget(m_list);
}

void CommandHistoryPanel::addCommand(const QString &cmd)
{
    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
    m_list->insertItem(0, QStringLiteral("[%1] %2").arg(timestamp, cmd));

    while (m_list->count() > MAX_ITEMS)
        delete m_list->takeItem(m_list->count() - 1);
}

void CommandHistoryPanel::clear()
{
    m_list->clear();
}
