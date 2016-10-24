/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_CORE_SIGNALSLOTABLE_HH
#define	KARABO_CORE_SIGNALSLOTABLE_HH

#include "OutputChannel.hh"
#include "InputChannel.hh"
#include "Signal.hh"
#include "Slot.hh"

#include "karabo/util/Configurator.hh"
#include "karabo/util/PackParameters.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/JmsConnection.hh"
#include "karabo/net/JmsConsumer.hh"
#include "karabo/net/JmsProducer.hh"
#include "karabo/net/PointToPoint.hh"

#include <queue>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

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
        class SignalSlotable : public boost::enable_shared_from_this<SignalSlotable> {

            friend class Signal;

            // Forward
        protected:
            class Requestor;

        public:

            KARABO_CLASSINFO(SignalSlotable, "SignalSlotable", "1.0")

            typedef boost::function<void (const std::string& /*instanceId*/,
                                          const karabo::util::Hash& /*instanceInfo*/) > InstanceInfoHandler;

            typedef boost::function<void (const std::string& /*slotFunction*/,
                                          const std::string& /*callee*/) > SlotCallGuardHandler;

            typedef boost::function<void (float /*avgBrokerLatency*/, unsigned int /*maxBrokerLatency*/,
                                          float /*avgProcessingLatency*/, unsigned int /*maxProcessingLatency*/,
                                          unsigned int /*queueSize*/) > UpdatePerformanceStatisticsHandler;

            typedef boost::function<void (const karabo::util::Hash&) > DataHandler;

            typedef boost::function<void (InputChannel&) > InputHandler;

            // TODO Check why handlers of input and output are different! One is pointer, the other is reference!
            typedef boost::function<void (const OutputChannel::Pointer&) > OutputHandler;

            typedef boost::shared_ptr<karabo::xms::Slot> SlotInstancePointer;

            typedef boost::shared_ptr<karabo::xms::Signal> SignalInstancePointer;

            typedef std::map<std::string, InputChannel::Pointer> InputChannels;

            typedef std::map<std::string, OutputChannel::Pointer> OutputChannels;



            /**
             * This constructor does nothing. Call init() afterwards for setting up.
             */
            SignalSlotable();

            /**
             * Creates a functional SignalSlotable object using an existing connection.
             * Don't call init() afterwards.
             * @param instanceId The future instanceId of this object in the distributed system
             * @param connection An existing broker connection
             */
            SignalSlotable(const std::string& instanceId,
                           const karabo::net::JmsConnection::Pointer& connection);

            /**
             * Creates a function SignalSlotable object allowing to configure the broker connection.
             * Don't call init() afterwards.
             * @param instanceId The future instanceId of this object in the distributed system
             * @param brokerType The broker type (currently only JMS available)
             * @param brokerConfiguration The sub-configuration for the respective broker type
             */
            SignalSlotable(const std::string& instanceId,
                           const std::string& connectionClass = "JmsConnection",
                           const karabo::util::Hash& brokerConfiguration = karabo::util::Hash());

            virtual ~SignalSlotable();

            /**
             * Initialized the SignalSlotable object (only use in conjunction with empty constructor)
             * @param instanceId The future instanceId of this object in the distributed system
             * @param connection An existing broker connection
             */
            void init(const std::string& instanceId,
                      const karabo::net::JmsConnection::Pointer& connection);


            /**
             * Single call that leads to a tracking of all instances if called before the event loop is started
             */
            void trackAllInstances();

            karabo::util::Hash getAvailableInstances(const bool activateTracking = false);

            std::vector<std::string> getAvailableSignals(const std::string& instanceId);

            std::vector<std::string> getAvailableSlots(const std::string& instanceId);


            /**
             * Retrieves currently logged in username (empty if not logged in)
             * @return string username
             */
            // TODO Check whether it can be removed
            const std::string& getUserName() const;

            /**
             * Sets number of threads that will work on the registered slots.
             * Re-entry of the same slot on a different thread will never happen.
             * Only different slots may run concurrently (if nThreads > 1)
             * NOTE: This function takes only affect BEFORE the event loop was started
             * @param nThreads The number of threads that should be used
             */
            void setNumberOfThreads(int nThreads);

            /**
             * This function will block the main-thread.
             */
            void runEventLoop(int heartbeatIntervall = 10, const karabo::util::Hash& instanceInfo = karabo::util::Hash());

            /**
             * This function will stop all consumers and un-block the runEventLoop() function
             */
            void stopEventLoop();

            /**
             * Access to the identification of the current instance using signals and slots
             * @return instanceId
             */
            const std::string& getInstanceId() const;

            void updateInstanceInfo(const karabo::util::Hash& update);

            const karabo::util::Hash& getInstanceInfo() const;

            // TODO This hook is dead-code currently, bring back in shape with event-loop refactoring
            void registerInstanceNewHandler(const InstanceInfoHandler& instanceNewHandler);

            // TODO This hook is dead-code currently, bring back in shape with event-loop refactoring
            void registerInstanceGoneHandler(const InstanceInfoHandler& instanceGoneHandler);

            // TODO This hook is dead-code currently, bring back in shape with event-loop refactoring
            void registerInstanceUpdatedHandler(const InstanceInfoHandler& instanceUpdatedHandler);

            void registerSlotCallGuardHandler(const SlotCallGuardHandler& slotCallGuardHandler);

            void registerPerformanceStatisticsHandler(const UpdatePerformanceStatisticsHandler& updatePerformanceStatisticsHandler);

            karabo::net::JmsConnection::Pointer getConnection() const;

            /**
             * This function must only be called within a slotFunctions body. It returns the current object handling
             * the callback which provides more information on the sender.
             * @param slotFunction The string-ified name of the slotFunction you are currently in
             * @return instance of a Slot object (handler object for this callback)
             */
            const SlotInstancePointer& getSenderInfo(const std::string& slotFunction);

            /**
             * This function tries to establish a connection between a signal
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
             * @return whether connection is already succesfully established
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
             * Emits a signal, i.e. publishes the given payload
             * Emitting a signal is a fire-and-forget activity. The function returns immediately.
             * @param signalFunction The name of the previously registered signal
             * @param args... A variadic number of arguments to be published
             */
            template <typename ...Args>
            void emit(const std::string& signalFunction, const Args&... args) const;

            /**
             * Calls a (remote) function.
             * Calling a remote function is a fire-and-forget activity. The function returns immediately.
             * @param instanceId Instance to be called
             * @param functionName Function on instance to be called (must be a registered slot)
             * @param args Arguments with which to call the slot
             */
            template <typename ...Args>
            void call(const std::string& instanceId, const std::string& functionName, const Args&... args) const;

            template <typename ...Args>
            SignalSlotable::Requestor request(const std::string& instanceId, const std::string& functionName,
                                              const Args&... args);

            template <typename ...Args>
            SignalSlotable::Requestor requestNoWait(const std::string& requestInstanceId,
                                                    const std::string& requestFunctionName,
                                                    const std::string& replyInstanceId,
                                                    const std::string& replyFunctionName,
                                                    const Args&... args);

            template <typename ...Args>
            void reply(const Args&... args);

            template <typename ...Args>
            void registerSignal(const std::string& funcName);

            template <typename ...Args>
            void registerSystemSignal(const std::string& funcName);

            /**
             * Register a new slot function for a slot. A new slot is generated
             * if so necessary. It is checked that the signature of the new
             * slot is the same as an already registered one.
             */
            void registerSlot(const boost::function<void ()>& slot,
                              const std::string& funcName);

            template <class A1>
            void registerSlot(const boost::function<void (const A1&) >& slot,
                              const std::string& funcName);

            template <class A1, class A2>
            void registerSlot(const boost::function<void (const A1&, const A2&) >& slot,
                              const std::string & funcName);

            template <class A1, class A2, class A3>
            void registerSlot(const boost::function<void (const A1&, const A2&, const A3&) >& slot,
                              const std::string & funcName);

            template <class A1, class A2, class A3, class A4>
            void registerSlot(const boost::function<void (const A1&, const A2&, const A3&, const A4&) >& slot,
                              const std::string & funcName);

            // TODO This function is not used anywhere -> decide what to do
            bool connectChannels(std::string outputInstanceId, const std::string& outputName,
                                 std::string inputInstanceId, const std::string& inputName,
                                 const bool isVerbose = false);

            // TODO This function is not used anywhere -> decide what to do
            bool disconnectChannels(std::string outputInstanceId, const std::string& outputName,
                                    std::string inputInstanceId, const std::string& inputName,
                                    const bool isVerbose = false);

            virtual InputChannel::Pointer createInputChannel(const std::string& channelName,
                                                             const karabo::util::Hash& config,
                                                             const DataHandler& onDataAvailableHandler = DataHandler(),
                                                             const InputHandler& onInputAvailableHandler = InputHandler(),
                                                             const InputHandler& onEndOfStreamEventHandler = InputHandler());

            virtual OutputChannel::Pointer createOutputChannel(const std::string& channelName,
                                                               const karabo::util::Hash& config,
                                                               const OutputHandler& onOutputPossibleHandler = OutputHandler());

            const InputChannels& getInputChannels() const;

            const OutputChannels& getOutputChannels() const;

            const OutputChannel::Pointer& getOutputChannel(const std::string& name);

            const InputChannel::Pointer& getInputChannel(const std::string& name);

            void registerInputHandler(const std::string& channelName,
                                      const boost::function<void (karabo::xms::InputChannel&) >& handler);

            void registerDataHandler(const std::string& channelName,
                                     const boost::function<void (const karabo::util::Hash&) >& handler);

            void registerEndOfStreamHandler(const std::string& channelName,
                                            const boost::function<void (karabo::xms::InputChannel&) >& handler);

            /**
             * Connects and input channel to those as defined on the input channel's configuration.
             * The function is synchronous and blocks until a connection is established.
             */
            void connectInputChannel(const InputChannel::Pointer& channel, int trails = 8, int sleep = 1);

            // TODO This function seems to be called only internally, private candidate?
            void connectInputToOutputChannel(const InputChannel::Pointer& channel,
                                             const std::string& outputChannelString, int trails = 8, int sleep = 1);

            // TODO Not used see what will happen
            void connectInputChannelAsync(const InputChannel::Pointer& channel,
                                          const boost::function<void()>& handler);

            void connectInputChannels();

            void reconnectInputChannels(const std::string& instanceId);

            void disconnectInputChannels(const std::string& instanceId);

            std::pair<bool, std::string> exists(const std::string& instanceId);

            template <class T>
            static std::string generateInstanceId();

            bool ensureOwnInstanceIdUnique();

            void injectConnection(const std::string& instanceId,
                                  const karabo::net::JmsConnection::Pointer& connection);

            void setDeviceServerPointer(boost::any serverPtr);

            void inputHandlerWrap(const boost::function<void (karabo::xms::InputChannel&)>& handler,
                                  InputChannel& input);

            void dataHandlerWrap(const boost::function<void (const karabo::util::Hash&) >& handler,
                                 const karabo::util::Hash& data);

            void endOfStreamHandlerWrap(const boost::function<void (InputChannel&) >& handler,
                                        InputChannel& input);

            bool connectP2P(const std::string& instanceId);

            void disconnectP2P(const std::string& instanceId);

        protected:

            class Requestor {

            public:

                explicit Requestor(SignalSlotable* signalSlotable);

                virtual ~Requestor();

                template <typename ...Args>
                Requestor& request(const std::string& slotInstanceId,
                                   const std::string& slotFunction,
                                   const Args&... args);

                template <typename ...Args>
                Requestor& requestNoWait(
                                         const std::string& requestSlotInstanceId,
                                         const std::string& requestSlotFunction,
                                         const std::string replySlotInstanceId,
                                         const std::string& replySlotFunction,
                                         const Args&... args);

                void receiveAsync(const boost::function<void () >& replyCallback);

                template <class A1>
                void receiveAsync(const boost::function<void (const A1&) >& replyCallback);

                template <class A1, class A2>
                void receiveAsync(const boost::function<void (const A1&, const A2&) >& replyCallback);

                template <class A1, class A2, class A3>
                void receiveAsync(const boost::function<void (const A1&, const A2&, const A3&) >& replyCallback);

                template <class A1, class A2, class A3, class A4>
                void receiveAsync(const boost::function<void (const A1&, const A2&, const A3&, const A4&) >& replyCallback);

                template <typename ...Args>
                void receive(Args&... args);

                Requestor& timeout(const int& milliseconds);

                karabo::util::Hash::Pointer prepareRequestHeader(const std::string& slotInstanceId,
                                                                 const std::string& slotFunction);

                karabo::util::Hash::Pointer prepareRequestNoWaitHeader(const std::string& requestSlotInstanceId,
                                                                       const std::string& requestSlotFunction,
                                                                       const std::string& replySlotInstanceId,
                                                                       const std::string& replySlotFunction);

                void registerRequest();

                static std::string generateUUID();

                void sendRequest(const std::string& slotInstanceId, const karabo::util::Hash::Pointer& header,
                                 const karabo::util::Hash::Pointer& body) const;

                void receiveResponse(karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body);

            protected:

                SignalSlotable* m_signalSlotable;

            private:

                std::string m_replyId;
                bool m_isRequested;
                bool m_isReceived;
                int m_timeout;
                static boost::uuids::random_generator m_uuidGenerator;

            };

            struct SignalSlotConnection {

                SignalSlotConnection(const std::string& signalInstanceId, const std::string& signal,
                                     const std::string& slotInstanceId, const std::string& slot) :
                    m_signalInstanceId(signalInstanceId), m_signal(signal),
                    m_slotInstanceId(slotInstanceId), m_slot(slot) {
                }

                // needed to put into std::set:
                bool operator<(const SignalSlotConnection& other) const;

                std::string m_signalInstanceId;
                std::string m_signal;
                std::string m_slotInstanceId;
                std::string m_slot;
            };

            // Use (mutex, condition variable) pair for waiting simultaneously many events

            struct BoostMutexCond {

                boost::mutex m_mutex;
                boost::condition_variable m_cond;

                BoostMutexCond() : m_mutex(), m_cond() {
                }
            private:
                BoostMutexCond(BoostMutexCond&);
            };

            std::string m_instanceId;
            karabo::util::Hash m_instanceInfo;
            std::string m_username;
            int m_nThreads;

            typedef std::map<std::string, SignalInstancePointer> SignalInstances;
            SignalInstances m_signalInstances;

            typedef std::map<std::string, SlotInstancePointer> SlotInstances;
            SlotInstances m_slotInstances;

            // TODO Split into two mutexes
            mutable boost::mutex m_signalSlotInstancesMutex;

            // key is instanceId of signal or slot
            typedef std::map<std::string, std::set<SignalSlotConnection> > SignalSlotConnections;
            SignalSlotConnections m_signalSlotConnections; // keep track of established connections
            boost::mutex m_signalSlotConnectionsMutex;

            bool m_connectionInjected;
            karabo::net::JmsConnection::Pointer m_connection;
            karabo::net::JmsProducer::Pointer m_producerChannel;
            karabo::net::JmsConsumer::Pointer m_consumerChannel;
            karabo::net::JmsProducer::Pointer m_heartbeatProducerChannel;
            karabo::net::JmsConsumer::Pointer m_heartbeatConsumerChannel;

            boost::mutex m_waitMutex;
            boost::condition_variable m_hasNewEvent;

            typedef std::pair<karabo::util::Hash::Pointer /*header*/, karabo::util::Hash::Pointer /*body*/> Event;
            std::deque<Event> m_eventQueue;
            boost::mutex m_eventQueueMutex;

            bool m_runEventLoop;
            boost::thread_group m_eventLoopThreads;

            int m_randPing;

            // Reply/Request related
            typedef std::map<boost::thread::id, karabo::util::Hash> Replies;
            Replies m_replies;
            mutable boost::mutex m_replyMutex;


            typedef std::map<std::string, Event > ReceivedReplies;
            ReceivedReplies m_receivedReplies;
            mutable boost::mutex m_receivedRepliesMutex;


            typedef std::map<std::string, boost::shared_ptr<BoostMutexCond> > ReceivedRepliesBMC;
            ReceivedRepliesBMC m_receivedRepliesBMC;
            mutable boost::mutex m_receivedRepliesBMCMutex;

            karabo::util::Hash m_emitFunctions;
            std::vector<boost::any> m_slots;            

            karabo::util::Hash m_trackedInstances;
            bool m_trackAllInstances;
            int m_heartbeatInterval;

            mutable boost::mutex m_trackedInstancesMutex;

            boost::thread m_trackingThread;
            bool m_doTracking;

            boost::thread m_heartbeatThread;
            bool m_sendHeartbeats;

            boost::thread m_brokerThread;

            std::vector<std::pair<std::string, karabo::util::Hash> > m_availableInstances;

            // IO channel related
            InputChannels m_inputChannels;
            OutputChannels m_outputChannels;

            // InstanceInfo Handlers
            InstanceInfoHandler m_instanceNewHandler;
            InstanceInfoHandler m_instanceGoneHandler;
            InstanceInfoHandler m_instanceUpdatedHandler;

            SlotCallGuardHandler m_slotCallGuardHandler;
            UpdatePerformanceStatisticsHandler m_updatePerformanceStatistics;

            static std::map<std::string, SignalSlotable*> m_instanceMap;
            static boost::mutex m_instanceMapMutex;

            bool m_discoverConnectionResourcesMode;
            static std::map<std::string, std::string> m_connectionStrings;
            static boost::mutex m_connectionStringsMutex;

            static karabo::net::PointToPoint::Pointer m_pointToPoint;

            // TODO This is a helper variable
            std::string m_topic;

        protected: // Functions

            void startEmittingHeartbeats(const int heartbeatInterval);

            void stopEmittingHearbeats();

            void startBrokerMessageConsumption();

            void stopBrokerMessageConsumption();

            void injectEvent(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);

            void handleReply(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);

            void injectHeartbeat(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);

            bool eventQueueIsEmpty();

            bool tryToPopEvent(Event& event);

            void processEvents();

            bool heartbeatQueueIsEmpty();

            bool tryToPopHeartbeat(Event& event);

            void processHeartbeats();

            /**
             * Parses out the instanceId part of signalId or slotId
             * @param signalOrSlotId
             * @return A string representing the instanceId
             */
            std::string fetchInstanceId(const std::string& signalOrSlotId) const;

            std::pair<std::string, std::string> splitIntoInstanceIdAndFunctionName(const std::string& signalOrSlotId,
                                                                                   const char sep = ':') const;

            template <class TFunc>
            void storeSignal(const std::string& signalFunction, const SignalInstancePointer& signalInstance,
                             const TFunc& emitFunction) {
                storeSignal(signalFunction, signalInstance);
                m_emitFunctions.set(signalFunction, emitFunction);
            }

            template <class TFunc>
            void retrieveEmitFunction(const std::string& signalFunction, TFunc& emitFunction) const {
                try {
                    emitFunction = m_emitFunctions.get<TFunc > (signalFunction);
                } catch (const karabo::util::CastException& e) {
                    throw KARABO_SIGNALSLOT_EXCEPTION("Argument mismatch: The requested signal \"" +
                                                      signalFunction +
                                                      "\" was registered with a different number of arguments before.");
                } catch (const karabo::util::ParameterException& e) {
                    throw KARABO_SIGNALSLOT_EXCEPTION("The requested signal \"" +
                                                      signalFunction +
                                                      "\" could not be found. Ensure proper registration of the signal before you call emit.");
                }
            }

            void registerReply(const karabo::util::Hash& reply);

            // Thread-safe, locks m_signalSlotInstancesMutex
            SignalInstancePointer getSignal(const std::string& signalFunction) const;

            karabo::util::Hash::Pointer prepareCallHeader(const std::string& slotInstanceId,
                                                          const std::string& slotFunction) const;

            void doSendMessage(const std::string& instanceId, const karabo::util::Hash::Pointer& header,
                               const karabo::util::Hash::Pointer& body, int prio, int timeToLive,
                               const std::string& topic = "") const;

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
            template <typename ...Args>
            SignalInstancePointer addSignalIfNew(const std::string& signalFunction, int priority = KARABO_SYS_PRIO,
                                                 int messageTimeToLive = KARABO_SYS_TTL);

            /**
             * Helper to store signalInstance in container with signalFunction as key.
             */
            void storeSignal(const std::string &signalFunction, SignalInstancePointer signalInstance);

            void _runEventLoop();

            void _runHeartbeatLoop();

            void sanifyInstanceId(std::string& instanceId) const;

            std::pair<bool, std::string> isValidInstanceId(const std::string& instanceId);

            void slotInstanceNew(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void slotPing(const std::string& instanceId, int rand, bool trackPingedInstance);

            void slotPingAnswer(const std::string& instanceId, const karabo::util::Hash& hash);

            void replyException(const karabo::util::Hash& header, const std::string& message);

            void sendPotentialReply(const karabo::util::Hash& header, bool global);

            void sendErrorHappenedReply(const karabo::util::Hash& header, const std::string& errorMesssage);

            void emitHeartbeat();

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

            bool tryToConnectToSignal(const std::string& signalInstanceId, const std::string& signalFunction,
                                      const std::string& slotInstanceId, const std::string& slotFunction);

            SlotInstancePointer findSlot(const std::string &funcName);

            /**
             * Register a new slot *instance* under name *funcName*.
             * This will raise an error if the slot already exists.
             */
            void registerNewSlot(const std::string &funcName, SlotInstancePointer instance);

            void slotConnectToSignal(const std::string& signalFunction, const std::string& slotInstanceId,
                                     const std::string& slotFunction);

            /// True if instance with ID 'slotInstanceId' has slot 'slotFunction'.
            /// Internally uses "slotHasSlot" for remote instances, but shortcuts if ID is the own one.
            /// Always true if 'slotInstanceId == "*"' (i.e. global slot).
            bool instanceHasSlot(const std::string& slotInstanceId, const std::string& slotFunction);

            /// Slot to tell whether instance has a slot of given name.
            void slotHasSlot(const std::string& slotFunction);

            bool tryToDisconnectFromSignal(const std::string& signalInstanceId, const std::string& signalFunction,
                                           const std::string& slotInstanceId, const std::string& slotFunction);

            void slotDisconnectFromSignal(const std::string& signalFunction, const std::string& slotInstanceId,
                                          const std::string& slotFunction);

            static std::string prepareFunctionSignature(const std::string& funcName) {
                std::string f(boost::trim_copy(funcName));
                return f.substr(0, f.find_first_of('-')) + "|";
            }

            bool slotConnectToOutputChannel(const std::string& inputName, const karabo::util::Hash& outputChannelInfo,
                                            bool connect);

            void slotHeartbeat(const std::string& networkId, const int& heartbeatInterval,
                               const karabo::util::Hash& instanceInfo);

            void letInstanceSlowlyDieWithoutHeartbeat();

            void decreaseCountdown(std::vector<std::pair<std::string, karabo::util::Hash> >& deadOnes);


            // Thread safe
            void addTrackedInstance(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            // Thread safe
            bool hasTrackedInstance(const std::string& instanceId);

            // Thread safe
            void eraseTrackedInstance(const std::string& instanceId);

            void updateTrackedInstanceInfo(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void addTrackedInstanceConnection(const std::string& instanceId, const karabo::util::Hash& connection);

            void slotGetAvailableFunctions(const std::string& type);

            void cleanSignals(const std::string& instanceId);

            void stopTracking(const std::string& instanceId);

            // IO channel related
            karabo::util::Hash slotGetOutputChannelInformation(const std::string& ioChannelId, const int& processId);

            static int godEncode(const std::string& password);

            // Thread-safe, locks m_signalSlotInstancesMutex
            bool hasSlot(const std::string& slotFunction) const;

            // Thread-safe, locks m_signalSlotInstancesMutex
            SlotInstancePointer getSlot(const std::string& slotFunction) const;

            // Thread-safe, locks m_signalSlotInstancesMutex
            void removeSlot(const std::string& slotFunction);

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

            bool timedWaitAndPopReceivedReply(const std::string& replyId, karabo::util::Hash::Pointer& header,
                                              karabo::util::Hash::Pointer& body, int timeout);

            long long getEpochMillis() const;

            void onInputChannelConnectInfo(const InputChannel::Pointer& channel,
                                           const boost::function<void()>& handler,
                                           const std::string& instanceId, const std::string& channelId,
                                           bool channelExists, const karabo::util::Hash& info);

            bool tryToCallDirectly(const std::string& slotInstanceId,
                                   const karabo::util::Hash::Pointer& header,
                                   const karabo::util::Hash::Pointer& body) const;

            bool tryToCallP2P(const std::string& slotInstanceId, const karabo::util::Hash::Pointer& header,
                              const karabo::util::Hash::Pointer& body, int prio) const;

            // TODO This is a helper function during multi-topic refactoring
            void setTopic(const std::string& topic = "");


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

            mutable boost::mutex m_latencyMutex;
            LatencyStats m_brokerLatency; // all in milliseconds
            LatencyStats m_processingLatency; // dito

        };

        /**** Requestor Implementation ****/

        template <typename ...Args>
        SignalSlotable::Requestor& SignalSlotable::Requestor::request(const std::string& slotInstanceId,
                                                                      const std::string& slotFunction,
                                                                      const Args&... args) {
            karabo::util::Hash::Pointer header = prepareRequestHeader(slotInstanceId, slotFunction);
            auto body = boost::make_shared<karabo::util::Hash>();
            pack(*body, args...);
            sendRequest(slotInstanceId, header, body);
            return *this;
        }

        template <typename ...Args>
        SignalSlotable::Requestor& SignalSlotable::Requestor::requestNoWait(const std::string& requestSlotInstanceId,
                                                                            const std::string& requestSlotFunction,
                                                                            const std::string replySlotInstanceId,
                                                                            const std::string& replySlotFunction,
                                                                            const Args&... args) {

            karabo::util::Hash::Pointer header = prepareRequestNoWaitHeader(requestSlotInstanceId,
                                                                            requestSlotFunction,
                                                                            replySlotInstanceId,
                                                                            replySlotFunction);

            auto body = boost::make_shared<karabo::util::Hash>();
            pack(*body, args...);
            sendRequest(requestSlotInstanceId, header, body);
            return *this;
        }

        template <class A1>
        void SignalSlotable::Requestor::receiveAsync(const boost::function<void (const A1&) >& replyCallback) {
            m_signalSlotable->registerSlot<A1>(replyCallback, m_replyId);
        }

        template <class A1, class A2>
        void SignalSlotable::Requestor::receiveAsync(const boost::function<void (const A1&, const A2&) >& replyCallback) {
            m_signalSlotable->registerSlot<A1, A2>(replyCallback, m_replyId);
        }

        template <class A1, class A2, class A3>
        void SignalSlotable::Requestor::receiveAsync(const boost::function<void (const A1&, const A2&, const A3&) >& replyCallback) {
            m_signalSlotable->registerSlot<A1, A2, A3>(replyCallback, m_replyId);
        }

        template <class A1, class A2, class A3, class A4>
        void SignalSlotable::Requestor::receiveAsync(const boost::function<void (const A1&, const A2&, const A3&, const A4&) >& replyCallback) {
            m_signalSlotable->registerSlot<A1, A2, A3, A4>(replyCallback, m_replyId);
        }

        template <typename ...Args>
        void SignalSlotable::Requestor::receive(Args&... args) {
            try {
                karabo::util::Hash::Pointer header, body;
                receiveResponse(header, body);
                if (header->has("error") && header->get<bool>("error")) {
                    throw karabo::util::RemoteException(body->get<std::string>("a1"),
                                                        header->get<std::string>("signalInstanceId"));
                }
                karabo::util::unpack(*body, args...);

                if (sizeof...(Args) != body->size()) {
                    int nArgs = body->size() - sizeof...(Args);
                    KARABO_LOG_FRAMEWORK_DEBUG << "Ignoring the last " << nArgs << " arguments of response:\n"
                            << *body;
                }

            } catch (const karabo::util::TimeoutException&) {
                KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
            } catch (const karabo::util::CastException &) {
                KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Error while receiving message on instance \""
                                                              + m_signalSlotable->getInstanceId() + "\""));
            }
        }

        /**** SignalSlotable Template Function Implementations ****/

        template <typename ...Args>
        void SignalSlotable::emit(const std::string& signalFunction, const Args&... args) const {
            SignalInstancePointer s = getSignal(signalFunction);
            if (s) {
                auto hash = boost::make_shared<karabo::util::Hash>();
                pack(*hash, args...);
                s->emit < Args...>(hash);
            }
        }

        template <typename ...Args>
        void SignalSlotable::call(const std::string& instanceId, const std::string& functionName,
                                  const Args&... args) const {
            const std::string& id = (instanceId.empty() ? m_instanceId : instanceId);
            auto body = boost::make_shared<karabo::util::Hash>();
            pack(*body, args...);
            karabo::util::Hash::Pointer header = prepareCallHeader(id, functionName);
            doSendMessage(id, header, body, KARABO_SYS_PRIO, KARABO_SYS_TTL);
        }

        template <typename ...Args>
        SignalSlotable::Requestor SignalSlotable::request(const std::string& instanceId,
                                                          const std::string& functionName,
                                                          const Args&... args) {
            const std::string& id = (instanceId.empty() ? m_instanceId : instanceId);
            return SignalSlotable::Requestor(this).request(id, functionName, args...);
        }

        template <typename ...Args>
        SignalSlotable::Requestor SignalSlotable::requestNoWait(const std::string& requestInstanceId,
                                                                const std::string& requestFunctionName,
                                                                const std::string& replyInstanceId,
                                                                const std::string& replyFunctionName,
                                                                const Args&... args) {
            const std::string& reqId = (requestInstanceId.empty() ? m_instanceId : requestInstanceId);
            const std::string& repId = (replyInstanceId.empty() ? m_instanceId : replyInstanceId);
            return SignalSlotable::Requestor(this).requestNoWait(reqId,
                                                                 requestFunctionName,
                                                                 repId,
                                                                 replyFunctionName,
                                                                 args...);
        }

        template <typename ...Args>
        void SignalSlotable::reply(const Args&... args) {
            karabo::util::Hash hash;
            karabo::util::pack(hash, args...);
            registerReply(hash);
        }

        template <typename ...Args>
        void SignalSlotable::registerSignal(const std::string& funcName) {
            addSignalIfNew < Args...>(funcName, KARABO_PUB_PRIO, KARABO_PUB_TTL);
        }

        template <typename ...Args>
        void SignalSlotable::registerSystemSignal(const std::string& funcName) {
            addSignalIfNew < Args...>(funcName);
        }

        template <class A1>
        void SignalSlotable::registerSlot(const boost::function<void (const A1&) >& slot,
                                          const std::string& funcName) {
            // About the dynamic_pointer_cast: see non-template version of registerSlot.
            auto s = boost::dynamic_pointer_cast < SlotN<void, A1> >(findSlot(funcName));
            if (!s) {
                s = boost::make_shared < SlotN <void, A1> >(funcName);
                registerNewSlot(funcName, boost::static_pointer_cast<Slot>(s));
            }
            s->registerSlotFunction(slot);
        }

        template <class A1, class A2>
        void SignalSlotable::registerSlot(const boost::function<void (const A1&, const A2&) >& slot,
                                          const std::string & funcName) {
            // About the dynamic_pointer_cast: see non-template version of registerSlot.
            auto s = boost::dynamic_pointer_cast < SlotN<void, A1, A2> >(findSlot(funcName));
            if (!s) {
                s = boost::make_shared < SlotN <void, A1, A2> >(funcName);
                registerNewSlot(funcName, boost::static_pointer_cast<Slot>(s));
            }
            s->registerSlotFunction(slot);
        }

        template <class A1, class A2, class A3>
        void SignalSlotable::registerSlot(const boost::function<void (const A1&, const A2&, const A3&) >& slot,
                                          const std::string & funcName) {
            // About the dynamic_pointer_cast: see non-template version of registerSlot.
            auto s = boost::dynamic_pointer_cast < SlotN<void, A1, A2, A3> >(findSlot(funcName));
            if (!s) {
                s = boost::make_shared < SlotN <void, A1, A2, A3> >(funcName);
                registerNewSlot(funcName, boost::static_pointer_cast<Slot>(s));
            }
            s->registerSlotFunction(slot);
        }

        template <class A1, class A2, class A3, class A4>
        void SignalSlotable::registerSlot(const boost::function<void (const A1&, const A2&, const A3&, const A4&) >& slot,
                                          const std::string & funcName) {
            // About the dynamic_pointer_cast: see non-template version of registerSlot.
            auto s = boost::dynamic_pointer_cast < SlotN<void, A1, A2, A3, A4> >(findSlot(funcName));
            if (!s) {
                s = boost::make_shared < SlotN <void, A1, A2, A3, A4> >(funcName);
                registerNewSlot(funcName, boost::static_pointer_cast<Slot>(s));
            }
            s->registerSlotFunction(slot);
        }

        template <typename ...Args>
        SignalSlotable::SignalInstancePointer SignalSlotable::addSignalIfNew(const std::string& signalFunction,
                                                                             int priority, int messageTimeToLive) {
            {
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                if (m_signalInstances.find(signalFunction) != m_signalInstances.end()) {
                    SignalInstancePointer s;
                    return s;
                }
            }
            // TODO Check mutex here
            auto s = boost::make_shared<Signal>(this, m_producerChannel, m_instanceId, signalFunction, priority,
                                                messageTimeToLive);
            s->setSignature < Args...>();
            storeSignal(signalFunction, s);
            return s;
        }

        template <class T>
        std::string SignalSlotable::generateInstanceId() {
            std::string hostname(boost::asio::ip::host_name());
            std::vector<std::string> tokens;
            boost::split(tokens, hostname, boost::is_any_of("."));
            return std::string(tokens[0] + "_" + T::classInfo().getClassId() + "_" + karabo::util::toString(getpid()));
        }
    }
}

