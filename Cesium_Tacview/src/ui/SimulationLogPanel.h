#ifndef SIMULATIONLOGPANEL_H
#define SIMULATIONLOGPANEL_H

#include <QDockWidget>
#include <QPlainTextEdit>

class SimulationLogPanel : public QDockWidget
{
    Q_OBJECT
public:
    explicit SimulationLogPanel(QWidget *parent = nullptr);

public slots:
    void appendLog(const QString &msg);
    void clear();

private:
    QPlainTextEdit *m_logEdit;
};

#endif // SIMULATIONLOGPANEL_H
