..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _states:

******
States
******


Karabo has a fixed set of provided states, all of which are listed in the
tables below. *States* are classified in *base states*, which can be seen
as set of more general states, and *device type states*, which map closer
to the type of hardware being controlled or to certain types
of software devices, but also always map to a base state. Each base state has a
assigned color coding, making it easy to view are devices state at first
glance.

.. digraph:: state_transitions

    "UNKNOWN"[shape = box style=filled, fillcolor="#FFAA00"]
    "DISABLED"[shape = box style=filled, fillcolor="#FF00FF"]
    "ERROR"[shape = box style=filled, fillcolor=red]
    "INIT"[shape = box style=filled, fillcolor="#E6E6AA"]
    "KNOWN"[shape = box style=filled, fillcolor="#C8C8C8"]
    "STATIC"[shape = box style=filled, fillcolor="#00AA00"]
    "NORMAL"[shape = box style=filled, fillcolor="#C8C8C8"]
    "PAUSED"[shape = box style=filled, fillcolor="#FF00FF"]
    "KNOWN" -> "NORMAL"
    "KNOWN" -> "ERROR"
    "KNOWN" -> "DISABLED"
    "DISABLED" -> "PAUSED"
    "NORMAL" -> "STATIC"
    "NORMAL" -> "RUNNING"
    "RUNNING"[shape = box style=filled, fillcolor="#99CCFF"]
    "ACQUIRING"[shape = box style=filled, fillcolor="#99CCFF"]
    "RUNNING" -> "ACQUIRING"
    "PROCESSING"[shape = box style=filled, fillcolor="#99CCFF"]
    "RUNNING" -> "PROCESSING"
    "PASSIVE"[shape = box style=filled, fillcolor="#CCCCFF"]
    "STATIC" -> "PASSIVE"
    "ACTIVE"[shape = box style=filled, fillcolor="#78FF00"]
    "STATIC" -> "ACTIVE"
    "CHANGING"[shape = box style=filled, fillcolor="#00AAFF"]
    "NORMAL" -> "CHANGING"
    "DECREASING"[shape = box style=filled, fillcolor="#00AAFF"]
    "CHANGING" -> "DECREASING"
    "INCREASING"[shape = box style=filled, fillcolor="#00AAFF"]
    "CHANGING" -> "INCREASING"


Base and derived states are defined as follows:

.. graphviz::

    digraph unknown {UNKNOWN [shape=box, style=filled, fillcolor="#FFAA00"]}

``UNKNOWN`` should be used if the Karabo device has no connection to hardware,
or it cannot be assured that the correct state of the hardware is reported by
the device. The state can also be set if an unknown software error
occurs in the device code.

.. graphviz::

    digraph init {INIT [shape=box, style=filled, fillcolor="#E6E6AA"]}

The ``INIT`` state in which a Karabo device should transition into upon
initialization. During initialization connection to the hardware should
be established.
After initialization the device state should thus be ``KNOWN``, and the device
should transition either to ``DISABLED``, ``ERROR`` or one of the states which
are derived from ``NORMAL``. If no connection can be established the device
should be placed into the ``UNKNOWN`` state.

The ``KNOWN`` base state is the counterpart to the ``UNKNOWN`` state and should
not usually be set to device. Instead, the device logic should decide which
of the states deriving from ``KNOWN`` should be entered after initialization.
All states listed in the following derive from ``KNOWN``.

.. graphviz::

    digraph disabled {DISABLED [shape=box, style=filled, fillcolor="#FF00FF"]}

``DISABLED`` is used if the device will not normally act on commands and reconfigurations.
However, a disabled device is connected to the Karabo system,
i.e. (some) value may be read back, at the very least it is able to notify
Karabo of its disabled status.

.. graphviz::

    digraph error {ERROR [shape=box, style=filled, fillcolor=red]}

The ``ERROR`` state is reserved for hardware errors. It must only be used for an
error pertinent to the hardware component.

.. graphviz::

    digraph NORMAL {NORMAL [shape=box, style=filled, fillcolor="#C8C8C8"]}

The ``NORMAL`` base state should not usually be entered programmatically.
Similar to ``KNOWN``, device logic should rather transition the device
into one of the derived states. The following states derive from and compare
equal to ``NORMAL``.

