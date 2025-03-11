/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef KARABO_CORE_GUISERVERDEVICE_HH
#define KARABO_CORE_GUISERVERDEVICE_HH

#include <atomic>
#include <memory>
#include <set>
#include <shared_mutex>
#include <unordered_map>

#include "karabo/core/Device.hh"
#include "karabo/devices/GuiServerTemporarySessionManager.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/Connection.hh"
#include "karabo/net/UserAuthClient.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/Schema.hh"
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
        class GuiServerDevice : public karabo::core::Device {
            struct DeviceInstantiation {
                std::weak_ptr<karabo::net::Channel> channel;
                karabo::util::Hash hash;
            };

            struct ChannelData {
                std::set<std::string> visibleInstances;       // deviceIds
                std::set<std::string> requestedDeviceSchemas; // deviceIds
                // key in map is the serverId, values in set are classIds
                std::map<std::string, std::set<std::string>> requestedClassSchemas;
                karabo::util::Version clientVersion;
                // The userId for a GUI Client session. If the client session
                // uses user authentication this will be the authenticated
                // user; otherwise it will be the user running the GUI client
                // on the remote host.
                std::string userId;
                // The one-time token for an authenticated GUI Client session -
                // used for logging locally - the log files must, for privacy
                // reasons not contain any userId associated to execution of
                // operations.
                std::string oneTimeToken;
                // Timestamp for the start of the GUI Client session.
                karabo::util::Epochstamp sessionStartTime;
                // The userId for an authenticated GUI Client temporary session.
                // Temporary sessions can only be "derived" from a user authenticated
                // session and can only exist for a limited amount of time. A
                // temporary session is started after a successful authentication of
                // the user requesting its begining.
                std::string temporarySessionUserId;
                // The one time token for an authenticated GUI Client temporary
                // session. Only available for client sessions with user authentication
                // while inside the temporary session duration.
                std::string temporarySessionToken;
                // Timestamp for the start of the GUI Client session - only
                // available for client sessions with user authentication while
                // inside the temporary session duration.
                karabo::util::Epochstamp temporarySessionStartTime;
                // Access level when the user began the temporary session. Sent by the
                // client as part of a begin temporary session request so the server can
                // send it back later at temporary session end time.
                karabo::util::Schema::AccessLevel levelBeforeTemporarySession{
                      karabo::util::Schema::AccessLevel::OBSERVER};


                ChannelData() : clientVersion("0.0.0"), temporarySessionStartTime(0ULL, 0ULL){};

                ChannelData(const karabo::util::Version& version, const std::string& userId = "",
                            const std::string& oneTimeToken = "")
                    : clientVersion(version),
                      userId(userId),
                      oneTimeToken(oneTimeToken),
                      sessionStartTime(karabo::util::Epochstamp()),
                      temporarySessionStartTime(0ULL, 0ULL){};
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
            using WeakChannelPointerCompare = std::owner_less<WeakChannelPointer>;
            // There is no way to have a reliable unordered_set of weak pointers...
            // Before C++14 we cannot use unordered_map since that does not (yet) guarantee that we can erase
            // entries while looping over it.
            typedef std::map<std::string, std::set<WeakChannelPointer, WeakChannelPointerCompare>> NetworkMap;

            enum QueueBehaviorsTypes {

                FAST_DATA = 2,
                REMOVE_OLDEST,
                LOSSLESS
            };

            karabo::net::Connection::Pointer m_dataConnection;

            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_serializer;
            std::map<karabo::net::Channel::Pointer, ChannelData> m_channels;
            std::queue<DeviceInstantiation> m_pendingDeviceInstantiations;

            mutable std::mutex m_channelMutex;
            mutable std::mutex m_networkMutex;
            mutable std::mutex m_pendingInstantiationsMutex;
            // TODO: remove this once "fast slot reply policy" is enforced
            mutable std::mutex m_timingOutDevicesMutex;

            boost::asio::steady_timer m_deviceInitTimer;
            boost::asio::steady_timer m_networkStatsTimer;
            boost::asio::steady_timer m_checkConnectionTimer;

            NetworkMap m_networkConnections;
            // Next map<string, ...> not unordered before use of C++14 because we erase from it while looping over it.
            std::map<std::string, std::map<WeakChannelPointer, bool, WeakChannelPointerCompare>>
                  m_readyNetworkConnections;

            karabo::net::Broker::Pointer m_guiDebugProducer;

            typedef std::map<karabo::net::Channel::Pointer, ChannelData>::const_iterator ConstChannelIterator;
            typedef std::map<karabo::net::Channel::Pointer, ChannelData>::iterator ChannelIterator;

            mutable std::mutex m_loggerMapMutex;
            karabo::util::Hash m_loggerMap;

            std::set<std::string> m_projectManagers;
            mutable std::shared_mutex m_projectManagerMutex;

            const bool m_isReadOnly;
            static const std::unordered_set<std::string> m_writeCommands;
            static const std::unordered_map<std::string, karabo::util::Version> m_minVersionRestrictions;
            /// In reported failure reasons, this delimiter comes between short message and details like a trace
            static const std::string m_errorDetailsDelim; //

            // TODO: remove this once "fast slot reply policy" is enforced
            // list of devices that do not respect fast slot reply policy
            std::unordered_set<std::string> m_timingOutDevices;

            std::atomic<int> m_timeout; // might overwrite timeout from client if client is smaller

            karabo::net::UserAuthClient m_authClient;
            // The temporary session manager has to be built after the GuiServer is fully constructed - it binds to a
            // method of the GuiServer.
            std::shared_ptr<GuiServerTemporarySessionManager> m_tempSessionManager;

            const bool m_onlyAppModeClients;

           public:
            KARABO_CLASSINFO(GuiServerDevice, "GuiServerDevice", "karabo-" + karabo::util::Version::getVersion())

            static void expectedParameters(karabo::util::Schema& expected);

            GuiServerDevice(const karabo::util::Hash& input);

            virtual ~GuiServerDevice();

            void initialize();

            virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration) override;

           private: // Functions
            /**
             * @brief Initializes the user actions log.
             *
             * The log contains entries describing the writing actions the GUI Server
             * performed upon request of a user. It is separated from the remaining
             * device server logs.
             */
            void initUsersActionsLog();

            /**
             * @brief Adds an entry with a given text to the user actions log.
             *
             * @param channel The TCP Channel connecting the GUI Server to the GUI Client that originated the action
             * execution request.
             * @param entryText A description of the action (and possibly its parameters)
             */
            void logUserAction(const WeakChannelPointer& channel, const std::string& entryText);

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
             * Deferred disconnect handler.
             */
            void deferredDisconnect(WeakChannelPointer channel);

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
             * @brief Checks whether a given reply type requested by a GUI Client is for a request involved in the Load
             * Project operation.
             *
             * @param replyType the string that specifies the reply type.
             * @return bool is the reply type involved in the Load Project operation?
             */
            bool isProjectLoadingReplyType(const std::string& replyType);

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

            /**
             * @brief Creates an internal ChannelData structure mapped to the TCP Channel in charge of
             * the connection between the GUI Client and the GUI Server. Also updates the number of
             * connected clients property of the GUI Server.
             *
             * Called after a successful user authorization based on a one-time token (when the
             * GUI Server requires user authentication).
             *
             * For GUI Servers that don't require user authentication / authorization, it's
             * called right after the message of "type" "login" is received by the server and
             * the client's version is verified as one being supported by the server.  The
             * client's version verification is also performed by a GUI Server that requires
             * authentication (right before the token validation).
             *
             * @note The access level is not being saved in the internal ChannelData structure
             * because all the access level enforcement is currently client side only. If any
             * enforcement is required on the server side, the access level information must
             * be stored in ChannelData and updated whenever the user downgrades his/her access
             * level on the GUI client.
             *
             * @param version the version of the connected GUI client
             * @param channel the TCP channel for the connection being registered
             * @param userId the ID of the user logged in the connected GUI Client
             * @param oneTimeToken the one-time token resulting from the user authentication
             * previously triggered by GUI client - the token is used by the GUI Server to
             * validate the authentication and to authorize the user (send the user's login access
             * level back to the GUI Client).
             */
            void registerConnect(const karabo::util::Version& version, const karabo::net::Channel::Pointer& channel,
                                 const std::string& userId = "", const std::string& oneTimeToken = "");

            /**
             * @brief Handler for login messages expected to be sent by a
             * GUI Client right after it establishes a TCP connection to the
             * GUI Server.
             *
             * Keeps discarding and logging warnings for any message whose
             * "type" is not "login". When such an unexpected message is received,
             * the GUI server binds this handler again to the TCP channel.
             *
             * When a message of "type" of "login" is received, its handling is
             * delegated to onLogin(const karabo::net::ErrorCode&, const karabo::net::Channel::Pointer&,
             * karabo::util::Hash&)
             *
             * @param e holds an error code if the eventloop cancels this task or the channel is closed
             * @param channel the TCP channel for the recently established connection with a GUI client
             * @param info a Hash containing the message "type" and, for "login" messages, some info related
             * to the login process, like the version of the connected GUI client and some user related info,
             * like the userID or the oneTimeToken the GUI Client received upon successfully authenticating
             * the user logging in.
             */
            void onWaitForLogin(const karabo::net::ErrorCode& e, const karabo::net::Channel::Pointer& channel,
                                karabo::util::Hash& info);

            bool isUserAuthActive() const;

            /**
             * @brief Handles a login request of a user on a GUI client. If the login credentials
             * are valid, the current system topology is sent to the client.
             *
             * @param channel the TCP channel between the GUI client and the GUI server
             * @param info Hash with information needed to validate the login request.
             *
             * @note for clients >= 2.20 a login message with no OneTimeToken is interpreted by
             * an authenticated GUI Server as a request for a read-only session. The GUI Server
             * will respond to such messages with Access Level OBSERVER and the read-only flag set
             * to true. For login messages with OneTimeToken the read-only flag will be always set
             * to false and the Access Level will be the one returned by the Karabo Authentication
             * Server.
             */
            void onLogin(const karabo::net::Channel::Pointer& channel, const karabo::util::Hash& info);

            /**
             * @brief Handles the result of the authorize one-time token operation performed as part of a GUI client
             * login on behalf of an authenticated user.
             *
             * @param channel the communication channel established with the GUI client logging in.
             * @param userId the ID of the user on whose behalf the login is being made.
             * @param cliVersion the version of the GUI client logging in.
             * @param oneTimeToken the one-time token sent by the GUI client logging in.
             * @param authResult the result of the one-time token authorization operation to be handled.
             */
            void onTokenAuthorizeResult(const WeakChannelPointer& channel, const std::string& userId,
                                        const karabo::util::Version& cliVersion, const std::string& oneTimeToken,
                                        const karabo::net::OneTimeTokenAuthorizeResult& authResult);

            /**
             * @brief Handles a temporary session expired event communicated by the internal instance of the
             * GuiServerTemporarySessionManager.
             *
             * The expiration is handled by sending a message of type "onTemporarySessionExpired" to the client
             * associated with the expired token. The message carries a Hash with paths "expiredToken" and
             * "expirationTime".
             *
             * @param info data about the expired temporary session.
             */
            void onTemporarySessionExpiration(const ExpiredTemporarySessionInfo& info);


            /**
             * @brief Handles a "temporary session about to expire" event.
             *
             * The eminent temporary session end is handled by sending a message of type "onEndTemporarySessionNotice"
             * to the client associated with the about to expire token. The message carries a Hash with paths
             * "toExpireToken" and "secondsToExpiration".
             *
             * @param info data about the temporary session about to expire.
             */
            void onEndTemporarySessionNotice(const EminentExpirationInfo& info);

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
             *      requestGeneric                 onRequestGeneric
             *      subscribeLogs                  <no action anymore>
             *      setLogPriority                 onSetLogPriority
             *      beginTemporarySession          onBeginTemporarySession
             *      endTemporarySession            onEndTemporarySession
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
             * @param readOnly
             */
            void onRead(const karabo::net::ErrorCode& e, WeakChannelPointer channel, karabo::util::Hash& info,
                        const bool readOnly);


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
             * @param info is a Hash that should contain the slot information.
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
             *  .. note: If the info Hash from the client provides an `empty` property, an empty
             *           Hash is sent back to the client instead of the input Hash.
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
             * @brief Handles a message of type "beginTemporarySession" by starting a temporary session on top
             * of the current user-authenticated session (if there's one). The session begining is an asynchronous
             * operation whose completion (either successful or not) will be handled by the
             * onBeginTemporarySessionResult method.
             *
             * @param channel the TCP channel connecting to the client that requested the begining of the temporary
             * session.
             * @param info a Hash which is supposed to contain an "temporarySessionToken" whose value is a one-time
             * token that must be successfuly authorized for the temporary session to be started.
             */
            void onBeginTemporarySession(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * @brief Handles the result of an "beginTemporarySession" request sent by a connected client.
             *
             * @param channel the TCP channel connecting to the client that requested the temporary session that will be
             * used to send a message of type "onBeginTemporarySession" with the begin operation results back to the
             * client.
             * @param levelBeforeTemporarySession sent by the client as part of the begin temporary session request to
             * be sent back when the temporary session ends.
             * @param result the results of the begin temporary session operation that will be sent back to the client.
             */
            void onBeginTemporarySessionResult(WeakChannelPointer channel,
                                               karabo::util::Schema::AccessLevel levelBeforeTemporarySession,
                                               const BeginTemporarySessionResult& result);

            /**
             * @brief Handles a message of type "endTemporarySession" by ending the current temporary session (if
             * there's one). The end of the session is performed synchronously (there's no I/O involved) and its
             * results are transmitted back to the client through a message of type "onEndTemporarySession".
             *
             * @param channel the TCP channel connecting to the client that requested the end of the temporary session.
             * Will be used to send the response back to the client.
             * @param info a Hash which is supposed to contain an "temporarySessionToken" whose value is a one-time
             * token that must match the one associated to the temporary session being terminated.
             *
             * @note the hash with the results of the ending operation sent back to the requesting client has the fields
             * "success", "reason" and "temporarySessionToken" (an echo of the token provided in the request).
             */
            void onEndTemporarySession(WeakChannelPointer channel, const karabo::util::Hash& info);

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
            void onKillServer(WeakChannelPointer channel, const karabo::util::Hash& info);

            /**
             * instructs the device specified by ``deviceId`` in ``info`` to shutdown.
             * @param info
             */
            void onKillDevice(WeakChannelPointer channel, const karabo::util::Hash& info);

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
             * Kept to reply back that log subscription not supported anymore after 2.16.X
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

            void slotLoggerMap(const karabo::util::Hash& loggerMap);


            /**
             * @brief Retrieve information about the current client sessions of
             * the GUI server.
             *
             * @param options Hash with a single key "onlyWithTempSession" and a
             * boolean value that when "true" makes the slot include only
             * client sessions with active temporary sessions in the reply.
             *
             * The reply is a hash with a single key, "clientSessions", whose
             * value is a vector of hashes with one hash per existing client
             * connection. The hash for each connection has the following keys:
             *
             *     . "clientVersion": string with the version of the connected
             *       client;
             *
             *     . "sessionStartTime": UTC timestamp of session start time as
             *       an ISO8601 string;
             *
             *     . "sessionToken": one-time token for the client session.
             *       Will be empty if the GUI Server is not in authenticated
             *       mode;
             *
             *     . "temporarySessionStartTime": UTC timestamps of temporary
             *       session start time as an ISO8601 string. If there's no
             *       active temporary session on top of the client session, an
             *       empty string is returned;
             *
             *     . "temporarySessionToken": one-time token for the temporary
             *       session (an empty string if there's no active temporary
             *       session).
             */
            void slotGetClientSessions(const karabo::util::Hash& options);

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
             * Slot to provide scene
             *
             * @param info Hash with key "name" that provides string identifying which scene
             */
            void requestScene(const karabo::util::Hash& info);

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
             * Check if a given project manager identified by id is known in the distributed system
             * @param channel to forward a failure message to if not
             * @param deviceId of the project manager device
             * @param type of the request
             * @return true if the project manager id exists in the distributed system
             */
            bool checkProjectManagerId(WeakChannelPointer channel, const std::string& deviceId, const std::string& type,
                                       const std::string& reason);

            /**
             * Utility for getting a "name" from client connections.
             */
            std::string getChannelAddress(const karabo::net::Channel::Pointer& channel) const;

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
