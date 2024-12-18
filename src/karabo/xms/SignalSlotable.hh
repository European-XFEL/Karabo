/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
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

#ifndef KARABO_CORE_SIGNALSLOTABLE_HH
#define KARABO_CORE_SIGNALSLOTABLE_HH

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <map>
#include <queue>
#include <tuple>
#include <unordered_map>

#include "InputChannel.hh"
#include "OutputChannel.hh"
#include "Signal.hh"
#include "Slot.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/Strand.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/PackParameters.hh"


/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace xms {

        /**
         * The SignalSlotable class.
         * This class implements the so-called "Signal-Slot" design pattern orginally termed
         * by the Qt-Gui framework. However, signals and slots are not restricted to a local application
         * but can be connected and triggered across the network. This allows for programming with network components
         * in the same intuitive (event-driven) way as Qt allows to do with its local components (e.g. widgets).
         *
         * Moreover does this implementation (unlike Qt) not require any pre-processing.
         * Another additional feature is the ability to setup new signals and/or slots at runtime.
         *
         * Furthermore, this class implements functions for the common request/response patterns.
         *
         * For a full documentation of the signal-slot component see the documentation in the software-guide.
         *
         */
        class SignalSlotable : public std::enable_shared_from_this<SignalSlotable> {
            friend class Signal;

           protected:
            // Forward
            class Requestor;

           public:
            KARABO_CLASSINFO(SignalSlotable, "SignalSlotable", "1.0")

            // Forward
            class AsyncReply;

            typedef std::function<void(const std::string& /*slotFunction*/, const std::string& /*callee*/)>
                  SlotCallGuardHandler;

            typedef std::function<void(const karabo::util::Hash::Pointer& /*performanceMeasures*/)>
                  UpdatePerformanceStatisticsHandler;

            typedef std::function<void(const std::string& /*message*/)> BrokerErrorHandler;

            typedef InputChannel::DataHandler DataHandler;

            typedef InputChannel::InputHandler InputHandler;

            typedef std::function<void(const OutputChannel::Pointer&)> OutputHandler;

            typedef std::shared_ptr<karabo::xms::Slot> SlotInstancePointer;

            typedef std::shared_ptr<karabo::xms::Signal> SignalInstancePointer;

            typedef std::map<std::string, InputChannel::Pointer> InputChannels;

            typedef std::map<std::string, OutputChannel::Pointer> OutputChannels;

            /// A structure to keep information of a signal-slot connection
            struct SignalSlotConnection {
                SignalSlotConnection(const std::string& signalInstanceId, const std::string& signal,
                                     const std::string& slotInstanceId, const std::string& slot)
                    : signalInstanceId(signalInstanceId), signal(signal), slotInstanceId(slotInstanceId), slot(slot) {}

                // needed to put into std::set:
                bool operator<(const SignalSlotConnection& other) const;

                std::string signalInstanceId;
                std::string signal;
                std::string slotInstanceId;
                std::string slot;
            };

            /**
             * This constructor does nothing. Call init() afterwards for setting up.
             */
            SignalSlotable();

            /**
             * Creates a functional SignalSlotable object using an existing connection.
             *
             * Don't call init() afterwards.
             * @param instanceId The future instanceId of this object in the distributed system
             * @param connection An existing broker connection
             * @param heartbeatInterval The interval (in s) in which a heartbeat is emitted
             * @param instanceInfo A hash containing any important additional information
             */
            SignalSlotable(const std::string& instanceId, const karabo::net::Broker::Pointer& connection,
                           const int heartbeatInterval = 30,
                           const karabo::util::Hash& instanceInfo = karabo::util::Hash());

            /**
             * Creates a function SignalSlotable object allowing to configure the broker connection.
             *
             * Don't call init() afterwards.
             * @param instanceId The future instanceId of this object in the distributed system
             * @param brokerConfiguration A single keyed Hash where the key is the broker type
             *                            and the Hash at that key is the configuration for the respective broker type
             *                            (a given instanceId it will be replaced by the first constructor argument).
             *                            Can be empty or can contain an empty Hash() at the single key, i.e. will be
             *                            expanded from defaults.
             * @param heartbeatInterval The interval (in s) in which a heartbeat is emitted
             * @param instanceInfo A hash containing any important additional information
             */
            explicit SignalSlotable(const std::string& instanceId,
                                    const karabo::util::Hash& brokerConfiguration = karabo::util::Hash(),
                                    const int heartbeatInterval = 30,
                                    const karabo::util::Hash& instanceInfo = karabo::util::Hash());

            virtual ~SignalSlotable();

            /**
             * Initializes the SignalSlotable object (only use in conjunction with empty constructor).
             *
             * @param instanceId The future instanceId of this object in the distributed system
             * @param connection An existing broker connection
             * @param heartbeatInterval The interval (in s) in which a heartbeat is emitted
             * @param instanceInfo A hash containing any important additional information
             * @param consumeBroadcasts if true (default), receive messages addressed to everybody (i.e. to '*')
             *                          on its own. If false, some other mechanism has to ensure to deliver these.
             */
            void init(const std::string& instanceId, const karabo::net::Broker::Pointer& connection,
                      const int heartbeatInterval, const karabo::util::Hash& instanceInfo,
                      bool consumeBroadcasts = true);

            /**
             * This function starts the communication.
             *
             * After a call to this non-blocking function the object starts
             * listening to messages. The uniqueness of the instanceId is validated
             * (throws SignalSlotException if not unique) and if successful the object registers with
             * a call to "slotInstanceNew" to the distributed system.
             */
            void start();

            void trackAllInstances();

            /**
             * Erase instance from container of tracked instances
             *
             * To be called if one is tracking instances and is sure that the given instance is not alive anymore
             * (e.g. if another instance in the same process is dead as well).
             * If erroneously called, the next arriving heartbeat of the instance will trigger an instanceNew event
             *
             * @param instanceId that shall be treated as not alive anymore
             * @return whether instanceId was tracked before
             */
            bool eraseTrackedInstance(const std::string& instanceId);

            karabo::util::Hash getAvailableInstances(bool /*unused*/ = false);

            /**
             * This is a synchronous call with timeout in milliseconds return vector of device signals.
             * @param instanceId of the device
             * @param timeout in milliseconds
             * @return vector of device's signal names
             */
            std::vector<std::string> getAvailableSignals(const std::string& instanceId, int timeout = 100);

            /**
             * This is a synchronous call with timeout in milliseconds return vector of device slots.
             * @param instanceId of the device
             * @param timeout in milliseconds
             * @return vector of device's slot names
             */
            std::vector<std::string> getAvailableSlots(const std::string& instanceId, int timeout = 100);

            /**
             * Retrieves currently logged in username (empty if not logged in)
             * @return string username
             */
            // TODO Check whether it can be removed
            const std::string& getUserName() const;

            /**
             * Access to the identification of the current instance using signals and slots
             * @return instanceId
             */
            const std::string& getInstanceId() const;

            /**
             * Update and publish the instanceInfo
             * @param update: a Hash containing new or updated keys - or keys to remove
             * @param remove: if false (default), merge 'update' to existing instance info, otherwise subtract it
             */
            void updateInstanceInfo(const karabo::util::Hash& update, bool remove = false);

            karabo::util::Hash getInstanceInfo() const;

            void registerSlotCallGuardHandler(const SlotCallGuardHandler& slotCallGuardHandler);

            void registerPerformanceStatisticsHandler(
                  const UpdatePerformanceStatisticsHandler& updatePerformanceStatisticsHandler);

            void registerBrokerErrorHandler(const BrokerErrorHandler& errorHandler);

            karabo::net::Broker::Pointer getConnection() const;

            // TODO This will BREAK during multi-topic refactoring
            const std::string& getTopic() const {
                return m_topic;
            }

            /**
             * This function must only be called within a slotFunctions body. It returns the current object handling
             * the callback which provides more information on the sender.
             * @param slotFunction The string-ified name of the slotFunction you are currently in
             * @return instance of a Slot object (handler object for this callback)
             */
            const SlotInstancePointer& getSenderInfo(const std::string& slotFunction);

            /**
             * This function tries to establish synchronously a connection between a signal
             * and a slot, identified both by their respective instance IDs and
             * signatures.
             * Moreover, this SignalSlotable obeys (throughout its full lifetime
             * or until "disconnect" is called with the same arguments) the
             * responsibility  to keep this connection alive, i.e. to reconnect
             * if either signal or slot instance come back after they have
             * shutdown or if they come up the first time.
             *
             * @param signalInstanceId is the instance ID of the signal (if empty use this instance)
             * @param signalSignature is the signature of the signal
             * @param slotInstanceId is the instance ID of the slot (if empty use this instance)
             * @param slotSignature is the signature of the slot
             * @return whether connection is already successfully established
             */
            bool connect(const std::string& signalInstanceId, const std::string& signalSignature,
                         const std::string& slotInstanceId, const std::string& slotSignature);

            /**
             * This function tries to establish a connection between a signal
             * and a slot as "connect" with four arguments does, so see there
             * for more details.
             * If signal or slot instance IDs are not specified, they are
             * interpreted as local and automatically assigned a "self"
             * instanceId
             *
             * @param signal <signalInstanceId>:<signalSignature>
             * @param slot <slotInstanceId>:<slotSignature>
             * @return whether connection is already succesfully established
             */
            bool connect(const std::string& signal, const std::string& slot);

            /// An AsyncErrorHandler takes no argument, but it will be called such that it can rethrow and then catch
            /// exceptions. The caught exception indicates the failure reason, e.g.:
            ///
            /// void asyncErrorHandler () {
            ///     try {
            ///         throw;
            ///     } catch (std::exception& e) { // or any other exception type - or several catch statements
            ///         KARABO_LOG_FRAMEWORK_WARN << "Probem when trying to do something: " << e.what();
            ///     }
            ///  }
            typedef std::function<void()> AsyncErrorHandler;

            /**
             * This function tries to establish asynchronously a connection between a signal
             * and a slot, identified both by their respective instance IDs and
             * signatures.
             * Moreover, this SignalSlotable obeys (throughout its full lifetime
             * or until "disconnect" is called with the same arguments) the
             * responsibility  to keep this connection alive, i.e. to reconnect
             * if either signal or slot instance come back after they have
             * shutdown or if they come up the first time.
             *
             * @param signalInstanceId is the instance ID of the signal (if empty use this instance)
             * @param signalSignature is the signature of the signal
             * @param slotInstanceId is the instance ID of the slot (if empty use this instance)
             * @param slotSignature is the signature of the slot
             * @param successHandler is called when connection is established (maybe be empty [=default])
             * @param failureHandler is called when connection could not be established, in the same way as an
             *                            Requestor::AsyncErrorHandler - if Signal or Slot do not exist, the exception
             *                            is a SignalSlotException
             * @param timeout in milliseconds for internal async requests - non-positive (default) means the very long
             *                            default timeout
             */
            void asyncConnect(const std::string& signalInstanceId, const std::string& signalSignature,
                              const std::string& slotInstanceId, const std::string& slotSignature,
                              const std::function<void()>& successHandler = std::function<void()>(),
                              const AsyncErrorHandler& failureHandler = AsyncErrorHandler(), int timeout = 0);

            /**
             * This function tries to establish asynchronously a connection between several signals and slots.
             *
             * One of the two handlers will be called exactly once.
             * The failureHandler will be called if any signal slot connection failed, no matter whether other
             * connections succeeded or not.
             *
             * @param signalSlotConnections e.g. vector<SignalSlotConnection>{SignalSlotConnection("sigInst", "signal",
             * "slotInst", "slot"), ...}
             * @param successHandler is called when all connections are established (maybe be empty [=default])
             * @param failureHandler is called when any of the connections could not be established, no matter whether
             *                            the others failed or not, in the same way as a Requestor::AsyncErrorHandler.
             * @param timeout in milliseconds for internal async requests - non-positive (default) means the very long
             *                            default timeout
             */
            void asyncConnect(const std::vector<SignalSlotConnection>& signalSlotConnections,
                              const std::function<void()>& successHandler = std::function<void()>(),
                              const AsyncErrorHandler& failureHandler = AsyncErrorHandler(), int timeout = 0);
            /**
             * Disconnects a slot from a signal, identified both by their
             * respective instance IDs and signatures.
             * In case the connection was established by this instance, also
             * erase it from the list of connections that have to re-established
             * in case signal or slot instances come back after a shutdown.
             *
             * @param signalInstanceId is the instance ID of the signal (if empty use this instance)
             * @param signalSignature is the signature of the signal
             * @param slotInstanceId is the instance ID of the slot (if empty use this instance)
             * @param slotSignature is the signature of the slot
             * @return whether connection is successfully stopped, e.g.
             *         false if there was no such connection or if remote signal instance ID did not confirm in time
             */
            bool disconnect(const std::string& signalInstanceId, const std::string& signalFunction,
                            const std::string& slotInstanceId, const std::string& slotFunction);

            /**
             * This function tries to disconnect a previously established connection between
             * a signal and a slot. These two are identified both by their respective
             * instance IDs and signatures.
             * In case the connection was established by this instance, the function also
             * erases it from the list of connections that have to be re-established
             * in case signal or slot instances come back after a shutdown.
             *
             * @param signalInstanceId is the instance ID of the signal (if empty use this instance)
             * @param signalSignature is the signature of the signal
             * @param slotInstanceId is the instance ID of the slot (if empty use this instance)
             * @param slotSignature is the signature of the slot
             * @param successHandler is called when connection is successfully stopped (maybe be empty [=default])
             * @param failureHandler is called when the disconnection failed (maybe be empty [=default])
             * @param timeout in milliseconds for internal async requests - non-positive (default) means the very long
             *                default timeout
             */
            void asyncDisconnect(const std::string& signalInstanceId, const std::string& signalFunction,
                                 const std::string& slotInstanceId, const std::string& slotFunction,
                                 const std::function<void()>& successHandler = std::function<void()>(),
                                 const AsyncErrorHandler& failureHandler = AsyncErrorHandler(), int timeout = 0);
            /**
             * Emits a signal, i.e. publishes the given payload
             * Emitting a signal is a fire-and-forget activity. The function returns immediately.
             * @param signalFunction The name of the previously registered signal
             * @param args... A variadic number of arguments to be published
             */
            template <typename... Args>
            void emit(const std::string& signalFunction, const Args&... args) const;

            /**
             * Calls a (remote) function.
             * Calling a remote function is a fire-and-forget activity. The function returns immediately after sending
             * the message.
             * @param instanceId Instance to be called
             * @param functionName Function on instance to be called (must be a registered slot)
             * @param args Arguments with which to call the slot
             */
            template <typename... Args>
            void call(const std::string& instanceId, const std::string& functionName, const Args&... args) const;

            template <typename... Args>
            SignalSlotable::Requestor request(const std::string& instanceId, const std::string& functionName,
                                              const Args&... args);

            template <typename... Args>
            SignalSlotable::Requestor requestNoWait(const std::string& requestInstanceId,
                                                    const std::string& requestFunctionName,
                                                    const std::string& replyInstanceId,
                                                    const std::string& replyFunctionName, const Args&... args);
            /**
             * Place the reply of a slot call
             *
             * To be used inside a method registered as a slot. The reply is not directly sent, but it is registered
             * to be sent once all methods registered to the slot (usually only one) have finished execution.
             * So if called several times in a slot, the last call defines the actual reply.
             *
             * If this method is not called inside a slot, an "empty" reply will be send without arguments.
             * But note that Device::updateState(const State s, ...) implicitly calls reply(s.name()).
             *
             * See about AsyncReply to avoid blocking the thread in case reply values are known only later, e.g. after
             * some IO operations.
             *
             * @param args 0 to 4 objects of the types known to serialisation, e.g. float, vector<long long>, Hash,...
             */
            template <typename... Args>
            void reply(const Args&... args);

            template <typename... Args>
            void registerSignal(const std::string& funcName);

            template <typename... Args>
            void registerSystemSignal(const std::string& funcName);

            /**
             * Register a new slot function for a slot. A new slot is generated
             * if so necessary. It is checked that the signature of the new
             * slot is the same as an already registered one.
             */
            void registerSlot(const std::function<void()>& slot, const std::string& funcName);

            template <class A1>
            void registerSlot(const std::function<void(const A1&)>& slot, const std::string& funcName);

            template <class A1, class A2>
            void registerSlot(const std::function<void(const A1&, const A2&)>& slot, const std::string& funcName);

            template <class A1, class A2, class A3>
            void registerSlot(const std::function<void(const A1&, const A2&, const A3&)>& slot,
                              const std::string& funcName);

            template <class A1, class A2, class A3, class A4>
            void registerSlot(const std::function<void(const A1&, const A2&, const A3&, const A4&)>& slot,
                              const std::string& funcName);

            /**
             * Create and register an InputChannel together with handlers
             *
             * @param channelName name of the channel, e.g. its path in the schema
             * @param config is a Hash with a Hash at 'channelName' which will be passed to InputChannel::create
             * @param onDataAvailableHandler is a DataHandler called for each data item coming through the pipeline
             * @param onInputAvailableHandler is an InputHandler called when new data arrives - user has to loop over
             * all items
             * @param onEndOfStreamEventHandler is an InputHandler called when EOS is received
             * @param connectTracker will be called whenever the connection status of the created channel changes
             *
             * @return the created InputChannel - better do not store it anywhere, it can be received via
             * getInputChannel(channelName)
             */
            virtual InputChannel::Pointer createInputChannel(
                  const std::string& channelName, const karabo::util::Hash& config,
                  const DataHandler& onDataAvailableHandler = DataHandler(),
                  const InputHandler& onInputAvailableHandler = InputHandler(),
                  const InputHandler& onEndOfStreamEventHandler = InputHandler(),
                  const InputChannel::ConnectionTracker& connectTracker = InputChannel::ConnectionTracker());

            /**
             * Remove the InputChannel created via createInputChannel
             *
             * @param channelName identifies the channel (first argument that was given to createInputChannel)
             * @return true if such a channel existed and could be removed
             */
            virtual bool removeInputChannel(const std::string& channelName);

            /**
             * Create an OutputChannel under the given name
             *
             * If there is already one for than name, that one (and thus all other copies of its shared_ptr)
             * will be disabled to disconnect any connection.
             *
             * @param channelName the name for the channel
             * @param config must have a Hash at key channelName - that is passed (after removeal of the "schema" key)
             *                                                     to Configurator<OutputChannel>::create
             * @param onOutputPossibleHandler ?
             * @return pointer to created channel - do not store anywhere!
             *        If needed, retrieve again via getOutputChannel(channelName).
             */
            virtual OutputChannel::Pointer createOutputChannel(
                  const std::string& channelName, const karabo::util::Hash& config,
                  const OutputHandler& onOutputPossibleHandler = OutputHandler());

            /**
             * Remove the OutputChannel created via createOutputChannel
             *
             * Before removal, it (and thus all other copies of its shared_ptr) will be disabled to disconnect any
             * connection.
             *
             * @param channelName identifies the channel (first argument that was given to createOutputChannel)
             * @return true if such a channel existed and could be removed
             */
            virtual bool removeOutputChannel(const std::string& channelName);

            InputChannels getInputChannels() const;

            OutputChannels getOutputChannels() const;

            /**
             * Access pointer to OutputChannel with given name. Throws ParameterException if no such output channel.
             * @param name of output channel (e.g. path in expectedParameters)
             * @return OutpuChannel::Pointer
             */
            OutputChannel::Pointer getOutputChannel(const std::string& name);

            /**
             * Access pointer to OutputChannel with given name.
             * @param name of output channel (e.g. path in expectedParameters)
             * @return OutputChannel::Pointer - empty if no channel of that name
             */
            OutputChannel::Pointer getOutputChannelNoThrow(const std::string& name);

            std::vector<std::string> getOutputChannelNames() const;

            /**
             * Access pointer to InputChannel with given name. Throws ParameterException if no such input channel.
             * @return InputChannel::Pointer
             */
            InputChannel::Pointer getInputChannel(const std::string& name);

            /**
             * Access pointer to InputChannel with given name.
             * @param name of input channel (e.g. path in expectedParameters)
             * @return InputChannel::Pointer - empty if no channel of that name
             */
            InputChannel::Pointer getInputChannelNoThrow(const std::string& name);

            void registerInputHandler(const std::string& channelName, const InputHandler& handler);

            void registerDataHandler(const std::string& channelName, const DataHandler& handler);

            void registerEndOfStreamHandler(const std::string& channelName, const InputHandler& handler);

            /**
             * Deprecated, use asyncConnectInputChannel!
             *
             * Connects an input channel to those as defined on the input channel's configuration.
             * The function is asynchronous, but gives no feedback about success or failure.
             */
            void connectInputChannel(const InputChannel::Pointer& channel, int trails = 8);

            /**
             * Connect input channel to output channels defined in its configuration.
             *
             * Proper asynchronous implementation with feedback handler
             *
             * @param channel pointer to InputChannel
             * @param handler to report success or failure. In the latter case the argument is false and more
             * information about the failure can be retrieved via try { throw; } catch (const std::exception&e) { const
             * std::string reason(e.what());} in the same way as in SignalSlotable::AsyncErrorHandler
             * @param outputChannelsToIgnore outputChannels that shall not be connected, e.g. because they are already
             *                               connected (defaults to empty vector)
             */
            void asyncConnectInputChannel(
                  const InputChannel::Pointer& channel, const std::function<void(bool)>& handler,
                  const std::vector<std::string>& outputChannelsToIgnore = std::vector<std::string>());

            /**
             *  Trigger connection of all not connected input channel connections
             *
             * Re-triggers itself regularly via internal  m_channelConnectTimer
             *
             * @param e do nothing if evaluates to true
             */
            void connectInputChannels(const boost::system::error_code& e);

            void reconnectInputChannels(const std::string& instanceId);

            std::pair<bool, std::string> exists(const std::string& instanceId);

            template <class T>
            static std::string generateInstanceId();

            /**
             * A functor to place an asynchronous reply during slot execution.
             *
             * - Create only inside a slot call of a SignalSlotable
             * - Can be copied around as wished
             * - Call either the operator or the error(..) method exactly once for one of the copies.
             * - Call these methods from another context/thread, i.e. between creation and use of its methods you have
             *   to leave the thread at least once)
             * - Must not be used once the SignalSlotable object that created it is (being) destructed (e.g. protect
             *   by bind_weak to member functions of the SignalSlotable)
             */
            class AsyncReply {
               public:
                /**
                 * Construct functor for an asynchronous reply.
                 * Create only within a slot call of a SignalSlotable
                 * @param signalSlotable pointer to the SignalSlotable whose slot is currently executed (usually: this)
                 */
                explicit AsyncReply(SignalSlotable* signalSlotable)
                    : m_signalSlotable(signalSlotable), m_slotInfo(m_signalSlotable->registerAsyncReply()) {}
                // ~AsyncReply(); // not needed, nor has to be virtual

                /**
                 * Place the reply - almost like using SignalSlotable::reply in the synchronous case.
                 * The difference is that here the reply is immediately sent and cannot be overwritten
                 * by a following call.
                 *
                 * @param args 0-4 objects of the types known to serialisation, e.g. float, vector<long long>, Hash,...
                 */
                template <typename... Args>
                void operator()(const Args&... args) const;

                /**
                 * If a proper reply cannot be placed, please use this to reply an error
                 * @param message is the short text for the RemoteException
                 * @param details further details, usually an exception trace (e.g. util::Exception::detailedMsg()),
                 *                default is empty for backward compatibility reasons
                 */
                void error(const std::string& message, const std::string& details = std::string()) const;

               private:
                SignalSlotable* const m_signalSlotable; // pointer is const - but may call non-const methods
                const std::tuple<karabo::util::Hash::Pointer, std::string, bool> m_slotInfo;
            };

            static std::string generateUUID();

           protected:
            class Requestor {
               public:
                static const int m_defaultAsyncTimeout = 2 * KARABO_SYS_TTL;

                typedef SignalSlotable::AsyncErrorHandler AsyncErrorHandler;

                explicit Requestor(SignalSlotable* signalSlotable);

                virtual ~Requestor();

                template <typename... Args>
                Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction,
                                   const Args&... args);

                template <typename... Args>
                Requestor& requestNoWait(const std::string& requestSlotInstanceId,
                                         const std::string& requestSlotFunction, const std::string replySlotInstanceId,
                                         const std::string& replySlotFunction, const Args&... args);

                void receiveAsync(const std::function<void()>& replyCallback,
                                  const AsyncErrorHandler& errorHandlerHandler = AsyncErrorHandler());

                template <class A1>
                void receiveAsync(const std::function<void(const A1&)>& replyCallback,
                                  const AsyncErrorHandler& errorHandlerHandler = AsyncErrorHandler());

                template <class A1, class A2>
                void receiveAsync(const std::function<void(const A1&, const A2&)>& replyCallback,
                                  const AsyncErrorHandler& errorHandlerHandler = AsyncErrorHandler());

                template <class A1, class A2, class A3>
                void receiveAsync(const std::function<void(const A1&, const A2&, const A3&)>& replyCallback,
                                  const AsyncErrorHandler& errorHandlerHandler = AsyncErrorHandler());

                template <class A1, class A2, class A3, class A4>
                void receiveAsync(const std::function<void(const A1&, const A2&, const A3&, const A4&)>& replyCallback,
                                  const AsyncErrorHandler& errorHandlerHandler = AsyncErrorHandler());

                template <typename... Args>
                void receive(Args&... args);

                std::vector<std::any> receiveAsVecOfAny();

                Requestor& timeout(const int& milliseconds);

               protected:
                void receiveResponse(karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body);

                karabo::util::Hash::Pointer prepareRequestHeader(const std::string& slotInstanceId,
                                                                 const std::string& slotFunction);

                karabo::util::Hash::Pointer prepareRequestNoWaitHeader(const std::string& requestSlotInstanceId,
                                                                       const std::string& requestSlotFunction,
                                                                       const std::string& replySlotInstanceId,
                                                                       const std::string& replySlotFunction);

                void registerRequest(const std::string& slotInstanceId, const karabo::util::Hash::Pointer& header,
                                     const karabo::util::Hash::Pointer& body);

                void sendRequest() const;

               private:
                /**
                 * @brief Receives the reply for the current request, returning shared pointers to the reply's header
                 * and body hashes.
                 *
                 * @returns A pair with a shared pointer to the header hash of the reply in the first position and a
                 * shared pointer to the body in the second.
                 */
                std::pair<karabo::util::Hash::Pointer, karabo::util::Hash::Pointer> receiveResponseHashes();

                /// Register handler for errors in async requests, e.g. timeout or remote exception
                void registerErrorHandler(const AsyncErrorHandler& errorHandler);

                /**
                 * @brief Extracts the value of the SignalInstanceId path in a response header hash.
                 *
                 * @param header response header hash from where the value will be extracted
                 * @param result the string that will be assigned the value of the SignalInstanceId path
                 */
                void getSignalInstanceId(const karabo::util::Hash::Pointer& header, std::string& result);

               protected:
                SignalSlotable* m_signalSlotable;

               private:
                std::string m_replyId;
                std::string m_slotInstanceId;
                karabo::util::Hash::Pointer m_header;
                karabo::util::Hash::Pointer m_body;
                int m_timeout;
            };

            // Use (mutex, condition variable) pair for waiting simultaneously many events
            struct BoostMutexCond {
                std::mutex m_mutex;
                std::condition_variable m_cond;

                BoostMutexCond() : m_mutex(), m_cond() {}

               private:
                BoostMutexCond(BoostMutexCond&);
            };

            std::string m_instanceId;
            mutable std::shared_mutex m_instanceInfoMutex;
            karabo::util::Hash m_instanceInfo;
            std::string m_username;

            typedef std::map<std::string, SignalInstancePointer> SignalInstances;
            SignalInstances m_signalInstances;

            typedef std::map<std::string, SlotInstancePointer> SlotInstances;
            SlotInstances m_slotInstances;

            typedef std::map<std::string, std::pair<std::shared_ptr<boost::asio::steady_timer>, AsyncErrorHandler>>
                  ReceiveAsyncErrorHandles;
            ReceiveAsyncErrorHandles m_receiveAsyncErrorHandles;

            // TODO Split into two mutexes
            mutable std::mutex m_signalSlotInstancesMutex;

            // key is instanceId of signal or slot
            typedef std::map<std::string, std::set<SignalSlotConnection>> SignalSlotConnections;
            SignalSlotConnections m_signalSlotConnections; // keep track of established connections
            std::mutex m_signalSlotConnectionsMutex;

            karabo::net::Broker::Pointer m_connection;

            int m_randPing;

           private:
            karabo::net::Strand::Pointer m_broadcastEventStrand;
            std::mutex m_unicastEventStrandsMutex;
            std::unordered_map<std::string, karabo::net::Strand::Pointer> m_unicastEventStrands; // one per sender

            // Reply/Request related

            // Map slot name and whether it was called globally
            std::map<boost::thread::id, std::pair<std::string, bool>>
                  m_currentSlots; // unordered? needs std::hash<boost::thread::id>...
            mutable std::mutex m_currentSlotsMutex;

            // which one succeeded, successHandler, errorHandler
            typedef std::tuple<std::vector<bool>, std::function<void()>, std::function<void()>> MultiAsyncConnectInfo;
            std::unordered_map<std::string, MultiAsyncConnectInfo> m_currentMultiAsyncConnects;
            std::mutex m_currentMultiAsyncConnectsMutex;

            static std::mutex m_uuidGeneratorMutex;
            static boost::uuids::random_generator m_uuidGenerator;

           protected:
            typedef std::map<boost::thread::id, karabo::util::Hash::Pointer> Replies;
            Replies m_replies;
            mutable std::mutex m_replyMutex;

            typedef std::pair<karabo::util::Hash::Pointer /*header*/, karabo::util::Hash::Pointer /*body*/> Event;
            typedef std::map<std::string, Event> ReceivedReplies;
            ReceivedReplies m_receivedReplies;
            mutable std::mutex m_receivedRepliesMutex;

            typedef std::map<std::string, std::shared_ptr<BoostMutexCond>> ReceivedRepliesBMC;
            ReceivedRepliesBMC m_receivedRepliesBMC;
            mutable std::mutex m_receivedRepliesBMCMutex;

            karabo::util::Hash m_trackedInstances;
            bool m_trackAllInstances;
            int m_heartbeatInterval;

            mutable std::mutex m_trackedInstancesMutex;

            boost::asio::steady_timer m_trackingTimer;
            boost::asio::steady_timer m_heartbeatTimer;
            boost::asio::steady_timer m_performanceTimer;

            // IO channel related
            boost::asio::steady_timer m_channelConnectTimer;
            mutable std::mutex m_pipelineChannelsMutex;
            InputChannels m_inputChannels;
            OutputChannels m_outputChannels;

            SlotCallGuardHandler m_slotCallGuardHandler;
            UpdatePerformanceStatisticsHandler m_updatePerformanceStatistics;

            std::mutex m_brokerErrorHandlerMutex;
            BrokerErrorHandler m_brokerErrorHandler;

            // Two maps of local (i.e. same process) instances:
            // A shared static one that everybody registers and deregisters, and a copy for each instance that can be
            // accessed whenever a message is sent.
            static std::unordered_map<std::string, SignalSlotable::WeakPointer> m_sharedInstanceMap;
            static std::shared_mutex m_sharedInstanceMapMutex;

            std::unordered_map<std::string, SignalSlotable::WeakPointer> m_myInstanceMap;
            mutable std::mutex m_myInstanceMapMutex;

            // TODO This is a helper variable
            std::string m_topic;

           protected: // Functions
            void delayedEmitHeartbeat(int delayInSeconds);

            void stopEmittingHearbeats();

            void consumerErrorNotifier(const std::string& consumer, karabo::net::consumer::Error ec,
                                       const std::string& message);

            void onHeartbeatMessage(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);

            void handleReply(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body,
                             long long whenPostedEpochMs);

            void processEvent(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);

            /**
             * Parses out the instanceId part of signalId or slotId
             * @param signalOrSlotId
             * @return A string representing the instanceId
             */
            std::string fetchInstanceId(const std::string& signalOrSlotId) const;

            std::pair<std::string, std::string> splitIntoInstanceIdAndFunctionName(const std::string& signalOrSlotId,
                                                                                   const char sep = ':') const;

            void registerReply(const karabo::util::Hash::Pointer& reply);

            // Thread-safe, locks m_signalSlotInstancesMutex
            SignalInstancePointer getSignal(const std::string& signalFunction) const;

            karabo::util::Hash::Pointer prepareCallHeader(const std::string& slotInstanceId,
                                                          const std::string& slotFunction) const;

            void doSendMessage(const std::string& instanceId, const karabo::util::Hash::Pointer& header,
                               const karabo::util::Hash::Pointer& body, int prio, int timeToLive,
                               const std::string& topic = "", bool forceViaBroker = false) const;

            // protected since needed in DeviceServer
            bool tryToCallDirectly(const std::string& slotInstanceId, const karabo::util::Hash::Pointer& header,
                                   const karabo::util::Hash::Pointer& body) const;

            /**
             * Register a handler to be called for every received message that is addressed to everybody.
             * NOTE:
             * This is not thread safe - call before SignalSlotable::start starts receiving messages.
             *
             * @param handler with header and body (as Hash::Pointer) of the message
             */
            void registerBroadcastHandler(std::function<void(const karabo::util::Hash::Pointer& header,
                                                             const karabo::util::Hash::Pointer& body)>
                                                handler);

           private: // Functions
            /**
             * Helper for register(System)Signal: If signalFunction is not yet known, creates a signal corresponding
             * to the template argument signature and adds it to the internal container.
             * Otherwise an empty pointer is returned.
             * @param signalFunction
             * @param priority is passed further to the Signal
             * @param messageTimeToLive is passed further to the Signal
             * @return pointer to new Signal or empty pointer
             */
            template <typename... Args>
            SignalInstancePointer addSignalIfNew(const std::string& signalFunction, int priority = KARABO_SYS_PRIO,
                                                 int messageTimeToLive = KARABO_SYS_TTL);

            /**
             * If instanceId has invalid characters, throws SignalSlotException.
             */
            void ensureInstanceIdIsValid(const std::string& instanceId);

            /**
             * If instanceId not unique in system, throws SignalSlotException.
             */
            void ensureInstanceIdIsUnique(const std::string& instanceId);

            void slotInstanceNew(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void slotInstanceUpdated(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void slotPing(const std::string& instanceId, int rand, bool trackPingedInstance);

            void slotPingAnswer(const std::string& instanceId, const karabo::util::Hash& hash);

            // Get the strand for the stated sender - may create it if not yet there.
            karabo::net::Strand::Pointer getUnicastEventStrand(const std::string& signalInstanceId);

            void processSingleSlot(const std::string& slotFunction, bool globalCall,
                                   const std::string& signalInstanceId, const karabo::util::Hash::Pointer& header,
                                   const karabo::util::Hash::Pointer& body, long long whenPostedEpochMs);

            void replyException(const karabo::util::Hash& header, const std::string& message,
                                const std::string& details);

            void sendPotentialReply(const karabo::util::Hash& header, const std::string& slotFunction, bool global);

            /**
             * Internal method to provide info for AsyncReply object
             * @return tuple of slot header, slot name and whether it is a global slot call
             */
            std::tuple<karabo::util::Hash::Pointer, std::string, bool> registerAsyncReply();

            void emitHeartbeat(const boost::system::error_code& e);

            void registerDefaultSignalsAndSlots();

            /// Calls connect for all signal-slot connections that involve
            /// 'newInstanceId' (be it on signal or slot side) and for which
            /// this instance is responsible, i.e. its "connect" has been called
            /// for this connection before (with or without immediate success).
            /// Calling "disconnect" stops this responsibility.
            void reconnectSignals(const std::string& newInstanceId);

            /// Register myself for short-cut messaging (i.e. bypass broker if in same process).
            /// Must not be called before instance ID is checked to be unique in overall system.
            void registerForShortcutMessaging();

            /// Deregister myself from short-cut messaging.
            void deregisterFromShortcutMessaging();

            void startTrackingSystem();

            void stopTrackingSystem();

            void startPerformanceMonitor();

            void stopPerformanceMonitor();

            void updatePerformanceStatistics(const boost::system::error_code& e);

            void updateLatencies(const karabo::util::Hash::Pointer& header, long long whenPostedEpochMs);

            bool tryToConnectToSignal(const std::string& signalInstanceId, const std::string& signalFunction,
                                      const std::string& slotInstanceId, const std::string& slotFunction);

            SlotInstancePointer findSlot(const std::string& funcName);

            /**
             * Register a new slot *instance* under name *funcName*.
             * This will raise an error if the slot already exists.
             */
            void registerNewSlot(const std::string& funcName, SlotInstancePointer instance);

            /// Register signal-slot connection on signal side
            void slotConnectToSignal(const std::string& signalFunction, const std::string& slotInstanceId,
                                     const std::string& slotFunction);

            /// Slot to subscribe to remote signal
            void slotSubscribeRemoteSignal(const std::string& signalInstanceId, const std::string& signalFunction);

            /// Slot to un-subscribe from remote signal
            void slotUnsubscribeRemoteSignal(const std::string& signalInstanceId, const std::string& signalFunction);

            /// True if instance with ID 'slotInstanceId' has slot 'slotFunction'.
            /// Internally uses "slotHasSlot" for remote instances, but shortcuts if ID is the own one.
            /// Always true if 'slotInstanceId == "*"' (i.e. global slot).
            bool instanceHasSlot(const std::string& slotInstanceId, const std::string& unmangledSlotFunction);

            /// Slot to tell whether instance has a slot of given name.
            void slotHasSlot(const std::string& unmangledSlotFunction);

            void storeConnection(const std::string& signalInstanceId, const std::string& signalFunction,
                                 const std::string& slotInstanceId, const std::string& slotFunction);
            bool removeStoredConnection(const std::string& signalInstanceId, const std::string& signalFunction,
                                        const std::string& slotInstanceId, const std::string& slotFunction);
            bool tryToDisconnectFromSignal(const std::string& signalInstanceId, const std::string& signalFunction,
                                           const std::string& slotInstanceId, const std::string& slotFunction);

            void slotDisconnectFromSignal(const std::string& signalFunction, const std::string& slotInstanceId,
                                          const std::string& slotFunction);

            static std::string prepareFunctionSignature(const std::string& funcName) {
                std::string f(boost::trim_copy(funcName));
                return f.substr(0, f.find_first_of('-')) + "|";
            }

            void slotHeartbeat(const std::string& networkId, const int& heartbeatInterval,
                               const karabo::util::Hash& heartbeatInfo);

            void letInstanceSlowlyDieWithoutHeartbeat(const boost::system::error_code& e);

            void decreaseCountdown(std::vector<std::pair<std::string, karabo::util::Hash>>& deadOnes);

            // Thread safe
            void addTrackedInstance(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            // Thread safe
            bool hasTrackedInstance(const std::string& instanceId);

            // In public section: void eraseTrackedInstance(const std::string& instanceId);

            void updateTrackedInstanceInfo(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void slotGetAvailableFunctions(const std::string& type);

            void cleanSignals(const std::string& instanceId);

            // IO channel related
            std::pair<bool, karabo::util::Hash> slotGetOutputChannelInformationImpl(const std::string& channelId,
                                                                                    const int& processId,
                                                                                    const char* slotName);
            // we need two wrappers, one for legacy
            void slotGetOutputChannelInformation(const std::string& channelId, const int& processId);
            // and one for generic GUI requests
            void slotGetOutputChannelInformationFromHash(const karabo::util::Hash& hash);

            void connectInputToOutputChannel(const InputChannel::Pointer& channel,
                                             const std::string& outputChannelString,
                                             const std::function<void(bool)>& handler);

            /// helper for connectInputChannels()
            void handleInputConnected(bool success, const std::string& channel, const std::shared_ptr<std::mutex>& mut,
                                      const std::shared_ptr<std::vector<karabo::net::AsyncStatus>>& status, size_t i,
                                      size_t numOutputsToIgnore);

            void connectSingleInputHandler(
                  std::shared_ptr<
                        std::tuple<std::mutex, std::vector<karabo::net::AsyncStatus>, std::function<void(bool)>>>
                        status,
                  unsigned int counter, const std::string& outputChannelString, bool singleSuccess);

            void connectInputChannelHandler(const InputChannel::Pointer& inChannel,
                                            const std::string& outputChannelString,
                                            const std::function<void(bool)>& handler, bool outChannelExists,
                                            const karabo::util::Hash& outChannelInfo);

            // Deprecated since only used in the deprecated connectInputChannel(..)
            void connectInputToOutputChannel_old(const InputChannel::Pointer& channel,
                                                 const std::string& outputChannelString, int trails = 8);

            // Deprecated since only used in the logic of the deprecated connectInputChannel(..)
            void connectInputChannelHandler_old(const InputChannel::Pointer& inChannel,
                                                const std::string& outputChannelString, bool outChannelExists,
                                                const karabo::util::Hash& outChannelInfo);

            // Deprecated since only used in the logic of the deprecated connectInputChannel(..)
            void connectInputChannelErrorHandler_old(const InputChannel::Pointer& inChannel,
                                                     const std::string& outputChannelString, int trials,
                                                     unsigned int nextTimeout);

            // Thread-safe, locks m_signalSlotInstancesMutex
            bool hasSlot(const std::string& unmangledSlotFunction) const;

            // Thread-safe, locks m_signalSlotInstancesMutex
            SlotInstancePointer getSlot(const std::string& unmangledSlotFunction) const;

            // Thread-safe, locks m_signalSlotInstancesMutex
            void removeSlot(const std::string& unmangledSlotFunction);

            // Thread-safe, locks m_signalSlotInstancesMutex
            bool hasSignal(const std::string& signalFunction) const;

            /** Try to undo registration of a slot "slotInstanceId.slotFunction".
             * Thread-safe, locks m_signalSlotInstancesMutex.
             * @param signalFunction name of local signal
             * @param slotInstanceId instance id that carries the slot
             * @param slotFunction the slot - if empty, all registered slots of slotInstanceId
             * @return bool true if signal existed and given slot was registered before
             */
            bool tryToUnregisterSlot(const std::string& signalFunction, const std::string& slotInstanceId,
                                     const std::string& slotFunction);

            bool hasReceivedReply(const std::string& replyFromValue) const;

            void popReceivedReply(const std::string& replyFromValue, karabo::util::Hash::Pointer& header,
                                  karabo::util::Hash::Pointer& body);

            void registerSynchronousReply(const std::string& replyId);

            bool timedWaitAndPopReceivedReply(const std::string& replyId, karabo::util::Hash::Pointer& header,
                                              karabo::util::Hash::Pointer& body, int timeout);
            long long getEpochMillis() const;

            void slotGetOutputChannelNames();

            // TODO This is a helper function during multi-topic refactoring
            void setTopic(const std::string& topic = "");

            void receiveAsyncTimeoutHandler(const boost::system::error_code& e, const std::string& replyId,
                                            const AsyncErrorHandler& errorHandler);

            /// For the given replyId of a 'request.receiveAsync', register error handling,
            /// i.e. the timer for timeout and the handler for remote exceptions.
            void addReceiveAsyncErrorHandles(const std::string& replyId,
                                             const std::shared_ptr<boost::asio::steady_timer>& timer,
                                             const AsyncErrorHandler& errorHandler);

            std::pair<std::shared_ptr<boost::asio::steady_timer>, AsyncErrorHandler> getReceiveAsyncErrorHandles(
                  const std::string& replyId) const;

            /// Helper that calls 'handler' such that it can do
            ///
            ///  try { throw; } catch (const SignalSlotException &e) { <action>}
            ///
            /// @param message text given to the SignalSlotException
            static void callErrorHandler(const AsyncErrorHandler& handler, const std::string& message);

            void multiAsyncConnectSuccessHandler(const std::string& uuid, size_t requestNum);

            void multiAsyncConnectFailureHandler(const std::string& uuid);

           private: // Members
            // Performance statistics

            struct LatencyStats {
                unsigned int sum;
                unsigned int counts;
                unsigned int maximum;

                LatencyStats();
                void add(unsigned int latency);
                void clear();
                float average() const;
            };

            mutable std::mutex m_latencyMutex;
            LatencyStats m_processingLatency; // measurements in milliseconds
            LatencyStats m_eventLoopLatency;  // measurements in milliseconds for

            std::function<void(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body)>
                  m_broadCastHandler;
        };

        /**** Requestor Implementation ****/

        template <typename... Args>
        SignalSlotable::Requestor& SignalSlotable::Requestor::request(const std::string& slotInstanceId,
                                                                      const std::string& slotFunction,
                                                                      const Args&... args) {
            karabo::util::Hash::Pointer header = prepareRequestHeader(slotInstanceId, slotFunction);
            auto body = std::make_shared<karabo::util::Hash>();
            pack(*body, args...);
            registerRequest(slotInstanceId, header, body);
            return *this;
        }

        template <typename... Args>
        SignalSlotable::Requestor& SignalSlotable::Requestor::requestNoWait(const std::string& requestSlotInstanceId,
                                                                            const std::string& requestSlotFunction,
                                                                            const std::string replySlotInstanceId,
                                                                            const std::string& replySlotFunction,
                                                                            const Args&... args) {
            karabo::util::Hash::Pointer header = prepareRequestNoWaitHeader(requestSlotInstanceId, requestSlotFunction,
                                                                            replySlotInstanceId, replySlotFunction);

            auto body = std::make_shared<karabo::util::Hash>();
            pack(*body, args...);
            m_signalSlotable->doSendMessage(requestSlotInstanceId, header, body, KARABO_SYS_PRIO, KARABO_SYS_TTL);
            return *this;
        }

        template <class A1>
        void SignalSlotable::Requestor::receiveAsync(const std::function<void(const A1&)>& replyCallback,
                                                     const AsyncErrorHandler& errorHandler) {
            m_signalSlotable->registerSlot<A1>(replyCallback, m_replyId);
            registerErrorHandler(errorHandler);
            sendRequest();
        }

        template <class A1, class A2>
        void SignalSlotable::Requestor::receiveAsync(const std::function<void(const A1&, const A2&)>& replyCallback,
                                                     const AsyncErrorHandler& errorHandler) {
            m_signalSlotable->registerSlot<A1, A2>(replyCallback, m_replyId);
            registerErrorHandler(errorHandler);
            sendRequest();
        }

        template <class A1, class A2, class A3>
        void SignalSlotable::Requestor::receiveAsync(
              const std::function<void(const A1&, const A2&, const A3&)>& replyCallback,
              const AsyncErrorHandler& errorHandler) {
            m_signalSlotable->registerSlot<A1, A2, A3>(replyCallback, m_replyId);
            registerErrorHandler(errorHandler);
            sendRequest();
        }

        template <class A1, class A2, class A3, class A4>
        void SignalSlotable::Requestor::receiveAsync(
              const std::function<void(const A1&, const A2&, const A3&, const A4&)>& replyCallback,
              const AsyncErrorHandler& errorHandler) {
            m_signalSlotable->registerSlot<A1, A2, A3, A4>(replyCallback, m_replyId);
            registerErrorHandler(errorHandler);
            sendRequest();
        }

        template <typename... Args>
        void SignalSlotable::Requestor::receive(Args&... args) {
            auto headerBodyPair = receiveResponseHashes();
            const karabo::util::Hash::Pointer& header = headerBodyPair.first;
            const karabo::util::Hash::Pointer& body = headerBodyPair.second;
            try {
                karabo::util::unpack(*body, args...); // ParameterException if args is too long

                if (sizeof...(Args) != body->size()) {
                    const int nArgs = body->size() - sizeof...(Args);
                    KARABO_LOG_FRAMEWORK_DEBUG << "Ignoring the last " << nArgs << " arguments of response:\n" << *body;
                }

            } catch (const karabo::util::CastException&) {
                std::string signalInstanceId("unknown");
                getSignalInstanceId(header, signalInstanceId);
                const std::string msg("'" + m_signalSlotable->getInstanceId() +
                                      "' received incompatible response "
                                      "from '" +
                                      signalInstanceId + "'");
                KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION(msg));
            } catch (const karabo::util::Exception& e) {
                std::string signalInstanceId("unknown");
                getSignalInstanceId(header, signalInstanceId);
                const std::string msg("Error while '" + m_signalSlotable->getInstanceId() +
                                      "' received following reply from '" + signalInstanceId +
                                      "': " + (body ? toString(*body) : std::string()));
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION(msg));
            }
        }

        /**** AsyncReply Template Function Implementation ****/

        template <typename... Args>
        void SignalSlotable::AsyncReply::operator()(const Args&... args) const {
            // See SignalSlotable::registerAsyncReply() about non-existing header pointer
            const util::Hash::Pointer& headerPtr = std::get<0>(m_slotInfo);
            if (!headerPtr) return;

            // Place reply and send it
            m_signalSlotable->reply(args...);
            m_signalSlotable->sendPotentialReply(*headerPtr, std::get<1>(m_slotInfo), std::get<2>(m_slotInfo));
        }

        /**** SignalSlotable Template Function Implementations ****/

        template <typename... Args>
        void SignalSlotable::emit(const std::string& signalFunction, const Args&... args) const {
            SignalInstancePointer s = getSignal(signalFunction);
            if (s) {
                auto hash = std::make_shared<karabo::util::Hash>();
                pack(*hash, args...);
                s->emit<Args...>(hash);
            }
        }

        template <typename... Args>
        void SignalSlotable::call(const std::string& instanceId, const std::string& functionName,
                                  const Args&... args) const {
            const std::string& id = (instanceId.empty() ? m_instanceId : instanceId);
            auto body = std::make_shared<karabo::util::Hash>();
            pack(*body, args...);
            karabo::util::Hash::Pointer header = prepareCallHeader(id, functionName);
            doSendMessage(id, header, body, KARABO_SYS_PRIO, KARABO_SYS_TTL);
        }

        template <typename... Args>
        SignalSlotable::Requestor SignalSlotable::request(const std::string& instanceId,
                                                          const std::string& functionName, const Args&... args) {
            const std::string& id = (instanceId.empty() ? m_instanceId : instanceId);
            return SignalSlotable::Requestor(this).request(id, functionName, args...);
        }

        template <typename... Args>
        SignalSlotable::Requestor SignalSlotable::requestNoWait(const std::string& requestInstanceId,
                                                                const std::string& requestFunctionName,
                                                                const std::string& replyInstanceId,
                                                                const std::string& replyFunctionName,
                                                                const Args&... args) {
            const std::string& reqId = (requestInstanceId.empty() ? m_instanceId : requestInstanceId);
            const std::string& repId = (replyInstanceId.empty() ? m_instanceId : replyInstanceId);
            return SignalSlotable::Requestor(this).requestNoWait(reqId, requestFunctionName, repId, replyFunctionName,
                                                                 args...);
        }

        template <typename... Args>
        void SignalSlotable::reply(const Args&... args) {
            karabo::util::Hash::Pointer hash = std::make_shared<karabo::util::Hash>();
            karabo::util::pack(*hash, args...);
            registerReply(hash);
        }

        template <typename... Args>
        void SignalSlotable::registerSignal(const std::string& funcName) {
            addSignalIfNew<Args...>(funcName, KARABO_PUB_PRIO, KARABO_PUB_TTL);
        }

        template <typename... Args>
        void SignalSlotable::registerSystemSignal(const std::string& funcName) {
            addSignalIfNew<Args...>(funcName);
        }

        template <class A1>
        void SignalSlotable::registerSlot(const std::function<void(const A1&)>& slot, const std::string& funcName) {
            // About the dynamic_pointer_cast: see non-template version of registerSlot.
            auto s = std::dynamic_pointer_cast<SlotN<void, A1>>(findSlot(funcName));
            if (!s) {
                s = std::make_shared<SlotN<void, A1>>(funcName);
                registerNewSlot(funcName, std::static_pointer_cast<Slot>(s));
            }
            s->registerSlotFunction(slot);
        }

        template <class A1, class A2>
        void SignalSlotable::registerSlot(const std::function<void(const A1&, const A2&)>& slot,
                                          const std::string& funcName) {
            // About the dynamic_pointer_cast: see non-template version of registerSlot.
            auto s = std::dynamic_pointer_cast<SlotN<void, A1, A2>>(findSlot(funcName));
            if (!s) {
                s = std::make_shared<SlotN<void, A1, A2>>(funcName);
                registerNewSlot(funcName, std::static_pointer_cast<Slot>(s));
            }
            s->registerSlotFunction(slot);
        }

        template <class A1, class A2, class A3>
        void SignalSlotable::registerSlot(const std::function<void(const A1&, const A2&, const A3&)>& slot,
                                          const std::string& funcName) {
            // About the dynamic_pointer_cast: see non-template version of registerSlot.
            auto s = std::dynamic_pointer_cast<SlotN<void, A1, A2, A3>>(findSlot(funcName));
            if (!s) {
                s = std::make_shared<SlotN<void, A1, A2, A3>>(funcName);
                registerNewSlot(funcName, std::static_pointer_cast<Slot>(s));
            }
            s->registerSlotFunction(slot);
        }

        template <class A1, class A2, class A3, class A4>
        void SignalSlotable::registerSlot(const std::function<void(const A1&, const A2&, const A3&, const A4&)>& slot,
                                          const std::string& funcName) {
            // About the dynamic_pointer_cast: see non-template version of registerSlot.
            auto s = std::dynamic_pointer_cast<SlotN<void, A1, A2, A3, A4>>(findSlot(funcName));
            if (!s) {
                s = std::make_shared<SlotN<void, A1, A2, A3, A4>>(funcName);
                registerNewSlot(funcName, std::static_pointer_cast<Slot>(s));
            }
            s->registerSlotFunction(slot);
        }

        template <typename... Args>
        SignalSlotable::SignalInstancePointer SignalSlotable::addSignalIfNew(const std::string& signalFunction,
                                                                             int priority, int messageTimeToLive) {
            SignalInstancePointer s;
            {
                std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
                if (m_signalInstances.find(signalFunction) == m_signalInstances.end()) {
                    s = std::make_shared<Signal>(this, m_connection, m_instanceId, signalFunction, priority,
                                                 messageTimeToLive);
                    s->setSignature<Args...>();
                    m_signalInstances[signalFunction] = s;
                }
            }
            return s;
        }

        template <class T>
        std::string SignalSlotable::generateInstanceId() {
            std::string hostname(boost::asio::ip::host_name());
            std::vector<std::string> tokens;
            boost::split(tokens, hostname, boost::is_any_of("."));
            return std::string(tokens[0] + "_" + T::classInfo().getClassId() + "_" + karabo::util::toString(getpid()));
        }
    } // namespace xms
} // namespace karabo

