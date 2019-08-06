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


Step 2: Local Configuration Database
====================================

In order to start a local database, one needs to run
by typing::

  karabo-startconfigdb

To stop the database type::

  karabo-stopconfigdb

The `karabo-startconfigdb` will attempt to pull an docker image 
`europeanxfel/existdb:2.2` from `hub.docker.com`. In case the test system
is isolated one needs to provide the image either by connecting the host
temporarily to the internet, or by using the
`docker save <https://docs.docker.com/engine/reference/commandline/save/>`_
and `docker load <https://docs.docker.com/engine/reference/commandline/load/>`_
commands.

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
