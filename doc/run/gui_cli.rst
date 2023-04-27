..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.


Starting the GUI
================

Having Karabo installed properly type::

  karabo-gui

Instructions of how to use the GUI can be found :ref:`here <concepts-gui>`.

If you log into the correct GUI server, you will be immediately able to view and
control (given your access persmissions allow it) devices that are currently
online.


Starting the command line
=========================

Having Karabo installed properly type::

  ikarabo

Using the command line interface is described :ref:`here <concepts-ikarabo>`.

For Karabo framework developers another command line interface exists::

  karabo-cli

This command line is slightly different in API and completely differs in 
the way it is implemented (namely through using the C++ code under the hood).

.. warning::
   
   This API is not officially supported anymore, and only maintained for 
   development and debugging purposes. You should never use it for scripting of 
   devices in a production environment!



