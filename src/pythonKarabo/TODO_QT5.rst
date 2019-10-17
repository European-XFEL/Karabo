GUI Refactor Qt5:
=================

The Qt5 refactor should derive on a single branch providing the CONDA environment
for Qt5. All merges are done on that development branch.
At the end of the process, the single branch is merged into master.

At first all the imports should be checked against Qt5 in this order.

Tasks
-----

- [] Port the Topology:
    - [] ProjectDevice
    - [] SystemTopology
    - [] SystemTree

- [] Port Core Singletons:
    - [] Port Manager
    - [] Port Network
    - [] Port PanelWrangler

- [] Port the Navigation Panels
    - [] Port DeviceView
    - [] Port SystemView

- [] Port the Project View:
    - [] Port DeviceInstanceController
    - [] Port MacroController
    - [] Port DeviceConfigurationController
    - [] Port ServerController

- [] Port the Dialogs:
    - [] Check tons of dialogs, text dialog, etc.

- [] Port the Dialogs:
    - [] Check tons of dialogs, text dialog, etc.

- [] Move the Widget Controllers [LARGE]
    - [] Port DisplayWidgets
    - [] Port Editable Widgets
    - [] Port Tests

- [] Move the graph package
    - [] Port homebrew dialogs
    - [] Port Image Widgets Base
    - [] Port Plots Widgets Base
    - [] Port Tests

- [] Port the Alarms:
    - [] Check FilterModel

- [] Port the Scene:
    - [] Port BaseContainers
    - [] Port ConfigurationDropHandler
    - [] Port Layouts
    - [] Port WidgetSceneHandler (widget selection menu)
    - [] Port NavigationDropHandler
    - [] Port LinkWidgets
    - [] Port the Workflow
    - [] Small fixups in SceneView

- [X] Port the Configurator:
    - [] Port ConfigurationTreeModel
    - [] Port TableEditDialog
    - [] Port EditWidgetWrapper
    - [] Port SlotButtonDelegate
    - [] Fix ConfigurationTreeView

- [] Misc. Small Tasks [SMALL]:
    - [] Port DeviceScenesDialog
    - [] Port MacroPanel
    - [] Port ScriptingPanel
    - [] Port IPythonWidget

- [] Merge to master
- [] Update global dependencies and check build scripts
