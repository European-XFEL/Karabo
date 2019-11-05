Developing in KaraboGUI
=======================

For developing in KaraboGUI using Qt5 we need to be inside a conda environment

After installing your miniconda3 distribution, install the package 
`conda-devenv` from the conda-forge channel:

    conda install conda-devenv -c conda-forge
    
conda-devenv is a tool for creating a development environment that always
follows the dependencies specified in your environment.devenv.yml, purging
any other dependencies left behind.

Now, on your `pythonGui` directory, run:

    conda devenv
    
Or, if you're not in the directory:

    conda devenv --file <pythonGUI dir>/environment.devenv.yml
    
This will solve your environment dependencies and create an environment
called `karabogui`. Call `conda activate karabogui` to activate it.

Now all the code from `karabogui`, `common` and `native` will be on 
your `PYTHONPATH`. No need to install using the setup.

Configuring the environment in PyCharm
--------------------------------------

If you're using PyCharm, it's usually better to spawn the IDE from your
already created environment, so it can correctly get the environment
variables.

Also, it might be needed to link the `python` executable from the IDE itself.
For that, to go `Settings -> Project: Framework -> Project Interpreter`. Go to
`Add Python Interpreter`, and configure it as the following:

![Configure Conda Interpreter](/docs/images/addinterpreter.png)

After applying, your IDE will index everything and you're good to go.

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
