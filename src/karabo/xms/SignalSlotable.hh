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
#include <karabo/net/PointToPoint.hh>

#include "OutputChannel.hh"
#include "InputChannel.hh"

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

            friend class Signal;

        protected:

            class Caller {


                const SignalSlotable* m_signalSlotable;

            public:

                explicit Caller(const SignalSlotable* signalSlotable);

                void call(const std::string& slotInstanceId, const std::string& slotFunction) const {
                    sendMessage(slotInstanceId, slotFunction,
                                karabo::util::Hash::Pointer(new karabo::util::Hash));
                }

                template <class A1>
                void call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1) const {
                    sendMessage(slotInstanceId, slotFunction,
                                karabo::util::Hash::Pointer(new karabo::util::Hash("a1", a1)));
                }

                template <class A1, class A2>
                void call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2) const {
                    sendMessage(slotInstanceId, slotFunction,
                                karabo::util::Hash::Pointer(new karabo::util::Hash("a1", a1, "a2", a2)));
                }

                template <class A1, class A2, class A3>
                void call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3) const {
                    sendMessage(slotInstanceId, slotFunction,
                                karabo::util::Hash::Pointer(new karabo::util::Hash("a1", a1, "a2", a2, "a3", a3)));

                }

                template <class A1, class A2, class A3, class A4>
                void call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) const {
                    sendMessage(slotInstanceId, slotFunction,
                                karabo::util::Hash::Pointer(new karabo::util::Hash("a1", a1, "a2", a2, "a3", a3, "a4", a4)));
                }

                void sendMessage(const std::string& slotInstanceId, const std::string& slotFunction, const karabo::util::Hash::Pointer& body) const;

                karabo::util::Hash::Pointer prepareHeader(const std::string& slotInstanceId, const std::string& slotFunction) const;


            };

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
                    karabo::util::Hash::Pointer header = prepareHeader(slotInstanceId, slotFunction);
                    karabo::util::Hash::Pointer body(new karabo::util::Hash());
                    sendRequest(slotInstanceId, header, body);
                    return *this;
                }

                template <class A1>
                Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1) {
                    karabo::util::Hash::Pointer header = prepareHeader(slotInstanceId, slotFunction);
                    karabo::util::Hash::Pointer body(new karabo::util::Hash("a1", a1));
                    sendRequest(slotInstanceId, header, body);
                    return *this;
                }

                template <class A1, class A2>
                Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2) {
                    karabo::util::Hash::Pointer header = prepareHeader(slotInstanceId, slotFunction);
                    karabo::util::Hash::Pointer body(new karabo::util::Hash("a1", a1, "a2", a2));
                    sendRequest(slotInstanceId, header, body);
                    return *this;
                }

                template <class A1, class A2, class A3>
                Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3) {
                    karabo::util::Hash::Pointer header = prepareHeader(slotInstanceId, slotFunction);
                    karabo::util::Hash::Pointer body(new karabo::util::Hash("a1", a1, "a2", a2, "a3", a3));
                    sendRequest(slotInstanceId, header, body);
                    return *this;
                }

                template <class A1, class A2, class A3, class A4>
                Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                    karabo::util::Hash::Pointer header = prepareHeader(slotInstanceId, slotFunction);
                    karabo::util::Hash::Pointer body(new karabo::util::Hash("a1", a1, "a2", a2, "a3", a3, "a4", a4));
                    sendRequest(slotInstanceId, header, body);
                    return *this;
                }

                // TODO: Add another API layer, which is called execute and does synchronous slot execution

                Requestor& requestNoWait(
                                         const std::string& requestSlotInstanceId,
                                         const std::string& requestSlotFunction,
                                         const std::string replySlotInstanceId,
                                         const std::string& replySlotFunction) {
                    karabo::util::Hash::Pointer header = prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction);
                    karabo::util::Hash::Pointer body(new karabo::util::Hash());
                    sendRequest(requestSlotInstanceId, header, body);
                    return *this;
                }

                template <class A1>
                Requestor& requestNoWait(
                                         const std::string& requestSlotInstanceId,
                                         const std::string& requestSlotFunction,
                                         const std::string replySlotInstanceId,
                                         const std::string& replySlotFunction,
                                         const A1& a1) {
                    karabo::util::Hash::Pointer header = prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction);
                    karabo::util::Hash::Pointer body(new karabo::util::Hash("a1", a1));
                    sendRequest(requestSlotInstanceId, header, body);
                    return *this;
                }

                template <class A1, class A2>
                Requestor& requestNoWait(
                                         const std::string& requestSlotInstanceId,
                                         const std::string& requestSlotFunction,
                                         const std::string replySlotInstanceId,
                                         const std::string& replySlotFunction,
                                         const A1& a1, const A2& a2) {
                    karabo::util::Hash::Pointer header = prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction);
                    karabo::util::Hash::Pointer body(new karabo::util::Hash("a1", a1, "a2", a2));
                    sendRequest(requestSlotInstanceId, header, body);
                    return *this;
                }

                template <class A1, class A2, class A3>
                Requestor& requestNoWait(
                                         const std::string& requestSlotInstanceId,
                                         const std::string& requestSlotFunction,
                                         const std::string replySlotInstanceId,
                                         const std::string& replySlotFunction, const A1& a1, const A2& a2, const A3& a3) {
                    karabo::util::Hash::Pointer header = prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction);
                    karabo::util::Hash::Pointer body(new karabo::util::Hash("a1", a1, "a2", a2, "a3", a3));
                    sendRequest(requestSlotInstanceId, header, body);
                    return *this;
                }

                template <class A1, class A2, class A3, class A4>
                Requestor& requestNoWait(const std::string& requestSlotInstanceId,
                                         const std::string& requestSlotFunction,
                                         const std::string replySlotInstanceId,
                                         const std::string& replySlotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                    karabo::util::Hash::Pointer header = prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction);
                    karabo::util::Hash::Pointer body(new karabo::util::Hash("a1", a1, "a2", a2, "a3", a3, "a4", a4));
                    sendRequest(requestSlotInstanceId, header, body);
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

                karabo::util::Hash::Pointer prepareHeader(const std::string& slotInstanceId, const std::string& slotFunction);

                karabo::util::Hash::Pointer prepareHeaderNoWait(const std::string& requestSlotInstanceId, const std::string& requestSlotFunction,
                                                                const std::string& replySlotInstanceId, const std::string& replySlotFunction);

                void registerRequest();

                static std::string generateUUID();

                void sendRequest(const std::string& slotInstanceId, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) const;

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
            typedef boost::function<void (float /*avgBrokerLatency*/, unsigned int /*maxBrokerLatency*/,
                                          float /*avgProcessingLatency*/, unsigned int /*maxProcessingLatency*/,
                                          unsigned int /*queueSize*/) > UpdatePerformanceStatisticsHandler;

            typedef std::map<std::string, InputChannel::Pointer> InputChannels;
            typedef std::map<std::string, OutputChannel::Pointer> OutputChannels;

            std::string m_instanceId;
            karabo::util::Hash m_instanceInfo;
            std::string m_username;
            int m_nThreads;

            SignalInstances m_signalInstances;
            SlotInstances m_slotInstances;
            // TODO Split into two mutexes
            mutable boost::mutex m_signalSlotInstancesMutex;

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
            // key is instanceId of signal or slot
            typedef std::map<std::string, std::set<SignalSlotConnection> > SignalSlotConnections;
            SignalSlotConnections m_signalSlotConnections; /// keep track of established connections
            boost::mutex m_signalSlotConnectionsMutex;

            karabo::net::BrokerIOService::Pointer m_ioService;
            karabo::net::BrokerConnection::Pointer m_connection;
            bool m_connectionInjected;
            karabo::net::BrokerChannel::Pointer m_producerChannel;
            karabo::net::BrokerChannel::Pointer m_consumerChannel;
            karabo::net::BrokerChannel::Pointer m_heartbeatProducerChannel;
            karabo::net::BrokerChannel::Pointer m_heartbeatConsumerChannel;

            boost::mutex m_waitMutex;
            boost::condition_variable m_hasNewEvent;

            typedef std::pair<karabo::util::Hash::Pointer /*header*/, karabo::util::Hash::Pointer /*body*/> Event;

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

            static std::map<std::string, SignalSlotable*> m_instanceMap;
            static boost::mutex m_instanceMapMutex;

            bool m_discoverConnectionResourcesMode;
            static std::map<std::string, std::string> m_connectionStrings;
            static boost::mutex m_connectionStringsMutex;

            static karabo::net::PointToPoint::Pointer m_pointToPoint;

        public:

            KARABO_CLASSINFO(SignalSlotable, "SignalSlotable", "1.0")

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

