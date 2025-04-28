# Adding Functionality to Karabo using Devices

The device is the core concept of Karabo. Basically, Karabo consists of a set
of devices running somewhere on the network. Every device is an instance of an
object-oriented class which implements the device interface and logic.
Each device serves one logical and encapsulated service, i.e. 1 device = 1
service.

## Device Development

The PyPi installable packages are not intended to facilitate development of
devices for Karabo's C++ API. If you need to develop C++ devices, consider
creating a self-consistent development  environment using the instructions
at https://github.com/European-XFEL/Karabo.

Development of devices against these packages using the Bound and Karathon
Python APIs should be trivially possible, as long as the device packages
define the correct entrypoints.

For the Karathon API, a line similar to this one needs to exist in
your `pyproject.toml`:

```toml
[project.entry-points."karabo.middlelayer_device"]
PropertyTestMDL = "karabo.middlelayer_devices.property_test:PropertyTestMDL"
```

For the Bound API, a line similar to this one should exist in
your `pyproject.toml`.

```toml
[project.entry-points."karabo.bound_device"]
PropertyTest = "karabo.bound_devices.property_test:PropertyTest"
```

> [!TIP]
> Only use imports from the top-level `karabo.middlelayer` and `karabo.bound`
> packages. The API of subpackages, such as `karabo.middlelayer.proxy` is
> not guaranteed to be stable.
