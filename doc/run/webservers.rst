..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _webservers:

***********
Web Servers
***********

The karabo framework offers the possiblity to start and stop the ``daemontools``
services from a web interface.

This interface operates by design outside the control system's communication
infrastructure as it is a monitoring tool of the Karabo control system.

karabo-webserver
================

The web server provides a browser based interface to the daemontools services
as well as a RESTful api to access them ::

    karabo-webserver serverId=webserver
    
The exectuable offers few customization options.

Using the ``--filter`` option, one can specify if only the services included in
the list are allowed to be controlled via the web interfaces. The filter list 
should contain a list of services running on the ``var/services`` directory,
note that the service names are case-sensitive.

Using the ``--webserver_aggregators``, one can provide a list of urls matching
the urls of a web aggregator (see below). The web server will periodically
subscribe to the web aggregators in this list.

Using the option ``--port``, one can provide the port the server listen to. The
deafault port is set to 8080.

The web interface offers the possibility to access the log file or to follow
their content live.

karabo-webaggregatorserver
==========================

In an attempt to minimize the information needed to identify the webservers
located in the multiple hosts involved in the control system, Karabo offers
a service that will aggregate the ``karabo-webserver``s. This server can be
started with the command ::

    karabo-webaggregatorserver serverId=webaggregator --port portnumber

This server offers a summary table of all services subscibed to itself
and links to the relevant webservers' main page. The ``--port`` option is not
mandatory, the default port is 8585.
