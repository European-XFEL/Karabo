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
    1. Install Conda (through a `recommended conda installer <https://docs.desy.de/maxwell/documentation/licensing/conda_terms/>`_);
    2. Configure its channels;
    3. Install the desired package

Installing Conda
================

The following steps are needed in order to obtain Conda.

* Download and install miniconda (Python version >= 3) using the provided script `here <https://github.com/conda-forge/miniforge>`_,
  available for Linux, Mac, and Windows, to simplify the process and avoid manual downloads
* Open your terminal (Miniforge3 Prompt on Windows or Bash on Linux and MacOS)
* If conda is in your path, you should be able to run **conda --version**
    * If it isn't, you need to activate conda first
        * **Linux/MacOS**: ``source <miniconda_path>/etc/profile.d/conda.sh``
        * **Win**: ``CALL <miniconda_path>/condabin/activate.bat``

If you find errors such as "Your shell has not been properly configured to use 'conda activate'", run the following
command:

    . <your miniconda3 path>/etc/profile.d/conda.sh

Or add it to your ``.bashrc`` (or similar).

Channel Configuration
=====================

The Conda packages of Karabo are currently only available in an internal conda channel
and a few configuration steps are needed for `Conda` to find the right package.

Also, please be aware of the licenses issues of the various channels as highlighted
`here <https://docs.desy.de/maxwell/documentation/licensing/conda_terms/>`_
and `here <https://mamba.readthedocs.io/en/latest/user_guide/troubleshooting.html#defaults-channels>`_.

.. _framework/xfel_installation:

Installations in the XFEL network or using the VPN (recommended)
----------------------------------------------------------------

For KaraboGUI, some package channels are needed besides Conda's defaults. You
only need to do this once and it can be done either from command line or
editting Conda's configuration file (``.condarc``).
You can find your configuration file location typing ``conda info`` on your terminal.

From your terminal, add the needed channels executing the following commands::

    conda config --add channels http://exflctrl01.desy.de/karabo/channel
    conda config --add channels http://exflctrl01.desy.de/karabo/channel/mirror/conda-forge

.. _framework/remote_installation:

Remote installations using SSH tunneling
----------------------------------------

The channel ``http://exflctrl01.desy.de/karabo/channel`` is not reachable outside the
XFEL office network. For this reason if one is installing from a network outside
the DESY internal network and not using VPN, the connection can be established with
an SSH tunnel.

In order to create the tunnel to ``exflctrl01``, one can open a terminal or `PuTTY`
(e.g. via ``ssh <user>@bastion.desy.de -L 8081:exflctrl01.desy.de:80`` in Linux and MacOS
and the equivalent for the PuTTY SSH client in Windows), and then configure
the following channel definitions on your local machine (may be from a new terminal)::

    conda config --add channels http://localhost:8081/karabo/channel
    conda config --add channels http://localhost:8081/karabo/channel/mirror/conda-forge

Please note that if you added the channels as instructed in :ref:`framework/xfel_installation`
you will have to remove the channels for the internal network with the commands::

    conda config --remove channels http://exflctrl01.desy.de/karabo/channel
    conda config --remove channels http://exflctrl01.desy.de/karabo/channel/mirror/conda-forge


Remote installations using dynamic SSH tunneling (SOCKS)
--------------------------------------------------------

The second option is to use a proxy and dynamic port forwarding, i.e.
setup connection::

    ssh <user>@bastion.desy.de -D 9090

and tell ``conda`` to use a proxy server from your local machine (may be from a new terminal)::

    conda config --set proxy_servers.http socks5://localhost:9090

After everything is installed, you may want to remove the proxy definition
again::

    conda config --remove-key proxy_servers.http

Installing KaraboGUI
====================

When using conda one should preferrably not alter the base environment that
comes with it, therefore we should install the GUI in a separate environment.
If needed, one can safely remove this environment with no fear with corrupting
the base environment.

* Create a target environment for KaraboGUI with any name you want.
    * ``conda create -n karabogui<version> karabogui=<version> --yes``

* Or run this to get the latest version (recommended)
    * ``conda create -n karabogui_latest karabogui --yes``
  To upgrade this environement, run these commands:
    * ``conda env remove -n karabogui_latest``
    * ``conda create -n karabogui_latest karabogui --yes``

* Or, if you have an existing conda environment, install the package directly:
    * ``conda install karabogui=<version> --yes``
        Leave the version out to get the latest one: ``conda install karabogui --yes``
        NOTE: Dependencies might clash with your environment, use at your risk


Helpful commands are available below:

* ``conda search karabogui`` will show you all the available versions in the channels
  you have configured

* ``conda env remove -n your_karabo_environment`` will remove the environment called ``your_karabo_environment``.

Running KaraboGUI
=================

After successfully installing KaraboGUI, you will have access to the following entry-points:
    * karabo-gui;
    * karabo-cinema;
    * karabo-theatre;
    * karabo-update-extensions

.. note::
    From now on, all you need to do to run KaraboGUI is:
        * Open your terminal/prompt
        * ``conda activate <your_karabo_environment>``
        * ``karabo-gui``


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

After installing your miniconda3 distribution, install the package
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

Finally, generate the version file using::

    python setup.py develop

inside the ``pythonGui`` directory. Repeat that step from time to time to keep
the version number up-to-date.


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
