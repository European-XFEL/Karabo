GUI Refactor TODO:
==================

Tasks
-----

- [X] Port the Topology [LARGE]:
    - [X] ProjectDevice
    - [X] SystemTopology
    - [X] SystemTree

- [X] Port Core Singletons [LARGE]:
    - [X] Port Manager
    - [X] Port Network

- [X] Port the Navigation Panel [SMALL]
    - [X] Small fixups in NavigationModel

- [X] Port the Project View [SMALL]:
    - [X] Port DeviceInstanceController
    - [X] Port MacroController

- [X] Move the Widget Controllers

- [X] Port the Scene [LARGE]:
    - [X] Port BaseWidgetContainer into WidgetContainer
    - [X] Remove the widget wrappers!
    - [X] Port scene building
        - [X] Use the binding controller registry instead of boilerplate mappings
    - [X] Port ConfigurationDropHandler
    - [X] Port BoxSelectionTool
    - [X] Port WidgetSceneHandler (widget selection menu)
    - [X] Port NavigationDropHandler
    - [X] Port the Workflow (pipeline connections)
        - [X] Models
        - [X] Rendering
        - [X] [dis]connect_channels (WorkflowConnectionTool)
    - [X] Small fixups in SceneView
    - [ ] EXTRA: Introduce a DeprecatedWidget
        - [ ] Remove Monitor
        - [ ] Remove Knob

- [X] Port the Configurator [LARGE]:
    - [X] Port ConfigurationTreeModel
    - [X] Port TableEditDialog
    - [X] Port EditWidgetWrapper
    - [X] Port SlotButtonDelegate
    - [X] Fix ConfigurationTreeView

- [X] Misc. Small Tasks [SMALL]:
    - [X] Remove RunConfiguratorPanel
    - [X] Port DeviceScenesDialog
    - [X] Port MacroPanel
    - [X] Port ScriptingPanel
      - [X] Port IPythonWidget

Wrap-up
-------

- [ ] Integrate and Test
- [ ] Party!


Interaction with the System Topology:
-------------------------------------

This is a rough outline of how I see this working in the GUI client codebase:

* The system topology will keep a collection of `Schema` objects for each
  device and device class.

  - Similarly, a `Hash` configuration should be kept for all device instances.

* When some higher-level code needs a data binding, one should be created from
  the `Schema` which is in the topology. The data binding should go into a map
  so that the same binding is always used for a given device instance or class.
* While the user is interacting with a binding, any changes are stored in the
  `value` traits of the binding. If an associated device gets a configuration
  update which conflicts with a user modification, it should _not_ overwrite
  value stored in the binding.

  - Later after some user interaction, the value can be pulled from the cached
    configuration of the device instance.

* For project devices, the `ProjectDeviceInstance` class should keep a copy of
  the offline configuration.

  - `ProjectDeviceInstance` should have a single `DeviceProxy` object which it
    swaps the schema of (between the device instance and device class) when
    switching between online and offline.
  - `ProjectDeviceInstance` should continue to manage the `status` of its proxy

* The system topology should manage the `status` of proxies for normal devices
