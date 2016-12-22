.. _gui_server:

**************
The GUI Server
**************

The Karabo gui-server is the access-point for GUI clients into the DCS. Clients
connect to it using a point-to-point interface. It forwards updates of device
properties to the connected clients requesting monitoring of these properties,
bundles pipeline-processing communication to the GUI and acts as
an interface to the Karabo project manager and data loggers.

The gui server supports two message priorities: those which are guaranteed
to be delivered to the gui clients, i.e. *lossless* and those which get
prioritized such that update rate to the GUI is reasonably throttled.

Client Interface
================

In the following an overview of the gui server's client interface, i.e the
public interface GUI clients act upon, is given.

.. function:: safeClientWrite(channel, message, prio = LOSSLESS)

    writes ``message`` to the specified ``channel`` with the given ``prio``rity


.. function:: safeAllClientsWrite(message, prio = LOSSLESS)

    writes ``message`` to all channels connected to the gui-server device

.. function:: onError(channel, errorMessage)

    an error specified by ``errorMessage`` occurred on the given ``channel``.
    After an error the gui-server will attempt to close the connection on this
    channels, and perform a clean-up of members related to this channel.

.. function:: onGuiError(hash)

    an error further specified by ``hash`` occurred on a connection to a gui
    client. The gui-server will attempt to forward the error to the debug
    channel of the gui client.

.. function:: onConnect(channel)

   connects a client on to the gui server on ``channel``. The channel is
   registered with two priority handlers: remove oldest and loss-less. The
   ``onRead`` and ``onError`` handlers are registered to handle incoming data
   and faults on the channel. Both upon successful completion and exceptions
   in the process the acceptor socket of the gui-server is re-registered so
   that new client connections may be established.

.. function:: onRead(channel, info)

    handles incoming data in the Hash  ``info`` from ``channel``.
    The further actions are determined by the contents of the ``type`` property
    in ``info``. Valid types and their mapping to methods are given in the
    following table:

    .. table:: ``onRead`` allowed types

        ======================= =========================
        type                    resulting method call
        ----------------------- -------------------------
        login                   onLogin
        reconfigure             onReconfigure
        execute                 onExecute
        getDeviceConfiguration  onGetDeviceConfiguration
        getDeviceSchema         onGetDeviceSchema
        getClassSchema          onGetClassSchema
        initDevice              onInitDevice
        killServer              onKillServer
        killDevice              onKillDevice
        startMonitoringDevice   onStartMonitoringDevice
        stopMonitoringDevice    onStopMonitoringDevice
        getPropertyHistory      onGetPropertyHistory
        subscribeNetwork        onSubscribeNetwork
        error                   onGuiError
        getAvailableProjects    onGetAvailableProjects
        newProject              onNewProject
        loadProject             onLoadProject
        saveProject             onSaveProject
        closeProject            onCloseProject
        ======================= =========================

    Both upon successful completion of the request or in case of an exception
    the onRead function is bound to the channel again, maintaining the connection
    of the client to the gui-server.

.. todo::

    The project related calls should likely be moved to the new project
    manager service.


.. function:: onLogin(channel, info)

    Handles a login request of a user on a gui client. If the login credentials
    are valid the current system topology is returned.

.. todo::

    Currently, nothing actually happens here. I would not prioritize user
    management right now, but it should be done when time allows.

.. function:: onReconfigure(info)

    Calls the ``onReconfigure`` slot on the device specified by ``deviceId``
    in ``info`` with the new ``configuration`` from the sent by the gui client.


.. function:: onExecute(info)

    Calls the ``command`` slot on the device specified by ``deviceId``
    in ``info``.


.. function:: onInitDevice(channel, info);

    Instructs the server at ``serverId`` to try initializing the device
    at ``deviceId`` as given in ``info``. The reply from the device server
    is registered to the ``initReply`` callback.

