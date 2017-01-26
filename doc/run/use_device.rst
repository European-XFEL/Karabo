.. _run/use_device:

********************
Working with devices
********************

After installation Karabo is still "empty" (similar to a smartphone without any
apps installed). Things get interesting once you are equipping Karabo with devices.

Using Karabo's plugin technology, so-called ``device packages`` can be added
to extend the core system. 

A device package is an own software project (managed as a git repository) 
which typically contains a single device class.

However, device packages can also contain several karabo classes or even Karabo
unrelated code that serves as a dependency to one or more Karabo device(s).

All device packages are maintained in remote git repositories made easily 
accessible via a GitLab server under::

  https://git.xfel.eu/gitlab/karaboDevices

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

Directly after, you can start start it with the corresponding server as explained
:ref:`here <run/server>`.

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

is for you, where <API> must be one of the three ``cpp``, ``pyhton`` or
``middleLayer``.

Once executed a run-able, though "empty" device code is placed in the ``devices``
folder. Use ``karabo develop myFancyDevice`` (see above) to let Karabo now about
it. 

.. note::

   Initially, the device code is wrapped into a git repository existing
   only locally. However, everything is prepared to allow a 
   ``git push -u orign master`` for adding this device to Karabo's central device
   storage.


   



  
