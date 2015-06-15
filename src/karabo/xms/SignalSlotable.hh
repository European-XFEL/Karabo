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

#include <queue>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include <karabo/util/Configurator.hh>
#include <karabo/log/Logger.hh>
#include <karabo/net/BrokerConnection.hh>

#include "OutputChannel.hh"
#include "InputChannel.hh"
#include "Data.hh"

#include "Signal.hh"
#include "Slot.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    // Forward
    namespace webAuth {
        class Authenticator;
    }

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
            // Performance statistics
            mutable boost::mutex m_latencyMutex;
            std::pair<int, int> m_brokerLatency;
            std::pair<int, int> m_processingLatency;


        protected:

            class Requestor {
                SignalSlotable* m_signalSlotable;
                std::string m_replyId;
                bool m_isRequested;
                bool m_isReceived;
                int m_timeout;
                static boost::uuids::random_generator m_uuidGenerator;

            public:
                explicit Requestor(SignalSlotable* signalSlotable);

                virtual ~Requestor() {
                }

                Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction) {
                    sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash());
                    return *this;
                }

                template <class A1>
                Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1) {
                    sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1));
                    return *this;
                }

                template <class A1, class A2>
                Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2) {
                    sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2));
                    return *this;
                }

                template <class A1, class A2, class A3>
                Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3) {
                    sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3));
                    return *this;
                }

                template <class A1, class A2, class A3, class A4>
                Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                    sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3, "a4", a4));
                    return *this;
                }

                // TODO: Add another API layer, which is called execute and does synchronous slot execution

                Requestor& requestNoWait(
                        const std::string& requestSlotInstanceId,
                        const std::string& requestSlotFunction,
                        const std::string replySlotInstanceId,
                        const std::string& replySlotFunction) {
                    sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), karabo::util::Hash());
                    return *this;
                }

                template <class A1>
                Requestor& requestNoWait(
                        const std::string& requestSlotInstanceId,
                        const std::string& requestSlotFunction,
                        const std::string replySlotInstanceId,
                        const std::string& replySlotFunction,
                        const A1& a1) {
                    sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), karabo::util::Hash("a1", a1));
                    return *this;
                }

                template <class A1, class A2>
                Requestor& requestNoWait(
                        const std::string& requestSlotInstanceId,
                        const std::string& requestSlotFunction,
                        const std::string replySlotInstanceId,
                        const std::string& replySlotFunction,
                        const A1& a1, const A2& a2) {
                    sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), karabo::util::Hash("a1", a1, "a2", a2));
                    return *this;
                }

                template <class A1, class A2, class A3>
                Requestor& requestNoWait(
                        const std::string& requestSlotInstanceId,
                        const std::string& requestSlotFunction,
                        const std::string replySlotInstanceId,
                        const std::string& replySlotFunction, const A1& a1, const A2& a2, const A3& a3) {
                    sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3));
                    return *this;
                }

                template <class A1, class A2, class A3, class A4>
                Requestor& requestNoWait(const std::string& requestSlotInstanceId,
                        const std::string& requestSlotFunction,
                        const std::string replySlotInstanceId,
                        const std::string& replySlotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                    sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3, "a4", a4));
                    return *this;
                }

                void receiveAsync(const boost::function<void () >& replyCallback) {
                    m_signalSlotable->registerSlot(replyCallback, m_replyId);
                }

                template <class A1>
                void receiveAsync(const boost::function<void (const A1&) >& replyCallback) {
                    m_signalSlotable->registerSlot<A1>(replyCallback, m_replyId);
                }

                template <class A1, class A2>
                void receiveAsync(const boost::function<void (const A1&, const A2&) >& replyCallback) {
                    m_signalSlotable->registerSlot<A1, A2>(replyCallback, m_replyId);
                }

                template <class A1, class A2, class A3>
                void receiveAsync(const boost::function<void (const A1&, const A2&, const A3&) >& replyCallback) {
                    m_signalSlotable->registerSlot<A1, A2, A3>(replyCallback, m_replyId);
                }

                template <class A1, class A2, class A3, class A4>
                void receiveAsync(const boost::function<void (const A1&, const A2&, const A3&, const A4&) >& replyCallback) {
                    m_signalSlotable->registerSlot<A1, A2, A3, A4>(replyCallback, m_replyId);
                }

                class Receiver {
                public:
                    void receive(Requestor *);
                protected:
                    virtual void inner(karabo::util::Hash::Pointer) = 0;
                };

                class Receiver0 : public Receiver {
                public:

                    Receiver0() {
                    }
                protected:

                    virtual void inner(karabo::util::Hash::Pointer) {
                    }
                };

                void receive() {
                    Receiver0 receiver;
                    receiver.receive(this);
                }

                template <class A1>
                class Receiver1 : public Receiver {
                    A1 &m_a1;

                public:

                    Receiver1(A1 &a1) : m_a1(a1) {
                    }

                protected:

                    virtual void inner(karabo::util::Hash::Pointer body) {
                        m_a1 = body->get<A1>("a1");
                    }
                };

                template <class A1>
                void receive(A1& a1) {
                    Receiver1<A1> receiver(a1);
                    receiver.receive(this);
                }

                template <class A1, class A2>
                class Receiver2 : public Receiver {
                    A1 &m_a1;
                    A2 &m_a2;

                public:

                    Receiver2(A1 &a1, A2 &a2) : m_a1(a1), m_a2(a2) {
                    }

                protected:

                    virtual void inner(karabo::util::Hash::Pointer body) {
                        m_a1 = body->get<A1>("a1");
                        m_a2 = body->get<A2>("a2");
                    }
                };

                template <class A1, class A2>
                void receive(A1& a1, A2& a2) {
                    Receiver2<A1, A2> receiver(a1, a2);
                    receiver.receive(this);
                }

                template <class A1, class A2, class A3>
                class Receiver3 : public Receiver {
                    A1 &m_a1;
                    A2 &m_a2;
                    A3 &m_a3;

                public:

                    Receiver3(A1 &a1, A2 &a2, A3 &a3) : m_a1(a1), m_a2(a2), m_a3(a3) {
                    }

                protected:

                    virtual void inner(karabo::util::Hash::Pointer body) {
                        m_a1 = body->get<A1>("a1");
                        m_a2 = body->get<A2>("a2");
                        m_a3 = body->get<A3>("a3");
                    }
                };

                template <class A1, class A2, class A3>
                void receive(A1& a1, A2& a2, A3& a3) {
                    Receiver3<A1, A2, A3> receiver(a1, a2, a3);
                    receiver.receive(this);
                }

                template <class A1, class A2, class A3, class A4>
                class Receiver4 : public Receiver {
                    A1 &m_a1;
                    A2 &m_a2;
                    A3 &m_a3;
                    A4 &m_a4;

                public:

                    Receiver4(A1 &a1, A2 &a2, A3 &a3, A4 &a4) : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4) {
                    }

                protected:

                    virtual void inner(karabo::util::Hash::Pointer body) {
                        m_a1 = body->get<A1>("a1");
                        m_a2 = body->get<A2>("a2");
                        m_a3 = body->get<A3>("a3");
                        m_a4 = body->get<A4>("a4");
                    }
                };

                template <class A1, class A2, class A3, class A4>
                void receive(A1& a1, A2& a2, A3& a3, A4& a4) {
                    Receiver4<A1, A2, A3, A4> receiver(a1, a2, a3, a4);
                    receiver.receive(this);
                }

                Requestor& timeout(const int& milliseconds);

            protected: // functions

                karabo::util::Hash prepareHeader(const std::string& slotInstanceId, const std::string& slotFunction);

                karabo::util::Hash prepareHeaderNoWait(const std::string& requestSlotInstanceId, const std::string& requestSlotFunction,
                        const std::string& replySlotInstanceId, const std::string& replySlotFunction);

                void registerRequest();

                static std::string generateUUID();

                void sendRequest(const karabo::util::Hash& header, const karabo::util::Hash& body) const;

                void receiveResponse(karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body);

            };


        protected:

            // Internally used typedefs
            typedef boost::shared_ptr<karabo::xms::Signal> SignalInstancePointer;
            typedef std::map<std::string, SignalInstancePointer> SignalInstances;
            typedef SignalInstances::const_iterator SignalInstancesConstIt;

            typedef boost::shared_ptr<karabo::xms::Slot> SlotInstancePointer;
            typedef std::map<std::string, SlotInstancePointer> SlotInstances;
            typedef SlotInstances::const_iterator SlotInstancesConstIt;

            typedef std::map<std::string, karabo::net::BrokerChannel::Pointer> SlotChannels;
            typedef SlotChannels::const_iterator SlotChannelsConstIt;

            typedef std::map<boost::thread::id, karabo::util::Hash> Replies;

            typedef std::pair<std::string, int> AssocEntry;
            typedef std::set<AssocEntry> AssocType;
            typedef AssocType::const_iterator AssocTypeConstIterator;

            typedef boost::function<void (const std::string& /*instanceId*/, const karabo::util::Hash& /*instanceInfo*/) > InstanceNotAvailableHandler;
            typedef boost::function<void (const std::string& /*instanceId*/, const karabo::util::Hash& /*instanceInfo*/) > InstanceAvailableAgainHandler;
            typedef boost::function<void (const std::string& /*instanceId*/, const karabo::util::Hash& /*instanceInfo*/) > InstanceNewHandler;
            typedef boost::function<void (const karabo::util::Exception& /*exception*/) > ExceptionHandler;
            typedef boost::function<bool (const std::string& /*slotFunction*/) > SlotCallGuardHandler;
            typedef boost::function<void (float /*brokerLatency*/, float /*processingLatency*/, unsigned int /*queueSize*/) > UpdatePerformanceStatisticsHandler;

            typedef std::map<std::string, InputChannel::Pointer> InputChannels;
            typedef std::map<std::string, OutputChannel::Pointer> OutputChannels;

            std::string m_instanceId;
            karabo::util::Hash m_instanceInfo;
            std::string m_username;
            int m_nThreads;
            
            SignalInstances m_signalInstances;
            SlotInstances m_localSlotInstances;
            SlotInstances m_globalSlotInstances;
            // TODO Split into two mutexes
            mutable boost::mutex m_signalSlotInstancesMutex;

            karabo::net::BrokerIOService::Pointer m_ioService;
            karabo::net::BrokerConnection::Pointer m_connection;
            karabo::net::BrokerChannel::Pointer m_producerChannel;
            karabo::net::BrokerChannel::Pointer m_consumerChannel;
            karabo::net::BrokerChannel::Pointer m_heartbeatProducerChannel;
            karabo::net::BrokerChannel::Pointer m_heartbeatConsumerChannel;

            boost::mutex m_waitMutex;
            boost::condition_variable m_hasNewEvent;

            typedef std::pair<karabo::util::Hash::Pointer /*header*/, karabo::util::Hash::Pointer /*body*/> Event;

            struct CompareEventPriority {

                bool operator()(const Event& lhs, const Event& rhs) const {
                    return lhs.first->get<signed char>("MQPriority") < rhs.first->get<signed char>("MQPriority");
                }
            };
            //std::priority_queue< Event, std::vector<Event>, CompareEventPriority > m_eventQueue;
            std::deque<Event> m_eventQueue;
            boost::mutex m_eventQueueMutex;
            bool m_runEventLoop;
            boost::thread_group m_eventLoopThreads;

            int m_randPing;

            // Reply/Request related
            Replies m_replies;
            boost::mutex m_replyMutex;


            typedef std::map<std::string, Event > ReceivedReplies;
            ReceivedReplies m_receivedReplies;
            mutable boost::mutex m_receivedRepliesMutex;

            // Use (mutex, condition variable) pair for waiting simultaneously many events

            struct BoostMutexCond {
                boost::mutex m_mutex;
                boost::condition_variable m_cond;

                BoostMutexCond() : m_mutex(), m_cond() {
                }
            private:
                BoostMutexCond(BoostMutexCond&);
            };

            typedef std::map<std::string, boost::shared_ptr<BoostMutexCond> > ReceivedRepliesBMC;
            ReceivedRepliesBMC m_receivedRepliesBMC;
            mutable boost::mutex m_receivedRepliesBMCMutex;

            karabo::util::Hash m_emitFunctions;
            std::vector<boost::any> m_slots;

            SlotChannels m_slotChannels;

            karabo::util::Hash m_trackedComponents;
            int m_heartbeatInterval;
            static std::set<int> m_reconnectIntervals;


            mutable boost::mutex m_heartbeatMutex;

            boost::thread m_trackingThread;
            bool m_doTracking;

            boost::thread m_heartbeatThread;
            bool m_sendHeartbeats;

            boost::thread m_brokerThread;

            std::vector<std::pair<std::string, karabo::util::Hash> > m_availableInstances;

            boost::shared_ptr<karabo::webAuth::Authenticator> m_authenticator;
            int m_defaultAccessLevel;
            karabo::util::Hash m_accessList;
            mutable boost::mutex m_accessLevelMutex;

            // IO channel related
            InputChannels m_inputChannels;
            OutputChannels m_outputChannels;

            // Handlers
            InstanceNotAvailableHandler m_instanceNotAvailableHandler;
            InstanceAvailableAgainHandler m_instanceAvailableAgainHandler;
            SlotCallGuardHandler m_slotCallGuardHandler;
            ExceptionHandler m_exceptionHandler;
            UpdatePerformanceStatisticsHandler m_updatePerformanceStatistics;




        public:

            KARABO_CLASSINFO(SignalSlotable, "SignalSlotable", "1.0")

            /**
             * Slots may be of two different types:
             * SPECIFIC: The slot is unique in the given network
             * GLOBAL: Any signal that is connected with a compatible function signature will trigger this slot
             *
             */
            enum SlotType {
                LOCAL,
                GLOBAL
            };

            enum ConnectionType {
                NO_TRACK,
                TRACK,
                RECONNECT
            };

            #define SIGNAL0(signalName) this->registerSignal(signalName);
            #define SIGNAL1(signalName, a1) this->registerSignal<a1>(signalName);
            #define SIGNAL2(signalName, a1, a2) this->registerSignal<a1,a2>(signalName);
            #define SIGNAL3(signalName, a1, a2, a3) this->registerSignal<a1,a2,a3>(signalName);
            #define SIGNAL4(signalName, a1, a2, a3, a4) this->registerSignal<a1,a2,a3,a4>(signalName);            

            #define KARABO_SIGNAL0(signalName) this->registerSignal(signalName);
            #define KARABO_SIGNAL1(signalName, a1) this->registerSignal<a1>(signalName);
            #define KARABO_SIGNAL2(signalName, a1, a2) this->registerSignal<a1,a2>(signalName);
            #define KARABO_SIGNAL3(signalName, a1, a2, a3) this->registerSignal<a1,a2,a3>(signalName);
            #define KARABO_SIGNAL4(signalName, a1, a2, a3, a4) this->registerSignal<a1,a2,a3,a4>(signalName);

            #define SLOT0(slotName) this->registerSlot(boost::bind(&Self::slotName,this),#slotName);
            #define SLOT1(slotName, a1) this->registerSlot<a1>(boost::bind(&Self::slotName,this,_1),#slotName);
            #define SLOT2(slotName, a1, a2) this->registerSlot<a1,a2>(boost::bind(&Self::slotName,this,_1,_2),#slotName);
            #define SLOT3(slotName, a1, a2, a3) this->registerSlot<a1,a2,a3>(boost::bind(&Self::slotName,this,_1,_2,_3),#slotName);
            #define SLOT4(slotName, a1, a2, a3, a4) this->registerSlot<a1,a2,a3,a4>(boost::bind(&Self::slotName,this,_1,_2,_3,_4),#slotName);

            #define KARABO_SLOT0(slotName) this->registerSlot(boost::bind(&Self::slotName,this),#slotName);
            #define KARABO_SLOT1(slotName, a1) this->registerSlot<a1>(boost::bind(&Self::slotName,this,_1),#slotName);
            #define KARABO_SLOT2(slotName, a1, a2) this->registerSlot<a1,a2>(boost::bind(&Self::slotName,this,_1,_2),#slotName);
            #define KARABO_SLOT3(slotName, a1, a2, a3) this->registerSlot<a1,a2,a3>(boost::bind(&Self::slotName,this,_1,_2,_3),#slotName);
            #define KARABO_SLOT4(slotName, a1, a2, a3, a4) this->registerSlot<a1,a2,a3,a4>(boost::bind(&Self::slotName,this,_1,_2,_3,_4),#slotName);

            #define GLOBAL_SLOT0(slotName) this->registerSlot(boost::bind(&Self::slotName,this),#slotName, GLOBAL);
            #define GLOBAL_SLOT1(slotName, a1) this->registerSlot<a1>(boost::bind(&Self::slotName,this,_1),#slotName, GLOBAL);
            #define GLOBAL_SLOT2(slotName, a1, a2) this->registerSlot<a1,a2>(boost::bind(&Self::slotName,this,_1,_2),#slotName, GLOBAL);
            #define GLOBAL_SLOT3(slotName, a1, a2, a3) this->registerSlot<a1,a2,a3>(boost::bind(&Self::slotName,this,_1,_2,_3),#slotName, GLOBAL);
            #define GLOBAL_SLOT4(slotName, a1, a2, a3, a4) this->registerSlot<a1,a2,a3,a4>(boost::bind(&Self::slotName,this,_1,_2,_3,_4),#slotName, GLOBAL);

            #define KARABO_GLOBAL_SLOT0(slotName) this->registerSlot(boost::bind(&Self::slotName,this),#slotName, GLOBAL);
            #define KARABO_GLOBAL_SLOT1(slotName, a1) this->registerSlot<a1>(boost::bind(&Self::slotName,this,_1),#slotName, GLOBAL);
            #define KARABO_GLOBAL_SLOT2(slotName, a1, a2) this->registerSlot<a1,a2>(boost::bind(&Self::slotName,this,_1,_2),#slotName, GLOBAL);
            #define KARABO_GLOBAL_SLOT3(slotName, a1, a2, a3) this->registerSlot<a1,a2,a3>(boost::bind(&Self::slotName,this,_1,_2,_3),#slotName, GLOBAL);
            #define KARABO_GLOBAL_SLOT4(slotName, a1, a2, a3, a4) this->registerSlot<a1,a2,a3,a4>(boost::bind(&Self::slotName,this,_1,_2,_3,_4),#slotName, GLOBAL);
            
            #define KARABO_ON_DATA(channelName, funcName) this->registerDataHandler(channelName, boost::bind(&Self::funcName,this,_1));
            #define KARABO_ON_EOS(channelName, funcName) this->registerEndOfStreamHandler(channelName, boost::bind(&Self::funcName,this,_1));
            

            #define _KARABO_SIGNAL_N(x0,x1,x2,x3,x4,x5,FUNC, ...) FUNC
            #define _KARABO_SLOT_N(x0,x1,x2,x3,x4,x5,FUNC, ...) FUNC
            #define _KARABO_GLOBAL_SLOT_N(x0,x1,x2,x3,x4,x5,FUNC, ...) FUNC

            #define KARABO_SIGNAL(...) \