.. function:: initReply(channel, deviceId, success, message)

    is the callback for the ``onInitDevice`` method. It is called upon reply
    from the device server handling the initialization request. The reply is
    passed to the calling ``channel`` in form of a hash message with
    ``type=initReply``, ``deviceId``, ``success`` and ``message`` fields.


.. function:: onGetDeviceConfiguration(channel, info)

    requests the current device configuration for ``deviceId`` specified in
    ``info`` and sends it back in a hash message on ``channel``. The message
    contains the following fields: ``type=deviceConfiguration``, ``deviceId``
    and ``configuration``. The configuration is retrieved using the device
    client interface.

.. function:: onKillServer(info)

    instructs the server specified by ``serverId`` in ``info`` to shutdown.

.. function:: onKillDevice(info)

    instructs the device specified by ``deviceId`` in ``info`` to shutdown.

.. function:: onStartMonitoringDevice(channel, info)

    registers a monitor on the device specified by ``deviceId`` in ``info``
    The monitor is registered as a call-back ``deviceChangedHandler`` on
    the device client. Upon changes of device properties they will be forwarded
    to ``channel`` from this handler. Only one channel per client is maintained
    for passing monitoring information and only on monitor is registered by
    the gui-server for any number of clients monitoring ``deviceId``.

    After successful registration the current device configuration is returned
    by calling ``onGetDeviceConfiguration`` for ``channel``.

.. function:: deviceChangedHandler(deviceId, what)

    acts upon incoming configuration updates from ``deviceId``. It is called
    back by a monitor registered on the device client. The reconfiguration
    contained in the ``what`` hash is forwarded to any channels connected to the
    monitor by ``onStartMonitoringDevice``. The message format of the hash
    sent out is ``type=deviceConfiguration``, ``deviceId`` and ``configuration``,
    the latter containing ``what``.

.. function:: onStopMonitoringDevice(channel, info)

    Deregisters the client connected by ``channel`` from the device specified
    by ``deviceId`` in ``info``. If this is the last channel monitoring
    ``deviceId`` the corresponding monitor is also deregistered from the
    device client.


.. function:: onGetClassSchema(channel, info)

    requests a class schema for the ``classId`` on the server specified by
    ``serverId`` in ``info``. This is done throught the device client. A
    hash reply is sent out over ``channel`` containg ``type=classSchema``,
    ``serverId``, ``classId`` and ``schema``.

.. function:: onGetDeviceSchema(channel, info)

    requests a device schema for the device specified by
    ``deviceId`` in ``info``. This is done throught the device client. A
    hash reply is sent out over ``channel`` containg ``type=deviceSchema``,
    ``deviceId``, and ``schema``.

.. function:: onGetPropertyHistory(channel, info)

    requests the history for a ``property`` on ``deviceId`` in the time range
    ``t0`` and ``t1`` as specified in ``info``. Additionally, the maximum number
    of data points may be specified in ``maxNumData``. The request is
    asynchronously sent to the device logger logging information for ``deviceId``.
    The reply from the logger is then forwarded to the client on ``channel``
    using the ``propertyHistory`` history callback.


.. function:: propertyHistory(channel, deviceId, property, data)

    is the callback for ``onGetPropertyHistory``. It forwards the history reply
    in ``data`` for the ``property`` on ``deviceId`` to the client connected
    on ``channel``. The hash reply is of the format ``type=propertyHistory``,
    ``deviceId``, ``property`` and ``data``.

.. function:: onSubscribeNetwork(channel, info)

    registers the client connected on ``channel`` to a *pipe-lined processing*
    channel identified by ``channelName`` in ``info`` in case ``subscribe``
    is True. In case the *pipe-lined processing* channel is already connected
    to the gui-server no further action is taken. Otherwise, a new connection
    is opened, set to *copy* and  *dropping* behaviour in case the gui-server is busy, and
    with a maximum update frequency as defined by the ``delayOnInput`` property
    of the gui server. Network data from the *pipe-lined processing* connection
    is handled by the ``onNetworkData`` callback.

    In this way only one connection to a given *pipe-lined processing* channel
    is maintained, even if multiple gui-clients listen to it. The gui-server
    thus acts as a kind of hub for *pipe-lined processing* onto gui-clients.

    If subscribe is set to False, the connection is removed from the list of
    registered connections, but is kept open.

