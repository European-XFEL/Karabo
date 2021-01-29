.. _run/all_local:

***************************************
Starting a full Karabo system all local
***************************************

Here we describe how you can bring up a basic Karabo system on a single host. 
Make sure you have understood the :ref:`fundamental concepts <fundamentals>` before proceeding here.

Lets go step by step. The only prerequisit is, that you have installed and 
**activated** Karabo as described :ref:`here <installation/binary>`.

Step 1: Local Message Broker
============================

The first thing that must run for Karabo to work is the message broker. 
If you are in the European XFEL GmbH network you can use our always running 
broker (`tcp://exfl-broker:7777`) and skip to the next step. 
If you are outside of the European XFEL network you must start the broker
locally on your computer. 
To accomplish this type::

  karabo-startbroker

This will start the broker as a background process. 
It will be reachable under *tcp://localhost:7777* from now on.

To stop the broker simply type::

  karabo-stopbroker


Step 2: Local Project Database
==============================

Since version 2.6 the Karabo binaries are not shipped with the code necessary
to run the Configuration Database itself. The consequence of this choice is
that a completely local system must be able to run
`docker <https://docs.docker.com/install/linux/docker-ce/binaries/>`_
containers as the user running Karabo.

This is either achieved by having the Karabo user added to the group ``docker``
or by installing ``docker`` in 
`rootless <https://docs.docker.com/engine/security/rootless/>`_ mode
(which is deemed more secure and available as an experimental feature since
version 19.03 of the ``docker engine``).

The complexity of how to install docker on any target system exceeds the scope
of this documentation. Assuming one has the target system set up correctly, 
to start a local database, one needs to run by typing::

  karabo-startprojectdb

To stop the database type::

  karabo-stopprojectdb

The `karabo-startprojectdb` will attempt to pull the docker image 
`europeanxfel/existdb:2.2` from `hub.docker.com`. In case the `all local`
system is isolated from the internet, one needs to provide the image either
by connecting the host temporarily to the internet, or by using the
`docker save <https://docs.docker.com/engine/reference/commandline/save/>`_
and `docker load <https://docs.docker.com/engine/reference/commandline/load/>`_
commands.

The DB's data resides in the container and will be archived at 00 and 12 hours
by default. The data saved with such a procedure, is saved in the 
``var/data/exist_data`` folder. While the runtime data is stored in the
container storage. To wipe the memory, one needs to remove the container
using the `docker rm <https://docs.docker.com/engine/reference/commandline/rm/>`_
command, i.e. stopping the container with `karabo-stopprojectdb` and running
`docker rm karabo_existdb`.

In case one wishes to change the main username and password for the database,
one should follow the instructions in the `README.md` of the
`source <https://git.xfel.eu/gitlab/ITDM/docker_existdb>`_ of the image, and
update the content of the `KARABO_PROJECT_DB_USER` and
`KARABO_PROJECT_DB_PASSWORD` accordingly.

Step 3: Edit the environment files
==================================

In the karabo folder navigate to ``var/environment`` and set the content file
named ``KARABO_BROKER`` to ``tcp://localhost:7777``
and the content of the file named ``KARABO_PROJECT_DB`` to ``localhost``

Step 4: Start the karabo backbone
=================================

Now you can start an "empty" Karabo by typing::

  karabo-start

and stop it with::

  karabo-stop

If you succeeded up to now you are ready to start additional servers, develop
you own device plugins etc.
