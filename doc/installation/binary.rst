.. _installation/binary:

**************************
 Binary download for Linux
**************************

Use this installation option if you want to use Karabo for:

- Running the GUI or ikarabo on your Laptop or PC in order to view/control an existing installation

- Starting Karabo all local, see how it works, develop macros and/or devices

- Build up small control systems involving few computers

- Developing devices or macros for Karabo

If one of the option is for you simply follow the few steps below:


Get and install Karabo Framework
===================================

A self extracting shell script is available `here <http:exflctrl01.desy.de/karabo/karaboFramework/tags>`_.

Select the correct installer for your operating system and download it to some local folder.

Change permissions and extract it using the following commands::

    chmod +x karabo-*.sh
    ./karabo-*.sh

The script will install a single ``karabo`` folder, containing everything needed (including dependencies)
to get going with Karabo.

Source the activate script be able to run all karabo applications and use the correct (shipped)
Python environment::

  cd <path-to-karabo>
  source activate

.. note::

   Before using Karabo in a new shell you **always** have to source 
   the ``activate`` script.

Once ``activate`` is sourced, your environment is ready and you should be 
able to e.g. run::

  karabo-gui


.. warning::

   Shell aliases and paths may conflict with the environment ``activate`` sets up. If ``which python`` 
   does not point to the Karabo-provided executable after sourcing activate, check your shell configuration
   e.g. `.bash_rc` if any aliases or python paths are set therein and comment the respective lines.


