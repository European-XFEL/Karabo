/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_GUISERVERDEVICE_HH
#define	KARABO_CORE_GUISERVERDEVICE_HH

#include "karabo/devices/ProjectManager.hh"
#include "karabo/net/JmsProducer.hh"
#include "karabo/net/Connection.hh"
#include "karabo/xms/InputChannel.hh"

#include "karabo/core/Device.hh"
#include "karabo/core/OkErrorFsm.hh"

/**
 * The main karabo namespace
 */
namespace karabo {

    namespace devices {

        /**
         * @class GuiServerDevice
         * @brief The GuiServerDevice mediates between GUI clients and the distributed system.
         * 
         * The GuiServerDevice acts as a mediator between the distributed system and GUI clients, 
         * which connect to it through p2p channels. The device centrally manages updates from
         * the distributed system and pushes them to the clients. Conversly, it handles requests
         * by clients and passes them on to devices in the distributed system. 
         */
        class GuiServerDevice : public karabo::core::Device<> {

            struct NetworkConnection {

                std::string name;
                karabo::net::Channel::Pointer channel;
            };

            typedef std::multimap<karabo::xms::InputChannel::Pointer, NetworkConnection> NetworkMap;

            enum QueueBehaviorsTypes {

                REMOVE_OLDEST = 3, LOSSLESS
            };

            karabo::net::Connection::Pointer m_dataConnection;

            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_serializer;
            std::map<karabo::net::Channel::Pointer, std::set<std::string> > m_channels;
            mutable boost::mutex m_channelMutex;
            mutable boost::mutex m_monitoredDevicesMutex;
            mutable boost::mutex m_networkMutex;
            mutable boost::mutex m_pendingAttributesMutex;

            karabo::net::JmsConsumer::Pointer m_loggerConsumer;
            std::map<std::string, int> m_monitoredDevices;
            NetworkMap m_networkConnections;

            karabo::net::JmsProducer::Pointer m_guiDebugProducer;

            std::map<std::string, std::vector<karabo::util::Hash> > m_pendingAttributeUpdates;

            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator ConstChannelIterator;
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::iterator ChannelIterator;

            karabo::util::Hash m_loggerMap;
            karabo::util::Hash m_loggerInput;

            std::set<std::string> m_projectManagers;
            mutable boost::shared_mutex m_projectManagerMutex;

        public:

            KARABO_CLASSINFO(GuiServerDevice, "GuiServerDevice", "2.0")

            static void expectedParameters(karabo::util::Schema& expected);

            GuiServerDevice(const karabo::util::Hash& input);

            virtual ~GuiServerDevice();

            void initialize();


        private: // Functions

            /**
             * writes a message  to the specified channel with the given priority
             * @param channel
             * @param message
             * @param prio
             */
            void safeClientWrite(const karabo::net::Channel::Pointer channel, const karabo::util::Hash& message, int prio = LOSSLESS);

            /**
             * writes message to all channels connected to the gui-server device
             * @param message
             * @param prio
             */
            void safeAllClientsWrite(const karabo::util::Hash& message, int prio = LOSSLESS);

            /**
             * an error specified by ErrorCode e occurred on the given channel.
             * After an error the GUI-server will attempt to close the connection on this
             * channels, and perform a clean-up of members related to this channel.
             * @param e
             * @param channel
             */
            void onError(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel);

            /**
             * an error further specified by hash occurred on a connection to a GUI
             * client. The GUI-server will attempt to forward the error to the debug
             * channel of the GUI client.
             * @param hash
             */
            void onGuiError(const karabo::util::Hash& hash);

            /**
             * connects a client on to the GUI server on channel. The channel is
             * registered with two priority handlers: remove oldest and loss-less. The
             * onRead and onError handlers are registered to handle incoming data
             * and faults on the channel. Both upon successful completion and exceptions
             * in the process the acceptor socket of the GUI-server is re-registered so
             * that new client connections may be established.
             * @param e holds an error code if any error occurs when calling this slot
             * @param channel
             */
            void onConnect(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel);