#define KARABO_SYSTEM_SIGNAL0(signalName) this->registerSystemSignal(signalName);
#define KARABO_SYSTEM_SIGNAL1(signalName, a1) this->registerSystemSignal<a1>(signalName);
#define KARABO_SYSTEM_SIGNAL2(signalName, a1, a2) this->registerSystemSignal<a1,a2>(signalName);
#define KARABO_SYSTEM_SIGNAL3(signalName, a1, a2, a3) this->registerSystemSignal<a1,a2,a3>(signalName);
#define KARABO_SYSTEM_SIGNAL4(signalName, a1, a2, a3, a4) this->registerSystemSignal<a1,a2,a3,a4>(signalName);

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
             * Single call that leads to a tracking of all instances if called before the event loop is started
             */
            void trackAllInstances();


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

            /**
             * Access to the identification of the current instance using signals and slots
             * @return instanceId
             */
            const std::string& getInstanceId() const;

            void updateInstanceInfo(const karabo::util::Hash& update);

            const karabo::util::Hash& getInstanceInfo() const;

            /**
             * This function will actively start tracking of specific instance.  It will throw parameter exception
             * if the instance doesn't exist.  No active instance tracking in this case.
             * @param instanceId to be tracked
             * @throw KARABO_PARAMETER_EXCEPTION if instanceId is not responding.
             */
            void trackExistenceOfInstance(const std::string& instanceId);

            /**
             * This function stops tracking activity for the specific instance.
             * @param instanceId to be un-tracked.
             */
            void stopTrackingExistenceOfInstance(const std::string& instanceId);

            void registerInstanceNotAvailableHandler(const InstanceNotAvailableHandler& instanceNotAvailableCallback);

            void registerInstanceAvailableAgainHandler(const InstanceAvailableAgainHandler& instanceAvailableAgainCallback);

            void registerExceptionHandler(const ExceptionHandler& exceptionFoundCallback);

            //void registerInstanceNewHandler(const InstanceNewHandler& instanceNewCallback);

            void registerSlotCallGuardHandler(const SlotCallGuardHandler& slotCallGuardHandler);

            void registerPerformanceStatisticsHandler(const UpdatePerformanceStatisticsHandler& updatePerformanceStatisticsHandler);

            karabo::net::BrokerConnection::Pointer getConnection() const;

            karabo::util::Hash getAvailableInstances(const bool activateTracking = false);

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
             * @return whether connection is successfully stopped, e.g. false if there was no such connection or if remote signal instance ID did not confirm in time
             */
            bool disconnect(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction);

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
                SignalSlotable::Caller(this).call(instanceId, functionName);
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
                SignalSlotable::Caller(this).call(instanceId, functionName, a1);
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
                SignalSlotable::Caller(this).call(instanceId, functionName, a1, a2);
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
                SignalSlotable::Caller(this).call(instanceId, functionName, a1, a2, a3);
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
                SignalSlotable::Caller(this).call(instanceId, functionName, a1, a2, a3, a4);
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
                SignalInstancePointer s = addSignalIfNew(funcName, KARABO_PUB_PRIO, KARABO_PUB_TTL);
                if (s) {
                    boost::function<void() > f(boost::bind(&karabo::xms::Signal::emit0, s));
                    m_emitFunctions.set(funcName, f);
                }
            }

            template <class A1>
            void registerSignal(const std::string& funcName) {
                SignalInstancePointer s = addSignalIfNew(funcName, KARABO_PUB_PRIO, KARABO_PUB_TTL);
                if (s) {
                    boost::function<void (const A1&) > f(boost::bind(&karabo::xms::Signal::emit1<A1>, s, _1));
                    m_emitFunctions.set(funcName, f);
                }
            }

            template <class A1, class A2>
            void registerSignal(const std::string& funcName) {
                SignalInstancePointer s = addSignalIfNew(funcName, KARABO_PUB_PRIO, KARABO_PUB_TTL);
                if (s) {
                    boost::function<void (const A1&, const A2&) > f(boost::bind(&karabo::xms::Signal::emit2<A1, A2>, s, _1, _2));
                    m_emitFunctions.set(funcName, f);
                }
            }

            template <class A1, class A2, class A3>
            void registerSignal(const std::string& funcName) {
                SignalInstancePointer s = addSignalIfNew(funcName, KARABO_PUB_PRIO, KARABO_PUB_TTL);
                if (s) {
                    boost::function<void (const A1&, const A2&, const A3&) > f(boost::bind(&karabo::xms::Signal::emit3<A1, A2, A3>, s, _1, _2, _3));
                    m_emitFunctions.set(funcName, f);
                }
            }

            template <class A1, class A2, class A3, class A4>
            void registerSignal(const std::string& funcName) {
                SignalInstancePointer s = addSignalIfNew(funcName, KARABO_PUB_PRIO, KARABO_PUB_TTL);
                if (s) {
                    boost::function<void (const A1&, const A2&, const A3&, const A4&) > f(boost::bind(&karabo::xms::Signal::emit4<A1, A2, A3, A4>, s, _1, _2, _3, _4));
                    m_emitFunctions.set(funcName, f);
                }
            }

            void registerSystemSignal(const std::string& funcName) {
                SignalInstancePointer s = addSignalIfNew(funcName);
                if (s) {
                    boost::function<void() > f(boost::bind(&karabo::xms::Signal::emit0, s));
                    m_emitFunctions.set(funcName, f);
                }
            }

            template <class A1>
            void registerSystemSignal(const std::string& funcName) {
                SignalInstancePointer s = addSignalIfNew(funcName);
                if (s) {
                    boost::function<void (const A1&) > f(boost::bind(&karabo::xms::Signal::emit1<A1>, s, _1));
                    m_emitFunctions.set(funcName, f);
                }
            }

            template <class A1, class A2>
            void registerSystemSignal(const std::string& funcName) {
                SignalInstancePointer s = addSignalIfNew(funcName);
                if (s) {
                    boost::function<void (const A1&, const A2&) > f(boost::bind(&karabo::xms::Signal::emit2<A1, A2>, s, _1, _2));
                    m_emitFunctions.set(funcName, f);
                }
            }

            template <class A1, class A2, class A3>
            void registerSystemSignal(const std::string& funcName) {
                SignalInstancePointer s = addSignalIfNew(funcName);
                if (s) {
                    boost::function<void (const A1&, const A2&, const A3&) > f(boost::bind(&karabo::xms::Signal::emit3<A1, A2, A3>, s, _1, _2, _3));
                    m_emitFunctions.set(funcName, f);
                }
            }

            template <class A1, class A2, class A3, class A4>
            void registerSystemSignal(const std::string& funcName) {
                SignalInstancePointer s = addSignalIfNew(funcName);
                if (s) {
                    boost::function<void (const A1&, const A2&, const A3&, const A4&) > f(boost::bind(&karabo::xms::Signal::emit4<A1, A2, A3, A4>, s, _1, _2, _3, _4));
                    m_emitFunctions.set(funcName, f);
                }
            }

            /**
             * Register a new slot function for a slot. A new slot is generated
             * if so necessary. It is checked that the signature of the new
             * slot is the same as an already registered one.
             */
            void registerSlot(const boost::function<void () >& slot, const std::string& funcName) {
                // Note that the dynamic_pointer_cast will destroy the result if the function
                // signatures don't match, registerNewSlot will complain then later.
                karabo::xms::Slot0::Pointer s = boost::dynamic_pointer_cast<karabo::xms::Slot0>(findSlot(funcName));
                if (!s) {
                    s = karabo::xms::Slot0::Pointer(new karabo::xms::Slot0(funcName));
                    registerNewSlot(funcName, s);
                }
                s->registerSlotFunction(slot);
            }

            template <class A1>
            void registerSlot(const boost::function<void (const A1&) >& slot, const std::string& funcName) {
                // About the dynamic_pointer_cast: see non-template version of registerSlot.
                typename karabo::xms::Slot1<A1>::Pointer s = boost::dynamic_pointer_cast<karabo::xms::Slot1<A1> >(findSlot(funcName));
                if (!s) {
                    s = typename boost::shared_ptr<karabo::xms::Slot1<A1> >(new karabo::xms::Slot1<A1 >(funcName));
                    registerNewSlot(funcName, s);
                }
                s->registerSlotFunction(slot);
            }

            template <class A1, class A2>
            void registerSlot(const boost::function<void (const A1&, const A2&) >& slot, const std::string& funcName) {
                // About the dynamic_pointer_cast: see non-template version of registerSlot.
                typename karabo::xms::Slot2<A1, A2>::Pointer s = boost::dynamic_pointer_cast<karabo::xms::Slot2<A1, A2> >(findSlot(funcName));
                if (!s) {
                    s = typename karabo::xms::Slot2<A1, A2>::Pointer(new karabo::xms::Slot2<A1, A2>(funcName));
                    registerNewSlot(funcName, s);
                }
                s->registerSlotFunction(slot);
            }

            template <class A1, class A2, class A3>
            void registerSlot(const boost::function<void (const A1&, const A2&, const A3&) >& slot, const std::string& funcName) {
                // About the dynamic_pointer_cast: see non-template version of registerSlot.
                typename karabo::xms::Slot3<A1, A2, A3>::Pointer s = boost::dynamic_pointer_cast<karabo::xms::Slot3<A1, A2, A3> >(findSlot(funcName));
                if (!s) {
                    s = typename karabo::xms::Slot3<A1, A2, A3>::Pointer(new karabo::xms::Slot3<A1, A2, A3>(funcName));
                    registerNewSlot(funcName, s);
                }
                s->registerSlotFunction(slot);
            }

            template <class A1, class A2, class A3, class A4>
            void registerSlot(const boost::function<void (const A1&, const A2&, const A3&, const A4&) >& slot, const std::string& funcName) {
                // About the dynamic_pointer_cast: see non-template version of registerSlot.
                typename karabo::xms::Slot4<A1, A2, A3, A4>::Pointer s = boost::dynamic_pointer_cast<karabo::xms::Slot4<A1, A2, A3, A4> >(findSlot(funcName));
                if (!s) {
                    s = typename karabo::xms::Slot4<A1, A2, A3, A4>::Pointer(new karabo::xms::Slot4<A1, A2, A3, A4>(funcName));
                    registerNewSlot(funcName, s);
                }
                s->registerSlotFunction(slot);
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
                                                             const boost::function<void (const karabo::util::Hash::Pointer&) >& onDataAvailableHandler = boost::function<void (const karabo::util::Hash::Pointer&) >(),
                                                             const boost::function<void (const InputChannel::Pointer&) >& onInputAvailableHandler = boost::function<void (const InputChannel::Pointer&) >(),
                                                             const boost::function<void (const InputChannel::Pointer&)>& onEndOfStreamEventHandler = boost::function<void (const InputChannel::Pointer&)>());

            virtual OutputChannel::Pointer createOutputChannel(const std::string& channelName, const karabo::util::Hash& config, const boost::function<void (const OutputChannel::Pointer&) >& onOutputPossibleHandler = boost::function<void (const OutputChannel::Pointer&) >());

            //TODO: make this function private: it is for internal use
            const InputChannels& getInputChannels() const;

            //TODO: make this function private: it is for internal use
            const OutputChannels& getOutputChannels() const;

            const OutputChannel::Pointer& getOutputChannel(const std::string& name);

            const InputChannel::Pointer& getInputChannel(const std::string& name);

            void registerInputHandler(const std::string& channelName, const boost::function<void (const karabo::xms::InputChannel::Pointer&) >& handler);

            void registerDataHandler(const std::string& channelName, const boost::function<void (const karabo::util::Hash::Pointer&) >& handler);

            void registerEndOfStreamHandler(const std::string& channelName, const boost::function<void (const karabo::xms::InputChannel::Pointer&) >& handler);

            void connectInputChannel(const InputChannel::Pointer& channel, int trails = 8, int sleep = 1);

            void connectInputToOutputChannel(const InputChannel::Pointer& channel, const std::string& outputChannelString, int trails = 8, int sleep = 1);

            void connectInputChannelAsync(const InputChannel::Pointer& channel, const boost::function<void()>& handler);

            void connectInputChannels();

            void reconnectInputChannels(const std::string& instanceId);

            void disconnectInputChannels(const std::string& instanceId);

            std::pair<bool, std::string> exists(const std::string& instanceId);

            int getAccessLevel(const std::string& instanceId) const;

            template <class T>
            static std::string generateInstanceId() {
                std::string hostname(boost::asio::ip::host_name());
                std::vector<std::string> tokens;
                boost::split(tokens, hostname, boost::is_any_of("."));
                return std::string(tokens[0] + "_" + T::classInfo().getClassId() + "_" + karabo::util::toString(getpid()));
            }

            bool ensureOwnInstanceIdUnique();

            void injectConnection(const std::string& instanceId, const karabo::net::BrokerConnection::Pointer& connection);

            void setDeviceServerPointer(boost::any serverPtr);

            void inputHandlerWrap(const boost::function<void (const karabo::xms::InputChannel::Pointer&)>& handler,
                                  const karabo::xms::InputChannel::Pointer& input);

            void dataHandlerWrap(const boost::function<void (const karabo::util::Hash::Pointer&) >& handler,
                                 const karabo::util::Hash::Pointer& data);

            void endOfStreamHandlerWrap(const boost::function<void (const boost::shared_ptr<InputChannel>&) >& handler,
                                        const boost::shared_ptr<InputChannel>& input);

            bool connectP2P(const std::string& instanceId);

            void disconnectP2P(const std::string& instanceId);

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

            std::pair<std::string, std::string> splitIntoInstanceIdAndFunctionName(const std::string& signalOrSlotId, const char sep = ':') const;

            template <class TFunc>
            void storeSignal(const std::string& signalFunction, const SignalInstancePointer& signalInstance, const TFunc& emitFunction) {
                storeSignal(signalFunction, signalInstance);
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

            /**
             * Helper for register(System)Signal: If signalFunction is not yet known, creates the corresponding
             * Signal and adds it to the internal container. Otherwise an empty pointer is returned.
             * @param signalFunction
             * @param priority is passed further to the Signal
             * @param messageTimeToLive is passed further to the Signal
             * @return pointer to new Signal or empty pointer
             */
            SignalInstancePointer addSignalIfNew(const std::string& signalFunction, int priority = KARABO_SYS_PRIO, int messageTimeToLive = KARABO_SYS_TTL);

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

            bool tryToConnectToSignal(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction);

            SlotInstancePointer findSlot(const std::string &funcName);

            /**
             * Register a new slot *instance* under name *funcName*.
             * This will raise an error if the slot already exists.
             */
            void registerNewSlot(const std::string &funcName, SlotInstancePointer instance);

            void slotConnectToSignal(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction);

            /// True if instance with ID 'slotInstanceId' has slot 'slotFunction'.
            /// Internally uses "slotHasSlot" for remote instances, but shortcuts if ID is the own one.
            /// Always true if 'slotInstanceId == "*"' (i.e. global slot).
            bool instanceHasSlot(const std::string& slotInstanceId, const std::string& slotFunction);

            /// Slot to tell whether instance has a slot of given name.
            void slotHasSlot(const std::string& slotFunction);

            bool tryToDisconnectFromSignal(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction);

            void slotDisconnectFromSignal(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction);

            static std::string prepareFunctionSignature(const std::string& funcName) {
                std::string f(boost::trim_copy(funcName));
                return f.substr(0, f.find_first_of('-')) + "|";
            }

            bool slotConnectToOutputChannel(const std::string& inputName, const karabo::util::Hash& outputChannelInfo, bool connect);

            void slotHeartbeat(const std::string& networkId, const int& heartbeatInterval, const karabo::util::Hash& instanceInfo);

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

            karabo::util::Hash prepareConnectionNotAvailableInformation(const karabo::util::Hash& signals) const;

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
            bool tryToUnregisterSlot(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction);

            bool hasReceivedReply(const std::string& replyFromValue) const;

            void popReceivedReply(const std::string& replyFromValue, karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body);

            bool timedWaitAndPopReceivedReply(const std::string& replyId, karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body, int timeout);

            long long getEpochMillis() const;

            void onInputChannelConnectInfo(const InputChannel::Pointer& channel,
                                           const boost::function<void()>& handler,
                                           const std::string& instanceId, const std::string& channelId,
                                           bool channelExists, const karabo::util::Hash& info);

            bool tryToCallDirectly(const std::string& slotInstanceId,
                                   const karabo::util::Hash::Pointer& header,
                                   const karabo::util::Hash::Pointer& body) const;

            bool tryToCallP2P(const std::string& slotInstanceId, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body, int prio) const;

            void doSendMessage(const std::string& instanceId, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body, int prio, int timeTpLive) const;

            void channelErrorHandler(karabo::net::BrokerChannel::Pointer channel, const std::string& info);
        };

    }
}

#endif
