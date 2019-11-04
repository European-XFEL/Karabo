*******************
 Conda GUI client
*******************
From version 2.6, the KaraboGui installation for windows and the karabo-gui
executable shipped with the precompiled binaries, is deprecated. Conda is a package
manager and dependency resolution tool which offers a very straightforward way
to install packages and its dependencies.

In order to use Conda, three basic steps are needed:
    1. Install Conda (through an Anaconda or Miniconda distribution);
    2. Configure its channels;
    3. Install the desired package

Installing Conda
================

The following steps are needed in order to obtain Conda.

* Download and install miniconda from `here <https://docs.conda.io/en/latest/miniconda.html>`_
* Open your terminal (Anaconda Prompt on Windows or a bash on linux)
* If conda is in your path, you should be able to run **conda --version**
    * If it isn't, you need to activate conda first
        * **Linux**: ``source <miniconda_path>/etc/profile.d/conda.sh``
        * **Win**: ``CALL <miniconda_path>/condabin/activate.bat``

Channel Configuration
=====================

For KaraboGUI, some package channels are needed besides Conda's defaults. You
only need to do this once and it can be done either from command line or
editting Conda's configuration file (`.condarc`).
You can find your configuration file location typing ``conda info`` on your terminal.

From your terminal, add the needed channels executing the following commands::

    conda config --add channels http://exflserv05.desy.de/karabo/channel
    conda config --add channels conda-forge

Installing KaraboGUI
====================

When using conda we don't want to mess the base environment that comes with it,
therefore ideally we should install the GUI in a separate environment.
Then, when we don't need it anymore one can safely remove it with no fear with
corrupting the base environment.

* Create a target environment for KaraboGUI with any name you want.
    * ``conda create -n karabogui<version> --yes``
* Activate the environment you created
    * ``conda activate karabogui<version>``
* Install your application
    * ``conda install karabogui=<version> --yes``
        * Leave the version out to get the latest one: ``conda install karabogui --yes``
* ``conda search karabogui`` will show you all the available versions

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

Uninstalling
============

In order to uninstall KaraboGUI, always opt for removing the complete environment
itself: ``conda remove -n <environment_name> --all``