.. graphviz::

    digraph static {STATIC [shape=box, style=filled, fillcolor="#00AA00"]}

``STATIC`` is itself a base state to the ``ACTIVE`` and ``PASSIVE`` states.
It is the counterpart to the changing states and rarely used.

.. graphviz::

    digraph paused {PAUSED [shape=box, style=filled, fillcolor="#FF00FF"]}

``PAUSED`` Data Acquisition will be paused while the device is in this state.
[TODO: add better (or more complete) description for ``PAUSED``]

.. graphviz::

    digraph active {ACTIVE [shape=box, style=filled, fillcolor="#78FF00"]}

The ``ACTIVE`` state is derived from ``STATIC`` and should usually be used
only for comparison purposes. Rather developers should transition into a device
state derived from it. It is the counterpart to ``PASSIVE``.

.. graphviz::

    digraph passive {PASSIVE [shape=box, style=filled, fillcolor="#CCCCFF"]}

The ``PASSIVE`` state is derived from ``STATIC`` and should usually be used
only for comparison purposes. Rather developers should transition into a
device state derived from it. It is the counterpart to ``ACTIVE``.

.. graphviz::

    digraph running {RUNNING [shape=box, style=filled, fillcolor="#99CCFF"]}

The state ``RUNNING`` is a base state is related to data acquisition devices.
This base state has two children, ``ACQUIRING`` and ``PROCESSING`` and is
colored blueish to indicate that data is flowing.
The ``ACQUIRING`` state is essentially used for detector devices when the data
acquisition is active, while the ``PROCESSING`` state is present in downstream
pipeline devices to show they are receiving and processing the detector data.

.. graphviz::

    digraph changing {CHANGING [shape=box, style=filled, fillcolor="#00AAFF"]}

The state ``CHANGING`` is a base state to the ``INCREASING`` and
``DECREASING`` states. It may however  also directly be used, e.g. if a device
is changing in a way that a directional indication does not make sense. It is
the counterpart to the ``STATIC`` state. ``CHANGING`` and derived states should
be used when a device is transitioning to a new target condition, e.g. a motor
moving to a new position, a power supply ramping to a given voltage or a pump
spinning up to speed. Once the target value is reached the device should
transition into a ``STATIC`` state.

.. graphviz::

    digraph increasing {INCREASING [shape=box, style=filled, fillcolor="#00AAFF"]}

The state ``INCREASING`` is derived from ``CHANGING`` and should be used if
it makes sense to indicate a directional transition of the hardware.
It is the counterpart to ``DECREASING``.

.. graphviz::

    digraph decreasing {DECREASING [shape=box, style=filled, fillcolor="#00AAFF"]}

The state ``DECREASING`` is derived from ``CHANGING`` and should be used
if it makes sense to indicate a directional transition of the hardware.
It is the counterpart to ``INCREASING``.

.. warning::

    The ``ERROR`` state is reserved for hardware errors. Errors due to
    communication problems or software errors should result in a transition
    into the ``UNKNOWN`` state. Generally though, software errors should not
    occur and if they do the device should recover into an operational
    mode. Composite devices should transition to ``UNKNOWN`` if they are not
    able to contact a device they are to control, as they might not have
    all the information available to work properly.

.. warning::

    Devices requiring to establish connections to hardware first, e.g. through the
    network, or some other interface, do this either in the ``INIT`` state.
    Connection functionality **must** be implemented in the ``initialization hooks``,
    **not** in the constructor or ``__init__`` methods. It might take time,
    and would otherwise yield the device unresponsive.


The following diagram shows how base states and derived states are connected,
and which transitions are allowed. Upon initialization, devices generally
transition from ``UNKNOWN`` into one of the states derived from the ``KNOWN``
base state. This is done by passing through the ``INIT`` state, where the
connection to hardware should be established. Note that a connection error
should not put the device into an ``ERROR`` state but rather back into
``UNKNOWN``!

As shown in the diagram a transition to any of the states deriving from
the ``KNOWN`` base state back to ``UNKNOWN`` is possible, this should e.g.
occur if the connection to the hardware is lost. Restablishing a ``KNOWN`` state
should happen by passing through the ``INIT`` state``.

