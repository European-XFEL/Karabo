**************
The Karabo GUI
**************

Getting Started
===============

The GUI starts up into a non-connected state, meaning that you need to login
to a specific GUI server with your login credentials. By doing so your access
level is also determined and the appropriate options will be available to you.

.. code-block:: bash

    karabo-gui

will open up this panel:

.. figure:: images/gui/not_connected.png
   :scale: 60 %

   The GUI view upon GUI startup

You can see that most of the panels are still empty, as you are in a
non-connected state. The panels are in clock-wise, starting at the center-top
(12 o'clock) position

* The central scene, which is used to display custom views
* The configurator panel, which lists *all* properties and slots available for
    a device as appropriate for your access level
* The logging panel, giving access to global logging and notification messages,
    alarms and an an iKarabo console
* The project panel, giving you a logical view on the projects you have loaded
* The navigation panel, which gives you a live view of the system in a flat
    hierarchy and allows for filtering.

In order to connect yourself to a domain, click on the connect icon. The
following dialoag will appear, asking you for your credentials and the domain
you would like to connect to. The ``GUI server`` combo-box will update itself
to the list of GUI-servers available for this domain. Further you can choose
if the GUI should open the last view you had configured for this client.

.. figure:: images/gui/connect_dialog_current.png

   The GUI connection dialog.

Upon connection the GUI will switch to a connected state, which already shows
you a live view of the system.

Any of the listed panels may be detached and arranged separatly on the screen.
This arrangement is saved as part of your last connected view. In the following
we will discuss each panel in more detail, followed by a discussion of best-
practices when creating custom panels for distributed control systems.

The Navigation Panel
====================

The navigation panel gives you a live view of the system, in a tree hierarchy.
This is in contrast to the project panel, which presents a logical view of
the system, as grouped by components.

In its default view, the navigation panel shows the system topology in tree
hierarchy showing the unified and alarm states of the running devices.

.. figure:: images/gui/navigation_panel_current.png

   The Karabo navigation panel. Icons next to the device instance id give you
   the color code state of the device, as well as the alarm condition the device
   is in. Finally, the large lock indicated that a device is locked due to
   ``Topic locking``, the smaller, framed lock that it is locked due to
   ``Device Locking``.


The Project Panel
=================

The project panel is the main access point for interacting with projects and
devices in a hierarchical fashion.


.. figure:: images/gui/project_panel_current.png

   The Karabo project panel.

It allows you to open an existing project, which is then included on the top-
hierarchy level, or to add members into existing projects.

A project has different categories namely:

- Macros
- Scenes
- Device Servers
- Subprojects

The context menu allows to either add or load project members to a project. 

Adding a sub project or loading a project from the repository is done through a
dialog, which allows to browse for existing configurations.

.. figure:: images/gui/project_panel_open_current.png

   The project load dialog.


Configuring Servers
+++++++++++++++++++

Servers are configured using a dialog which is either accessible from the
server's context menue or opens when a new server is added as a member.
The dialog also (optionally) configures the host, which is useful if servers
are deployed through Karabo. Instances are added to the server either through
the context menue or by dragging them onto the server from the live view.

.. figure:: images/gui/project_panel_server_config_current.png

   The server config dialog.

Configuring Instances
+++++++++++++++++++++

Instances are configured using the corresponding dialog, or through the context
menu, which allows direct selection and creation of configurations.

.. figure:: images/gui/project_panel_instance_config_current.png

   The instance config dialog.

.. figure:: images/gui/project_panel_instance_current.png

   The instance context menu.

.. note::

    For configuring new Beckhoff servers it is best practice to instantiate
    the server somewhere in the domain of the project, i.e. manually on the
    host and let the server create generic devices. This will yield the proper
    instance ids on each device, as configured on the PLC. You should then
    drag the server into your project and then reassing a specialized class
    for each device instance.


The Alarm Service
+++++++++++++++++

Alarms are acknowledged through the alarm service. It uses the following
custom widget.

.. figure:: images/gui/alarm_service.png
   :alt: alarm_service.png

   The alarm service widget.

The Central Scene
=================

.. figure:: images/gui/master_panel.png
   :alt: master_panel.png

   An example of a master panel in a central scene. Examples of analogue
   gauges for value display, spark lines indicating trends, and state + alarm
   conditions composite values are shown. The Sub-system boxes link to the
   respective detailed scense. Note how with one look onto the gauge widgets
   an operator can access the system state.

.. figure:: images/gui/detail_panel.png
   :alt: detail_panel.png

   An example of a detail panel in a central scene. Examples of analogue
   gauges for value display, spark lines indicating trends, and state + alarm
   conditions composite values are shown. Note how the state and alarm condition
   are separated for the gauge **Gauge_Down2**. The bottom buttons are hyper-links
   to the other detail panels and the master panel.


Scene Composition
+++++++++++++++++

Plotting Widgets
++++++++++++++++

Trendlines
~~~~~~~~~~

Trendlines show the evolution of a value over time. Multiple values may
be grouped into one trendline plot. Quick access buttons exist to scale
the trendline to the display the last 10 minutes, one hour, one day and one week
of data logs. The trendline may be set to either display the full range of values,
or as usually more useful, a detail range, selectable by relative deviation from
the mean of the last 10 values. Optionally, the alarm ranges are indicated
in the trendline.



.. figure:: images/gui/trendline.png
   :alt: trendline.png


.. note::

   You may miss the red, orange and yellow tones from the color selection options
   for data series lines. This is on purpose, as in Karabo these colors are
   reserved for alarm condition indication, and should not be used for other
   purposes!

.. todo::

   Implement this or a similar concept. Specifically, the quick access buttons
   and the alarm ranges are needed in my opinion.


Plotting X vs. Y Values
~~~~~~~~~~~~~~~~~~~~~~~





Image Widgets
+++++++++++++

The Table Element
+++++++++++++++++

Hyperlinks between Scenes
+++++++++++++++++++++++++

Run Configuration
=================

The Logging Panel
=================