#define KARABO_SIGNAL0(signalName) this->registerSignal(signalName);
#define KARABO_SIGNAL1(signalName, a1) this->registerSignal<a1>(signalName);
#define KARABO_SIGNAL2(signalName, a1, a2) this->registerSignal<a1,a2>(signalName);
#define KARABO_SIGNAL3(signalName, a1, a2, a3) this->registerSignal<a1,a2,a3>(signalName);
#define KARABO_SIGNAL4(signalName, a1, a2, a3, a4) this->registerSignal<a1,a2,a3,a4>(signalName);

#define KARABO_SYSTEM_SIGNAL0(signalName) this->registerSystemSignal(signalName);
#define KARABO_SYSTEM_SIGNAL1(signalName, a1) this->registerSystemSignal<a1>(signalName);
#define KARABO_SYSTEM_SIGNAL2(signalName, a1, a2) this->registerSystemSignal<a1,a2>(signalName);
#define KARABO_SYSTEM_SIGNAL3(signalName, a1, a2, a3) this->registerSystemSignal<a1,a2,a3>(signalName);
#define KARABO_SYSTEM_SIGNAL4(signalName, a1, a2, a3, a4) this->registerSystemSignal<a1,a2,a3,a4>(signalName);

#define KARABO_SLOT0(slotName) this->registerSlot(boost::bind(&Self::slotName,this),#slotName);
#define KARABO_SLOT1(slotName, a1) this->registerSlot<a1>(boost::bind(&Self::slotName,this,_1),#slotName);
#define KARABO_SLOT2(slotName, a1, a2) this->registerSlot<a1,a2>(boost::bind(&Self::slotName,this,_1,_2),#slotName);
#define KARABO_SLOT3(slotName, a1, a2, a3) this->registerSlot<a1,a2,a3>(boost::bind(&Self::slotName,this,_1,_2,_3),#slotName);
#define KARABO_SLOT4(slotName, a1, a2, a3, a4) this->registerSlot<a1,a2,a3,a4>(boost::bind(&Self::slotName,this,_1,_2,_3,_4),#slotName);