The ``ERROR`` and ``DISABLED`` states may be transitioned into from any of the
states deriving from the ``NORMAL`` base state. Conversely, the device may
implement logic to recover from an ``ERROR`` state into any of the ``NORMAL``
-derived states, or from ``DISABLED`` into these.


.. graphviz::

    digraph state_uml {

        compound=true;
        rankdir = LR;
        graph [pad="1.", ranksep="0.95", nodesep="1.2", splines=ortho];
        unknown
        [
            shape = box
            style = filled
            fillcolor = "#FFAA00"
            label = "UNKNOWN"
        ]

        init
        [
            shape = box
            style = filled
            fillcolor = "#E6E6AA"
            label = "INIT"
        ]

        subgraph cluster1 {

        label = "KNOWN";

        disabled
        [
            shape = box
            style = filled
            fillcolor = "#FF00FF"
            label = "DISABLED"
        ]

        error
        [
            shape = box
            style = filled
            fillcolor = red
            label = "ERROR"
        ]

            subgraph cluster0 {

                label = "NORMAL";
                on
                [
                    shape = box
                    style = filled
                    fillcolor = green
                    label = "ACTIVE"
                ]

                changing
                [
                    shape = box
                    style = filled
                    fillcolor = "#00AAFF"
                    label = "CHANGING"
                ]
                on -> changing
                changing -> on
            }
        }

        unknown -> init

        on -> unknown [ltail=cluster1]
        init -> on [lhead=cluster1];
        init -> unknown

        on -> error [ltail=cluster0]
        error ->  on [lhead=cluster0]

        disabled -> error
        error -> disabled
        disabled -> on [lhead=cluster0]
        on -> disabled [ltail=cluster0]

    }



Most Significant State
======================

Especially for middle-layer devices a recurring scenario is the evaluation of
the most significant state, or composite state of a group of states. This is
where state trumping must be used. In Karabo, state trumping is centralized
in the sense that a set of standard trumping rules are provided, giving the
base states a particular order.
In the flat base-state hierarchy the following graph is being followed
in *trump* evaluation, where ``DISABLED`` is trumped by all other states and
``UNKNOWN`` will trump all other states.

.. graphviz::

    digraph state_trumps {

        rankdir = LR;
        compound=true;
        graph [pad="1.", ranksep="0.95", nodesep="1.2", splines=ortho];



        disabled
        [
            shape = box
            style = filled
            fillcolor = "#FF00FF"
            label = "DISABLED"
        ]

        subgraph cluster0 {
            label = "STATIC";
            style = filled
            fillcolor = "#00AA00"

            active
            [
                shape = box
                style = filled
                fillcolor = "#78FF00"
                label = "ACTIVE"
            ]

            passive
            [
                shape = box
                style = filled
                fillcolor = "#CCCCFF"
                label = "PASSIVE"
            ]

            active->passive [arrowhead=none, style=dashed]

        }

        running
        [
            shape = box
            style = filled
            fillcolor = "#99CCFF"
            label = "RUNNING"
        ]

        paused
        [
            shape = box
            style = filled
            fillcolor = "#FF00FF"
            label = "PAUSED"
        ]

        subgraph cluster1 {
            label = "CHANGING";
            style = filled
            fillcolor = "#00AAFF"

            increasing
            [
                shape = box
                style = filled
                fillcolor = "#00AAFF"
                label = "INCREASING"
            ]

            decreasing
            [
                shape = box
                style = filled
                fillcolor = "#00AAFF"
                label = "DECREASING"
            ]

            increasing->decreasing [arrowhead=none, style=dashed]

        }

        init
        [
            shape = box
            style = filled
            fillcolor = "#E6E6AA"
            label = "INIT"
        ]

        interlocked
        [
            shape = box
            style = filled
            fillcolor = "#FF00FF"
            label = "INTERLOCKED"
        ]

        error
        [
            shape = box
            style = filled
            fillcolor = red
            label = "ERROR"
        ]

        unknown
        [
            shape = box
            style = filled
            fillcolor = "#FFAA00"
            label = "UNKNOWN"
        ]

        disabled -> active [lhead=cluster0]
        active  -> running [ltail=cluster0]
        running -> paused
        paused -> increasing [lhead=cluster1]
        decreasing -> interlocked [ltail=cluster1]
        interlocked -> error
        error -> init
        init -> unknown

    }

