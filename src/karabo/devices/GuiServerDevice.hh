/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_GUISERVERDEVICE_HH
#define KARABO_CORE_GUISERVERDEVICE_HH

#include <atomic>
#include <krb_log4cpp/Priority.hh>
#include <set>
#include <unordered_map>

#include "karabo/core/Device.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/Connection.hh"
#include "karabo/net/UserAuthClient.hh"
#include "karabo/util/Version.hh"
#include "karabo/xms/InputChannel.hh"

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
         * which connect to it through (tcp) channels. The device centrally manages updates from
         * the distributed system and pushes them to the clients. Conversly, it handles requests
         * by clients and passes them on to devices in the distributed system.
         */
        class GuiServerDevice : public karabo::core::Device<> {
            struct DeviceInstantiation {
                boost::weak_ptr<karabo::net::Channel> channel;
                karabo::util::Hash hash;
            };

            struct ChannelData {
                std::set<std::string> visibleInstances;       // deviceIds
                std::set<std::string> requestedDeviceSchemas; // deviceIds
                // key in map is the serverId, values in set are classIds
                std::map<std::string, std::set<std::string>> requestedClassSchemas;
                karabo::util::Version clientVersion;
                bool sendLogs;

                ChannelData() : clientVersion("0.0.0"), sendLogs(true){};

                ChannelData(const karabo::util::Version& version)
                    : clientVersion(version), sendLogs(clientVersion <= karabo::util::Version("2.11.1")){};
            };

            enum NewInstanceAttributeUpdateEvents {

                INSTANCE_NEW_EVENT = 0x01,
                DEVICE_SERVER_REPLY_EVENT = 0x02,

                FULL_MASK_EVENT = INSTANCE_NEW_EVENT | DEVICE_SERVER_REPLY_EVENT,
                INSTANCE_GONE_EVENT
            };

            struct AttributeUpdates {
                int eventMask;
                std::vector<karabo::util::Hash> updates;
            };

            typedef karabo::net::Channel::WeakPointer WeakChannelPointer;
            // There is no way to have a reliable unordered_set of weak pointers...
            // Before C++14 we cannot use unordered_map since that does not (yet) guarantee that we can erase
            // entries while looping over it.
            typedef std::map<std::string, std::set<WeakChannelPointer>> NetworkMap;

            enum QueueBehaviorsTypes {

                FAST_DATA = 2,
                REMOVE_OLDEST,
                LOSSLESS
            };

            karabo::net::Connection::Pointer m_dataConnection;

            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_serializer;
            std::map<karabo::net::Channel::Pointer, ChannelData> m_channels;
            std::map<std::string, AttributeUpdates> m_pendingAttributeUpdates;
            std::queue<DeviceInstantiation> m_pendingDeviceInstantiations;

            mutable boost::mutex m_channelMutex;
            mutable boost::mutex m_networkMutex;
            mutable boost::mutex m_forwardLogsMutex;
            mutable boost::mutex m_pendingAttributesMutex;
            mutable boost::mutex m_pendingInstantiationsMutex;
            // TODO: remove this once "fast slot reply policy" is enforced
            mutable boost::mutex m_timingOutDevicesMutex;

            boost::asio::deadline_timer m_deviceInitTimer;
            boost::asio::deadline_timer m_networkStatsTimer;
            boost::asio::deadline_timer m_forwardLogsTimer;
            boost::asio::deadline_timer m_checkConnectionTimer;

            karabo::net::Broker::Pointer m_loggerConsumer;
            NetworkMap m_networkConnections;
            // Next map<string, ...> not unordered before use of C++14 because we erase from it while looping over it.
            std::map<std::string, std::map<WeakChannelPointer, bool>> m_readyNetworkConnections;

            karabo::net::Broker::Pointer m_guiDebugProducer;

            typedef std::map<karabo::net::Channel::Pointer, ChannelData>::const_iterator ConstChannelIterator;
            typedef std::map<karabo::net::Channel::Pointer, ChannelData>::iterator ChannelIterator;

            mutable boost::mutex m_loggerMapMutex;
            karabo::util::Hash m_loggerMap;
            karabo::util::Hash m_loggerInput;
            std::vector<karabo::util::Hash> m_logCache;

            krb_log4cpp::Priority::Value m_loggerMinForwardingPriority;

            std::set<std::string> m_projectManagers;
            mutable boost::shared_mutex m_projectManagerMutex;

            bool m_isReadOnly;
            static const std::unordered_set<std::string> m_writeCommands;
            static const std::unordered_map<std::string, karabo::util::Version> m_minVersionRestrictions;
            /// In reported failure reasons, this delimiter comes between short message and details like a trace
            static const std::string m_errorDetailsDelim; //

            // TODO: remove this once "fast slot reply policy" is enforced
            // list of devices that do not respect fast slot reply policy
            std::unordered_set<std::string> m_timingOutDevices;

            std::atomic<int> m_timeout; // might overwrite timeout from client if client is smaller

            karabo::net::UserAuthClient m_authClient;

           public:
            KARABO_CLASSINFO(GuiServerDevice, "GuiServerDevice", "karabo-" + karabo::util::Version::getVersion())

            static void expectedParameters(karabo::util::Schema& expected);

            GuiServerDevice(const karabo::util::Hash& input);

            virtual ~GuiServerDevice();

            void initialize();

            virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration) override;

           private: // Functions
            /** Wrapping requestNoWait */
            void loggerMapConnectedHandler();

            /** Called if configuration changed from outside. */
            virtual void postReconfigure();

            /**
             * Starts the deadline timer which throttles device instantiation.
             */
            void startDeviceInstantiation();

            /**
             * Starts the deadline timer which triggers network stats collection
             */
            void startNetworkMonitor();

            /**
             * Starts the deadline timer which forwards the cached log messages
             */
            void startForwardingLogs();

            /**
             * Starts the deadline timer which monitors connection queues
             *
             * @param currentSuspects Hash with pending message counts - keys are bad client addresses
             */
            void startMonitorConnectionQueues(const karabo::util::Hash& currentSuspects);

            /**
             * Perform network stats collection
             */
            void collectNetworkStats(const boost::system::error_code& error);

            /**
             * writes a message  to the specified channel with the given priority
             * @param channel
             * @param message
             * @param prio
             */

            /**
             * Perform forwarding logs
             */
            void forwardLogs(const boost::system::error_code& error);

            /**
             * Deferred disconnect handler launched by a deadline timer.
             */
            void deferredDisconnect(const boost::system::error_code& err, WeakChannelPointer channel,
                                    boost::shared_ptr<boost::asio::deadline_timer> timer);

            void safeClientWrite(const WeakChannelPointer channel, const karabo::util::Hash& message,
                                 int prio = LOSSLESS);

            /**
             * writes message to all channels connected to the gui-server device
             * @param message
             * @param prio
             */
            void safeAllClientsWrite(const karabo::util::Hash& message, int prio = LOSSLESS);


            /**
             * @brief Sends a login error message to the client currently connected and
             * closes the connection after a time interval elapses.
             *
             * @param channel the channel the client to be notified and disconnected is connected.
             * @param userId the id of the user whose login attempt failed.
             * @param cliVersion the version of the GUI client attempting to login.
             * @param errorMsg the error message to be sent to the client.
             */
            void sendLoginErrorAndDisconnect(const karabo::net::Channel::Pointer& channel, const std::string& userId,
                                             const std::string& cliVersion, const std::string& errorMsg);

            /**
             * an error specified by ErrorCode e occurred on the given channel.
             * After an error the GUI-server will attempt to disconnect this channel.
             * @param e
             * @param channel
             */
            void onError(const karabo::net::ErrorCode& e, WeakChannelPointer channel);

            /**
             * validate the incoming type and info hash if a readOnly command is requested to be executed
             * @param type
             * @param info
             * @return bool whether the request violates read-only restrictions
             */
            bool violatesReadOnly(const std::string& type, const karabo::util::Hash& info);

            /**
             * validates the client configuration
             *
             * currently only validating the type versus the client version.
             * @param type
             * @param channel
             * @return bool whether the request violates client validation
             */
            bool violatesClientConfiguration(const std::string& type, WeakChannelPointer channel);

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

            /** Creates the internal ChannelData entry and update the device Configuration
             */
            void registerConnect(const karabo::util::Version& version, const karabo::net::Channel::Pointer& channel);

            /** handles incoming data in the Hash  ``info`` from ``channel``
             *
             * When the client has connected, only the ``login`` ``type`` is
             * allowed.
             * The expected hash must contain a ``version`` string and a ``user`` string.
             * The ``version`` string is verified against the minimum client version.
             * The ``user`` string is required but currently not used.
             *
             * Upon successful completion of the login request the ``onRead``
             * function is bound to the channel, allowing normal operation.
             * In case of failure the ```onLoginMessage` is bound again to the channel.
             *
             * @param e holds an error code if the eventloop cancel this task or the channel is closed
             * @param channel
             * @param info
             */
            void onLoginMessage(const karabo::net::ErrorCode& e, const karabo::net::Channel::Pointer& channel,
                                karabo::util::Hash& info);

            /**
             * Handles a login request of a user on a gui client. If the login credentials
             * are valid the current system topology is returned.
             * @param channel
             * @param info
             */
            void onLogin(const karabo::net::Channel::Pointer& channel, const karabo::util::Hash& info);

            /**
             * @brief Handles the result of the authorize one-time token operation performed as part of a GUI client
             * login on behalf of an authenticated user.
             *
             * @param channel the communication channel established with the GUI client logging in.
             * @param userId the ID of the user on whose behalf the login is being made.
             * @param cliVersion the version of the GUI client logging in.
             * @param authResult the result of the one-time token authorization operation to be handled.
             */
            void onTokenAuthorizeResult(const WeakChannelPointer& channel, const std::string& userId,
                                        const karabo::util::Version& cliVersion,
                                        const karabo::net::OneTimeTokenAuthorizeResult& authResult);

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
             *      =============================  =========================
             *      type                           resulting method call
             *      -----------------------------  -------------------------
             *      requestFromSlot                onRequestFromSlot
             *      reconfigure                    onReconfigure
             *      execute                        onExecute
             *      getDeviceConfiguration         onGetDeviceConfiguration
             *      getDeviceSchema                onGetDeviceSchema
             *      getClassSchema                 onGetClassSchema
             *      initDevice                     onInitDevice
             *      killServer                     onKillServer
             *      killDevice                     onKillDevice
             *      startMonitoringDevice          onStartMonitoringDevice
             *      stopMonitoringDevice           onStopMonitoringDevice
             *      getPropertyHistory             onGetPropertyHistory
             *      getConfigurationFromPast       onGetConfigurationFromPast
             *      subscribeNetwork               onSubscribeNetwork
             *      requestNetwork                 onRequestNetwork
             *      error                          onGuiError
             *      acknowledgeAlarm               onAcknowledgeAlarm
             *      requestAlarms                  onRequestAlarms
             *      updateAttributes               onUpdateAttributes
             *      projectUpdateAttribute         onProjectUpdateAttribute
             *      projectBeginUserSession        onProjectBeginUserSession
             *      projectEndUserSession          onProjectEndUserSession
             *      projectSaveItems               onProjectSaveItems
             *      projectLoadItems               onProjectLoadItems
             *      projectListProjectManagers     onProjectListProjectManagers
             *      projectListItems               onProjectListItems
             *      projectListProjectsWithDevice  onProjectListProjectsWithDevice
             *      projectListDomains             onProjectListDomains
             *      requestGeneric                 onRequestGeneric
             *      subscribeLogs                  onSubscribeLogs
             *      setLogPriority                 onSetLogPriority
             *      =============================  =========================
             *
             * \endverbatim
             *
             * Both upon successful completion of the request or in case of an exception
             * the ``onRead`` function is bound to the channel again, maintaining the connection
             * of the client to the gui-server.
             * @param e holds an error code if the eventloop cancel this task or the channel is closed
             * @param channel
             * @param info
             */
            void onRead(const karabo::net::ErrorCode& e, WeakChannelPointer channel, karabo::util::Hash& info);

            /**
             * Sets the appropriate timeout to a Requestor
             *
             * If input has a "timeout" key, set the maximum value of that and the gui server timeout on the requestor,
             * except if input.get<std::string>(instanceKey) is one instance of the classes in "ignoreTimeoutClasses".
             */
            void setTimeout(karabo::xms::SignalSlotable::Requestor& requestor, const karabo::util::Hash& input,
                            const std::string& instanceKey);

            /**
             * Callback helper for ``onReconfigure``
             *
             * @param success whether call succeeded
             * @param channel who requested the call
             * @param input will be copied to the key ``input`` of the reply message
             */
            void forwardReconfigureReply(bool success, WeakChannelPointer channel, const karabo::util::Hash& input);

            /**
             * Callback helper for generic actions called by the gui server.
             *
             * @param success whether call succeeded
             * @param channel who requested the call
             * @param info the input info Hash
             * @param reply the reply from the remote device or an empty Hash on failure
             */
            void forwardHashReply(bool success, WeakChannelPointer channel, const karabo::util::Hash& info,
                                  const karabo::util::Hash& reply);

            /**
             * Request a generic action internally.
             * @param channel from which the request originates
             * @param info is a Hash that should containing the slot information.
             *  - type: requestGeneric
             *  - instanceId: the instanceId to be called
             *  - slot: the slot name of the instance
             *  - empty: if this property is provided, the input Hash is not bounced back
             *  - replyType (optional): the value of the key ``type`` in the reply to the client
             *  - timeout (optional) [s]: account for the slot call a specified timeout in seconds!
             *  - args: The Hash containing the parameters for the slot call
             *
             *  Generic interface to call slots that take a single Hash as argument and reply
             *  with a single Hash.
             *
             *  The `forwardHashReply` method is used to relay information to the gui client.
             *
             *  Returns:
             *  --------
             *
             *  In the default case, the return Hash is composed as follows::
             *
             *  - success: boolean to indicate if the generic request was successful
             *  - reason: information on the error if not succesful otherwise empty
             *  - type: if specified in the input Hash, the `replyType` is used otherwise `requestGeneric`
             *  - request: the full input Hash information, including `args`
             *  - reply: The reply Hash of the instanceId
             *
             *  .. note: If the info Hash from the client provides a `empty` property, an empty
             *           Hash is send back to the client instead of the input Hash.
             */
            void onRequestGeneric(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Calls the Device::onReconfigure slot on the device specified in ``info``.
             *
             * The info should have the following entries:
             * - string at ``deviceId`` defines the target device
             * - Hash at ``configuration`` is the configuration update to apply
             * - bool at ``reply``: if given and true, success or failure will be reported back
             *                      to channel by a message of type ``reconfigureReply`` that contains
             *                      * ``input``: the Hash given here as ``info``
             *                      * ``success``: bool whether reconfiguration succeeded
             *                      * ``failureReason``: string with failure reason
             * - optional int at ``timeout``: if a reply should be reported back, defines seconds of timeout.
             *                                In case ``timeout`` is missing, timeout errors will report ``success``
             *                                as true but provides a ``failureReason`` mentioning the timeout
             *
             * @param channel to potentially send "reconfigureReply"
             * @param info
             */
            void onReconfigure(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Callback helper for ``onExecute``
             *
             * @param success whether call succeeded
             * @param channel who requested the call
             * @param input will be copied to the key ``input`` of the reply message
             */
            void forwardExecuteReply(bool success, WeakChannelPointer channel, const karabo::util::Hash& input);

            /**
             * Calls a ``command`` slot on a specified device.
             *
             * The info should have the following entries:
             * - string at ``deviceId`` defines the target device
             * - string at ``command`` is the slot to call
             * - bool at ``reply``: if given and true, success or failure will be reported back
             *                      to channel by a message of type ``executeReply`` that contains
             *                      * ``input``: the Hash given here as ``info``
             *                      * ``success``: bool whether execution succeeded
             *                      * ``failureReason``: string with failure reason
             * - optional int at ``timeout``: if a reply should be reported back, defines seconds of timeout.
             *                                In case ``timeout`` is missing, timeout errors will report ``success``
             *                                as true but provides a ``failureReason`` mentioning the timeout
             * @param channel
             * @param info
             */
            void onExecute(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Enqueues a future device instantiation. The relevant information will be
             * stored in ``m_pendingDeviceInstantiations`` and ``initSingleDevice``
             * will take care of the actual instantiation when it is called by the
             * instantiation timer.
             * @param channel
             * @param info
             */
            void onInitDevice(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Instructs the server at ``serverId`` to try initializing the device
             * at ``deviceId`` as given in ``info``. The reply from the device server
             * is registered to the ``initReply`` callback.
             *
             * NOTE: This should only be called by ``m_deviceInitTimer``
             * @param error
             */
            void initSingleDevice(const boost::system::error_code& error);

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
            void initReply(WeakChannelPointer channel, const std::string& givenDeviceId,
                           const karabo::util::Hash& givenConfig, bool success, const std::string& message,
                           bool isFailureHandler);

            /**
             * requests the current device configuration for ``deviceId`` specified in
             * ``info`` and sends it back in a hash message on ``channel``. The message
             * contains the following fields: ``type=deviceConfiguration``, ``deviceId``
             * and ``configuration``. The configuration is retrieved using the device
             * client interface.
             * @param channel
             * @param info
             */
            void onGetDeviceConfiguration(WeakChannelPointer channel, const karabo::util::Hash& info);

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
             * Registers a monitor on the device specified by ``deviceId`` in ``info``
             * Upon changes of device properties they will be forwarded to ``channel``
             * from a handler for changes in configurations of monitored devices that
             * is kept internally by the gui-server.
             *
             * Only one channel per client is maintained for passing monitoring
             * information and only one monitor is registered by the gui-server for any
             * number of clients monitoring ``deviceId``.
             *
             * After successful registration the current device configuration is returned
             * by calling ``onGetDeviceConfiguration`` for ``channel``.
             * @param channel
             * @param info
             */
            void onStartMonitoringDevice(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * De-registers the client connected by ``channel`` from the device specified
             * by ``deviceId`` in ``info``. If this is the last channel monitoring
             * ``deviceId`` the device is removed from the set of devices monitored by the
             * device-client.
             *
             * @param channel
             * @param info
             */
            void onStopMonitoringDevice(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * requests a class schema for the ``classId`` on the server specified by
             * ``serverId`` in ``info``. This is done through the device client. A
             * hash reply is sent out over ``channel`` containing ``type=classSchema``,
             * ``serverId``, ``classId`` and ``schema``.
             *
             * @param channel
             * @param info
             */
            void onGetClassSchema(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * requests a device schema for the device specified by
             * ``deviceId`` in ``info``. This is done through the device client. A
             * hash reply is sent out over ``channel`` containing ``type=deviceSchema``,
             * ``deviceId``, and ``schema``.
             * @param channel
             * @param info
             */
            void onGetDeviceSchema(WeakChannelPointer channel, const karabo::util::Hash& info);

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
            void onGetPropertyHistory(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Callback for ``onGetPropertyHistory``.
             * It forwards the history reply in ``data`` for the ``property`` on ``deviceId`` to the client connected
             * on ``channel``. The hash reply is of the format ``type=propertyHistory``,
             * ``deviceId``, ``property``, ``success``, ``data`` and ``failureReason``
             * which states the failure reason if any.
             * @param channel
             * @param success whether the request succeeded
             * @param deviceId
             * @param property
             * @param data
             */
            void propertyHistory(WeakChannelPointer channel, bool success, const std::string& deviceId,
                                 const std::string& property, const std::vector<karabo::util::Hash>& data);

            /**
             * Request configuration for a ``device`` at point in time ``time`` as specified in ``info``.
             * The ``info`` hash can as well have a ``preview`` boolean which is send back to the client.
             * The request is asynchronously sent to the device logger logging information for ``deviceId``.
             * The reply from the logger is then forwarded to the client on ``channel``
             * using the ``configurationFromPast`` history callback in case of success or ``configurationFromPastError``
             * for failures.
             */
            void onGetConfigurationFromPast(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Success callback for ``onGetDeviceConfiguration``
             */
            void configurationFromPast(WeakChannelPointer channel, const std::string& deviceId, const std::string& time,
                                       const bool& preview, const karabo::util::Hash& config,
                                       const karabo::util::Schema& /*schema*/, const bool configAtTimepoint,
                                       const std::string& configTimepoint);

            /**
             * Failure callback for ``onGetDeviceConfiguration``
             */
            void configurationFromPastError(WeakChannelPointer channel, const std::string& deviceId,
                                            const std::string& time);

            /**
             * Helper for history retrieval functions
             * @param deviceId of the device whose history is searched for
             * @return id of DataLogReader device to ask for history
             */
            std::string getDataReaderId(const std::string& deviceId) const;

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
             * If ``subscribe`` is set to false, the connection is removed from the list of
             * registered connections, but is kept open.
             *
             * @param channel
             * @param info
             */
            void onSubscribeNetwork(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * registers the client connected on ``channel`` to the system logs
             * in case ``subscribe`` is true.
             * If ``subscribe`` is set to false, the logs will not be sent to the client.
             *
             * @param channel
             * @param info
             */
            void onSubscribeLogs(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * sets the Log priority on a server. The ``info`` hash should contain a ``priority``
             * string and a ``instanceId`` string.
             *
             * @param channel
             * @param info
             */
            void onSetLogPriority(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Callback helper for ``onSetLogPriority``
             *
             * @param success whether call succeeded
             * @param channel who requested the call
             * @param input will be copied to the key ``input`` of the reply message
             */
            void forwardSetLogReply(bool success, WeakChannelPointer channel, const karabo::util::Hash& input);

            /**
             * Receives a message from the GUI client that it processed network data from
             * an output channel with name ``channelName`` in the info Hash.
             *
             * @param channel
             * @param info
             */
            void onRequestNetwork(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * handles data from the pipe-lined processing channels the gui-server is
             * subscribed to and forwards it to the relevant client channels, which have
             * connected via ``onSubscribeNetwork``. The incoming data is forwarded
             * to all channels connected to this pipe-lined processing channel using
             * the following hash message format: ``type=networkData``, ``name`` is the
             * channel name and ``data`` holding the data.
             *
             * @param channelName: name of the InputChannel that provides these data
             * @param data: the data coming from channelName
             * @param meta: corresponding meta data
             */
            void onNetworkData(const std::string& channelName, const karabo::util::Hash& data,
                               const karabo::xms::InputChannel::MetaData& meta);

            /**
             * sends the current system topology to the client connected on ``channel``.
             * The hash reply contains ``type=systemTopology`` and the ``systemTopology``.
             * @param channel
             */
            void sendSystemTopology(WeakChannelPointer channel);

            /**
             * sends the current system topology to the client connected on ``channel``.
             * The hash reply contains ``type=systemVersion`` and the ``systemVersion``.
             *
             * @param channel
             */

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& /* instInfo */);

            /**
             * Handles events related to instances: new instance, instance updated, instance gone.
             *
             * @Note: Its signature matches karabo::core::InstanceChangeThrottler::InstanceChangeHandler).
             */
            void instanceChangeHandler(const karabo::util::Hash& instChangeData);

            /**
             * Acts upon incoming configuration updates from one or more devices. It is called
             * back by a monitor registered on the device client. The reconfiguration
             * contained in the ``what`` hash is forwarded to any channels connected to the
             * monitor by ``onStartMonitoringDevice``.
             *
             * The message type of the hash sent out is type="deviceConfigurations". The hash
             * has a second first level key, named "configurations", whose value is a hash with
             * the deviceIds as keys and the configuration changes for the corresponding deviceId
             * as values.
             *
             * @param what A hash containing all the configuration changes that happened to one
             *        or more monitored devices since the last update. Each node under the key
             *        "configurations" has the 'deviceId' as key and the changed configurations
             *        as a value of type Hash.
             */
            void devicesChangedHandler(const karabo::util::Hash& what);

            void classSchemaHandler(const std::string& serverId, const std::string& classId,
                                    const karabo::util::Schema& classSchema);

            void schemaUpdatedHandler(const std::string& deviceId, const karabo::util::Schema& schema);

            void logHandler(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);

            void slotLoggerMap(const karabo::util::Hash& loggerMap);

            /**
             * Called from projectManagers to notify about updated Projects
             * @param info: the info hash containing the information about the updated projects
             * @param instanceId: the instance id of the project manager device
             */
            void slotProjectUpdate(const karabo::util::Hash& info, const std::string& instanceId);

            /**
             * Slot to dump complete debug info to log file
             *
             * Same info as received from 'slotDumpDebugInfo' with empty input Hash
             */
            void slotDumpToLog();

            void slotDumpDebugInfo(const karabo::util::Hash& info);

            /**
             * Slot to send a notification message to all clients connected - replies empty Hash
             *
             * @param info a Hash with following keys
             *              * "message": a string containing the notification type
             *              * "contentType": a string defining the type of notification as the GUI client understands it
             *                               - "banner" means message will go to the GUI banner. Therefore it will be
             * stored in the "bannerMessage" property of the GuiServerDevice and sent to any client that connects.
             *                               - other types will likely just be shown in a pop-up window of the client
             */
            void slotNotify(const karabo::util::Hash& info);

            /**
             * Slot to send a Hash to the GUI clients connected - replies empty Hash
             *
             * WARNING: No checks are performed on this slot. This slot can possibly disconnect all clients.
             *          Do not use it unless you understand the risks.
             *
             * @param info a Hash with at least the following keys.
             *              * "message": a Hash that will be sent to the client(s).
             *                           It should contain a "type" string.
             *              * "clientAddress": a string containing the GUI client address as coded in the
             *                                 `slotDumpDebugInfo` results.
             *                                 If the value for this key is an empty string, all clients will be
             * notified.
             */
            void slotBroadcast(const karabo::util::Hash& info);

            /**
             * Helper for 'slotDumpToLog' and 'slotDumpDebugInfo'
             */
            karabo::util::Hash getDebugInfo(const karabo::util::Hash& info);

            void monitorConnectionQueues(const boost::system::error_code& err,
                                         const karabo::util::Hash& lastCheckSuspects);

            /**
             * Slot to force disconnection of client. Reply is whether specified client found.
             *
             * @param client string to identify client, as can be received via slotDumpDebugInfo(Hash("clients", 0))
             */
            void slotDisconnectClient(const std::string& client);

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
            void slotAlarmSignalsUpdate(const std::string& alarmServiceId, const std::string& type,
                                        const karabo::util::Hash& updateRows);

            /**
             * Called if the client wants to acknowledge an alarm, by sending a message of type "acknowledgeAlarm"
             * @param channel: the channel the client is connected to
             * @param info: the message from the client. Should contain the unique row ids of the alarms
             * to acknowledge. It is a Hash of Hashes of the same form as described for slotAlarmSignalsUpdate,
             * where the keys give the unique row indices
             */
            void onAcknowledgeAlarm(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Called if a client sends a message of type "requestAlarms"
             * @param channel: the channel the calling client is connected to
             * @param info: message passed from the client. It is a Hash that needs to contain
             * a string field "alarmInstanceId", which either contains a specifiy alarm service's
             * instance id, or an empty string. In the latter case a request for current alarms
             * is forwarded to all known alarm services, otherwise to the specific one. Replies
             * from the alarm services trigger calling "onRequestedAlarmsReply" asynchroniously.
             * @param replyToAllClients: If true, reply to all clients instead of only the
             * requesting client.
             */
            void onRequestAlarms(WeakChannelPointer channel, const karabo::util::Hash& info,
                                 const bool replyToAllClients = false);

            /**
             * Callback executed upon reply from an alarm service to "onRequestAlarms".
             * @param channel: the client channel the request came from, bound by "onRequestAlarms"
             * @param reply: reply from the alarm service, expected to contain fields
             * @param replyToAllClients: If true, reply to all clients
             *
             * instanceId -> string: instance id of the replying alarm service
             * alarms -> Nested Hash of form given in "slotAlarmSignalsUpdate"
             *
             * It sends out a Hash of form:
             * Hash h("type", "alarmInit", "instanceId", reply.get<std::string>("instanceId"), "rows",
             * reply.get<Hash>("alarms"));
             */
            void onRequestedAlarmsReply(WeakChannelPointer channel, const karabo::util::Hash& reply,
                                        const bool replyToAllClients);


            /**
             * Executed when the gui requests an update to schema attributes via the updateAttributes signal.
             * @param channel: gui channel that requested the update
             * @param info: updated attributes, expected to be of form Hash("instanceId", str, "updates", vector<Hash>)
             * where each entry in updates is of the form Hash("path", str, "attribute", str, "value", valueType)
             */
            void onUpdateAttributes(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Callback for onUpdateAttributes
             * @param channel: gui channel that requested the update
             * @param reply: reply from the device that performed the attribute update. Is of form
             * Hash("success" bool, "instanceId", str, "updatedSchema", Schema, "requestedUpdate", vector<Hash>)
             * where success indicates a successful update, instanceId the device that performed the update
             * updatedSchema the new valid schema, regardless of success or not, and requestedUpdates the
             * original update request, as received through onUpdateAttributes
             */
            void onRequestedAttributeUpdate(WeakChannelPointer channel, const karabo::util::Hash& reply);

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
            void typeAndInstanceFromTopology(const karabo::util::Hash& topologyEntry, std::string& type,
                                             std::string& instanceId);

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
             * Initialize a configuration database session for a user.
             * The token should continue to be passed to subsequent database
             * interactions to identify this session.
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - token: token of the database user
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectBeginUserSession(WeakChannelPointer channel, const karabo::util::Hash& info);


            /**
             * End a configuration database session for a user.
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - token: token of the database user
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectEndUserSession(WeakChannelPointer channel, const karabo::util::Hash& info);


            /**
             * Save items to the project database
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - token: token of the database user - identifies the session
             *          - items: a vector of Hashes where each Hash is of the form:
             *                   - xml: xml of the item
             *                   - uuid: uuid of the item
             *                   - overwrite: Boolean indicating behavior in case of revision conflict
             *                   - domain: to write this item to.
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectSaveItems(WeakChannelPointer channel, const karabo::util::Hash& info);


            /**
             * Load items from project database
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - token: token of the database user - identifies the session
             *          - items: a vector of Hashes where each Hash is of the form:
             *                   - uuid: uuid of the item
             *                   - revision (optional): revision to load. If not given the newest revision will be
             * returned
             *                   - domain: to load this item from.
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectLoadItems(WeakChannelPointer channel, const karabo::util::Hash& info);


            /**
             * Request the list of project manager devices known to the GUI server
             * @param channel from which the request originates
             * @param info is given for compatability with all other calls but not further evaluated.
             *
             * Will write to the calling channel a Hash where "type" = projectListProjectManagers and "reply" is a
             * vector of strings containing the project manager device ids. For the reply written to channel see the
             * documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectListProjectManagers(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Request a list of the items present in a domain. Optionally, an item type filter can be specified
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - token: token of the database user - identifies the session
             *          - domain: domain to list items from
             *          - item_types: a vector of strings indicating the itemtypes to filter for.
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectListItems(WeakChannelPointer channel, const karabo::util::Hash& info);


            /**
             * Request a list of the domains in the database.
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - token: token of the database user - identifies the session
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectListDomains(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Update item attributes in the project database
             * @param channel from which the request originates
             * @param info is a Hash that should contain:
             *          - projectManager: project manager device to forward request to
             *          - token: token of the database user - identifies the session
             *          - items: a vector of Hashes where each Hash is of the form:
             *                   - domain: to load this item from
             *                   - uuid: uuid of the item
             *                   - item_type: indicate type of item which attribute should be changed
             *                   - attr_name: name of attribute which should be changed
             *                   - attr_value: value of attribute which should be changed
             * For the reply written to channel see the documentation of karabo.bound_devices.ProjectManager
             */
            void onProjectUpdateAttribute(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Forward a reply from a remote slot call to a requesting GUI channel.
             * @param channel to forward reply to
             * @param replyType type of reply
             * @param reply the reply to forward
             */
            void forwardReply(WeakChannelPointer channel, const std::string& replyType,
                              const karabo::util::Hash& reply);

            /**
             * Forward a reply from a remote slot call to a requesting GUI channel adding a token relevant to the callee
             * to the response
             * @param channel to forward reply to
             * @param replyType type of reply
             * @param reply the reply to forward
             * @param token generated by the calling client
             */
            void forwardRequestReply(WeakChannelPointer channel, const karabo::util::Hash& reply,
                                     const std::string& origin);

            /**
             * Check if a given project manager identified by id is known in the distributed system
             * @param channel to forward a failure message to if not
             * @param deviceId of the project manager device
             * @param type of the request
             * @return true if the project manager id exists in the distributed system
             */
            bool checkProjectManagerId(WeakChannelPointer channel, const std::string& deviceId, const std::string& type,
                                       const std::string& reason);

            /**
             * Calls the ``request`` slot on the device specified by ``deviceId``
             * in ``info`` with args given in ``info.args`` and returns its reply.
             * @param info
             */
            void onRequestFromSlot(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * Error handler to be called in case of remote errors resulting from requests.
             */
            void onRequestFromSlotErrorHandler(WeakChannelPointer channel, const karabo::util::Hash& info,
                                               const std::string& token);

            /**
             * Utility for getting a "name" from client connections.
             */
            std::string getChannelAddress(const karabo::net::Channel::Pointer& channel) const;

            /**
             * Possibly update schema attributes on device
             */
            void tryToUpdateNewInstanceAttributes(const std::string& deviceId, const int callerMask);

            /**
             * Response handler for updating schema attributes on device
             */
            void onUpdateNewInstanceAttributesHandler(const std::string& deviceId, const karabo::util::Hash& response);

            /**
             * Helper Function to identify whether a device belongs to the timeout violation list
             * TODO: remove this once "fast slot reply policy" is enforced
             *
             * returns true if a `.timeout()` should be skipped on execution requestor
             */
            bool skipExecutionTimeout(const std::string& deviceId);

            /**
             * Helper Function to recalculate the list of timeout violating devices from the list of offending classes
             * TODO: remove this once "fast slot reply policy" is enforced
             */
            void recalculateTimingOutDevices(const karabo::util::Hash& topologyEntry,
                                             const std::vector<std::string>& timingOutClasses, bool clearSet);
        };
    } // namespace devices
} // namespace karabo

#endif
