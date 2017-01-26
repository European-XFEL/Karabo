.. _run/all_local:

***************************************
Starting a full Karabo system all local
***************************************

Here we describe how you can bring up a small local Karabo system on your computer. 
Make sure you have understood the :ref:`fundamental concepts <fundamentals>` before proceeding here.

Lets go step by step. The only prerequisit is, that you have installed and 
activated Karabo as described :ref:`here <installation/binary>`.

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

Similar to the broker start a local database by typing::

  karabo-startconfigdb

To stop the database type::

  karabo-stopconfigdb

Step 3: Edit the configuration file
===================================

In the karabo folder navigate to ``var/config`` and open the file ``config``
with your favorite editor.

In this file make sure you set the variable ``KARABO_BROKER`` like so::

  export KARABO_BROKER="tcp://localhost:7777"

and the variable ``KARABO_PROJECT_DB_HOST`` like::

  KARABO_PROJECT_DB_HOST="localhost"

Step 4: Start the karabo backbone
=================================

Now you can start an "empty" Karabo by typing::

  karabo-start

and stop it with::

  karabo-stop

If you succeeded up to know you are ready to start additional servers, develop
you own device plugins etc.
