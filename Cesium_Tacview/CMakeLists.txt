cmake_minimum_required(VERSION 3.16)

project(Cesium_Tacview VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    WebEngineWidgets
    WebChannel
    Network
)

set(PROJECT_SOURCES
    # App
    src/main.cpp
    src/app/MainWindow.h       src/app/MainWindow.cpp
    src/app/MapWindow.h        src/app/MapWindow.cpp
    src/app/DebugWindow.h      src/app/DebugWindow.cpp
    src/app/AppState.h         src/app/AppState.cpp
    src/app/ViteProcess.h      src/app/ViteProcess.cpp

    # Core
    src/core/AircraftState.h
    src/core/AircraftManager.h src/core/AircraftManager.cpp
    src/core/RouteState.h
    src/core/RouteManager.h    src/core/RouteManager.cpp
    src/core/SelectionManager.h src/core/SelectionManager.cpp
    src/core/CommandTypes.h
    src/core/ScenarioManager.h   src/core/ScenarioManager.cpp

    # Simulation
    src/simulation/SimulationEngine.h  src/simulation/SimulationEngine.cpp
    src/simulation/SimulationClock.h   src/simulation/SimulationClock.cpp
    src/simulation/KinematicModel.h    src/simulation/KinematicModel.cpp
    src/simulation/ManualController.h  src/simulation/ManualController.cpp
    src/simulation/AutopilotController.h src/simulation/AutopilotController.cpp
    src/simulation/TrackRecorder.h  src/simulation/TrackRecorder.cpp
    src/simulation/PlaybackEngine.h src/simulation/PlaybackEngine.cpp

    # Bridge
    src/bridge/CesiumBridge.h  src/bridge/CesiumBridge.cpp
    src/bridge/StateSerializer.h src/bridge/StateSerializer.cpp
    src/bridge/EventParser.h   src/bridge/EventParser.cpp

    # UI Panels
    src/ui/AircraftListPanel.h  src/ui/AircraftListPanel.cpp
    src/ui/AircraftInspector.h  src/ui/AircraftInspector.cpp
    src/ui/RouteEditorPanel.h   src/ui/RouteEditorPanel.cpp
    src/ui/SimulationLogPanel.h src/ui/SimulationLogPanel.cpp
    src/ui/CommandHistoryPanel.h src/ui/CommandHistoryPanel.cpp
    src/ui/MapToolbar.h         src/ui/MapToolbar.cpp
    src/ui/PlaybackControlPanel.h src/ui/PlaybackControlPanel.cpp
    src/ui/TelemetryPanel.h      src/ui/TelemetryPanel.cpp

    # Input
    src/input/InputManager.h   src/input/InputManager.cpp
    src/input/KeyBindings.h

    # Protocols
    src/protocols/IProtocolAdapter.h
    src/protocols/TacviewAdapter.h  src/protocols/TacviewAdapter.cpp
)

qt_add_executable(Cesium_Tacview
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_include_directories(Cesium_Tacview PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Cesium_Tacview PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    Qt6::Network
)

# Web content path passed to C++ as compile definition
target_compile_definitions(Cesium_Tacview PRIVATE
    WEB_CONTENT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../cesium_web"
)

set_target_properties(Cesium_Tacview PROPERTIES
    WIN32_EXECUTABLE TRUE
)

install(TARGETS Cesium_Tacview
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

qt_finalize_executable(Cesium_Tacview)