.. warning::

    The ``UNKNOWN`` state purposely trumps all other states, as the device is
    in a condition in which it does not have all the information necessary
    to determine the proper state. Thus the conservative assumption is
    that the device is in an error state.

.. note::

    When the input list of states contains two or more states that derive from 
    a common state in the trump list and that common parent is the most 
    significant among all the input states, the most significant state will 
    be the one that comes last in the input list. 
    
    To exemplify: if ``COOLING`` and ``RAMPING_DOWN``, which are derived from 
    ``DECREASING``,  are in the input list along with other states that are 
    less significant than ``DECREASING``, the most significant state will be 
    ``COOLING`` if it comes after ``RAMPING_DOWN`` in the input list. Otherwise, 
    the most significant will be ``RAMPING_DOWN``. 
    
    It is important to add in here that a state is considered to derive from itself
    (like classes are subclasses of themselves in most OOP languages). So, if in the 
    example above the classes were ``COOLING`` and ``DECREASING``, the same rule of the
    most significant being the one that comes closest to the end of the input
    list would apply. 

Device developers should however not implement trumping functionality themselves,
but instead use the ``StateSignifier().returnMostSignificant`` function
provided by Karabo.

.. code-block:: Python

    from karabo.middlelayer import State, StateSignifier

    trumpState = StateSignifier()

    listOfStates = [State.ERROR, State.MOVING, State.CHANGING]
    definingState = trumpState.returnMostSignificant(listOfStates)
    print(definingState)
    >>> State.ERROR


Calling ``returnMostSignificant`` from the ``StateSignifier`` without
additional keywords will result in returning evaluation substates
of ``STATIC`` and ``CHANGING``. A priority can be established between
the two direct descendants of ``STATIC`` (``ACTIVE`` and ``PASSIVE``) and
between the two direct descendants of ``CHANGING`` (``INCREASING`` and
``DECREASING``). Those priorities can be controlled by the following
two keywords:

staticSignificant = ``ACTIVE|PASSIVE``
    defines whether ``ACTIVE`` or  ``PASSIVE`` should evaluate as more significant.

changingSignificant = ``INCREASING|DECREASING``
    defines whether ``INCREASING`` or  ``DECREASING`` should evaluate as more significant.

In rare scenarios states might need to be trumped differently. Developers can
provide for a different trumping method in initialization of the ``StateSignifier``.
A list of base states should be provided as the trump list, the order of which
determines trumping and provides the same ``returnMostSignificant`` method as in the
default trumping implementation.

.. code-block:: Python

    from karabo.middlelayer import State, StateSignifier

    trumpList = []
    trumpList.append(State.DISABLED)
    trumpList.append(State.STATIC)
    trumpList.append(State.CHANGING)
    trumpList.append(State.INIT)
    trumpList.append(State.UNKNOWN)
    trumpList.append(State.ERROR)
    myStateSignifier = StateSignifier(trumpList)


    sState = myStateSignifier.returnMostSignificant([State.DISABLED,
                                                     State.INIT])

Derived States
==============

For certain device classes conventions on common state names have
historically grown. Karabo supports these existing state names, by providing
derived states. The diagrams below list these states, in terms
of from the base states they derive.

Interlocked Devices
-------------------

A device which may not be altered because it is in an ``INTERLOCKED`` state is
in a state derived from ``DISABLED``:

.. digraph:: state_transitions

    "DISABLED"[shape = box style=filled, fillcolor="#FF00FF"]
    "INTERLOCKED"[shape = box style=filled, fillcolor="#FF00FF"]

    "DISABLED" -> "INTERLOCKED"

.. note::

    Although the ``INTERLOCKED`` state derives from the ``DISABLED`` state, it
    is much more significant and is trumped by ``State.ERROR``, ``State.INIT`` and
    ``State.UNKNOWN``.

Devices with Binary-like behavior
---------------------------------

Many hardware devices have states which map to a kind of "binary" behavior,
i.e. two states which are the opposite or counterpart of each other, thus
deriving from ``ACTIVE`` and ``PASSIVE``. In each of this states the device
is rather ``STATIC``, which is the base state for both:


