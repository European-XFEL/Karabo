.. _run/use_device:

********************
Working with devices
********************

After installation Karabo is still "empty".
Things get interesting once you are equipping Karabo with devices.

Using Karabo's plugin technology, so-called ``device packages`` can be added
to extend the core system. 

A device package is a separate software project (managed as a git repository) 
which typically contains a single device class.

However, device packages can also contain several karabo classes or even Karabo
unrelated code that serves as a dependency to one or more Karabo device(s).

All device packages are maintained in remote git repositories and are easily 
accessible via a GitLab server under::

  https://git.xfel.eu/karaboDevices

To simplify interaction, Karabo provides a utility tool helping developers and 
system integrators to work with devices. 
The tool is called ``karabo`` and if you type::

  karabo

you will see a list of commands available which - roughly spoken - allow to 
create **new** devices, further **develop** or simply **install** existing devices.

Installing an existing device
=============================

Install a device if you are *not* intending to change its code, but just want
to use it as is.

For installation you have to specify the name and the version (i.e. the git tag
or branch name) of the device package. 

Example::

  karabo install dataGenerator 1.3.1-2.0

The Karabo script will sub-sequently download the respective code from the 
repository optionally (if C++) compile it and subsequently install it to Karabo.

.. note::
   By default, karabo will work with the device repositories at ssh://git@git.xfel.eu:10022/karaboDevices/<devicePackageName>.git.
   Therefore the user's public ssh key has to be submitted to git.xfel.eu before, otherwise command will fail. 
   The URL for adding a new ssh key to user settings is: 
   https://git.xfel.eu/-/profile/keys
   Instructions on how to generate the key are also linked there.

   
   Alternatively, one can talk via https to the repositories at 
   https://git.xfel.eu/karaboDevices/<devicePackageName>.git
   For this, one can use the ``karabo --git https://git.xfel.eu install``
   (-g flag will work as well as --git)

.. note::

   Please make sure that you use the correct case for the package name, as the command is case sensitive.
   "dataGenerator" and "DataGenerator" as a package name will lead to different results. 

Directly after, you can start start it with the corresponding server as explained
:ref:`here <run/servers>`.

.. note::

   Currently, installation is done from sources and the sources are placed into
   a folder named ``installed``. Do not edit the code here, as the download
   reflects only the HEAD revision of the respective branch and can't be properly
   versioned. Moreover, the installation packages may change to binaries in near
   future.


Developing an existing device
=============================

If your intention is to further develop or fix bugs on an existing device, you
want to use::

  karabo develop someDevice

If not already downloaded the script will do it for you. In develop mode it is
sufficient to edit the code and save it (additionally compile it if C++). After
restarting the server changes will be taken into account.

.. note::
   
   Technically, in Python the setup-tools are used and a ``pip install -e .``
   is used to achieve the behavior. In C++ a softlink of the shared-library is
   placed in the plugin folder.


Creating a new device
=====================

If you intention is to create a new device the command::

  karabo new myFancyDevice <API>

is for you, where <API> must be one of the three ``cpp``, ``python`` or
``middlelayer``.

Once executed an executable, though "empty" device code is placed in the ``devices``
folder. Use ``karabo develop myFancyDevice`` (see above) to let Karabo now about
it. 

.. note::

   Initially, the device code is wrapped into a git repository existing
   only locally. However, everything is prepared to allow a 
   ``git push -u origin master`` for adding this device to Karabo's central device
   storage.

Uninstalling a device
=============================

If your intention is to uninstall a device, you
want to use::

  karabo uninstall <package>

This command will call ``pip uninstall -y {device}`` for python devices. 
For C++ devices it will remove the directory and ".so" file (or symbolic link) from the plugin directory.

Example::

  karabo uninstall dataGenerator
  

  
