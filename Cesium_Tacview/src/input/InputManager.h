#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>
#include <QSet>
#include "simulation/ManualController.h"

class AppState;
class SimulationEngine;
class CesiumBridge;

class InputManager : public QObject
{
    Q_OBJECT
public:
    explicit InputManager(AppState *appState, SimulationEngine *engine,
                          CesiumBridge *bridge, QObject *parent = nullptr);

    void keyPressed(int key);
    void keyReleased(int key);

    InputState currentInputState() const;

signals:
    void createAircraftRequested();
    void deleteAircraftRequested();
    void logMessage(const QString &msg);

private:
    void handleAction(int key);
    void selectNextAircraft();

    AppState *m_appState;
    SimulationEngine *m_engine;
    CesiumBridge *m_bridge;
    QSet<int> m_pressedKeys;
};

#endif // INPUTMANAGER_H