            /**
             * handles incoming data in the Hash  ``info`` from ``channel``.
             * The further actions are determined by the contents of the ``type`` property
             * in ``info``. Valid types and there mapping to methods are given in the
             * following table:
             * 
             *  \verbatim embed:rst:leading-asterisk
             * 
             * .. table:: ``onRead`` allowed types
             * 
             *      ======================= =========================
             *      type                    resulting method call
             *      ----------------------- -------------------------
             *      login                   onLogin
             *      reconfigure             onReconfigure
             *      execute                 onExecute
             *      getDeviceConfiguration  onGetDeviceConfiguration
             *      getDeviceSchema         onGetDeviceSchema
             *      getClassSchema          onGetClassSchema
             *      initDevice              onInitDevice
             *      killServer              onKillServer
             *      killDevice              onKillDevice
             *      startMonitoringDevice   onStartMonitoringDevice
             *      stopMonitoringDevice    onStopMonitoringDevice
             *      getPropertyHistory      onGetPropertyHistory
             *      subscribeNetwork        onSubscribeNetwork
             *      error                   onGuiError
             *      getAvailableProjects    onGetAvailableProjects
             *      newProject              onNewProject
             *      loadProject             onLoadProject
             *      saveProject             onSaveProject
             *      closeProject            onCloseProject
             *      ======================= =========================
             * 
             * \endverbatim
             * 
             * Both upon successful completion of the request or in case of an exception
             * the onRead function is bound to the channel again, maintaining the connection
             * of the client to the gui-server.
             * @param e holds an error code if any error occurs when calling this slot
             * @param channel
             * @param info
             */
            void onRead(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel, karabo::util::Hash& info);

