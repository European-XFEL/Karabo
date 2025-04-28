# Installation

There are multiple installation options for Karabo. A full system will
require platform-compiled code and is currently only supported on x64
linux flavours with a reasonably modern (2.28+) `glibc`

## Standalone and Full

If you would like to try Karabo in a standalone fashion on a single machine
it's easiest to just run:

```bash
pip install karabo[full]
```

This will install a complete setup, including all essential service devices.
Note that this requires, compiled code to be installed. This will be done for
wheels on supported architectures, and a source compilate will be attempted
otherwise.

If that fails, we recommend you follow the instructions at
https://github.com/European-XFEL/Karabo which will result in a self-consistent
environment,  including a Python interpreter, with
similar functionality to a `karabo[full]` installation.

If you require the GUI, you will need to install it separately into a different
Pythonenvironment, as it does not require it's dependencies to align outside
those of `karabo.native`:

```bash
pip install karabo.gui
```

After installation you should initialize the environment with either

```bash
karabo-activate --init-to PATH --standalone
```

for a standalone system, or


```bash
karabo-activate --init-to PATH [--backbone] --broker-host amqp://{broker:port} --broker-topic {TOPIC}
```

for headless full installations. The `--backbone` option sets the core services up, and the `--broker-*`
parameters are used to configure the installation according to your AMQP broker setup.

## Additional options

- `karabo[karathon]` will install the Karathon Python API
- `karabo[cpp]` will install the Karabo C++ API
- `karabo[bound]` will install the Karabo Python bindings to C++
- `karabo[services]` will install service tools, allowing to manage installations with `karabo.daemontools`

Options can be combined, such as `karabo[karathon,services]`, which will result in a Karathon installation
with services tools installed.

To install the Karabo GUI, please use

```bash
pip install karabo.gui
```

## Starting a Karabo standalone system

If you wish to run Karabo standalone you can do this if you have `podman` or `docker` installed on the system.

First, run

```bash
karabo-activate --init-to $PATH --standalone
```

Then run

```bash
podman-compose -f $PATH/var/containers/compose.yaml up
```

or (`docker-compose ...`). This will start containerized version of the services a full
Karabo installation requires:

- A RabbitMQ broker (user: xfel, pw: karabo)
- An Influx database instance for logging (user: infadm, pw: admpwd)
- An ExistDB database as project database (user: admin, pw: change_me_please)
- A Grafana installation with the Influx database as a pre-provisioned source (user: admin, pw: admin)

Note that the Grafana provisioning assumes `karabo` as the broker topic in use. If you've
used `karabo-activate --broker-topic TOPIC` you'll need to edit the data source to refer to
a database of `TOPIC` from inside Grafana.

Finally, run

```bash
$PATH/activate
```

followed by

```bash
karabo-start
```

# Device Development

These packages are not intended to facilitate development of devices for Karabo's C++ API. Please
create a self-consistent development environment using the instructions at https://github.com/European-XFEL/Karabo.

Development of devices against these packages using the Bound and Karathon Python APIs should be trivially
possibly, as long as the device packages define the correct entrypoints.

For the Karathon API, a line similar to this one should exist in your `pyproject.toml`:

```toml
[project.entry-points."karabo.middlelayer_device"]
PropertyTestMDL = "karabo.middlelayer_devices.property_test:PropertyTestMDL"
```

For the Bound API, a line similar to this one should exist in your `pyproject.toml`.

```toml
[project.entry-points."karabo.bound_device"]
PropertyTest = "karabo.bound_devices.property_test:PropertyTest"
```
