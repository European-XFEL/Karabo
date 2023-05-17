..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _run/all_local:

***************************************
Starting a full Karabo system all local
***************************************

Here we describe how you can bring up a basic Karabo system on a single host, including the broker. Both, project manager and data logging, use their file based backend.

Make sure you have understood the fundamental concepts before proceeding here.

Lets go step by step. The only prerequisit is, that you have installed and 
**activated** Karabo as described :ref:`here <installation/binary>`.

Step 1: Install Services to run locally
=======================================

From a freshly installed Karabo system, one needs to install services
to that will run Karabo's default configuration in the local machine.

To install a local service suite including a JMS broker, simply type::

  karabo-create-services jms_local

Step 2: Edit the environment files
==================================

In the karabo folder navigate to ``var/environment``. If the file named ``KARABO_BROKER`` does not exist, create the file and change its content to ``tcp://localhost:7777``.


Step 3: Start the karabo backbone
=================================

Now you can start the Karabo installation by typing::

  karabo-start

and stop it with::

  karabo-stop

If you succeeded up to now you are ready to start additional servers, develop
you own device plugins etc.