            /**
             * Handles a login request of a user on a gui client. If the login credentials
             * are valid the current system topology is returned.
             * @param channel
             * @param info
             */
            void onLogin(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Calls the Device::onReconfigure slot on the device specified by ``deviceId``
             * in ``info`` with the new configuration from the sent by the GUI client.
             * @param info
             */
            void onReconfigure(const karabo::util::Hash& info);

            /**
             * Calls the ``command`` slot on the device specified by ``deviceId``
             * in ``info``.
             * @param info
             */
            void onExecute(const karabo::util::Hash& info);

            /**
             * Instructs the server at ``serverId`` to try initializing the device
             * at ``deviceId`` as given in ``info``. The reply from the device server
             * is registered to the ``initReply`` callback.
             * @param channel
             * @param info
             */
            void onInitDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * is the callback for the ``onInitDevice`` method. It is called upon reply
             * from the device server handling the initialization request. The reply is
             * passed to the calling ``channel`` in form of a hash message with
             * ``type=initReply``, ``deviceId``, ``success`` and ``message`` fields.
             * @param channel
             * @param givenDeviceId
             * @param givenConfig
             * @param success
             * @param message
             */
            void initReply(karabo::net::Channel::Pointer channel, const std::string& givenDeviceId, const karabo::util::Hash& givenConfig, bool success, const std::string& message);

            /**
             * requests the current device configuration for ``deviceId`` specified in
             * ``info`` and sends it back in a hash message on ``channel``. The message
             * contains the following fields: ``type=deviceConfiguration``, ``deviceId``
             * and ``configuration``. The configuration is retrieved using the device
             * client interface.
             * @param channel
             * @param info
             */
            void onGetDeviceConfiguration(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * instructs the server specified by ``serverId`` in ``info`` to shutdown.
             * @param info
             */
            void onKillServer(const karabo::util::Hash& info);

            /**
             * instructs the device specified by ``deviceId`` in ``info`` to shutdown.
             * @param info
             */
            void onKillDevice(const karabo::util::Hash& info);

            /**
             * registers a monitor on the device specified by ``deviceId`` in ``info``
             * The monitor is registered as a call-back ``deviceChangedHandler`` on
             * the device client. Upon changes of device properties they will be forwarded
             * to ``channel`` from this handler. Only one channel per client is maintained
             * for passing monitoring information and only on monitor is registered by
             * the gui-server for any number of clients monitoring ``deviceId``.
             * 
             * After successful registration the current device configuration is returned
             * by calling ``onGetDeviceConfiguration`` for ``channel``.
             * @param channel
             * @param info
             */
            void onStartMonitoringDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * De-registers the client connected by ``channel`` from the device specified
             * by ``deviceId`` in ``info``. If this is the last channel monitoring
             * ``deviceId`` the corresponding monitor is also de-registered from the
             * device-client.
             * 
             * @param channel
             * @param info
             */
            void onStopMonitoringDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * requests a class schema for the ``classId`` on the server specified by
             * ``serverId`` in ``info``. This is done through the device client. A
             * hash reply is sent out over ``channel`` containing ``type=classSchema``,
             * ``serverId``, ``classId`` and ``schema``.
             * 
             * @param channel
             * @param info
             */
            void onGetClassSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * requests a device schema for the device specified by
             * ``deviceId`` in ``info``. This is done through the device client. A
             * hash reply is sent out over ``channel`` containing ``type=deviceSchema``,
             * ``deviceId``, and ``schema``.
             * @param channel
             * @param info
             */
            void onGetDeviceSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * requests the history for a ``property`` on ``deviceId`` in the time range
             * ``t0`` and ``t1`` as specified in ``info``. Additional the maximum number
             * of data points may be specified in ``maxNumData``. The request is
             * asynchronously sent to the device logger logging information for ``deviceId``.
             * The reply from the logger is then forwarded to the client on ``channel``
             * using the ``propertyHistory`` history callback.
             * @param channel
             * @param info
             */
            void onGetPropertyHistory(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * is the callback for ``onGetPropertyHistory``. It forwards the history reply
             * in ``data`` for the ``property`` on ``deviceId`` to the client connected
             * on ``channel``. The hash reply is of the format ``type=propertyHistory``,
             * ``deviceId``, ``property`` and ``data``.
             * @param channel
             * @param deviceId
             * @param property
             * @param data
             */
            void propertyHistory(karabo::net::Channel::Pointer channel, const std::string& deviceId, const std::string& property, const std::vector<karabo::util::Hash>& data);

            /**
             * registers the client connected on ``channel`` to a pipe-lined processing
             * channel identified by ``channelName`` in ``info`` in case ``subscribe``
             * is true. In case the pipe-lined processing channel is already connected
             * to the gui-server no futher action is taken. Otherwise, a new connection
             * is opened, set to copy and  dropping behaviour in case the gui-server is busy, and
             * with a maximum update frequency as defined by the ``delayOnInput`` property
             * of the gui server. Network data from the pipe-lined processing connection
             * is handled by the ``onNetworkData`` callback.
             * 
             * In this way only one connection to a given pipe-lined processing channel
             * is maintained, even if multiple gui-clients listen to it. The gui-server
             * thus acts as a kind of hub for pipe-lined processing onto gui-clients.
             * 
             * If subscribe is set to False, the connection is removed from the list of
             * registered connections, but is kept open.
             * 
             * @param channel
             * @param info
             */
            void onSubscribeNetwork(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * handles ``input`` data from the pipe-lined processing channels the gui-server is
             * subscribed to and forwards it to the relevant client channels, which have
             * connected via ``onSubscribeNetwork``. The incoming data is forwarded
             * to all channels connected to this pipe-lined processing channels using
             * the following hash message format: ``type=networkData``, ``name`` is the
             * channel name and ``data`` holding ``input``.
             * 
             * @param input
             */
            void onNetworkData(const karabo::xms::InputChannel::Pointer& input);

            /**
             * requests the available projects from the Karabo project manager. The reply
             * is passed asynchronously through the ``availableProjects`` call-back.
             * @param channel
             */
            void onGetAvailableProjects(karabo::net::Channel::Pointer channel);

            /**
             * is called back upon a ``onGetAvailableProjects`` request. It forwards the
             * information provided by the Karabo project manager to the requesting channel:
             * ``type=availableProjects`` and ``availableProjects`` is the hash reply
             * from the project manager.
             * 
             * @param channel
             * @param projects
             */
            void availableProjects(karabo::net::Channel::Pointer channel,
                                   const karabo::util::Hash& projects);

            /**
             * triggers the creation of a new project by the client communicating via
             * ``channel`` by the Karabo project manager, where
             * ``info`` provides ``author``, ``projectName`` as well as project ``data``.
             * The response from the project manager is handled asynchronously using the
             * ``projectNew`` callback.
             * 
             * @param channel
             * @param info
             */
            void onNewProject(karabo::net::Channel::Pointer channel,
                              const karabo::util::Hash& info);

            /**
             * is called back upon reply from the project manager upon a ``onNewProject``
             * request. It forwards hashed information to the client communicating on
             * ``channel`` where ``type=projectNew``, ``name`` is the projectName,
             * ``success`` indicates successful project creation and ``data`` is the
             * project data.
             * 
             * @param channel
             * @param projectName
             * @param success
             * @param data
             */
            void projectNew(karabo::net::Channel::Pointer channel,
                            const std::string& projectName, bool success,
                            const std::vector<char>& data);

            /**
             * requests loading of a project by the client communicating on ``channel``
             * from the project manager. Here ``info`` contains the requesting ``user``
             * and project ``name``. Project loading is handled asyncronously by the
             * ``projectLoaded`` callback.
             * 
             * @param channel
             * @param info
             */
            void onLoadProject(karabo::net::Channel::Pointer channel,
                               const karabo::util::Hash& info);

            /**
             * is called back upon reply from the project manager upon a ``onLoadProject``
             * request from a client communicating on ``channel``. A message with
             * ``type=projectLoaded``, ``name`` as project name, ``metaData`` containing
             * project metadata and ``data`` containing the project data is forwarded to
             * the client on ``channel``.
             * 
             * @param channel
             * @param projectName
             * @param metaData
             * @param data
             */
            void projectLoaded(karabo::net::Channel::Pointer channel,
                               const std::string& projectName,
                               const karabo::util::Hash& metaData,
                               const std::vector<char>& data);

            /**
             * request saving a project from the client connected on ``channel`` by the
             * project manager. In ``info`` the ``user`` name, project ``name`` and
             * project ``data`` are expected to be passed. The reply of the request is
             * handled by the ``projectSaved`` callback.
             * 
             * @param channel
             * @param info
             */
            void onSaveProject(karabo::net::Channel::Pointer channel,
                               const karabo::util::Hash& info);

            /**
             * is called back upon reply from the project manager upon a ``onSaveProject``
             * request from a client communicating on ``channel``. A message with
             * ``type=projectSaved``, ``name`` as project name, ``success`` indicating
             * a successful save, and ``data`` containing the project data is forwarded to
             * the client on ``channel``.
             * 
             * @param channel
             * @param projectName
             * @param success
             * @param data
             */
            void projectSaved(karabo::net::Channel::Pointer channel,
                              const std::string& projectName, bool success,
                              const std::vector<char>& data);

            /**
             * requests closing a project open on the client communicating via ``channel``
             * by the project manager. The ``info`` hash should contain the ``user`` name
             * and project ``name``. The reply from the project manager is handled by the
             * ``projectClosed`` callback.
             * 
             * @param channel
             * @param info
             */
            void onCloseProject(karabo::net::Channel::Pointer channel,
                                const karabo::util::Hash& info);

            /**
             * is called back upon reply from the project manager upon a ``onCloseProject``
             * request from a client communicating on ``channel``. A message with
             * ``type=projectClosed``, ``name`` as project name, ``success`` indicating
             * a successful close, and ``data`` containing the project data is forwarded to
             * the client on ``channel``.
             * @param channel
             * @param projectName
             * @param success
             * @param data
             */
            void projectClosed(karabo::net::Channel::Pointer channel,
                               const std::string& projectName,
                               bool success,
                               const std::vector<char>& data);

            
            void registerConnect(const karabo::net::Channel::Pointer& channel);

            /**
             * sends the current system topology to the client connected on ``channel``.
             * The hash reply contains ``type=systemTopology`` and the ``systemTopology``.
             * @param channel
             */
            void sendSystemTopology(karabo::net::Channel::Pointer channel);
            
            /**
             * sends the current system topology to the client connected on ``channel``.
             * The hash reply contains ``type=systemVersion`` and the ``systemVersion``.
             * 
             * @param channel
             */
            void sendSystemVersion(karabo::net::Channel::Pointer channel);

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);

            void instanceUpdatedHandler(const karabo::util::Hash& topologyEntry);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            /**
             * acts upon incoming configuration updates from ``deviceId``. It is called
             * back by a monitor registered on the device client. The reconfiguration
             * contained in the ``what`` hash is forwarded to any channels connected to the
             * monitor by ``onStartMonitoringDevice``. The message format of the hash
             * sent out is ``type=deviceConfiguration``, ``deviceId`` and ``configuration``,
             * the latter containing ``what``.
             * 
             * @param channel
             * @param info
             */
            void deviceChangedHandler(const std::string& instanceId, const karabo::util::Hash& what);

            void classSchemaHandler(const std::string& serverId, const std::string& classId, const karabo::util::Schema& classSchema);

            void schemaUpdatedHandler(const std::string& deviceId, const karabo::util::Schema& schema);

            void logHandler(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);

            void slotNotification(const std::string& type, const std::string& shortMessage, const std::string& detailedMessage, const std::string& deviceId);

            void slotLoggerMap(const karabo::util::Hash& loggerMap);

            void onInputChannelConnected(const karabo::xms::InputChannel::Pointer& input, const karabo::net::Channel::Pointer& channel, const std::string& channelName);

            /**
             * Called from instanceNewHandler to handle schema attribute updates which
             * were received at initialization time. The slotUpdateSchemaAttributes slot
             * is invoked if any updates are pending.
             * @param deviceId: the instance id of the new device
             */
            void updateNewInstanceAttributes(const std::string& deviceId);

            /**
             * A slot called by alarm service devices if they want to notify of an alarm update
             * @param alarmServiceId: the instance id of the service device
             * @param type: the type of the update: can be alarmUpdate, alarmInit
             * @param updateRows: the rows which should be updated. This is a Hash of Hashes, were
             * the unique row indices, as managed by the alarm service are keys, and the values are
             * Hashes of the form updateType->entry
             *
             * updateType is a string of any of the following: init, update, delete,
             *  acknowledgeable, deviceKilled, refuseAcknowledgement
             *
             * entry is a Hash with the following entries:
             *
             *  timeOfFirstOccurrence -> string: timestamp of first occurrence of alarm
             *  trainOfFirstOccurrence -> unsigned long long: train id of first occurrence of alarm
             *  timeOfOccurrence -> string: timestamp of most resent occurrence of alarm
             *  trainOfOccurrence -> unsigned long long: train id of most resent occurrence of alarm
             *  needsAcknowledging -> bool: does the alarm require acknowledging
             *  acknowledgeable -> bool: can the alarm be acknowledged
             *  deviceId -> string: deviceId of device that raised the alarm
             *  property -> string: property the alarm refers to
             *  id -> the unique row id
             *
             * it send the following Hash to the client (lossless)
             *
             * Hash h("type", type, "instanceId", alarmServiceId, "rows", updateRows);
             */
            void slotAlarmSignalsUpdate(const std::string& alarmServiceId, const std::string& type, const karabo::util::Hash& updateRows);

            /**
             * Called if the client wants to acknowledge an alarm, by sending a message of type "acknowledgeAlarm"
             * @param channel: the channel the client is connected to
             * @param info: the message from the client. Should contain the unique row ids of the alarms
             * to acknowledge. It is a Hash of Hashes of the same form as described for slotAlarmSignalsUpdate,
             * where the keys give the unique row indices
             */
            void onAcknowledgeAlarm(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Called if a client sends a message of type "requestAlarms"
             * @param channel: the channel the calling client is connected to
             * @param info: message passed from the client. It is a Hash that needs to contain
             * a string field "alarmInstanceId", which either contains a specifiy alarm service's
             * instance id, or an empty string. In the latter case a request for current alarms
             * is forwarded to all known alarm services, otherwise to the specific one. Replies
             * from the alarm services trigger calling "onRequestedAlarmsReply" asynchroniously.
             */
            void onRequestAlarms(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Callback executed upon reply from an alarm service to "onRequestAlarms".
             * @param channel: the client channel the request came from, bound by "onRequestAlarms"
             * @param reply: reply from the alarm service, expected to contain fields
             *
             * instanceId -> string: instance id of the replying alarm service
             * alarms -> Nested Hash of form given in "slotAlarmSignalsUpdate"
             *
             * It sends out a Hash of form:
             * Hash h("type", "alarmInit", "instanceId", reply.get<std::string>("instanceId"), "rows", reply.get<Hash>("alarms"));
             */
            void onRequestedAlarmsReply(karabo::net::Channel::Pointer channel, const karabo::util::Hash& reply);


            /**
             * Executed when the gui requests an update to schema attributes via the updateAttributes signal.
             * @param channel: gui channel that requested the update
             * @param info: updated attributes, expected to be of form Hash("instanceId", str, "updates", vector<Hash>) where
             * each entry in updates is of the form Hash("path", str, "attribute", str, "value", valueType)
             */
            void onUpdateAttributes(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Callback for onUpdateAttributes
             * @param channel: gui channel that requested the update
             * @param reply: reply from the device that performed the attribute update. Is of form
             * Hash("success" bool, "instanceId", str, "updatedSchema", Schema, "requestedUpdate", vector<Hash>)
             * where success indicates a successful update, instanceId the device that performed the update
             * updatedSchema the new valid schema, regardless of success or not, and requestedUpdates the
             * original update request, as received through onUpdateAttributes
             */
            void onRequestedAttributeUpdate(karabo::net::Channel::Pointer channel, const karabo::util::Hash& reply);

            /**
             * Checks if an instance at instanceId is an alarmService and connects to its signals if it is.
             * @param topologyEntry: the topology Hash, from which the class of instanceId will be deduced
             */
            void connectPotentialAlarmService(const karabo::util::Hash& topologyEntry);

            /**
             * Returns the instance type and instance id from a topology entry
             * @param topologyEntry: a Hash of the topology format
             * @param type: string which will afterwards contain type
             * @param instanceId: string which will be filled with the instance id
             */
            void typeAndInstanceFromTopology(const karabo::util::Hash& topologyEntry, std::string& type, std::string& instanceId);

            /**
             * This device may not be locked
             * @return false
             */
            bool allowLock() const {
                return false;
            }

            /**
             * Checks if an instance at instanceId is a ProjectManager. If so, register it to the list of known project
             * services
             * @param topologyEntry: the topology Hash, from which the class of instanceId will be deduced
             */
            void registerPotentialProjectManager(const karabo::util::Hash& topologyEntry);

            /**
             * Return a list of project services known to this GUI server
             * @return
             */
            std::vector<std::string> getKnownProjectManagers() const;

            /**
             * Initialize a configuration database session for a user. This user should later always be passed to subsequent database
             * interactions to identify this session.
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - user: username of the database user
             *          - password: password for this user
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectBeginUserSession(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);


            /**
             * End a configuration database session for a user.
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - user: username of the database user
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectEndUserSession(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Save items to the project database
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - user: username of the database user - identifies the session
             *          - items: a vector of Hashes where each Hash is of the form:
             *                   - xml: xml of the item
             *                   - uuid: uuid of the item
             *                   - overwrite: Boolean indicating behavior in case of revision conflict
             *                   - domain: to write this item to.
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectSaveItems(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);


            /**
             * Load items from project database
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - user: username of the database user - identifies the session
             *          - items: a vector of Hashes where each Hash is of the form:
             *                   - uuid: uuid of the item
             *                   - revision (optional): revision to load. If not given the newest revision will be returned
             *                   - domain: to load this item from.
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectLoadItems(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);


            /**
             * Load items from project database and also batch load direct descendents
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - user: username of the database user - identifies the session
             *          - items: a vector of Hashes where each Hash is of the form:
             *                   - uuid: uuid of the item
             *                   - revision (optional): revision to load. If not given the newest revision will be returned
             *                   - domain: to load this item from.
             *                   - list_tags: list of enclosing tags under which child items are located, e.g. <projects>...</projects>,
             *                                where "project" is as list_tag
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectLoadItemsAndSubs(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);


            /**
             * Request version information for items from the database
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - user: username of the database user - identifies the session
             *          - items: a vector of Hashes where each Hash is of the form:
             *                   - uuid: uuid of the item
             *                   - domain: to load this item from.
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectGetVersionInfo(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Request the list of project manager devices known to the GUI server
             * @param channel from which the request originates
             * @param info is given for compatability with all other calls but not further evaluated.
             *
             * Will write to the calling channel a Hash where "type" = projectListProjectManagers and "reply" is a vector of strings containing
             * the project manager device ids.
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectListProjectManagers(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Request a list of the items present in a domain. Optionally, an item type filter can be specified
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - user: username of the database user - identifies the session
             *          - domain: domain to list items from
             *          - item_types: a vector of strings indicating the itemtypes to filter for.
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectListItems(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Request a list of the domains in the database.
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - user: username of the database user - identifies the session
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectListDomains(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Forward a reply from a remote slot call to a requesting GUI channel.
             * @param channel to forward reply to
             * @param replyType type of reply
             * @param reply the reply to forward
             */
            void forwardReply(karabo::net::Channel::Pointer channel, const std::string& replyType, const karabo::util::Hash& reply);

            /**
             * Check if a given project manager identified by id is known in the distributed system
             * @param channel to forward a failure message to if not
             * @param deviceId of the project manager device
             * @param type of the request
             * @return true if the project manager id exists in the distributed system
             */
            bool checkProjectManagerId(karabo::net::Channel::Pointer channel, const std::string& deviceId, const std::string & type);
        };
    }
}

#endif