_KARABO_SIGNAL_N(,##__VA_ARGS__, \
KARABO_SIGNAL4(__VA_ARGS__), \
KARABO_SIGNAL3(__VA_ARGS__), \
KARABO_SIGNAL2(__VA_ARGS__), \
KARABO_SIGNAL1(__VA_ARGS__), \
KARABO_SIGNAL0(__VA_ARGS__) \
)

            #define KARABO_SLOT(...) \
_KARABO_SLOT_N(,##__VA_ARGS__, \
KARABO_SLOT4(__VA_ARGS__), \
KARABO_SLOT3(__VA_ARGS__), \
KARABO_SLOT2(__VA_ARGS__), \
KARABO_SLOT1(__VA_ARGS__), \
KARABO_SLOT0(__VA_ARGS__) \
)

            #define KARABO_GLOBAL_SLOT(...) \
_KARABO_GLOBAL_SLOT_N(,##__VA_ARGS__, \
KARABO_GLOBAL_SLOT4(__VA_ARGS__), \
KARABO_GLOBAL_SLOT3(__VA_ARGS__), \
KARABO_GLOBAL_SLOT2(__VA_ARGS__), \
KARABO_GLOBAL_SLOT1(__VA_ARGS__), \
KARABO_GLOBAL_SLOT0(__VA_ARGS__) \
)

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
                    const karabo::net::BrokerConnection::Pointer& connection);

            /**
             * Creates a function SignalSlotable object allowing to configure the broker connection.
             * Don't call init() afterwards.
             * @param instanceId The future instanceId of this object in the distributed system
             * @param brokerType The broker type (currently only JMS available)
             * @param brokerConfiguration The sub-configuration for the respective broker type
             */
            SignalSlotable(const std::string& instanceId,
                    const std::string& brokerType = "Jms",
                    const karabo::util::Hash& brokerConfiguration = karabo::util::Hash());

            virtual ~SignalSlotable();

            /**
             * Initialized the SignalSlotable object (only use in conjunction with empty constructor)
             * @param instanceId The future instanceId of this object in the distributed system
             * @param connection An existing broker connection
             */
            void init(const std::string& instanceId,
                    const karabo::net::BrokerConnection::Pointer& connection);

            /**
             * Login in order to receive correct access rights
             * @param username Username 
             * @param password Password
             * @param provider Provider (currently only LOCAL and KERBEROS possible)
             * @return bool indicating success of failure
             */
            bool login(const std::string& username, const std::string& password, const std::string& provider);

            /**
             * Logout
             * @return bool indicating success or failure
             */
            bool logout();

            /**
             * Retrieves currently logged in username (empty if not logged in)
             * @return string username
             */
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

            // TODO make this functions private and allow SLOT to access them
            void setSenderInfo(const karabo::util::Hash& senderInfo);

            // TODO make this functions private and allow SLOT to access them
            const karabo::util::Hash& getSenderInfo() const;

            /**
             * Access to the identification of the current instance using signals and slots
             * @return instanceId
             */
            const std::string& getInstanceId() const;

            void updateInstanceInfo(const karabo::util::Hash& update);

            const karabo::util::Hash& getInstanceInfo() const;

            void trackExistenceOfInstance(const std::string& instanceId);

            void stopTrackingExistenceOfInstance(const std::string& instanceId);

            void registerInstanceNotAvailableHandler(const InstanceNotAvailableHandler& instanceNotAvailableCallback);

            void registerInstanceAvailableAgainHandler(const InstanceAvailableAgainHandler& instanceAvailableAgainCallback);

            void registerExceptionHandler(const ExceptionHandler& exceptionFoundCallback);

            //void registerInstanceNewHandler(const InstanceNewHandler& instanceNewCallback);

            void registerSlotCallGuardHandler(const SlotCallGuardHandler& slotCallGuardHandler);

            void registerPerformanceStatisticsHandler(const UpdatePerformanceStatisticsHandler& updatePerformanceStatisticsHandler);

            karabo::net::BrokerConnection::Pointer getConnection() const;

            const std::vector<std::pair<std::string, karabo::util::Hash> >& getAvailableInstances(const bool activateTracking = false);

            std::vector<std::string> getAvailableSignals(const std::string& instanceId);

            std::vector<std::string> getAvailableSlots(const std::string& instanceId);

            /**
             * This function must only be called within a slotFunctions body. It returns the current object handling
             * the callback which provides more information on the sender.
             * @param slotFunction The string-ified name of the slotFunction you are currently in
             * @return instance of a Slot object (handler object for this callback)
             */
            const SlotInstancePointer& getSenderInfo(const std::string& slotFunction);

            /**
             * Connects a signal and slot by explicitely seperating instanceId from the slotId/signalId.
             * @param signalInstanceId
             * @param signalSignature
             * @param slotInstanceId
             * @param slotSignature
             */
            bool connect(std::string signalInstanceId, const std::string& signalSignature, std::string slotInstanceId, const std::string& slotSignature, ConnectionType connectionType = TRACK, const bool isVerbose = false);

            /**
             * This function establishes a connection between a signal and a slot.
             * If the instanceId is not given, the signal/slot is interpreted as local and automatically
             * assigned a "self" instanceId
             */
            bool connect(const std::string& signal, const std::string& slot, ConnectionType connectionType = TRACK, const bool isVerbose = false);

            /**
             * This function establishes a connection between a signal and a slot.
             * The real functionality of the connection is NEITHER checked initially, NOR tracked during its lifetime.
             * Thus, an emitted signal may never reach the connected slot, still not triggering any error.
             *
             * @param signalInstanceId
             * @param signalSignature
             * @param slotInstanceId
             * @param slotSignature
             */
            KARABO_DEPRECATED void connectN(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string& slotSignature);

            /**
             * This function establishes a connection between a signal and a slot.
             * The real functionality of the connection will be checked initially and be tracked throughout its lifetime.
             * The virtual function connectionNotAvailable(const string& instanceId, const Hash& affectedSignals, const Hash& affectedSlots) will be called
             * in case the connection is/gets unavaible. NOTE: This notification is delayed by at least the heartbeat-rate of the connected component.
             *
             * @param signalInstanceId
             * @param signalSignature
             * @param slotInstanceId
             * @param slotSignature
             */
            KARABO_DEPRECATED void connectT(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string& slotSignature);

            /**
             * This function establishes a connection between a signal and a slot.
             * The real functionality of the connection will be checked initially and be tracked throughout its lifetime.
             * The virtual function connectionNotAvailable(const string& instanceId, const Hash& affectedSignals, const Hash& affectedSlots) will be called
             * in case the connection is/gets unavaible. NOTE: This notification is delayed by at least the heartbeat-rate of the connected component.
             * Furthermore will the lost connection automatically be tried to be re-established once the lost component(s) get(s) available again.
             * The re-connection requests will fade out with the power of 4 and last maximum 17 hours. After that and at any other time all dangling connections 
             * can actively tried to be re-connected by calling the function "slotTryReconnectNow()".
             *
             * @param signalInstanceId
             * @param signalSignature
             * @param slotInstanceId
             * @param slotSignature
             */
            KARABO_DEPRECATED void connectR(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string& slotSignature);

            /**
             * This function establishes a connection between a signal and a slot.
             * The real functionality of the connection is NEITHER checked initially, NOR tracked during its lifetime.
             * Thus, an emitted signal may never reach the connected slot, still not triggering any error.
             *
             * @param signalId
             * @param slotId
             */
            KARABO_DEPRECATED void connectN(const std::string& signalId, const std::string& slotId);

            /**
             * This function establishes a connection between a signal and a slot.
             * The real functionality of the connection will be checked initially and be tracked throughout its lifetime.
             * The virtual function "connectionNotAvailable(const string& instanceId, const Hash& affectedSignals, const Hash& affectedSlots)" will be called
             * in case the connection is/gets unavaible. NOTE: This notification is delayed by at least the heartbeat-rate of the connected component.
             *
             * @param signalId
             * @param slotId
             */
            KARABO_DEPRECATED void connectT(const std::string& signalId, const std::string& slotId);

            /**
             * This function establishes a connection between a signal and a slot.
             * The real functionality of the connection will be checked initially and be tracked throughout its lifetime.
             * The virtual function connectionNotAvailable(const string& instanceId, const Hash& affectedSignals, const Hash& affectedSlots) will be called
             * in case the connection is/gets unavailable. NOTE: This notification is delayed by at least the heartbeat-rate of the connected component.
             * Furthermore will the lost connection automatically be tried to be re-established once the lost component(s) get(s) available again.
             * The re-connection requests will fade out with the power of 4 and last maximum 17 hours. After that and at any other time all dangling connections
             * can actively tried to be re-connected by calling the function "slotTryReconnectNow()".
             *
             * @param signalId
             * @param slotId
             */
            KARABO_DEPRECATED void connectR(const std::string& signalId, const std::string& slotId);



            bool disconnect(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, const bool isVerbose = false);         

            /**
             * Emits a void signal.
             * @param signalFunction
             */
            void emit(const std::string& signalFunction) const {
                boost::function<void () > emitFunction;
                m_emitFunctions.get(signalFunction, emitFunction);
                emitFunction();
            }

            void call(std::string instanceId, const std::string& functionName) const {
                Signal s(this, m_producerChannel, m_instanceId, "__call__");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emit0();
            }

            SignalSlotable::Requestor request(std::string instanceId, const std::string& functionName) {
                if (instanceId.empty()) instanceId = m_instanceId;
                return SignalSlotable::Requestor(this).request(instanceId, functionName);
            }

            SignalSlotable::Requestor requestNoWait(std::string requestInstanceId, const std::string& requestFunctionName,
                    std::string replyInstanceId, const std::string& replyFunctionName) {
                if (requestInstanceId.empty()) requestInstanceId = m_instanceId;
                if (replyInstanceId.empty()) replyInstanceId = m_instanceId;
                return SignalSlotable::Requestor(this).requestNoWait(requestInstanceId, requestFunctionName, replyInstanceId, replyFunctionName);
            }

            /**
             * Emits a signal with one argument.
             * @param signalFunction
             * @param a1
             */
            template <class A1>
            void emit(const std::string& signalFunction, const A1& a1) const {
                boost::function<void (const A1&) > emitFunction;
                retrieveEmitFunction(signalFunction, emitFunction);
                try {
                    emitFunction(a1);
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Problems whilst emitting from a valid signal, please ask BH"))
                }
            }

            void emit(const std::string& signalFunction, const char* const& a1) const {
                emit(signalFunction, std::string(a1));
            }

            template <class A1>
            void call(std::string instanceId, const std::string& functionName, const A1& a1) const {
                Signal s(this, m_producerChannel, m_instanceId, "__call__");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emit1(a1);
            }

            template <class A1>
            SignalSlotable::Requestor request(std::string instanceId, const std::string& functionName, const A1& a1) {
                if (instanceId.empty()) instanceId = m_instanceId;
                return SignalSlotable::Requestor(this).request(instanceId, functionName, a1);
            }

            template <class A1>
            SignalSlotable::Requestor requestNoWait(std::string requestInstanceId, const std::string& requestFunctionName,
                    std::string replyInstanceId, const std::string& replyFunctionName, const A1& a1) {
                if (requestInstanceId.empty()) requestInstanceId = m_instanceId;
                if (replyInstanceId.empty()) replyInstanceId = m_instanceId;
                return SignalSlotable::Requestor(this).requestNoWait(requestInstanceId, requestFunctionName, replyInstanceId, replyFunctionName, a1);
            }

            /**
             * Emits a signal with two arguments.
             * @param signalFunction
             * @param a1
             * @param a2
             */
            template <class A1, class A2>
            void emit(const std::string& signalFunction, const A1& a1, const A2& a2) const {
                try {
                    boost::function<void (const A1&, const A2&) > emitFunction;
                    m_emitFunctions.get(signalFunction, emitFunction);
                    emitFunction(a1, a2);
                } catch (...) {
                    KARABO_RETHROW;
                }
            }

            template <class A1>
            void emit(const std::string& signalFunction, const A1& a1, const char* const& a2) const {
                emit(signalFunction, a1, std::string(a2));
            }

            template <class A2>
            void emit(const std::string& signalFunction, const char* const& a1, const A2& a2) const {
                emit(signalFunction, std::string(a1), a2);
            }

            void emit(const std::string& signalFunction, const char* const& a1, const char* const& a2) const {
                emit(signalFunction, std::string(a1), std::string(a2));
            }

            template <class A1, class A2>
            void call(std::string instanceId, const std::string& functionName, const A1& a1, const A2& a2) const {
                Signal s(this, m_producerChannel, m_instanceId, "__call__");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emit2(a1, a2);
            }

            template <class A1, class A2>
            SignalSlotable::Requestor request(std::string instanceId, const std::string& functionName, const A1& a1, const A2& a2) {
                if (instanceId.empty()) instanceId = m_instanceId;
                return SignalSlotable::Requestor(this).request(instanceId, functionName, a1, a2);
            }

            template <class A1, class A2>
            SignalSlotable::Requestor requestNoWait(std::string requestInstanceId, const std::string& requestFunctionName,
                    std::string replyInstanceId, const std::string& replyFunctionName, const A1& a1, const A2& a2) {
                if (requestInstanceId.empty()) requestInstanceId = m_instanceId;
                if (replyInstanceId.empty()) replyInstanceId = m_instanceId;
                return SignalSlotable::Requestor(this).requestNoWait(requestInstanceId, requestFunctionName, replyInstanceId, replyFunctionName, a1, a2);
            }

            /**
             * Emits a signal with three arguments.
             * @param signalFunction
             * @param a1
             * @param a2
             * @param a3
             */
            template <class A1, class A2, class A3>
            void emit(const std::string& signalFunction, const A1& a1, const A2& a2, const A3& a3) const {
                try {
                    boost::function<void (const A1&, const A2&, const A3&) > emitFunction;
                    m_emitFunctions.get(signalFunction, emitFunction);
                    emitFunction(a1, a2, a3);
                } catch (...) {
                    KARABO_RETHROW;
                }
            }

            template <class A1, class A2, class A3>
            void call(std::string instanceId, const std::string& functionName, const A1& a1, const A2& a2, const A3& a3) const {
                Signal s(this, m_producerChannel, m_instanceId, "__call__");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emit3(a1, a2, a3);
            }

            template <class A1, class A2, class A3>
            SignalSlotable::Requestor request(std::string instanceId, const std::string& functionName, const A1& a1, const A2& a2, const A3& a3) {
                if (instanceId.empty()) instanceId = m_instanceId;
                return SignalSlotable::Requestor(this).request(instanceId, functionName, a1, a2, a3);
            }

            template <class A1, class A2, class A3>
            SignalSlotable::Requestor requestNoWait(std::string requestInstanceId, const std::string& requestFunctionName,
                    std::string replyInstanceId, const std::string& replyFunctionName, const A1& a1, const A2& a2, const A3& a3) {
                if (requestInstanceId.empty()) requestInstanceId = m_instanceId;
                if (replyInstanceId.empty()) replyInstanceId = m_instanceId;
                return SignalSlotable::Requestor(this).requestNoWait(requestInstanceId, requestFunctionName, replyInstanceId, replyFunctionName, a1, a2, a3);
            }

            /**
             * Emits a signal with four arguments.
             * @param signalFunction
             * @param a1
             * @param a2
             * @param a3
             * @param a4
             */
            template <class A1, class A2, class A3, class A4>
            void emit(const std::string& signalFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) const {
                try {
                    boost::function<void (const A1&, const A2&, const A3&, const A4&) > emitFunction;
                    m_emitFunctions.get(signalFunction, emitFunction);
                    emitFunction(a1, a2, a3, a4);
                } catch (...) {
                    KARABO_RETHROW;
                }
            }

            template <class A1, class A2, class A3, class A4>
            void call(std::string instanceId, const std::string& functionName, const A1& a1, const A2& a2, const A3& a3, const A4& a4) const {
                Signal s(this, m_producerChannel, m_instanceId, "__call__");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emit4(a1, a2, a3, a4);
            }

            template <class A1, class A2, class A3, class A4>
            SignalSlotable::Requestor request(std::string instanceId, const std::string& functionName, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                if (instanceId.empty()) instanceId = m_instanceId;
                return SignalSlotable::Requestor(this).request(instanceId, functionName, a1, a2, a3, a4);
            }

            template <class A1, class A2, class A3, class A4>
            SignalSlotable::Requestor requestNoWait(std::string requestInstanceId, const std::string& requestFunctionName,
                    std::string replyInstanceId, const std::string& replyFunctionName, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                if (requestInstanceId.empty()) requestInstanceId = m_instanceId;
                if (replyInstanceId.empty()) replyInstanceId = m_instanceId;
                return SignalSlotable::Requestor(this).requestNoWait(requestInstanceId, requestFunctionName, replyInstanceId, replyFunctionName, a1, a2, a3, a4);
            }

            void reply() {
                registerReply(karabo::util::Hash());
            }

            template <class A1>
            void reply(const A1& a1) {
                registerReply(karabo::util::Hash("a1", a1));
            }

            template <class A1, class A2>
            void reply(const A1& a1, const A2& a2) {
                registerReply(karabo::util::Hash("a1", a1, "a2", a2));
            }

            template <class A1, class A2, class A3>
            void reply(const A1& a1, const A2& a2, const A3& a3) {
                registerReply(karabo::util::Hash("a1", a1, "a2", a2, "a3", a3));
            }

            template <class A1, class A2, class A3, class A4>
            void reply(const A1& a1, const A2& a2, const A3& a3, A4& a4) {
                registerReply(karabo::util::Hash("a1", a1, "a2", a2, "a3", a3, "a4", a4));
            }

            void registerSignal(const std::string& funcName) {
                if (m_signalInstances.find(funcName) != m_signalInstances.end()) return; // Already registered
                boost::shared_ptr<karabo::xms::Signal> s(new karabo::xms::Signal(this, m_producerChannel, m_instanceId, funcName));
                boost::function<void() > f(boost::bind(&karabo::xms::Signal::emit0, s));
                storeSignal(funcName, s, f);
            }

            template <class A1>
            void registerSignal(const std::string& funcName) {
                if (m_signalInstances.find(funcName) != m_signalInstances.end()) return;
                boost::shared_ptr<karabo::xms::Signal> s(new karabo::xms::Signal(this, m_producerChannel, m_instanceId, funcName));
                boost::function<void (const A1&) > f(boost::bind(&karabo::xms::Signal::emit1<A1>, s, _1));
                storeSignal(funcName, s, f);
            }

            template <class A1, class A2>
            void registerSignal(const std::string& funcName) {
                if (m_signalInstances.find(funcName) != m_signalInstances.end()) return;
                boost::shared_ptr<karabo::xms::Signal> s(new karabo::xms::Signal(this, m_producerChannel, m_instanceId, funcName));
                boost::function<void (const A1&, const A2&) > f(boost::bind(&karabo::xms::Signal::emit2<A1, A2>, s, _1, _2));
                storeSignal(funcName, s, f);
            }

            template <class A1, class A2, class A3>
            void registerSignal(const std::string& funcName) {
                if (m_signalInstances.find(funcName) != m_signalInstances.end()) return;
                boost::shared_ptr<karabo::xms::Signal> s(new karabo::xms::Signal(this, m_producerChannel, m_instanceId, funcName));
                boost::function<void (const A1&, const A2&, const A3&) > f(boost::bind(&karabo::xms::Signal::emit3<A1, A2, A3>, s, _1, _2, _3));
                storeSignal(funcName, s, f);
            }

            template <class A1, class A2, class A3, class A4>
            void registerSignal(const std::string& funcName) {
                if (m_signalInstances.find(funcName) != m_signalInstances.end()) return;
                boost::shared_ptr<karabo::xms::Signal> s(new karabo::xms::Signal(this, m_producerChannel, m_instanceId, funcName));
                boost::function<void (const A1&, const A2&, const A3&, const A4&) > f(boost::bind(&karabo::xms::Signal::emit4<A1, A2, A3, A4>, s, _1, _2, _3, _4));
                storeSignal(funcName, s, f);
            }

            template <class A1, class A2, class A3>
            void registerHeartbeatSignal(const std::string& funcName) {
                if (m_signalInstances.find(funcName) != m_signalInstances.end()) return;
                boost::shared_ptr<karabo::xms::Signal> s(new karabo::xms::Signal(this, m_heartbeatProducerChannel, m_instanceId, funcName, 9));
                boost::function<void (const A1&, const A2&, const A3&) > f(boost::bind(&karabo::xms::Signal::emit3<A1, A2, A3>, s, _1, _2, _3));
                storeSignal(funcName, s, f);
            }

            SlotInstances* getSlotInstancesPtr(const SlotType& slotType) {
                if (slotType == LOCAL) return &(m_localSlotInstances);
                else return &(m_globalSlotInstances);
            }

            void registerSlot(const boost::function<void () >& slot, const std::string& funcName, const SlotType& slotType = LOCAL) {

                SlotInstances* slotInstances = getSlotInstancesPtr(slotType);
                SlotInstances::const_iterator it = slotInstances->find(funcName);
                if (it != slotInstances->end()) { // Already registered, append another callback
                    (boost::static_pointer_cast<karabo::xms::Slot0 >(it->second))->registerSlotFunction(slot);
                } else { // New local slot
                    boost::shared_ptr<karabo::xms::Slot0> s(new karabo::xms::Slot0(funcName));
                    // Bind user's slot-function to Slot
                    s->registerSlotFunction(slot);
                    // Book-keep slot
                    (*slotInstances)[funcName] = s;
                }
            }

            template <class A1>
            void registerSlot(const boost::function<void (const A1&) >& slot, const std::string& funcName, const SlotType& slotType = LOCAL) {
                SlotInstances* slotInstances = getSlotInstancesPtr(slotType);
                SlotInstances::const_iterator it = slotInstances->find(funcName);
                if (it != slotInstances->end()) {
                    (boost::static_pointer_cast<karabo::xms::Slot1<A1> >(it->second))->registerSlotFunction(slot);
                } else {
                    boost::shared_ptr<karabo::xms::Slot1<A1> > s(new karabo::xms::Slot1<A1 > (funcName));
                    s->registerSlotFunction(slot);
                    (*slotInstances)[funcName] = s;

                }
            }

            template <class A1, class A2>
            void registerSlot(const boost::function<void (const A1&, const A2&) >& slot, const std::string& funcName, const SlotType& slotType = LOCAL) {
                SlotInstances* slotInstances = getSlotInstancesPtr(slotType);
                SlotInstances::const_iterator it = slotInstances->find(funcName);
                if (it != slotInstances->end()) {
                    (boost::static_pointer_cast<karabo::xms::Slot2<A1, A2> >(it->second))->registerSlotFunction(slot);
                } else {
                    boost::shared_ptr<karabo::xms::Slot2<A1, A2> > s(new karabo::xms::Slot2<A1, A2 > (funcName));
                    s->registerSlotFunction(slot);
                    (*slotInstances)[funcName] = s;
                }
            }

            template <class A1, class A2, class A3>
            void registerSlot(const boost::function<void (const A1&, const A2&, const A3&) >& slot, const std::string& funcName, const SlotType& slotType = LOCAL) {
                SlotInstances* slotInstances = getSlotInstancesPtr(slotType);
                SlotInstances::const_iterator it = slotInstances->find(funcName);
                if (it != slotInstances->end()) {
                    (boost::static_pointer_cast<karabo::xms::Slot3<A1, A2, A3> >(it->second))->registerSlotFunction(slot);
                } else {
                    boost::shared_ptr<karabo::xms::Slot3<A1, A2, A3> > s(new karabo::xms::Slot3<A1, A2, A3 > (funcName));
                    s->registerSlotFunction(slot);
                    (*slotInstances)[funcName] = s;

                }
            }

            template <class A1, class A2, class A3, class A4>
            void registerSlot(const boost::function<void (const A1&, const A2&, const A3&, const A4&) >& slot, const std::string& funcName, const SlotType& slotType = LOCAL) {
                SlotInstances* slotInstances = getSlotInstancesPtr(slotType);
                SlotInstances::const_iterator it = slotInstances->find(funcName);
                if (it != slotInstances->end()) {
                    (boost::static_pointer_cast<karabo::xms::Slot4<A1, A2, A3, A4> >(it->second))->registerSlotFunction(slot);
                } else {
                    boost::shared_ptr<karabo::xms::Slot4<A1, A2, A3, A4> > s(new karabo::xms::Slot4<A1, A2, A3, A4 > (funcName));
                    s->registerSlotFunction(slot);
                    (*slotInstances)[funcName] = s;
                }
            }

            template <class T>
            boost::shared_ptr<karabo::io::Input<T> > registerInputChannel(const std::string& name, const std::string& type, const karabo::util::Hash& config = karabo::util::Hash(),
                    const boost::function<void (const typename karabo::io::Input<T>::Pointer&) >& onInputAvailableHandler = boost::function<void (const typename karabo::io::Input<T>::Pointer&) >(),
                    const boost::function<void ()>& onEndOfStreamEventHandler = boost::function<void ()>()) {
                using namespace karabo::util;
                karabo::io::AbstractInput::Pointer channel = karabo::io::Input<T>::create(type, config);
                channel->setInstanceId(m_instanceId);
                channel->setInputHandlerType("c++", std::string(typeid (typename karabo::io::Input<T>).name()));
                if (!onInputAvailableHandler.empty()) {
                    channel->registerIOEventHandler(onInputAvailableHandler);
                }
                if (!onEndOfStreamEventHandler.empty()) {
                    channel->registerEndOfStreamEventHandler(onEndOfStreamEventHandler);
                }
                m_inputChannels[name] = channel;
                return boost::static_pointer_cast<karabo::io::Input<T> >(channel);
            }

            //            template <class T>
            //            boost::shared_ptr<karabo::io::Output<T> > registerOutputChannel(const std::string& name, const std::string& type, const karabo::util::Hash& config) {
            //                using namespace karabo::util;
            //                karabo::io::AbstractOutput::Pointer channel = karabo::io::Output<T>::create(type, config);
            //                channel->setInstanceId(m_instanceId);
            //                channel->setOutputHandlerType("c++");
            //                m_outputChannels[name] = channel;
            //                return boost::static_pointer_cast<karabo::io::Output<T> >(channel);
            //            }

            bool connectChannels(std::string outputInstanceId, const std::string& outputName, std::string inputInstanceId, const std::string& inputName, const bool isVerbose = false);

            bool disconnectChannels(std::string outputInstanceId, const std::string& outputName, std::string inputInstanceId, const std::string& inputName, const bool isVerbose = false);

