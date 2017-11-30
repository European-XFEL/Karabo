GUI Refactor TODO:
==================

Tasks
-----

- [X] Port the Topology [LARGE]:
    - [X] ProjectDevice
    - [X] SystemTopology
    - [X] SystemTree

- [X] Port Core Singletons [LARGE]:
    - [X] Port Manager (mostly done)
    - [X] Port Network (mostly done)

- [X] Port the Navigation Panel [SMALL]
    - [X] Small fixups in NavigationModel

- [ ] Port the Project View [SMALL]:
    - [ ] Port DeviceInstanceController
    - [ ] Port MacroController

- [X] Move the Widget Controllers

- [ ] Port the Scene [LARGE]:
    - [X] Port BaseWidgetContainer into WidgetContainer (started; refactor/scene-base branch)
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

- [ ] Port the Configurator [LARGE]: (started; refactor/configurator branch)
    - [ ] Port ConfigurationTreeModel
    - [ ] Port TableEditDialog
    - [ ] Port EditWidgetWrapper
    - [ ] Port SlotButtonDelegate
    - [ ] Fix ConfigurationTreeView

- [ ] Misc. Small Tasks [SMALL]:
    - [X] Remove RunConfiguratorPanel
    - [X] Port DeviceScenesDialog (staged; refactor/scene-dialog branch)
    - [ ] Port MacroPanel

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
