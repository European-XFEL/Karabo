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

* Download and install miniconda (Python version >= 3) from `here <https://docs.conda.io/en/latest/miniconda.html>`_
* Open your terminal (Anaconda Prompt on Windows or a bash on linux)
* If conda is in your path, you should be able to run **conda --version**
    * If it isn't, you need to activate conda first
        * **Linux**: ``source <miniconda_path>/etc/profile.d/conda.sh``
        * **Win**: ``CALL <miniconda_path>/condabin/activate.bat``

If you find errors such as "Your shell has not been properly configured to use 'conda activate'", run the following
command::

    . <your miniconda3 path>/etc/profile.d/conda.sh

Or add it to your ``.bashrc`` (or similar).

Channel Configuration
=====================

For KaraboGUI, some package channels are needed besides Conda's defaults. You
only need to do this once and it can be done either from command line or
editting Conda's configuration file (``.condarc``).
You can find your configuration file location typing ``conda info`` on your terminal.

From your terminal, add the needed channels executing the following commands::

    conda config --add channels http://exflserv05.desy.de/karabo/channel
    conda config --add channels http://exflserv05.desy.de/karabo/channel/mirror/conda-forge

N.B. The channel ``http://exflserv05.desy.de/karabo/channel`` is not open to
the public until Karabo will be released. For this reason if one is installing
from a network outside the DESY internal network, one should create an SSH
tunnel to the internal channel and configure that channel as a source.

Assuming, e.g. that one has created an ssh tunnel using an ssh command like
``ssh user@bastion.desy.de -L 8081:exflserv05.desy.de:80``, one should use
the following channel definitions::

    conda config --add channels http://localhost:8081/karabo/channel
    conda config --add channels http://localhost:8081/karabo/channel/mirror/conda-forge

N.B. the two channel definitions are mutually exclusive. Remove the channels that
are not reachable.

Installing KaraboGUI
====================

When using conda one should preferrably not alter the base environment that
comes with it, therefore we should install the GUI in a separate environment.
If needed, one can safely remove this environment with no fear with corrupting
the base environment.

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

Developing
==========

For developing in KaraboGUI using Qt5 we need to be inside a conda environment

After installing your miniconda3 distribution, install the package
``conda-devenv`` from the conda-forge channel:

```
conda install conda-devenv -c conda-forge
```

conda-devenv is a tool for creating a development environment that always
follows the dependencies specified in your environment.devenv.yml, purging
any other dependencies left behind.

Now, on your ``pythonGui`` directory, run:

```
conda devenv
```

Or, if you're not in the directory:

```
conda devenv --file <pythonGUI dir>/environment.devenv.yml
```

This will solve your environment dependencies and create an environment
called ``karabogui``. Call ``conda activate karabogui`` to activate it.

Now all the code from ``karabogui``, `common`` and ``native`` will be on
your ``PYTHONPATH``. No need to install using the setup.

Now, generate the version file using

```
python setup.py develop
```

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