.. digraph:: state_transitions

    rankdir = LR;

    "STATIC"[shape = box style=filled, fillcolor="#00AA00"]
    "PASSIVE"[shape = box style=filled, fillcolor="#CCCCFF"]
    "ACTIVE"[shape = box style=filled, fillcolor="#78FF00"]

    "COOLED"[shape = box style=filled, fillcolor="#78FF00"]
    "WARM"[shape = box style=filled, fillcolor="#CCCCFF"]

    "WARM"->"PASSIVE" [dir=back]

    "HEATED"[shape = box style=filled, fillcolor="#78FF00"]
    "COLD"[shape = box style=filled, fillcolor="#CCCCFF"]

    "COLD"->"PASSIVE" [dir=back]

    "EVACUATED"[shape = box style=filled, fillcolor="#78FF00"]
    "PRESSURIZED"[shape = box style=filled, fillcolor="#CCCCFF"]

    "PRESSURIZED"->"PASSIVE" [dir=back]
    "ACTIVE"->"EVACUATED"


    "OPENED"[shape = box style=filled, fillcolor="#78FF00"]
    "CLOSED"[shape = box style=filled, fillcolor="#CCCCFF"]

    "CLOSED"->"PASSIVE" [dir=back]

    "ON"[shape = box style=filled, fillcolor="#78FF00"]
    "OFF"[shape = box style=filled, fillcolor="#CCCCFF"]

    "OFF"->"PASSIVE" [dir=back]

    "EXTRACTED"[shape = box style=filled, fillcolor="#78FF00"]
    "INSERTED"[shape = box style=filled, fillcolor="#CCCCFF"]

    "INSERTED"->"PASSIVE" [dir=back]

    "STARTED"[shape = box style=filled, fillcolor="#78FF00"]
    "STOPPED"[shape = box style=filled, fillcolor="#CCCCFF"]

    "STOPPED"->"PASSIVE" [dir=back]

    "LOCKED"[shape = box style=filled, fillcolor="#78FF00"]
    "UNLOCKED"[shape = box style=filled, fillcolor="#CCCCFF"]

    "UNLOCKED"->"PASSIVE" [dir=back]

    "ENGAGED"[shape = box style=filled, fillcolor="#78FF00"]
    "DISENGAGED"[shape = box style=filled, fillcolor="#CCCCFF"]

    "DISENGAGED"->"PASSIVE" [dir=back]


    "PASSIVE" -> "STATIC"[dir=back]
    "STATIC" -> "ACTIVE"

    "ACTIVE"->"LOCKED"
    "ACTIVE"->"STARTED"
    "ACTIVE"->"EXTRACTED"
    "ACTIVE"->"ON"
    "ACTIVE"->"OPENED"
    "ACTIVE"->"HEATED"
    "ACTIVE"->"COOLED"
    "ACTIVE"->"ENGAGED"


Devices with Transitionatory Behavior
-------------------------------------

Frequently, a transition from one hardware state to another will not be immediate,
but rather take some time, e.g. if a stage is instructed to driver to a new
location, a power supply is ramping to a new voltage or a chiller is set to
a lower temperature. During a longer lasting transition such devices should be
placed into a ``CHANGING`` derived state, possibly also indicating if an increase
or decrease of the value is being performed.

