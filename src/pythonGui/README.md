GUI Refactor TODO:
==================

Tasks
-----

- [ ] Port the Configurator [LARGE]:
    - [ ] Port ConfigurationTreeModel
    - [ ] Port TableEditDialog
    - [ ] Port EditWidgetWrapper
    - [ ] Port SlotButtonDelegate
    - [ ] Fix ConfigurationTreeView

- [ ] Port the Project View [SMALL]:
    - [ ] Port DeviceInstanceController
    - [ ] Port MacroController

- [ ] Port the Scene [LARGE]:
    - [ ] Port BaseWidgetContainer into WidgetContainer (refactor/scene-base branch)
    - [ ] Remove the widget wrappers!
    - [ ] Port scene building
        - [ ] Use the binding controller registry instead of boilerplate mappings
    - [ ] Port ConfigurationDropHandler
    - [ ] Port BoxSelectionTool
    - [ ] Port WidgetSceneHandler (widget selection menu)
    - [ ] Port the Workflow (pipeline connections)
        - [ ] Model
        - [ ] [dis]connect_channels
    - [ ] Small fixups in SceneView
    - [ ] EXTRA: Introduce a DeprecatedWidget
        - [ ] Remove Monitor
        - [ ] Remove Knob

- [ ] Port Singletons [LARGE]: (refactor/box-cutter branch)
    - [ ] Port Manager (mostly done)
    - [ ] Port Network (mostly done)
    - [ ] Small fixups in NavigationModel

- [ ] Port the Topology [LARGE]: (refactor/box-cutter branch)
    - [ ] ProjectDevice (mostly done)
    - [ ] SystemTopology (mostly done)
    - [ ] SystemTree (mostly done)

- [ ] Misc. Small Conversions [SMALL]:
    - [X] Port DeviceScenesDialog (refactor/scene-dialog branch)
    - [X] Remove RunConfiguratorPanel
    - [ ] Port MacroPanel

Wrap-up
-------

- [ ] Delete Box/Configuration
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
