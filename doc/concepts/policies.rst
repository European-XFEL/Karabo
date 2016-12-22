
********
Policies
********

Device Class Names start Capitalized
    i.e. ``MyDevice``

Property names are lower case
    i.e. ``myProperty``

CamelCasing is preferred to using under_scores.
    although this is not strictly enforced. You should however use a consistent
    naming convention in your device implementations

No Personal Interlocks in Software
    There is a strict policy that Karabo may **never** be used to implement any
    logical functionality related to a personal safety interlock.

Karabo should only view interlocks
    Generally, interlocks should be implemented in hardware or on PLCs, not
    in device software. Karabo may however present a view on a hardware
    interlock by informing on its status.

No Device Releases without Documentation
    Devices without proper documentation may not be released for public use.
    Specifically, a device must document the states that might occur as part
    of its usage and any known limitations of the device.

Property Assignment is **not** an Action
    In Karabo assignment to a property should not trigger an action. The action
    should only be triggered by executing a command after the assignment.
    For example changing the target position of a linear stage will not make
    this stage move, the stage only moves if a ``move()`` command is issued.
    Certain types of hardware may not follow this principle, i.e. they may
    already take action upon setting a property. In these case it is up to
    the device developer to maintain the *assignment is not an action* policy
    by e.g. caching a property change in the device software and only
    forwarding it the hardware upon command execution. See Section
    :ref:`setandexecute` for examples. Exceptions may be mandated for
    specific application scenarios. In such cases the device developer must
    make sure that the ``setAndExecute`` attribute is set for the corresponding
    property.

Middlelayer Devices should not directly control Hardware
    Instead a *bound* API device should be used for the hardware
    interaction and the middle-layer device should provide a higher level
    view on the interface.

The ``ERROR`` State is reserved for Hardware Errors
    Software errors and communication problems should bring the device into
    the ``UNKNOWN`` state.

State-Prioritizating
    should be implemented using the Karabo-provided utilities:
    The default trumping sequence described in Section :ref:`states` is global to
    Karabo and guaranteed to always produce the same results. Accordingly
    you should always use the supplied ``states.returnMostSignificant`` function
    to evaluate the most prominent state. If you need to implement another
    ordering schema ``UserStateSignifier`` must be used.

Use Polling at Frequencies above 10Hz with Care
    If you poll at above 10Hz you may put significant stress on the
    distributed system which is designed to cope with XFEL's train frequency
    of 10Hz. Thus be prudent and only use higher frequencies if there is a
    significant information gain. Data at MHz pulse frequencies should
    rather be grouped into a train and send out as a single message.


Commands in Nodes
    pose a problem because a function name may not be nested as
    ``foo.bar()``, where ``foo`` is the Node (**not an object**) and ``bar``
    is the function. Karabo thus internally will replace the ``.`` separator
    with an underscore (``_``) yielding a function name which is valid in both
    Python and C++: ``foo_bar()``. If you define a slot under a node in the
    expected parameters section

    .. code-block:: Python

        def expectedParameters(expected):

            (
                NODE_ELEMENT(expected).key("foo")
                    .commit()
                    ,
                SLOT_ELEMENT(expected).key("foo.bar")
                    .commit()
            )

    you thus need to bind it to a function ``foo_bar()``:

    .. code-block:: Python

        def initialization(self):
            self.KARABO_SLOT(self.foo_bar)

        def foo_bar(self):
            #... do something useful



No Automatic Unit Conversions
    While Karabo has a notion of units and interprets them e.g. in the GUI
    it does **not** implement **any** unit conversions. It is thus up to the
    device developer to make sure that a calculations use a consistent base
    of units.

Communication Patterns in Karabo
    Karabo devices communicate slow control parameters via a configurable number
    brokers, identified by host and topic, which each
    usually manages an installation on the scale of a hutch. Broker communication
    has been tested to be able to cope with installations of approx. NNNN devices,
    providing event driven property updates in the ~10 Hz range.
    There are however two exceptions to this general rule: as the data loggers need to
    receive messages from every device in the system they connect to all other
    devices in the DCS with p2p connections, thus bypassing the broker. Additionally,
    larger data or per-pulse data should be passed via p2p interfaces. This
    is especially important with respect to data acquisition, as the DAQ system expects
    per pulse data via a p2p interface and **not** as a *slow control* parameter.



Shared Resources
    are devices that communicated over more than one broker. In this case
    outbound messages are simply set in copy to all configured brokers. Inbound
    messages are received on any connection and taken care of in the sequential
    order they occur.



Installations (Topics)
    provide the interfaces which are to be used with the installation. This
    means that a control interface should not be implemented by means of
    inter topic communication in a different way in a different topic. Or
    specifically, an instrument should not reimplement the interface for the
    tunnel installations or the shared laser system.
