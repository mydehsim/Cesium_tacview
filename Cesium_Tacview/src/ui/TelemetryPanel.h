#ifndef TELEMETRYPANEL_H
#define TELEMETRYPANEL_H

#include <QWidget>
#include <QVector>

class QLabel;
class QProgressBar;
class AppState;

class TelemetryPanel : public QWidget
{
    Q_OBJECT
public:
    explicit TelemetryPanel(QWidget *parent = nullptr);

    void update(const AppState *state);
    void clear();

private:
    void setupUi();
    QWidget *createGauge(const QString &label, QProgressBar *&bar, QLabel *&valLabel);

    QLabel *m_titleLabel;

    QProgressBar *m_altBar;
    QProgressBar *m_speedBar;
    QProgressBar *m_headingBar;
    QProgressBar *m_gLoadBar;
    QProgressBar *m_vsBar;
    QProgressBar *m_rollBar;

    QLabel *m_altVal;
    QLabel *m_speedVal;
    QLabel *m_headingVal;
    QLabel *m_gLoadVal;
    QLabel *m_vsVal;
    QLabel *m_rollVal;

    // Mode / control info
    QLabel *m_modeLabel;
    QLabel *m_routeLabel;
    QLabel *m_posLabel;
};

#endif // TELEMETRYPANEL_H
