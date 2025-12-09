..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _installation/:

*******************
 Conda GUI client
*******************

KaraboGUI can now be installed in Windows, MacOS, and Ubuntu via ``conda``.
Conda is a cross-platform package manager and dependency resolution tool which offers a
way to install python packages and its dependencies.

In order to use Conda, three basic steps are needed:
    1. Install Conda (through a `recommended conda installer`_);
    2. Configure its channels;
    3. Install the desired package

Install Conda
=============

The following steps are needed in order to obtain Conda.

Download and install miniforge (Python version >= 3).

#. Download the installer script/executable.

   * Linux and Mac:
      Download and install by running

      .. code-block:: bash

         curl -L -O "https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-$(uname)-$(uname -m).sh"
         bash Miniforge3-$(uname)-$(uname -m).sh

   * Windows:
      Download `the installer
      <https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Windows-x86_64.exe>`_

      Double-click on ``Miniforge3-Windows-x86_64.exe`` to run the installer.



#. Follow the installation prompt. On Windows, remember to keep the "Create start menu shortcuts" option selected (by default, selected), in the final step.

#. Open your terminal (Miniforge3 Prompt on Windows or Bash on Linux and MacOS)

#. If conda is in your path, you should be able to run **conda --version**

    * If it isn't, you need to activate conda first

        * **Linux/MacOS**: ``source <miniforge_path>/etc/profile.d/conda.sh``
        * **Win**: ``CALL <miniforge_path>/condabin/activate.bat``

If you find errors such as "Your shell has not been properly configured to use 'conda activate'", run the following
command:

    . <your miniforge3 path>/etc/profile.d/conda.sh

Or add it to your ``.bashrc`` (or similar).

Channel Configuration
=====================

The Conda packages of Karabo are currently only available in an internal conda channel
and a few configuration steps are needed for `Conda` to find the right package.

Also, please be aware of the licenses issues of the various channels as highlighted
in this link `recommended conda installer`_
and `here <https://mamba.readthedocs.io/en/latest/user_guide/troubleshooting.html#defaults-channels>`__.

For KaraboGUI, some package channels are needed besides Conda's defaults. You
only need to do this once and it can be done either from command line or
editting Conda's configuration file (``.condarc``).
You can find your configuration file location typing ``conda info`` on your terminal.

From your terminal, add the needed channels executing the following commands::

    conda config --add channels http://exflctrl01.desy.de/karabo/channel
    conda config --add channels http://exflctrl01.desy.de/karabo/channel/mirror/conda-forge


Installing KaraboGUI
====================

.. _framework/xfel_installation:

The steps shown here will work from either the XFEL office network
or using a VPN. In case one is using a connection to an external network
or a guest network, follow the steps here :ref:`framework/remote_installation`.

When using ``conda`` one should preferrably not alter the base environment that
comes with it, therefore we should install the GUI in a separate environment.
If needed, one can safely remove this environment with no fear with corrupting
the base environment.

* Run this to create an environement pointing to the latest version (recommended)
    * ``conda create -n karabogui_latest karabogui --yes``
  To upgrade this environement, run these commands:
    * ``conda env remove -n karabogui_latest``
    * ``conda create -n karabogui_latest karabogui --yes``

* Or create a target environment for KaraboGUI pointing to a specific version:
    * ``conda create -n karabogui<version> karabogui=<version> --yes``

Helpful commands are available below:

* ``conda search karabogui`` will show you all the available versions in the channels
  you have configured

* ``conda env remove -n your_karabo_environment`` will remove the environment called ``your_karabo_environment``.

.. _framework/remote_installation:

Installation using dynamic SSH tunneling (SOCKS5)
--------------------------------------------------------

In case the internal conda channels are not directly reacheable
in the network, one can set up a dynamic tunneling with the
following command in a terminal (Linux and MacOS)::

    ssh <user>@bastion.desy.de -D 9090

For Windows, a similar tunnelling configuration can be achieved
with the PuTTY application.

After the connection is established, ``conda`` needs to be
configured to use the socks5 proxy server. In a new terminal execute::

    conda config --set proxy_servers.http socks5://localhost:9090

please note that: at the European XFEL, the GUI will needs to access 

Running KaraboGUI
=================

After successfully installing KaraboGUI, you will have access to the following entry-points:
    * karabo-gui;
    * karabo-cinema;
    * karabo-theatre;
    * karabo-update-extensions

From now on, all you need to do to run KaraboGUI is:
    * Open your terminal/prompt
    * ``conda activate <your_karabo_environment>``
    * ``karabo-gui``

in case you are connecting from an external network such a the one explained here
:ref:`framework/remote_installation`, you need to configure the GUI to proxy the
network connections with:
    * Open your terminal/prompt
    * ``conda activate <your_karabo_environment>``
    * ``https_proxy=socks5://localhost:9090 karabo-gui``

Upgrading
=========

When updating the KaraboGUI, it is recommended to install a clean environment.
Different versions of the KaraboGUI will be built against different dependencies.
For this reason, the safest way to upgrade is to either:
* install a new environment with a different environment name, or
* delete it with ``conda env remove -n <environment_name>``, and install as new.

Uninstalling
============

In order to uninstall KaraboGUI, always opt for removing the complete environment
itself: ``conda env remove -n <environment_name>``

Developing
==========

For developing in KaraboGUI using Qt5 we need to be inside a conda environment

After installing your miniforge3 distribution, install the package
``conda-devenv`` from the conda-forge channel::

    conda install conda-devenv -c conda-forge

``conda-devenv`` is a tool for creating a development environment that always
follows the dependencies specified in your environment.devenv.yml, purging
any other dependencies left behind.

If you are working outside the DESY network, use the second option
(using ``conda config --set proxy_servers.http ...`` ) mentioned
in :ref:`framework/remote_installation`.

From your Framework repository, you can now run the following command::

    conda devenv --file conda-recipes/karabogui/environment.devenv.yml

This will solve your environment dependencies and create an environment
called ``karabogui``. To change the name of the created Conda environment, you need to update the 'name' variable in the Framework/conda-recipes/karabogui/environment.devenv.yml file.

To activate conda environment call::

  conda activate karabogui

Still, the Karabo code has to be installed::

  cd src/pythonKarabo
  pip install -e . --no-deps
  cd ../../
  cd src/pythonGui
  pip install -e . --no-deps

Now all the code from ``karabogui``, ``common`` and ``native`` will be on
your ``PYTHONPATH``.

The last step (``pip install -e . --no-deps`` inside the activated conda
environment) should be repeated from time to time to keep the version
number up-to-date.


Configuring the environment in PyCharm
--------------------------------------

If you're using PyCharm, it's usually better to spawn the IDE from your
already created environment, so it can correctly get the environment
variables.

Also, it might be needed to link the `python` executable from the IDE itself.
For that, to go `Settings -> Project: Framework -> Project Interpreter`. Go to
`Add Python Interpreter`, and configure it as the following:

![Configure Conda Interpreter](./images/addinterpreter.png)

After applying, your IDE will index everything and you're good to go.

.. _recommended conda installer: https://docs.desy.de/maxwell/documentation/licensing/conda_terms/
