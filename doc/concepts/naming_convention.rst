.. _naming_convention:

**************************
Instance Naming Convention
**************************

General concept
===============

The general idea is to fix a naming convention that later allows to build up a hierarchical and unique name for all
equipment integrated into the photon beamline control system.

For the control system (Karabo) the current conventions are as follows:

Being a device-based control system, every Karabo device must have a unique name for addressing.
The name must have three fields using "/" as a separator.
The first field is named domain, the second field is named type and the last field is named member.

Hence, devices names are hierarchical:

* The domain groups devices related to which part of the instrument/tunnel/lab they belong to.
* The type specifies which kind of equipment within a domain.
* The member specifies which instance of a specific type.

A Karabo device name in general looks like:

domain/type/member

By convention all device names must be written in upper case characters and must not contain any special characters
(like %@^#) or white space, but only use alpha numeric ASCII text.


Domain Names
============

The domain name in general should help to locate control equipment. The basic decision taken here is to use functional
names instead of geographical indicators (like co-ordinates or distances in meter). The name should be immediately
understandable by the operators of the respective instruments.

The domain name is hierarchical in its structure and should narrow down the identification of controllable equipment
with deeper hierarchy levels. The number of levels is fixed to three with an underscore (“_”) as separator and an
optional suffix with a minus (“-“) as separator to disambiguate otherwise identical names. Entirely virtual devices of
the control system (such as compute nodes of an analysis pipeline) may violate the physical location nature of the
domain name and may introduce entirely abstract names into the hierarchy (such as ONC (“online cluster”) or the like).

| Level 1: Scope
| Level 2: Component Group
| Level 3: Component
| Level 4: Suffix (only if needed)

Note: As the number of things to name is very huge and largely diverse, the abstraction levels on identical hierarchy
levels may vary given the restraints of a fixed three-fold hierarchy.


Level 1 – Scope
+++++++++++++++

With respect to operating XFEL, the first level of the domain name (scope) has an instrument-based point of view.
The six instruments are proper scope names. Other valid scope names are SA1, SA2 and SA3 indicating that the control
equipment cannot be associated to an individual instrument but only to a common SASE. Other valid scope names are the
laboratories in the XHQ building, e.g. DET, LAS, DAQ, etc.

.. table:: Examples of scope names

    ========   ==============================================================
    Short      Comment
    --------   --------------------------------------------------------------
    FXE	       Control equipment associated to the FXE instrument
    SPB	       Control equipment associated to the SPB instrument
    SCS		   Control equipment associated to the SCS instrument
    SQS		   Control equipment associated to the SQS instrument
    HED	       Control equipment associated to the HED instrument
    MID	       Control equipment associated to the MID instrument
    SA1	       Control equipment associated to SASE 1
    SA2	       Control equipment associated to SASE 2
    SA3	       Control equipment associated to SASE 3
    DET        Control equipment associated to the detector laboratory in XHQ
    LAS        Control equipment associated to the laser laboratory in XHQ
    ========   ==============================================================

Formatting requirements:

* Scope names must be upper case and typically use 3-6 letters
* Scope names must only use alpha-numeric characters

Level 2 – Component Group
+++++++++++++++++++++++++

With respect to the operating XFEL, component groups are sections of
instrumentation that group together components that either logically belong
together, share some common support or a specific location. Component groups
of different abstraction levels are allowed to co-exist under the same scope
to satisfy the need of identifying equipment that cannot be associated to a
proper sub-location, logical set or support (much like FXE and SA1 for the
first level). Some reserved and standardized names for hutches/rooms (see
gray entries in table below) reflect proper component group names and must be
used consistently in naming across the whole XFEL. The abbreviations for
these standardized groups must not be used with a different meaning.
Different experiments however, may use finer grained component groups,
such as sub-divisions of hutches (e.g. EHU: experiment hutch upstream,
EHC: experiment hutch central, EHD: experiment hutch downstream, OHU: optic
hutch upstream), special supports (OGT1: optical granite table 1) or logical
sets of components (e.g. MKB: Micron scale KB) with individual names different
to the standardized ones. Any instrument using fine-grained names must
intrinsically know the mapping to the respective locations.

.. table:: Examples of component group names

    =====   =============================
    Short   Comment
    -----   -----------------------------
    OPT	    Optics hutch
    EXP	    Experiment hutch
    CTR	    Control room
    LAS	    Laser hutch
    RCK	    Rack hutch
    PMP	    Pumping room
    OHD	    Optics hutch downstream
    EHU	    Experiment hutch upstream
    MKB	    Micron scale KB
    OGT1    Optical granite table 1
    IRD	    Interaction region downstream
    =====   =============================

In gray standardized names are shown, the other rows are examples for
experiment-specific, more fine-grained names, which can be used in parallel.

Formatting requirements:

* Component group names must be upper case and typically use 3-6 letters
* Component group names must only use alpha-numeric characters

Level 3 - Component
+++++++++++++++++++

A component describes a set of equipment that together fulfills some
specific conceptual function, such as an attenuator (ATT), a
beam-imaging-unit (BIU) or a vacuum component (VAC). The VAC component
aggregates the individual vacuum equipment associated to other components
within the same component group. Similar to the VAC component an optional
movement component (MOV) aggregates motion axes that act on a whole set of
other components for alignment purposes.

Formatting and naming requirements:

* Component names must be upper case and typically use 3-6 letters
* Components with the same generic conceptual function should share a name
* Any two components, which are not the same in conceptual function must not
  share an identical name
* Component names must be unique within their hierarchy level (and may require
  a suffix to disambiguate)

.. table:: Examples of component names

    =====   =============================
    Short   Comment
    -----   -----------------------------
    ALAS    Alignment Laser
    ATT     Attenuator
    BIU     Beam imaging unit
    LPD1M   Large Pixel Detector 1Mpx
    IPM     I0 Intensity Position Monitor
    SHUT    Shutter
    =====   =============================

Level 4 – Suffix
++++++++++++++++

A suffix is required in case more than one component of the same name is part
of the same upper hierarchy (i.e. scope and component group). Suffixes are
simply counted in integer steps (1,2,3 and so forth). Components added later
and clashing with an existing one in the same hierarchy level will be
indexed beginning with 2. The already existing component will not be
re-named (i.e. stay without a suffix). Components that are unique within
the hierarchy level must not carry any suffix.



Type Names
==========

Type names describe the type (or class) of the individual equipment within
the component. More technically formulated, the type names reflect the driver
(device classes in case of Karabo) names needed to control the individual
equipment. Examples of type names are: MOTOR, AI, CAM, VALVE, etc.

Member names
============

Member names identify an individual instance of a specific equipment type
within a component. For the same components the member names should also be
identical.

Motion axis should be named consistently as described here.
Formatting and naming requirements:

* Member names must be upper-case
* Underscores can be used to identify sub-hierarchies
 
Examples
========

Below some examples of full Karabo device names are given (green = domain,
red = type, and blue = member):

**SA1_XTD9_IMAGPII45/CAM/BASLER_CTRL**

This name addresses a camera called “BASLER_CTRL”, which is part of the Pop
In Monitor Type II 45 (IMAGPII45) component located in tunnel XTD9, which
does not belong to an individual instrument (i.e. upstream of the
distribution mirror for FXE and SPB) and thus is associated to SASE1.


**SA1_XTD9_IMAGPII45/DO/BASLER_ONOFF**

This name addresses a digital output called ”BASLER_ONOFF”, which is part of
the Pop In Monitor Type II 45 (IMAGPII45) component located in tunnel XTD9,
which does not belong to an individual instrument (i.e. upstream of the
distribution mirror for FXE and SPB) and thus is associated to SASE1.

**MID_XTD6_ATT/MOTOR/BLADE_TOP**

This name addresses a motor called “BLADE_TOP”, which is part of the
Attenuator component (ATT) located in the MID beamline part of tunnel XTD6 (i
.e. after the distribution mirror).


**FXE_OGT2_BIU-2/MOTOR/SCREEN_Y**

This name addresses a motor called “SCREEN_Y”, which is part of the second
Beam Imaging Unit (BIU) component located on the Optical Granite Table 2
(OGT2) of the FXE experiment.


**SPB_EHU_VAC/TURBO/T1**

This name addresses a turbo pump called “T1”, which is part of the Vacuum
(VAC) component associated to the Experimental Hutch Upstream (EHU) of the
SPB device.



Karabo's property and command names
===================================

Every Karabo device class (like PUMP, MOTOR, VALVE) owns its own set of
properties and commands. Properties can typically be written to, or read
from a device (like targetPosition, currentTemperature, etc.). Commands are
executed on a device (like move, stop, start, etc.). The names of all
properties and commands are internally fixed and will be visible in the GUI.
The GUI however allows building expert panels in which aliases (labels) for
all names can be created.