//            template <class InputType>
//            boost::shared_ptr<InputType > createInputChannel(const std::string& name, const karabo::util::Hash input,
//                    const boost::function<void (const typename InputType::Pointer&) >& onInputAvailableHandler = boost::function<void (const typename InputType::Pointer&) >(),
//                    const boost::function<void ()>& onEndOfStreamEventHandler = boost::function<void ()>()) {
//                using namespace karabo::util;
//                karabo::io::AbstractInput::Pointer channel = InputType::createNode(name, "Network", input);
//                channel->setInstanceId(m_instanceId);
//                channel->setInputHandlerType("c++", std::string(typeid (InputType).name()));
//                if (!onInputAvailableHandler.empty()) {
//                    channel->registerIOEventHandler(onInputAvailableHandler);
//                }
//                if (!onEndOfStreamEventHandler.empty()) {
//                    channel->registerEndOfStreamEventHandler(onEndOfStreamEventHandler);
//                }
//                m_inputChannels[name] = channel;
//                return boost::static_pointer_cast<InputType >(channel);
//            }
            
            virtual InputChannel::Pointer createInputChannel(const std::string& channelName, const karabo::util::Hash& config, 
            const boost::function<void (const InputChannel::Pointer&) >& onInputAvailableHandler = boost::function<void (const InputChannel::Pointer&) >(),
            const boost::function<void (const InputChannel::Pointer&)>& onEndOfStreamEventHandler = boost::function<void (const InputChannel::Pointer&)>());

            virtual OutputChannel::Pointer createOutputChannel(const std::string& channelName, const karabo::util::Hash& config, const boost::function<void (const OutputChannel::Pointer&) >& onOutputPossibleHandler = boost::function<void (const OutputChannel::Pointer&) >());
            
            //TODO: make this function private: it is for internal use
            const InputChannels& getInputChannels() const;

            //TODO: make this function private: it is for internal use
            const OutputChannels& getOutputChannels() const;
            
            const OutputChannel::Pointer& getOutputChannel(const std::string& name);
                        
            const InputChannel::Pointer& getInputChannel(const std::string& name);
            
            void registerDataHandler(const std::string& channelName, const boost::function<void (const karabo::xms::InputChannel::Pointer&) >& handler);
            
            void registerEndOfStreamHandler(const std::string& channelName, const boost::function<void (const karabo::xms::InputChannel::Pointer&) >& handler);
            
            void connectInputChannel(const InputChannel::Pointer& channel, int trails = 8, int sleep = 1);

            void connectInputChannels();

            std::pair<bool, std::string> exists(const std::string& instanceId);

            int getAccessLevel(const std::string& instanceId) const;

            template <class T>
            static std::string generateInstanceId() {
                std::string hostname(boost::asio::ip::host_name());
                std::vector<std::string> tokens;
                boost::split(tokens, hostname, boost::is_any_of("."));
                return std::string(tokens[0] + "_" + T::classInfo().getClassId() + "_" + karabo::util::toString(getpid()));
            }

        protected: // Functions

            void startEmittingHeartbeats(const int heartbeatInterval);

            void stopEmittingHearbeats();

            void startBrokerMessageConsumption();

            void stopBrokerMessageConsumption();

            void injectEvent(karabo::net::BrokerChannel::Pointer /*channel*/, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);

            void handleReply(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);

            void injectHeartbeat(karabo::net::BrokerChannel::Pointer /*channel*/, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);

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

            std::pair<std::string, std::string> splitIntoInstanceIdAndFunctionName(const std::string& signalOrSlotId, const char sep = ':') const;

            std::string prepareInstanceId(const SlotType& slotType) const {
                if (slotType == LOCAL) return m_instanceId;
                else return "*";
            }

            template <class TFunc>
            void storeSignal(const std::string& signalFunction, const SignalInstancePointer& signalInstance, const TFunc& emitFunction) {
                m_signalInstances[signalFunction] = signalInstance;
                m_emitFunctions.set(signalFunction, emitFunction);
            }

            template <class TFunc>
            void retrieveEmitFunction(const std::string& signalFunction, TFunc& emitFunction) const {
                try {
                    emitFunction = m_emitFunctions.get<TFunc > (signalFunction);
                } catch (const karabo::util::CastException& e) {
                    throw KARABO_SIGNALSLOT_EXCEPTION("Argument mismatch: The requested signal \"" + signalFunction + "\" was registered with a different number of arguments before.");
                } catch (const karabo::util::ParameterException& e) {
                    throw KARABO_SIGNALSLOT_EXCEPTION("The requested signal \"" + signalFunction + "\" could not be found. Ensure proper registration of the signal before you call emit.");
                }
            }

            void registerReply(const karabo::util::Hash& reply);

        private: // Functions

            void _runEventLoop();

            void _runHeartbeatLoop();

            void sanifyInstanceId(std::string& instanceId) const;

            std::pair<bool, std::string> isValidInstanceId(const std::string& instanceId);

            void slotInstanceNew(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void slotPing(const std::string& instanceId, int rand, bool trackPingedInstance);

            void slotPingAnswer(const std::string& instanceId, const karabo::util::Hash& hash);

            void sendPotentialReply(const karabo::util::Hash& header, bool global);

            void sendErrorHappenedReply(const karabo::util::Hash& header, const std::string& errorMesssage);

            void emitHeartbeat();

            void registerDefaultSignalsAndSlots();

            void startTrackingSystem();

            void stopTrackingSystem();

            bool tryToConnectToSignal(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, const int& connectionType, const bool isVerbose);

            void slotConnectToSignal(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction, const int& connectionType);

            bool tryToConnectToSlot(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction, const int& connectionType, const bool isVerbose);

            void slotConnectToSlot(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotFunction, const int& connectionType);

            bool tryToDisconnectFromSignal(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, const bool isVerbose);

            void slotDisconnectFromSignal(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction);

            bool tryToDisconnectFromSlot(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, const bool isVerbose);

            void slotDisconnectFromSlot(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotFunction);

            static std::string prepareFunctionSignature(const std::string& funcName) {
                std::string f(boost::trim_copy(funcName));
                return f.substr(0, f.find_first_of('-')) + "|";
            }

            bool slotConnectToOutputChannel(const std::string& inputName, const karabo::util::Hash& outputChannelInfo, bool connect);

            void slotHeartbeat(const std::string& networkId, const int& heartbeatInterval, const karabo::util::Hash& instanceInfo);

            void refreshTimeToLiveForConnectedSlot(const std::string& networkId, int heartbeatInterval, const karabo::util::Hash& instanceInfo);

            void letConnectionSlowlyDieWithoutHeartbeat();

            void registerConnectionForTracking(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction, const int& connectionType);

            void unregisterConnectionFromTracking(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction);

            bool isConnectionTracked(const std::string& connectionId);

            void addTrackedComponent(const std::string& instanceId, const bool isExplicitlyTracked = false);

            void updateTrackedComponentInstanceInfo(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void addTrackedComponentConnection(const std::string& instanceId, const karabo::util::Hash& connection);

            karabo::util::Hash prepareConnectionNotAvailableInformation(const karabo::util::Hash& signals) const;

            void slotGetAvailableFunctions(const std::string& type);

            void cleanSignalsAndStopTracking(const std::string& instanceId);

            // IO channel related
            karabo::util::Hash slotGetOutputChannelInformation(const std::string& ioChannelId, const int& processId);

            static int godEncode(const std::string& password);

            // Thread-safe, locks m_signalSlotInstancesMutex
            bool hasLocalSlot(const std::string& slotFunction) const;

            // Thread-safe, locks m_signalSlotInstancesMutex
            SlotInstancePointer getLocalSlot(const std::string& slotFunction) const;

            // Thread-safe, locks m_signalSlotInstancesMutex
            void removeLocalSlot(const std::string& slotFunction);

            // Thread-safe, locks m_signalSlotInstancesMutex
            SlotInstancePointer getGlobalSlot(const std::string& slotFunction) const;

            // Thread-safe, locks m_signalSlotInstancesMutex
            bool hasSignal(const std::string& signalFunction) const;

            // Thread-safe, locks m_signalSlotInstancesMutex
            bool tryToUnregisterSlot(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction);

            bool hasReceivedReply(const std::string& replyFromValue) const;

            void popReceivedReply(const std::string& replyFromValue, karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body);

            bool timedWaitAndPopReceivedReply(const std::string& replyId, karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body, int timeout);

            long long getEpochMillis() {
                using namespace boost::gregorian;
                using namespace boost::local_time;
                using namespace boost::posix_time;
                
                ptime epochTime(date(1970,1,1));
                ptime nowTime = microsec_clock::local_time();
                time_duration difference = nowTime - epochTime;
                return difference.total_milliseconds();
            }
        };
    }
}

#endif
