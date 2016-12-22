.. _deployment:

*****************
Device Deployment
*****************

Deploying Devices
=================

Device deployment and configuration is carried out through the graphical user interface
(GUI). It consists of multiple steps:

- Devices need to be deployed as instances as part of a project. Instances may belong
  to multiple projects. Accordingly, an existing project needs to be opened, or a new 
  project needs to be created.
  
- As devices are bound to servers, in next step a device server hosting the required
  functionality is selected from the central repository or a new server is created as 
  described in `Creating a Device Server`_
  
- The device server needs to be assigned a host from the list of hosts available under 
  the instance path.
  
- Finally, the devices or the server are configured and this configuration is versioned,
  and possibly defined as the default configuration.
  
  
.. todo::

	Clarify if this is a viable concept. It then would need to be inplemented.
	I suggest to evaluate the use of an object oriented DB for storing the XML based
	configuration files we already have (goal is to have minimal changes to the existing
	code).
	
	Additionally, all control hosts in a domain would need to run a server hosting a
	deployment device, which after receive deployment commands from the GUI needs
	to install the requested device server configuration, and launch the device server.
	
	Device server installation and launching should be separated, so that device servers
	which are offline can be brought up through the deployment device. 
	
	In this scenario, the deployment device needs to be extremely stable and well tested
	and its hosting device server should not host any other devices which could bring it
	down.
	
	Comments by: Burkhard, Gero, Sergey. Implementation: GUI - Kerstin. Deployment device
	probably in Python by John or Sergey/Andrea?

Deploying Configurations
++++++++++++++++++++++++

Configurations are versioned in an object-oriented database, acting as a single repository,
on the device level.

Initial injection into this database happens upon completion of the initial configuration
during deployment. Any subsequent changes to the configuration are versioned (as a new
configuration, based on the initial one), and may be set as the default configuration 
to load for a specific instance id. New versions of a configuration may be given a new,
unique name.

.. todo::

	Does the name need to be unique? If so it probably always has to be globally
	unique to avoid conflicts if a domain-local configuration is later change to 
	global availability.

Users can specify if a configuration is globally available via the central repository,
or if access to it is limited to the domain the configured instance belongs to.

.. todo::

	Check this concept for feasibility. If implemented as such the central repository
	needs to provide for a kind of versioning system, as well as flags for default on
	a per instance level, and if it is a global configuration or limited to a specific
	domain.

Creating a Device Server
========================

.. todo::

	Burkhard and John should write the concepts for compiling in or wheeling in devices
	here.