#define KARABO_SIGNAL0(signalName) this->registerSignal(signalName);
#define KARABO_SIGNAL1(signalName, a1) this->template registerSignal<a1>(signalName);
#define KARABO_SIGNAL2(signalName, a1, a2) this->template registerSignal<a1, a2>(signalName);
#define KARABO_SIGNAL3(signalName, a1, a2, a3) this->template registerSignal<a1, a2, a3>(signalName);
#define KARABO_SIGNAL4(signalName, a1, a2, a3, a4) this->template registerSignal<a1, a2, a3, a4>(signalName);

#define KARABO_SYSTEM_SIGNAL0(signalName) this->registerSystemSignal(signalName);
#define KARABO_SYSTEM_SIGNAL1(signalName, a1) this->template registerSystemSignal<a1>(signalName);
#define KARABO_SYSTEM_SIGNAL2(signalName, a1, a2) this->template registerSystemSignal<a1, a2>(signalName);
#define KARABO_SYSTEM_SIGNAL3(signalName, a1, a2, a3) this->template registerSystemSignal<a1, a2, a3>(signalName);
#define KARABO_SYSTEM_SIGNAL4(signalName, a1, a2, a3, a4) \
    this->template registerSystemSignal<a1, a2, a3, a4>(signalName);

#define KARABO_SLOT0(slotName) this->registerSlot(std::bind(&Self::slotName, this), #slotName);
#define KARABO_SLOT1(slotName, a1) \
    this->template registerSlot<a1>(std::bind(&Self::slotName, this, std::placeholders::_1), #slotName);
#define KARABO_SLOT2(slotName, a1, a2)   \
    this->template registerSlot<a1, a2>( \
          std::bind(&Self::slotName, this, std::placeholders::_1, std::placeholders::_2), #slotName);
#define KARABO_SLOT3(slotName, a1, a2, a3)                                                                       \
    this->template registerSlot<a1, a2, a3>(                                                                     \
          std::bind(&Self::slotName, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), \
          #slotName);
#define KARABO_SLOT4(slotName, a1, a2, a3, a4)                                                                  \
    this->template registerSlot<a1, a2, a3, a4>(                                                                \
          std::bind(&Self::slotName, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, \
                    std::placeholders::_4),                                                                     \
          #slotName);

#define KARABO_ON_INPUT(channelName, funcName) \
    this->registerInputHandler(channelName, karabo::util::bind_weak(&Self::funcName, this, std::placeholders::_1));
#define KARABO_ON_DATA(channelName, funcName) \
    this->registerDataHandler(                \
          channelName, karabo::util::bind_weak(&Self::funcName, this, std::placeholders::_1, std::placeholders::_2));
#define KARABO_ON_EOS(channelName, funcName)      \
    this->registerEndOfStreamHandler(channelName, \
                                     karabo::util::bind_weak(&Self::funcName, this, std::placeholders::_1));

#define _KARABO_SIGNAL_N(x0, x1, x2, x3, x4, x5, FUNC, ...) FUNC
#define _KARABO_SYSTEM_SIGNAL_N(x0, x1, x2, x3, x4, x5, FUNC, ...) FUNC
#define _KARABO_SLOT_N(x0, x1, x2, x3, x4, x5, FUNC, ...) FUNC

#define KARABO_SIGNAL(...)                                                                      \
    _KARABO_SIGNAL_N(, ##__VA_ARGS__, KARABO_SIGNAL4(__VA_ARGS__), KARABO_SIGNAL3(__VA_ARGS__), \
                     KARABO_SIGNAL2(__VA_ARGS__), KARABO_SIGNAL1(__VA_ARGS__), KARABO_SIGNAL0(__VA_ARGS__))

#define KARABO_SYSTEM_SIGNAL(...)                                                                                    \
    _KARABO_SYSTEM_SIGNAL_N(, ##__VA_ARGS__, KARABO_SYSTEM_SIGNAL4(__VA_ARGS__), KARABO_SYSTEM_SIGNAL3(__VA_ARGS__), \
                            KARABO_SYSTEM_SIGNAL2(__VA_ARGS__), KARABO_SYSTEM_SIGNAL1(__VA_ARGS__),                  \
                            KARABO_SYSTEM_SIGNAL0(__VA_ARGS__))

#define KARABO_SLOT(...)                                                                                             \
    _KARABO_SLOT_N(, ##__VA_ARGS__, KARABO_SLOT4(__VA_ARGS__), KARABO_SLOT3(__VA_ARGS__), KARABO_SLOT2(__VA_ARGS__), \
                   KARABO_SLOT1(__VA_ARGS__), KARABO_SLOT0(__VA_ARGS__))

#endif
