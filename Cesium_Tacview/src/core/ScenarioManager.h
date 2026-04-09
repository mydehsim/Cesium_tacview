#ifndef SCENARIOMANAGER_H
#define SCENARIOMANAGER_H

#include <QObject>
#include <QString>

class AppState;
class TrackRecorder;

class ScenarioManager : public QObject
{
    Q_OBJECT
public:
    explicit ScenarioManager(QObject *parent = nullptr);

    // Save/Load scenario (.tacscen JSON)
    bool saveScenario(const QString &filePath, const AppState *state,
                      const TrackRecorder *recorder = nullptr);
    bool loadScenario(const QString &filePath, AppState *state);

    // Export/Import just the recording portion
    bool exportRecording(const QString &filePath, const TrackRecorder *recorder);
    bool importRecording(const QString &filePath, TrackRecorder *recorder);

    QString lastError() const { return m_lastError; }

signals:
    void logMessage(const QString &msg);

private:
    QString m_lastError;
};

#endif // SCENARIOMANAGER_H
