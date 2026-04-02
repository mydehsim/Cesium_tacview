#ifndef COMMANDHISTORYPANEL_H
#define COMMANDHISTORYPANEL_H

#include <QDockWidget>
#include <QListWidget>

class CommandHistoryPanel : public QDockWidget
{
    Q_OBJECT
public:
    explicit CommandHistoryPanel(QWidget *parent = nullptr);

public slots:
    void addCommand(const QString &cmd);
    void clear();

private:
    QListWidget *m_list;
    static constexpr int MAX_ITEMS = 200;
};

#endif // COMMANDHISTORYPANEL_H
