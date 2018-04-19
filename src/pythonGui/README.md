

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