.. digraph:: state_transitions

    rankdir = LR;

    subgraph cluster0{

        rank="same";
        style = invis;
        "ROTATING"[shape = box style=filled, fillcolor="#00AAFF"]
        "CHANGING"[shape = box style=filled, fillcolor="#00AAFF"]
        "MOVING"[shape = box style=filled, fillcolor="#00AAFF"]

        "SWITCHING"[shape = box style=filled, fillcolor="#00AAFF"]

        "ROTATING" -> "CHANGING"[constraint=false, dir=back]
        "CHANGING" -> "MOVING" [constraint=false]
        "CHANGING" -> "SWITCHING"[constraint=false]

    }


    "INCREASING" -> "MOVING" [style="invisible",dir="none"];
    "INCREASING" -> "ROTATING" [style="invisible",dir="none"];
    "INCREASING" -> "SWITCHING" [style="invisible",dir="none"];

    "INCREASING"[shape = box style=filled, fillcolor="#00AAFF"]
    "DECREASING"[shape = box style=filled, fillcolor="#00AAFF"]

    "COOLING"[shape = box style=filled, fillcolor="#00AAFF"]
    "HEATING"[shape = box style=filled, fillcolor="#00AAFF"]


    "MOVING_LEFT"[shape = box style=filled, fillcolor="#00AAFF"]
    "MOVING_RIGHT"[shape = box style=filled, fillcolor="#00AAFF"]
    "MOVING_DOWN"[shape = box style=filled, fillcolor="#00AAFF"]
    "MOVING_UP"[shape = box style=filled, fillcolor="#00AAFF"]
    "MOVING_FORWARD"[shape = box style=filled, fillcolor="#00AAFF"]
    "MOVING_BACK"[shape = box style=filled, fillcolor="#00AAFF"]


    "ROTATING_CLK"[shape = box style=filled, fillcolor="#00AAFF"]
    "ROTATING_CNTCLK"[shape = box style=filled, fillcolor="#00AAFF"]

    "RAMPING_DOWN"[shape = box style=filled, fillcolor="#00AAFF"]
    "RAMPING_UP"[shape = box style=filled, fillcolor="#00AAFF"]

    "EXTRACTING"[shape = box style=filled, fillcolor="#00AAFF"]
    "INSERTING"[shape = box style=filled, fillcolor="#00AAFF"]

    "STOPPING"[shape = box style=filled, fillcolor="#00AAFF"]
    "STARTING"[shape = box style=filled, fillcolor="#00AAFF"]

    "EMPTYING"[shape = box style=filled, fillcolor="#00AAFF"]
    "FILLING"[shape = box style=filled, fillcolor="#00AAFF"]

    "DISENGAGING"[shape = box style=filled, fillcolor="#00AAFF"]
    "ENGAGING"[shape = box style=filled, fillcolor="#00AAFF"]


    "SWITCHING_OFF"[shape = box style=filled, fillcolor="#00AAFF"]
    "SWITCHING_ON"[shape = box style=filled, fillcolor="#00AAFF"]

    "HEATING"->"INCREASING" [dir=back]
    "MOVING_RIGHT"->"INCREASING" [dir=back]
    "MOVING_UP"->"INCREASING" [dir=back]
    "MOVING_FORWARD"->"INCREASING" [dir=back]
    "ROTATING_CLK"->"INCREASING" [dir=back]
    "RAMPING_UP"->"INCREASING" [dir=back]
    "INSERTING"->"INCREASING" [dir=back]
    "STARTING"->"INCREASING" [dir=back]
    "FILLING"->"INCREASING" [dir=back]
    "ENGAGING"->"INCREASING" [dir=back]
    "SWITCHING_ON"->"INCREASING" [dir=back]


    "INCREASING" -> "CHANGING"[dir=back]
    "CHANGING" -> "DECREASING"



    "DECREASING" -> "COOLING"
    "DECREASING" -> "MOVING_LEFT"
    "DECREASING" -> "MOVING_DOWN"
    "DECREASING" -> "MOVING_BACK"
    "DECREASING" -> "ROTATING_CNTCLK"
    "DECREASING" -> "RAMPING_DOWN"
    "DECREASING" -> "EXTRACTING"
    "DECREASING" -> "STOPPING"
    "DECREASING" -> "EMPTYING"
    "DECREASING" -> "DISENGAGING"
    "DECREASING" -> "SWITCHING_OFF"


.. note::

    While comparisons between different derived states are guaranteed to work
    it is good practice to compare to the base state. You can also write
    ``if myState.isDerivedFrom(State.CHANGING)`` and **not**
    ``if myState == State.MOVING``.

Changing States
===============

The device state should be queried and set using the *getState()*
and *updateState()* methods in the *bound* APIs

.. code-block:: Python

    current_state = self.getState()
    ...
    self.updateState(State.MOVING)

In the *middle-layer* API normal property retrieval and assignment will
automatically map to these calls

.. code-block:: Python

    current_state = self.state
    self.state = State.MOVING

.. warning::

    While internally states are serialized as strings, states can only be
    updated by assigning a state enumerator object.

