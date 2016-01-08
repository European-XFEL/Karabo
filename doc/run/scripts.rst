.. _run-scripts:

###################################
Scripts to Control a Karabo Session
###################################

activate/deactivate
===================

Before running Karabo, you need to source its ``activate`` script. Doing this
adds Karabo's bin directory to your ``$PATH`` and sets the ``$KARABO``
environment variable which is used by various scripts and programs in Karabo.

To source the activate script, simply run this command::

  $ source <path to karabo>/karaboRun/bin/activate

Then later, if you would like to undo the environment changes resulting from
activation, the ``deactivate`` command can be run like this::

  $ source deactivate


TBD
===

Scripts: allStart, allStop, allRestart, allCheck, showLogs

Files: allInfo

Maybe also: queryBroker, startBrokerMessageLogger
