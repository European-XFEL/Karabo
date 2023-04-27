..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

************
Introduction
************

Karabo is the name of European XFEL GmbH's homogenoeus software
framework.

It essentially is a framework allowing the building of plugable distributed
(networked) applications that can be remotely controlled. 
Karabo can be used for control and data acquisition (i.e. as `SCADA
<http://en.wikipedia.org/wiki/SCADA>`_ system) but also for
data-management, and data analysis aspects.

If you want to know more about Karabo please read our `paper
<https://docs.xfel.eu/alfresco/d/a/workspace/SpacesStore/5be9f069-3d70-4625-bde1-f1c7ca06eaed/Karabo_Overview_2013.pdf>`_,
and/or have a look at this `presentation
<https://docs.xfel.eu/alfresco/d/a/workspace/SpacesStore/3f4fb9de-ea30-4f8f-9f5f-628ba8066241/Karabo_Overview_Presentation>`_.
Advanced readers may want to have a look at Karabo's `design concepts
<https://docs.xfel.eu/alfresco/d/a/workspace/SpacesStore/9b331f2f-fe2e-4ece-850d-96b486207f10/Karabo_Design_Concepts.pptx>`_.

 
If you want to cite Karabo please use this reference::

  Hauf, Steffen and Heisen, Burkhard et al., The Karabo distributed control system,
  Journal of Synchrotron Radiation 26, no. 5 (2019) 1448--1461 doi:10.1107/S1600577519006696

Find the doi information `here <https://doi.org/10.1107/S1600577519006696>`_.


What Karabo Is
==============

Karabo is a distributed control system (DCS) which allows for supervisory control
and data acquisition (SCADA). In this context it may be used to control and
monitor hardware devices through various interfaces, control groups of devices
and composition higher-level functionality on those, as well a implement
processing algorithms working on the data input into the system. In Karabo
(hardware) resources may be shared in a non-intrusive fashion, by using topics
assigned to installation domains. On top an alarm system assures that operators
are kept updated with significant deviations from normal system-performance.

What Karabo Is Not
==================

Karabo is by **no** means a fail-safe system. As such any core protection of
hardware, and especially personal may **not** be implemented in Karabo. Karabo
may only be used as a view and configuration tool for these protection systems.

Additionally, while the aim is to make interfacing with Karabo as simple as
possible, Karabo remains an expert tool. It is thus required for users to make
themselves aware with the relevant documentation *before* using the system.