#define KARABO_ON_INPUT(channelName, funcName) this->registerInputHandler(channelName, boost::bind(&Self::funcName,this,_1));
#define KARABO_ON_DATA(channelName, funcName) this->registerDataHandler(channelName, boost::bind(&Self::funcName, this,_1));
#define KARABO_ON_EOS(channelName, funcName) this->registerEndOfStreamHandler(channelName, boost::bind(&Self::funcName,this,_1));

#define _KARABO_SIGNAL_N(x0,x1,x2,x3,x4,x5,FUNC, ...) FUNC
#define _KARABO_SYSTEM_SIGNAL_N(x0,x1,x2,x3,x4,x5,FUNC, ...) FUNC
#define _KARABO_SLOT_N(x0,x1,x2,x3,x4,x5,FUNC, ...) FUNC

#define KARABO_SIGNAL(...) \
_KARABO_SIGNAL_N(,##__VA_ARGS__, \
KARABO_SIGNAL4(__VA_ARGS__), \
KARABO_SIGNAL3(__VA_ARGS__), \
KARABO_SIGNAL2(__VA_ARGS__), \
KARABO_SIGNAL1(__VA_ARGS__), \
KARABO_SIGNAL0(__VA_ARGS__) \
)

#define KARABO_SYSTEM_SIGNAL(...) \
_KARABO_SYSTEM_SIGNAL_N(,##__VA_ARGS__, \
KARABO_SYSTEM_SIGNAL4(__VA_ARGS__), \
KARABO_SYSTEM_SIGNAL3(__VA_ARGS__), \
KARABO_SYSTEM_SIGNAL2(__VA_ARGS__), \
KARABO_SYSTEM_SIGNAL1(__VA_ARGS__), \
KARABO_SYSTEM_SIGNAL0(__VA_ARGS__) \
)

#define KARABO_SLOT(...) \
_KARABO_SLOT_N(,##__VA_ARGS__, \
KARABO_SLOT4(__VA_ARGS__), \
KARABO_SLOT3(__VA_ARGS__), \
KARABO_SLOT2(__VA_ARGS__), \
KARABO_SLOT1(__VA_ARGS__), \
KARABO_SLOT0(__VA_ARGS__) \
)

#endif