.. function:: onNetworkData(input)

    handles ``input`` data from the *pipe-lined processing* channels the gui-server is
    subscribed to and forwards it to the relevant client channels, which have
    connected via ``onSubscribeNetwork``. The incoming data is forwarded
    to all channels connected to this *pipe-lined processing* channels using
    the following hash message format: ``type=networkData``, ``name`` is the
    channel name and ``data`` holding ``input``.

.. function:: onGetAvailableProjects(channel)

   requests the available projects from the Karabo project manager. The reply
   is passed asynchroniously through the ``availableProjects`` call-back.

.. function:: availableProjects(channel, projects)

    is called back upon a ``onGetAvailableProjects`` request. It forwards the
    information provided by the Karabo project manager to the requesting channel:
    ``type=availableProjects`` and ``availableProjects`` is the hash reply
    from the project manager.

.. function:: onNewProject(channel, info)

    triggers the creation of a new project by the client communicating via
    ``channel`` by the Karabo project manager, where
    ``info`` provides ``author``, ``projectName`` as well as project ``data``.
    The response from the project manager is handled asynchroniously using the
    ``projectNew`` callback.


.. function:: projectNew(channel, projectName, success,  data)

    is called back upon reply from the project manager upon a ``onNewProject``
    request. It forwards hashed information to the client communicating on
    ``channel`` where ``type=projectNew``, ``name`` is the projectName,
    ``success`` indicates successful project creation and ``data`` is the
    project data.

.. function:: onLoadProject(channel, info)

    requests loading of a project by the client communicating on ``channel``
    from the project manager. Here ``info`` contains the requesting ``user``
    and project ``name``. Project loading is handled asynchronously by the
    ``projectLoaded`` callback.

.. function:: projectLoaded(channel, projectName, metaData, data)

    is called back upon reply from the project manager upon a ``onLoadProject``
    request from a client communicating on ``channel``. A message with
    ``type=projectLoaded``, ``name`` as project name, ``metaData`` containing
    project metadata and ``data``containing the project data is forwarded to
    the client on ``channel``.

.. function:: onSaveProject(channel, info)

    request saving a project from the client connected on ``channel`` by the
    project manager. In ``info`` the ``user`` name, project ``name`` and
    project ``data`` are expected to be passed. The reply of the request is
    handled by the ``projectSaved`` callback.

.. function:: projectSaved(channel, projectName, success, data)

    is called back upon reply from the project manager upon a ``onSaveProject``
    request from a client communicating on ``channel``. A message with
    ``type=projectSaved``, ``name`` as project name, ``success`` indicating
    a successful save, and ``data``containing the project data is forwarded to
    the client on ``channel``.


.. function:: onCloseProject(channel, info)

    requests closing a project open on the client communicating via ``channel``
    by the project manager. The ``info`` hash should contain the ``user`` name
    and project ``name``. The reply from the project manager is handled by the
    ``projectClosed`` callback.

.. function:: projectClosed(channel, projectName, success, data)

    is called back upon reply from the project manager upon a ``onCloseProject``
    request from a client communicating on ``channel``. A message with
    ``type=projectClosed``, ``name`` as project name, ``success`` indicating
    a successful close, and ``data``containing the project data is forwarded to
    the client on ``channel``.

.. function:: sendSystemTopology(channel)

    sends the current system topology to the client connected on ``channel``.
    The hash reply contains ``type=systemTopology`` and the ``systemTopology``.

.. function:: sendSystemVersion(channel)

    sends the current system topology to the client connected on ``channel``.
    The hash reply contains ``type=systemVersion`` and the ``systemVersion``.

