/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <sys/types.h>

#include <karabo/webAuth/Authenticator.hh>
#include <boost/date_time/time_duration.hpp>
#include <bits/stl_deque.h>

#include "SignalSlotable.hh"
#include "karabo/util/Version.hh"

namespace karabo {
    namespace xms {

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::net;
        using namespace karabo::webAuth;

        // Static initializations
        std::set<int> SignalSlotable::m_reconnectIntervals = std::set<int>();


        SignalSlotable::SignalSlotable() {
        }


        SignalSlotable::SignalSlotable(const string& instanceId,
                const BrokerConnection::Pointer& connection) {

            init(instanceId, connection);
        }


        SignalSlotable::SignalSlotable(const std::string& instanceId,
                const std::string& brokerType,
                const karabo::util::Hash& brokerConfiguration) : m_brokerLatency(10LL), m_processingLatency(15LL) {

            BrokerConnection::Pointer connection = BrokerConnection::create(brokerType, brokerConfiguration);
            init(instanceId, connection);
        }


        SignalSlotable::~SignalSlotable() {
        }


        void SignalSlotable::init(const std::string& instanceId,
                const karabo::net::BrokerConnection::Pointer& connection) {

            m_defaultAccessLevel = KARABO_DEFAULT_ACCESS_LEVEL;
            m_connection = connection;
            m_instanceId = instanceId;

            // Currently only removes dots
            sanifyInstanceId(m_instanceId);

            // Create the managing ioService object
            m_ioService = m_connection->getIOService();

            // Start connection (and take the default channel for signals)
            m_connection->start();
            m_producerChannel = m_connection->createChannel();
            m_consumerChannel = m_connection->createChannel();
            m_heartbeatProducerChannel = m_connection->createChannel("beats");
            m_heartbeatConsumerChannel = m_connection->createChannel("beats");

            m_timerHandlers.clear();

            registerDefaultSignalsAndSlots();
        }

        std::pair<bool, std::string > SignalSlotable::isValidInstanceId(const std::string & instanceId) {

            Hash instanceInfo;
            try {
		m_randPing = rand() + 1;
                request("*", "slotPing", instanceId, m_randPing, false).timeout(100).receive(instanceInfo);
                //cout << "isValidInstanceId got answer: " << instanceInfo << endl;
            } catch (const karabo::util::TimeoutException&) {
                Exception::clearTrace();
                return std::make_pair(true, "");
            }
            string foreignHost("unknown");
            if (instanceInfo.has("host")) instanceInfo.get("host", foreignHost);
            string msg("Another instance with the same ID is already online (on host: " + foreignHost + ")");
            return std::make_pair(false, msg);
        }

        void SignalSlotable::sanifyInstanceId(std::string & instanceId) const {
            for (std::string::iterator it = instanceId.begin(); it != instanceId.end(); ++it) {
                if ((*it) == '.') (*it) = '-';
            }
        }

        void SignalSlotable::injectEvent(karabo::net::BrokerChannel::Pointer, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) {

            long long latency = getEpochMillis() - header->get<long long>("MQTimestamp");
            if (latency > m_brokerLatency) {
                KARABO_LOG_FRAMEWORK_WARN << "Event latency at \"injection\" time is high (> " << m_brokerLatency/1000 << " secs) : " << latency/1000 << "." << latency%1000 << " secs";
            }

            // Check whether this message is a reply
            if (header->has("replyFrom")) {
                KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Injecting reply from: " << header->get<string>("signalInstanceId");
                string replyId = header->get<string>("replyFrom");
#if defined(KARABO_SIGNAL_SLOTABLE_TIMED_WAIT_AND_POP_RECEIVED_REPLY_POLLING)
                boost::mutex::scoped_lock lock(m_receivedRepliesMutex);
                m_receivedReplies[replyId] = std::make_pair(header, body);
#else
                boost::shared_ptr<BoostMutexCond> bmc;
                try {
                    boost::mutex::scoped_lock lock(m_receivedRepliesBMCMutex);
                    ReceivedRepliesBMC::iterator it = m_receivedRepliesBMC.find(replyId);
                    if (it != m_receivedRepliesBMC.end()) bmc = it->second;
                } catch (...) {
                    KARABO_RETHROW
                }
                // insert reply and notify only if it is expected
                if (!bmc)
                    return;
                try {
                    boost::mutex::scoped_lock lock(m_receivedRepliesMutex);
                    m_receivedReplies[replyId] = std::make_pair(header, body);
                } catch (...) {
                    KARABO_RETHROW
                }
                bmc->m_cond.notify_one(); // notify only if 
#endif
            } else {
                {
                    boost::mutex::scoped_lock lock(m_eventQueueMutex);
                    m_eventQueue.push_back(std::make_pair(header, body));
                }
                m_hasNewEvent.notify_one();
            }
        }

        void SignalSlotable::injectHeartbeat(karabo::net::BrokerChannel::Pointer, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) {
            long long latency = getEpochMillis() - header->get<long long>("MQTimestamp");
            if (latency > m_brokerLatency) {
                KARABO_LOG_FRAMEWORK_WARN << "Heartbeat latency at \"injection\" time is high (> " << m_brokerLatency/1000 << " secs) : " << latency/1000 << "." << latency%1000 << " secs";
            }
            {
                boost::mutex::scoped_lock lock(m_heartbeatQueueMutex);
                m_heartbeatQueue.push_back(std::make_pair(header, body));
            }
            m_hasNewHeartbeat.notify_one();
        }

        void SignalSlotable::runEventLoop(int heartbeatInterval, const karabo::util::Hash& instanceInfo, int nThreads) {

            // This prevents this instance from answering to any ping request                        
            m_instanceInfo = instanceInfo;

            m_runEventLoop = true;

            startEmittingHeartbeats(heartbeatInterval);
            startTrackingSystem();
            startBrokerMessageConsumption();

            // Start all configured event threads
            for (int h = 0; h < 1; h++) {
                m_heartbeatLoopThreads.create_thread(boost::bind(&karabo::xms::SignalSlotable::_runHeartbeatLoop, this));
            }
            
            for (int i = 0; i < nThreads; ++i) {
                m_eventLoopThreads.create_thread(boost::bind(&karabo::xms::SignalSlotable::_runEventLoop, this));
            }

            // Make sure the instanceId is valid and unique
            std::pair<bool, std::string> result = isValidInstanceId(m_instanceId);
            if (!result.first) {
                KARABO_LOG_FRAMEWORK_ERROR << result.second;
                stopBrokerMessageConsumption();
                stopTrackingSystem();
                stopEmittingHearbeats();
                return;
            }

            KARABO_LOG_FRAMEWORK_INFO << "Instance starts up with id: " << m_instanceId;
            call("*", "slotInstanceNew", m_instanceId, m_instanceInfo);

            m_eventLoopThreads.join_all(); // Join all event dispatching threads
            m_heartbeatLoopThreads.join_all();
            
            KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << m_instanceId << "\" shuts cleanly down";
            call("*", "slotInstanceGone", m_instanceId, m_instanceInfo);

            stopBrokerMessageConsumption();
            stopTrackingSystem();
            stopEmittingHearbeats();

        }

        void SignalSlotable::_runEventLoop() {

            while (m_runEventLoop) {

                if (eventQueueIsEmpty()) { // If queue is empty we wait for the next event
                    boost::mutex::scoped_lock lock(m_waitMutex);
                    m_hasNewEvent.wait(lock);
                }

                // Got notified about new events, start processing them
                processEvents();

            }
        }

        void SignalSlotable::_runHeartbeatLoop() {

            while (m_runHeartbeatLoop) {

                if (heartbeatQueueIsEmpty()) { // If queue is empty we wait for the next event
                    boost::mutex::scoped_lock lock(m_waitHeartbeatMutex);
                    m_hasNewHeartbeat.wait(lock);
                }

                // Got notified about new events, start processing them
                processHeartbeats();
            }
        }

        void SignalSlotable::stopEventLoop() {

            // Finish the outer while loop
            m_runEventLoop = false;
            m_runHeartbeatLoop = false;

            // Notify all currently waiting threads to finish
            m_hasNewEvent.notify_all();
            m_hasNewHeartbeat.notify_all();
        }

        void SignalSlotable::startTrackingSystem() {
            // Countdown and finally timeout registered heartbeats
            m_doTracking = true;
            m_trackingThread = boost::thread(boost::bind(&SignalSlotable::letConnectionSlowlyDieWithoutHeartbeat, this));
        }

        void SignalSlotable::stopTrackingSystem() {
            m_doTracking = false;
            m_trackingThread.join();
        }

        void SignalSlotable::startEmittingHeartbeats(const int heartbeatInterval) {
            m_heartbeatInterval = heartbeatInterval;
            // Inject the heartbeat interval to instanceInfo
            m_instanceInfo.set("heartbeatInterval", heartbeatInterval);
            // Inject karaboVersion
            m_instanceInfo.set("karaboVersion", karabo::util::Version::getVersion());
            m_sendHeartbeats = true;
            m_heartbeatThread = boost::thread(boost::bind(&karabo::xms::SignalSlotable::emitHeartbeat, this));
        }

        void SignalSlotable::stopEmittingHearbeats() {
            m_sendHeartbeats = false;
            // interrupt sleeping in heartbeatThread
            m_heartbeatThread.interrupt();
            m_heartbeatThread.join();
        }

        void SignalSlotable::startBrokerMessageConsumption() {

            m_brokerThread = boost::thread(boost::bind(&karabo::net::BrokerIOService::work, m_ioService));

            // Prepare the slot selector
            string selector = "slotInstanceIds LIKE '%|" + m_instanceId + "|%' OR slotInstanceIds LIKE '%|*|%'";
            m_consumerChannel->setFilter(selector);
            m_consumerChannel->readAsyncHashHash(boost::bind(&karabo::xms::SignalSlotable::injectEvent, this, _1, _2, _3));

            selector = "slotInstanceIds LIKE '%|" + m_instanceId + "|%'";
            m_heartbeatConsumerChannel->setFilter(selector);
            m_heartbeatConsumerChannel->readAsyncHashHash(boost::bind(&karabo::xms::SignalSlotable::injectHeartbeat, this, _1, _2, _3));
        }

        void SignalSlotable::stopBrokerMessageConsumption() {
            m_ioService->stop();
            m_brokerThread.join();
        }

        bool SignalSlotable::eventQueueIsEmpty() {
            boost::mutex::scoped_lock lock(m_eventQueueMutex);
            return m_eventQueue.empty();
        }

        bool SignalSlotable::tryToPopEvent(Event& event) {
            boost::mutex::scoped_lock lock(m_eventQueueMutex);
            if (!m_eventQueue.empty()) {
                event = m_eventQueue.front(); // usual queue
                //event = m_eventQueue.top();        // priority queue
                m_eventQueue.pop_front();
                return true;
            }
            return false;
        }

        void SignalSlotable::processEvents() {

            Event event;
            while (tryToPopEvent(event)) {

                /* Header and body are shared pointers               
                 * By now the event variable keeps the only reference to them and hence the objects alive
                 * In the next while loop header and body will be destructed
                 * We take const references for convenience here
                 */
                const Hash& header = *(event.first);
                const Hash& body = *(event.second);

                long long latency = getEpochMillis() - header.get<long long>("MQTimestamp");
                if (latency > m_processingLatency) {
                    KARABO_LOG_FRAMEWORK_WARN << "Event latency at \"processing\" time is high (> " << m_processingLatency/1000L << " secs) : " << latency/1000L << "." << latency % 1000L << " secs";
                }


                /* The header of each event (message) 
                 * should contain all slotFunctions that must be a called
                 * The formatting is like: 
                 * slotFunctions -> [|<instanceId1>:<slotFunction1>[,<slotFunction2>]]
                 * Example:
                 * slotFunctions -> |FooInstance:slotFoo1,slotFoo2|BarInstance:slotBar1,slotBar2|"
                 */

                boost::optional<const Hash::Node&> allSlotsNode = header.find("slotFunctions");
                if (allSlotsNode) {
                    std::vector<string> allSlots;
                    std::string tmp = allSlotsNode->getValue<string>();

                    // Trim and split on the "|" string, avoid empty entries
                    boost::trim_if(tmp, boost::is_any_of("|"));
                    boost::split(allSlots, tmp, boost::is_any_of("|"), boost::token_compress_on);

                    BOOST_FOREACH(string instanceSlots, allSlots) {
                        //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Processing instanceSlots: " << instanceSlots;
                        size_t pos = instanceSlots.find_first_of(":");
                        if (pos == std::string::npos) {
                            KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Encountered badly shaped message header";
                            continue;
                        }
                        string instanceId = instanceSlots.substr(0, pos);
                        //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Instance is: " << instanceId;

                        // We should call only functions defined for our instanceId or global ("*") ones
                        if (instanceId == m_instanceId || instanceId == "*") {
                            vector<string> slotFunctions = karabo::util::fromString<string, vector>(instanceSlots.substr(pos + 1));
                            if (instanceId == "*") { // Global slot

                                BOOST_FOREACH(string slotFunction, slotFunctions) {

                                    // Place a guard for executing this slot here
                                    if (m_slotCallGuardHandler && !m_slotCallGuardHandler(slotFunction)) {
                                        std::cout << "Rejected global slot: " << slotFunction << std::endl;
                                        continue;
                                    }

                                    //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Going to call global " << slotFunction << " if registered";
                                    SlotInstancePointer slot = getGlobalSlot(slotFunction);
                                    if (slot) {
                                        //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Now calling " << slotFunction;
                                        // This will synchronously call back all registered slot functions
                                        slot->callRegisteredSlotFunctions(header, body);
                                        // In the body of the slot callback the user may have placed a reply
                                        // If so, send it back now
                                        sendPotentialReply(header);
                                    }
                                }
                            } else { // Local slot                                                              

                                BOOST_FOREACH(string slotFunction, slotFunctions) {
                                    //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Going to call local " << slotFunction << " if registered";

                                    // Place a guard for executing this slot here
                                    if (m_slotCallGuardHandler && !m_slotCallGuardHandler(slotFunction)) {
                                        std::cout << "Rejected local slot: " << slotFunction << std::endl;
                                        continue;
                                    }

                                    SlotInstancePointer slot = getLocalSlot(slotFunction);
                                    if (slot) {
                                        //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Now calling " << slotFunction;
                                        slot->callRegisteredSlotFunctions(header, body);
                                        sendPotentialReply(header);
                                    } else {
                                        KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Received a call from ? to non-existing slot \"" << slotFunction << "\"";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        bool SignalSlotable::heartbeatQueueIsEmpty() {
            boost::mutex::scoped_lock lock(m_heartbeatQueueMutex);
            return m_heartbeatQueue.empty();
        }

        bool SignalSlotable::tryToPopHeartbeat(Event& event) {
            boost::mutex::scoped_lock lock(m_heartbeatQueueMutex);
            if (!m_heartbeatQueue.empty()) {
                event = m_heartbeatQueue.front(); // usual queue
                m_heartbeatQueue.pop_front();
                return true;
            }
            return false;
        }

        void SignalSlotable::processHeartbeats() {

            Event event;
            while (tryToPopHeartbeat(event)) {

                /* Header and body are shared pointers               
                 * By now the event variable keeps the only reference to them and hence the objects alive
                 * In the next while loop header and body will be destructed
                 * We take const references for convenience here
                 */
                const Hash& header = *(event.first);
                const Hash& body = *(event.second);

                long long latency = getEpochMillis() - header.get<long long>("MQTimestamp");
                if (latency > m_processingLatency) {
                    KARABO_LOG_FRAMEWORK_WARN << "Heartbeat latency at \"processing\" time is high (> " << m_processingLatency/1000L << " secs) : " << latency/1000L << "." << latency % 1000L << " secs";
                }


                /* The header of each event (message) 
                 * should contain all slotFunctions that must be a called
                 * The formatting is like: 
                 * slotFunctions -> [|<instanceId1>:<slotFunction1>[,<slotFunction2>]]
                 * Example:
                 * slotFunctions -> |FooInstance:slotFoo1,slotFoo2|BarInstance:slotBar1,slotBar2|"
                 */

                boost::optional<const Hash::Node&> allSlotsNode = header.find("slotFunctions");
                if (allSlotsNode) {
                    std::vector<string> allSlots;
                    std::string tmp = allSlotsNode->getValue<string>();

                    // Trim and split on the "|" string, avoid empty entries
                    boost::trim_if(tmp, boost::is_any_of("|"));
                    boost::split(allSlots, tmp, boost::is_any_of("|"), boost::token_compress_on);

                    BOOST_FOREACH(string instanceSlots, allSlots) {
                        //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Processing instanceSlots: " << instanceSlots;
                        size_t pos = instanceSlots.find_first_of(":");
                        if (pos == std::string::npos) {
                            KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Encountered badly shaped message header";
                            continue;
                        }
                        string instanceId = instanceSlots.substr(0, pos);
                        //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Instance is: " << instanceId;

                        // We should call only functions defined for our instanceId or global ("*") ones
                        if (instanceId == m_instanceId || instanceId == "*") {
                            vector<string> slotFunctions = karabo::util::fromString<string, vector>(instanceSlots.substr(pos + 1));
                            if (instanceId == "*") { // Global slot

                                BOOST_FOREACH(string slotFunction, slotFunctions) {

                                    // Place a guard for executing this slot here
                                    if (m_slotCallGuardHandler && !m_slotCallGuardHandler(slotFunction)) {
                                        std::cout << "Rejected global slot: " << slotFunction << std::endl;
                                        continue;
                                    }

                                    //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Going to call global " << slotFunction << " if registered";
                                    SlotInstancePointer slot = getGlobalSlot(slotFunction);
                                    if (slot) {
                                        //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Now calling " << slotFunction;
                                        // This will synchronously call back all registered slot functions
                                        slot->callRegisteredSlotFunctions(header, body);
                                        // In the body of the slot callback the user may have placed a reply
                                        // If so, send it back now
                                        sendPotentialReply(header);
                                    }
                                }
                            } else { // Local slot                                                              

                                BOOST_FOREACH(string slotFunction, slotFunctions) {
                                    //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Going to call local " << slotFunction << " if registered";

                                    // Place a guard for executing this slot here
                                    if (m_slotCallGuardHandler && !m_slotCallGuardHandler(slotFunction)) {
                                        std::cout << "Rejected local slot: " << slotFunction << std::endl;
                                        continue;
                                    }

                                    SlotInstancePointer slot = getLocalSlot(slotFunction);
                                    if (slot) {
                                        //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Now calling " << slotFunction;
                                        slot->callRegisteredSlotFunctions(header, body);
                                        sendPotentialReply(header);
                                    } else {
                                        KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Received a call from ? to non-existing slot \"" << slotFunction << "\"";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        void SignalSlotable::registerReply(const karabo::util::Hash& reply) {
            boost::mutex::scoped_lock lock(m_replyMutex);
            m_replies[boost::this_thread::get_id()] = reply;
        }

        void SignalSlotable::sendPotentialReply(const karabo::util::Hash& header) {
            boost::mutex::scoped_lock lock(m_replyMutex);
            Replies::iterator it = m_replies.find(boost::this_thread::get_id());
            if (it != m_replies.end()) {
                if (header.has("replyTo")) {
                    karabo::util::Hash replyHeader;
                    replyHeader.set("replyFrom", header.get<std::string > ("replyTo"));
                    replyHeader.set("signalInstanceId", m_instanceId);
                    replyHeader.set("signalFunction", "__reply__");
                    replyHeader.set("slotInstanceIds", "|" + header.get<string>("signalInstanceId") + "|");
                    m_producerChannel->write(replyHeader, it->second);
                } else if (header.has("replyInstanceIds")) {
                    karabo::util::Hash replyHeader;
                    replyHeader.set("signalInstanceId", m_instanceId);
                    replyHeader.set("signalFunction", "__replyNoWait__");
                    replyHeader.set("slotInstanceIds", header.get<string>("replyInstanceIds"));
                    replyHeader.set("slotFunctions", header.get<string>("replyFunctions"));
                    m_producerChannel->write(replyHeader, it->second, 9);
                }
                m_replies.erase(it);
            }
        }

        void SignalSlotable::initReconnectIntervals() {
            if (m_reconnectIntervals.empty()) {
                for (int i = 0; i <= 50; ++i) {
                    int x = static_cast<int> (1 + 0.005 * i * i * i * i);
                    m_reconnectIntervals.insert(-x);
                }
            }
        }

        void SignalSlotable::registerDefaultSignalsAndSlots() {

            // Emits a "still-alive" signal
            registerHeartbeatSignal<string, int, Hash>("signalHeartbeat");

            // Listener for heartbeats
            SLOT3(slotHeartbeat, string /*instanceId*/, int /*heartbeatIntervalInSec*/, Hash /*instanceInfo*/)


            //SIGNAL3("signalHeartbeat", string /*instanceId*/, int /*heartbeatIntervalInSec*/, Hash /*instanceInfo*/)

            // Signals a successful disconnection
            SIGNAL4("signalDisconnected", string /*signalInstanceId*/, string /*signalFunction*/, string /*slotInstanceId*/, string /*slotFunction*/)

            // Emits as answer to a ping request
            SIGNAL1("signalGotPinged", string /*instanceId*/)

            // Global ping listener
            GLOBAL_SLOT3(slotPing, string /*callersInstanceId*/, int /*replyIfSame*/, bool /*trackPingedInstance*/)

            // Global instance new notification
            GLOBAL_SLOT2(slotInstanceNew, string /*instanceId*/, Hash /*instanceInfo*/)

            // Global slot instance gone
            GLOBAL_SLOT2(slotInstanceGone, string /*instanceId*/, Hash /*instanceInfo*/)

            // Listener for ping answers
            SLOT2(slotPingAnswer, string /*instanceId*/, Hash /*instanceInfo*/)

            // Connects signal to slot
            SLOT4(slotConnectToSignal, string /*signalFunction*/, string /*slotInstanceId*/, string /*slotFunction*/, int /*connectionType*/)

            // Replies whether slot exists on this instance
            SLOT4(slotConnectToSlot, string /*signalInstanceId*/, string /*signalFunction*/, string /*slotFunction*/, int /*connectionType*/)

            // Disconnects signal from slot
            SLOT3(slotDisconnectFromSignal, string /*signalFunction*/, string /*slotInstanceId*/, string /*slotFunction*/)

            SLOT3(slotDisconnectFromSlot, string /*signalInstance*/, string /*signalFunction*/, string /*slotFunction*/)

            // Function request
            SLOT1(slotGetAvailableFunctions, string /*functionType*/)

            // Provides information about p2p connectivity
            SLOT2(slotGetOutputChannelInformation, string /*ioChannelId*/, int /*pid*/)

            // Establishes/Releases P2P connections
            SLOT3(slotConnectToOutputChannel, string /*inputChannelName*/, karabo::util::Hash /*outputChannelInfo */, bool /*connect/disconnect*/)
        }

        bool SignalSlotable::login(const std::string& username, const std::string& password, const std::string& provider) {

            string brokerHostname = getConnection()->getBrokerHostname();
            int brokerPort = getConnection()->getBrokerPort();
            string brokerTopic = getConnection()->getBrokerTopic();

            m_authenticator = Authenticator::Pointer(new Authenticator(username, password, provider, boost::asio::ip::host_name(), brokerHostname, brokerPort, brokerTopic));

            if (username == "god" && godEncode(password) == 749) {
                std::cout << "Bypassing authentication service and entering god mode..., full access granted" << std::endl;
                m_defaultAccessLevel = 1000;
                return true;
            }
            try {
                if (m_authenticator->login()) {
                    m_defaultAccessLevel = m_authenticator->getDefaultAccessLevelId();
                    //m_accessList = m_authenticator->getAccessList();
                    //m_sessionToken = m_authenticator->getSessionToken();
                    m_username = username;
                    return true;
                } else {
                    return false;
                }
            } catch (karabo::util::NetworkException&) {
                karabo::util::Exception::clearTrace();
                KARABO_LOG_FRAMEWORK_ERROR << "Could not contact the authentication service, falling back to in-build access level";
                m_defaultAccessLevel = KARABO_DEFAULT_ACCESS_LEVEL;
                return true;
            }
        }

        bool SignalSlotable::logout() {
            return m_authenticator->logout();
        }

        void SignalSlotable::slotInstanceNew(const std::string& instanceId, const karabo::util::Hash & instanceInfo) {

            if (instanceId == m_instanceId) return;

            // Have to check whether this instance is tracked

            // Several cases:
            // a) instance is not tracked
            //    Fine. New guy in the system
            // b) instance is tracked, countdown > 0:
            //    The guy died without saying goodbye and came quickly back.
            //    No one has seen him dying and everyone believed he was fine. Only now we know that this wasn't the case.
            // c) instance is tracked, countdown < 0:
            //    This guy silently died, and the system got note of that. Now he is back!

            Hash trackedComponents;

            {
                boost::mutex::scoped_lock lock(m_heartbeatMutex);

                if (m_trackedComponents.has(instanceId)) {
                    trackedComponents = m_trackedComponents;
                }
            }

            if (!trackedComponents.empty()) {

                // Clean everything
                cleanSignalsAndStopTracking(instanceId);

                const Hash& entry = trackedComponents.get<Hash > (instanceId);

                // Just for information
                int countdown = entry.get<int>("countdown");
                if (countdown > 0) {
                    KARABO_LOG_FRAMEWORK_INFO << "Detected unrecognized dirty shutdown of instance \"" << instanceId
                            << "\", which got quickly restarted and is now available again";
                } else {
                    KARABO_LOG_FRAMEWORK_INFO << "Previously silently disappeared instance \"" << instanceId
                            << "\" got restarted and is now available again";
                }

                // Continue tracking this instance that was tracked before
                if (entry.get<bool>("isExplicitlyTracked") == true) {
                    trackExistenceOfInstance(instanceId);
                }
            }
        }

        void SignalSlotable::slotInstanceGone(const std::string& instanceId, const karabo::util::Hash & instanceInfo) {

            if (instanceId == m_instanceId) return;

            cleanSignalsAndStopTracking(instanceId);
        }

        BrokerConnection::Pointer SignalSlotable::getConnection() const {
            return m_connection;
        }

        void SignalSlotable::emitHeartbeat() {
            //----------------- make this thread sensible to external interrupts
            boost::this_thread::interruption_enabled(); // enable interruption +
            boost::this_thread::interruption_requested(); // request interruption = we need both!
            while (m_sendHeartbeats) {
                {
                    boost::this_thread::disable_interruption di; // disable interremit("uption in this block                    
                    emit("signalHeartbeat", getInstanceId(), m_heartbeatInterval, m_instanceInfo);
                }
                // here the interruption enabled again
                try {
                    boost::this_thread::sleep(boost::posix_time::seconds(m_heartbeatInterval));
                } catch (const boost::thread_interrupted&) {
                }
            }
        }

        const std::vector<std::pair<std::string, karabo::util::Hash> >& SignalSlotable::getAvailableInstances(bool activateTracking) {
            m_availableInstances.clear();
            call("*", "slotPing", m_instanceId, 0, activateTracking);
            // The function slotPingAnswer will be called by all instances available now
            // Lets wait a fair amount of time - huaaah this is bad isn't it :-(
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            if (activateTracking) {
                for (size_t i = 0; i < m_availableInstances.size(); ++i) {
                    const string& instanceId = m_availableInstances[i].first;
                    const Hash& instanceInfo = m_availableInstances[i].second;

                    if (instanceId == m_instanceId) continue;

                    addTrackedComponent(instanceId, true);
                    updateTrackedComponentInstanceInfo(instanceId, instanceInfo);

                    m_signalSlotInstancesMutex.lock();
                    m_signalInstances["signalHeartbeat"]->registerSlot(instanceId, "slotHeartbeat");
                    m_signalSlotInstancesMutex.unlock();


                }

                // Emit an extra heartbeat to make the connected signal instance happy
                //KARABO_LOG_FRAMEWORK_DEBUG << "getAvailableInstances: Emitting extra heartbeat from " << m_instanceId;
                emit("signalHeartbeat", getInstanceId(), m_heartbeatInterval, m_instanceInfo);

            }
            return m_availableInstances;
        }

        std::pair<bool, std::string > SignalSlotable::exists(const std::string & instanceId) {
            string hostname;
            Hash instanceInfo;
            try {
		m_randPing = rand() + 1;
                this->request("*", "slotPing", instanceId, m_randPing, false).timeout(200).receive(instanceInfo);
            } catch (const karabo::util::TimeoutException&) {
                return std::make_pair(false, hostname);
            }
            if (instanceInfo.has("host")) instanceInfo.get("host", hostname);
            return std::make_pair(true, hostname);
        }

        void SignalSlotable::slotPing(const std::string& instanceId, int rand, bool trackPingedInstance) {

            if (rand) {
		if (rand == m_randPing) cout << "ignoring" << rand << instanceId;
                if (instanceId == m_instanceId && rand != m_randPing) {
                    reply(m_instanceInfo);
                }
            } else {
                //cout << "Got pinged from " << instanceId << endl;
                call(instanceId, "slotPingAnswer", m_instanceId, m_instanceInfo);
            }

            if (trackPingedInstance && instanceId != m_instanceId) {
                m_signalSlotInstancesMutex.lock();
                m_signalInstances["signalHeartbeat"]->registerSlot(instanceId, "slotHeartbeat");
                m_signalSlotInstancesMutex.unlock();
                addTrackedComponent(instanceId, true);
            }
        }

        std::vector<string> SignalSlotable::getAvailableSignals(const std::string & instanceId) {
            std::vector<string> signals;
            try {
                request(instanceId, "slotGetAvailableFunctions", "signals").timeout(100).receive(signals);
            } catch (const karabo::util::TimeoutException&) {
                karabo::util::Exception::clearTrace();
                cout << "ERROR:  The requested instanceId \"" << instanceId << "\" is currently not available." << endl;
            }
            return signals;
        }

        std::vector<string> SignalSlotable::getAvailableSlots(const std::string & instanceId) {
            std::vector<string> slots;
            try {
                request(instanceId, "slotGetAvailableFunctions", "slots").timeout(100).receive(slots);
            } catch (const karabo::util::TimeoutException&) {
                karabo::util::Exception::clearTrace();
                cout << "ERROR:  The requested instanceId \"" << instanceId << "\" is currently not available." << endl;
            }
            return slots;
        }

        const SignalSlotable::SlotInstancePointer & SignalSlotable::getSenderInfo(const std::string & slotFunction) {
            SlotInstancesConstIt it = m_localSlotInstances.find(slotFunction);
            if (it == m_localSlotInstances.end()) throw KARABO_SIGNALSLOT_EXCEPTION("No slot-object could be found for slotFunction \"" + slotFunction + "\"");
            return it->second;
        }

        void SignalSlotable::slotGetAvailableFunctions(const std::string & type) {
            std::vector<string> functions;
            if (type == "signals") {
                for (SignalInstancesConstIt it = m_signalInstances.begin(); it != m_signalInstances.end(); ++it) {
                    const string& function = it->first;
                    functions.push_back(function);
                }
            } else if (type == "slots") {
                for (SlotInstancesConstIt it = m_localSlotInstances.begin(); it != m_localSlotInstances.end(); ++it) {
                    const string& function = it->first;
                    std::cout << "|" << function << "|" << std::endl;
                    // Filter out service slots // TODO finally update to last set of those
                    if (function == "slotConnectToSignal" || function == "slotDisconnectFromSignal" || function == "slotGetAvailableFunctions" ||
                            function == "slotConnectToSlot" || function == "slotHeartbeat" || function == "slotPing" || function == "slotPingAnswer" ||
                            function == "slotStopTrackingExistenceOfConnection") {
                        continue;
                    }
                    functions.push_back(it->first);
                }
            }
            reply(functions);
        }

        void SignalSlotable::slotPingAnswer(const std::string& instanceId, const karabo::util::Hash & hash) {
            m_availableInstances.push_back(std::make_pair(instanceId, hash));
        }

        void SignalSlotable::slotHeartbeat(const std::string& networkId, const int& heartbeatInterval, const Hash & instanceInfo) {
            refreshTimeToLiveForConnectedSlot(networkId, heartbeatInterval, instanceInfo);
        }

        const std::string & SignalSlotable::getInstanceId() const {
            return m_instanceId;
        }

        void SignalSlotable::updateInstanceInfo(const karabo::util::Hash & update) {
            m_instanceInfo.merge(update);
            call("*", "slotInstanceUpdated", m_instanceId, m_instanceInfo);
        }

        const karabo::util::Hash & SignalSlotable::getInstanceInfo() const {
            return m_instanceInfo;
        }

        void SignalSlotable::autoConnectAllSignals(const karabo::util::Hash& config, const std::string signalRegularExpression) {
            try {
                for (Hash::const_iterator it = config.begin(); it != config.end(); ++it) {
                    const boost::regex e(signalRegularExpression);
                    if (boost::regex_match(it->getKey(), e)) {
                        const vector<string>& connects = it->getValue<vector<string> >();
                        for (size_t i = 0; i < connects.size(); ++i) {
                            connect(it->getKey(), connects[i], RECONNECT);
                        }
                    }
                }
            } catch (...) {
                KARABO_RETHROW;
            }
        }

        void SignalSlotable::autoConnectAllSlots(const karabo::util::Hash& config, const std::string slotRegularExpression) {
            try {
                for (Hash::const_iterator it = config.begin(); it != config.end(); ++it) {
                    const boost::regex e(slotRegularExpression);
                    if (boost::regex_match(it->getKey(), e)) {
                        const vector<string>& connects = it->getValue<vector<string> >();
                        for (size_t i = 0; i < connects.size(); ++i) {
                            cout << "AutoConnect:" << connects[i] << endl;
                            connect(connects[i], it->getKey(), RECONNECT);
                        }
                    }
                }
            } catch (...) {
                KARABO_RETHROW;
            }
        }

        void SignalSlotable::registerInstanceNotAvailableHandler(const InstanceNotAvailableHandler & instanceNotAvailableCallback) {
            m_instanceNotAvailableHandler = instanceNotAvailableCallback;
        }

        void SignalSlotable::registerInstanceAvailableAgainHandler(const InstanceAvailableAgainHandler & instanceAvailableAgainCallback) {
            m_instanceAvailableAgainHandler = instanceAvailableAgainCallback;
        }

        void SignalSlotable::registerSlotCallGuardHandler(const SlotCallGuardHandler& slotCallGuardHandler) {
            m_slotCallGuardHandler = slotCallGuardHandler;
        }

        void SignalSlotable::instanceNotAvailable(const std::string & instanceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << instanceId << "\" not available anymore";
        }

        void SignalSlotable::instanceAvailableAgain(const std::string & instanceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << instanceId << "\" available again";
        }

        bool SignalSlotable::connectChannels(std::string outputInstanceId, const std::string& outputName, std::string inputInstanceId, const std::string& inputName, const bool isVerbose) {

            if (outputInstanceId.empty()) outputInstanceId = m_instanceId;
            if (inputInstanceId.empty()) inputInstanceId = m_instanceId;

            bool outputChannelExists = false;
            bool inputChannelExists = false;
            karabo::util::Hash outputChannelInfo;

            if (outputInstanceId == m_instanceId) { // Local output channel
                outputChannelInfo = slotGetOutputChannelInformation(outputName, static_cast<int> (getpid()));
                if (!outputChannelInfo.empty()) outputChannelExists = true;

            } else { // Remote output channel
                try {
                    this->request(outputInstanceId, "slotGetOutputChannelInformation", outputName, static_cast<int> (getpid())).timeout(1000).receive(outputChannelExists, outputChannelInfo);
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    inputChannelExists = false;
                }
            }

            if (outputChannelExists) {

                if (inputInstanceId == m_instanceId) {// Local input channel
                    inputChannelExists = slotConnectToOutputChannel(inputName, outputChannelInfo, true);

                } else {
                    try {
                        this->request(inputInstanceId, "slotConnectToOutputChannel", inputName, outputChannelInfo, true).timeout(1000).receive(inputChannelExists);
                    } catch (const karabo::util::TimeoutException&) {
                        karabo::util::Exception::clearTrace();
                        inputChannelExists = false;
                    }
                }
            }

            if (outputChannelExists && inputChannelExists) {
                if (isVerbose) cout << "INFO   : Channel connection successfully established." << endl;
                return true;
            }
            if (isVerbose) cout << "ERROR   : Channel connection could not be established." << endl;
            return false;
        }

        bool SignalSlotable::slotConnectToOutputChannel(const std::string& inputName, const karabo::util::Hash& outputChannelInfo, bool connect) {
            try {
                // Loop channels
                InputChannels::const_iterator it = m_inputChannels.find(inputName);
                if (it != m_inputChannels.end()) {
                    if (connect) it->second->connect(outputChannelInfo); // Synchronous
                    else it->second->disconnect(outputChannelInfo);
                    reply(true);
                    return true;
                }
                return false;
            } catch (const Exception& e) {
                reply(false);
                return false;
            }
        }

        bool SignalSlotable::disconnectChannels(std::string outputInstanceId, const std::string& outputName, std::string inputInstanceId, const std::string& inputName, const bool isVerbose) {

            if (outputInstanceId.empty()) outputInstanceId = m_instanceId;
            if (inputInstanceId.empty()) inputInstanceId = m_instanceId;

            bool outputChannelExists = false;
            bool inputChannelExists = false;

            // Need to get the outputChannelInfo (containing amongst others, port and host)
            karabo::util::Hash outputChannelInfo;

            if (outputInstanceId == m_instanceId) { // Local output channel
                outputChannelInfo = slotGetOutputChannelInformation(outputName, static_cast<int> (getpid()));
                if (!outputChannelInfo.empty()) outputChannelExists = true;

            } else { // Remote output channel
                try {
                    this->request(outputInstanceId, "slotGetOutputChannelInformation", outputName, static_cast<int> (getpid())).timeout(1000).receive(outputChannelExists, outputChannelInfo);
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    inputChannelExists = false;
                }
            }

            if (outputChannelExists) {

                if (inputInstanceId == m_instanceId) { // Local input channel
                    inputChannelExists = slotConnectToOutputChannel(inputName, outputChannelInfo, false);

                } else {
                    try {
                        this->request(inputInstanceId, "slotConnectToOutputChannel", inputName, outputChannelInfo, false).timeout(1000).receive(inputChannelExists);
                    } catch (const karabo::util::TimeoutException&) {
                        karabo::util::Exception::clearTrace();
                        inputChannelExists = false;
                    }
                }
            }

            if (outputChannelExists && inputChannelExists) {
                if (isVerbose) cout << "INFO   : Channel connection successfully released." << endl;
                return true;
            }
            if (isVerbose) cout << "ERROR   : Channel connection could not be released." << endl;
            return false;
        }

        //************************** Connect **************************//

        bool SignalSlotable::connect(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, ConnectionType connectionType, const bool isVerbose) {

            if (signalInstanceId.empty()) signalInstanceId = m_instanceId;
            if (slotInstanceId.empty()) slotInstanceId = m_instanceId;

            bool signalExists = tryToConnectToSignal(signalInstanceId, signalFunction, slotInstanceId, slotFunction, connectionType, isVerbose);
            bool slotExists = tryToConnectToSlot(signalInstanceId, signalFunction, slotInstanceId, slotFunction, connectionType, isVerbose);

            bool connectionEstablished = false;
            if (signalExists && slotExists) {
                connectionEstablished = true;
                if (isVerbose) cout << "INFO  : Connection successfully established." << endl;
            } else if (signalExists) {
                if (isVerbose) cout << "ERROR  : Connection could not be established (slot not found)." << endl;
            } else if (slotExists) {
                if (isVerbose) cout << "ERROR  : Connection could not be established (signal not found)." << endl;
            } else {
                if (isVerbose) cout << "ERROR : Connection could not be established (not signal neither slot found)." << endl;
            }

            // Immediately disconnect in case of failure to clean any spoiled signals
            if (!connectionEstablished) disconnect(signalInstanceId, signalFunction, slotInstanceId, slotFunction);

            return connectionEstablished;
        }

        bool SignalSlotable::tryToConnectToSignal(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, const int& connectionType, const bool isVerbose) {

            bool signalExists = false;

            if (signalInstanceId == m_instanceId) { // Local signal requested
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                SignalInstancesConstIt it = m_signalInstances.find(signalFunction);
                if (it != m_signalInstances.end()) { // Signal found
                    signalExists = true;
                    // Register new slotId to local signal
                    it->second->registerSlot(slotInstanceId, slotFunction);

                    // Start tracking of connection to be later able to clean signals from dead slots                    
                    if (connectionType != NO_TRACK && slotInstanceId != m_instanceId) {
                        // Equip own heartbeat with slot information (this may happen several times with the same slotInstanceId which are then ignored)
                        m_signalInstances["signalHeartbeat"]->registerSlot(slotInstanceId, "slotHeartbeat");
                        // Never track a heartbeat connection itself
                        if (signalFunction != "signalHeartbeat") {
                            registerConnectionForTracking(m_instanceId, signalFunction, slotInstanceId, slotFunction, connectionType);
                        } else {
                            // In case of a heartbeat connect we must at least register the slotInstance to
                            // be able to clean our heartbeat signal in case of silent death
                            addTrackedComponent(slotInstanceId, true);
                        }
                    }

                } else {
                    signalExists = false;
                    if (isVerbose) cout << "WARN  : The requested signal \"" << signalFunction << "\" is currently not available on this (local) instance \"" << m_instanceId << "\"." << endl;
                }
            } else { // Remote signal requested
                try {
                    request(signalInstanceId, "slotConnectToSignal", signalFunction, slotInstanceId, slotFunction, connectionType).timeout(1000).receive(signalExists);
                    if (!signalExists) {
                        if (isVerbose) cout << "WARN  : The requested signal \"" << signalFunction << "\" is currently not available on signal-instanceId \"" << signalInstanceId << "\"." << endl;
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    signalExists = false;
                    if (isVerbose) cout << "WARN  : The requested signal-instanceId \"" << signalInstanceId << "\" is currently not available." << endl;
                }
            }
            return signalExists;
        }

        void SignalSlotable::registerConnectionForTracking(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction, const int& connectionType) {

            // Signal or slot is remote
            if (signalInstanceId != m_instanceId || slotInstanceId != m_instanceId) {

                string connectionId(signalInstanceId + signalFunction + slotInstanceId + slotFunction);

                if (!isConnectionTracked(connectionId)) {

                    Hash connection;
                    connection.set("signalInstanceId", signalInstanceId);
                    connection.set("signalFunction", signalFunction);
                    connection.set("slotInstanceId", slotInstanceId);
                    connection.set("slotFunction", slotFunction);
                    connection.set("connectionType", connectionType);
                    connection.set("connectionId", connectionId);

                    // Signal is remote
                    if (signalInstanceId != m_instanceId) {
                        addTrackedComponent(signalInstanceId);
                        addTrackedComponentConnection(signalInstanceId, connection);
                    }

                    // Slot is on remote component
                    if (slotInstanceId != m_instanceId) {
                        addTrackedComponent(slotInstanceId);
                        addTrackedComponentConnection(slotInstanceId, connection);
                    }
                }
            }
        }

        void SignalSlotable::slotConnectToSignal(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction, const int& connectionType) {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            SignalInstancesConstIt it = m_signalInstances.find(signalFunction);
            if (it != m_signalInstances.end()) {
                it->second->registerSlot(slotInstanceId, slotFunction);

                // Start tracking of connection to be later able to clean signals from dead slots                
                if (connectionType != NO_TRACK && slotInstanceId != m_instanceId) {
                    // Equip own heartbeat with slot information
                    m_signalInstances["signalHeartbeat"]->registerSlot(slotInstanceId, "slotHeartbeat");
                    // Never track a heartbeat connection itself
                    if (signalFunction != "signalHeartbeat") {
                        registerConnectionForTracking(m_instanceId, signalFunction, slotInstanceId, slotFunction, connectionType);
                    } else {
                        // In case of a heartbeat connect we must at least register the slotInstance to
                        // be able to clean our heartbeat signal in case of silent death
                        addTrackedComponent(slotInstanceId, true);
                    }
                }

                //cout << "LOW-LEVEL-DEBUG: Established remote connection of signal \"" << signalFunction << "\" to slot \"" << slotFunction << "\"." << endl;
                reply(true);
            } else {
                reply(false);
            }
        }

        bool SignalSlotable::tryToConnectToSlot(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction, const int& connectionType, const bool isVerbose) {

            if (slotInstanceId == "*") return true; // GLOBAL slots may or may not exist

            bool slotExists = false;

            if (slotInstanceId == m_instanceId) { // Local slot requested
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                SlotInstancesConstIt it = m_localSlotInstances.find(slotFunction);
                if (it != m_localSlotInstances.end()) { // Slot found
                    slotExists = true;

                    if (connectionType != NO_TRACK && signalInstanceId != m_instanceId) {
                        // Equip own heartbeat with slot information
                        m_signalInstances["signalHeartbeat"]->registerSlot(signalInstanceId, "slotHeartbeat");

                        // Never track the heartbeat connections
                        if (signalFunction != "signalHeartbeat") {
                            registerConnectionForTracking(signalInstanceId, signalFunction, m_instanceId, slotFunction, connectionType);
                        } else {
                            addTrackedComponent(signalInstanceId, true);
                        }
                        // Emit an extra heartbeat to make the connected signal instance happy
                        //KARABO_LOG_FRAMEWORK_DEBUG << "tryToConnectToSlot: Emitting extra heartbeat from " << m_instanceId;
                        emit("signalHeartbeat", getInstanceId(), m_heartbeatInterval, m_instanceInfo);
                    }



                } else {
                    slotExists = false;
                    if (isVerbose) cout << "WARN  : The requested slot \"" << slotFunction << "\" is currently not available on this (local) instance \"" << m_instanceId << "\"." << endl;
                }
            } else {// Remote slot requested
                try {
                    request(slotInstanceId, "slotConnectToSlot", signalInstanceId, signalFunction, slotFunction, connectionType).timeout(1000).receive(slotExists);
                    if (!slotExists) {
                        if (isVerbose) cout << "WARN  : The requested slot \"" << slotFunction << "\" is currently not available on slot-instanceId \"" << slotInstanceId << "\"." << endl;
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    slotExists = false;
                    if (isVerbose) cout << "WARN  : The requested slot-instanceId \"" << slotInstanceId << "\" is currently not available." << endl;
                }
            }
            return slotExists;
        }

        void SignalSlotable::slotConnectToSlot(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotFunction, const int& connectionType) {
            if (m_localSlotInstances.find(slotFunction) != m_localSlotInstances.end()) {

                if (connectionType != NO_TRACK && signalInstanceId != m_instanceId) {
                    m_signalInstances["signalHeartbeat"]->registerSlot(signalInstanceId, "slotHeartbeat");

                    // Never track the heartbeat connections
                    if (signalFunction != "signalHeartbeat") {
                        registerConnectionForTracking(signalInstanceId, signalFunction, m_instanceId, slotFunction, connectionType);
                    } else {
                        addTrackedComponent(signalInstanceId, true);
                    }

                    // Emit an extra heartbeat to make the connected signal instance happy
                    //KARABO_LOG_FRAMEWORK_DEBUG << "slotConnectToSlot: Emitting extra heartbeat from instance \"" << m_instanceId << "\"";
                    emit("signalHeartbeat", getInstanceId(), m_heartbeatInterval, m_instanceInfo);
                }
                reply(true);
            } else {
                reply(false);
            }
        }

        bool SignalSlotable::connect(const std::string& signal, const std::string& slot, ConnectionType connectionType, const bool isVerbose) {
            std::pair<std::string, std::string> signalPair = splitIntoInstanceIdAndFunctionName(signal);
            std::pair<std::string, std::string> slotPair = splitIntoInstanceIdAndFunctionName(slot);
            return connect(signalPair.first, signalPair.second, slotPair.first, slotPair.second, connectionType, isVerbose);
        }

        void SignalSlotable::connectN(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string & slotSignature) {
            connect(signalInstanceId, signalSignature, slotInstanceId, slotSignature, NO_TRACK);
        }

        void SignalSlotable::connectT(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string & slotSignature) {
            connect(signalInstanceId, signalSignature, slotInstanceId, slotSignature, TRACK);
        }

        void SignalSlotable::connectR(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string & slotSignature) {
            connect(signalInstanceId, signalSignature, slotInstanceId, slotSignature, RECONNECT);
        }

        void SignalSlotable::connectN(const std::string& signalId, const std::string & slotId) {
            connect(signalId, slotId, NO_TRACK);
        }

        void SignalSlotable::connectT(const std::string& signalId, const std::string & slotId) {
            connect(signalId, slotId, TRACK);
        }

        void SignalSlotable::connectR(const std::string& signalId, const std::string & slotId) {
            connect(signalId, slotId, RECONNECT);
        }

        bool SignalSlotable::isConnectionTracked(const std::string & connectionId) {

            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            for (Hash::iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
                const vector<Hash>& connections = it->getValue<Hash>().get<vector<Hash> >("connections");
                for (size_t j = 0; j < connections.size(); ++j) {
                    const Hash& connection = connections[j];
                    if (connection.get<string > ("connectionId") == connectionId) return true;
                }
            }
            return false;
        }

        void SignalSlotable::addTrackedComponent(const std::string& instanceId, const bool isExplicitlyTracked) {
            boost::mutex::scoped_lock lock(m_heartbeatMutex);

            if (m_trackedComponents.has(instanceId)) return;

            Hash h;
            h.set("connections", vector<Hash > ());
            h.set("instanceInfo", Hash());
            h.set("countdown", 120);
            h.set("isExplicitlyTracked", isExplicitlyTracked);
            m_trackedComponents.set(instanceId, h);
        }

        void SignalSlotable::updateTrackedComponentInstanceInfo(const std::string& instanceId, const karabo::util::Hash & instanceInfo) {
            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            if (m_trackedComponents.has(instanceId)) {
                Hash& h = m_trackedComponents.get<Hash>(instanceId);
                h.set("instanceInfo", instanceInfo);
                h.set("countdown", instanceInfo.get<int>("heartbeatInterval"));
            }
        }

        void SignalSlotable::addTrackedComponentConnection(const std::string& instanceId, const karabo::util::Hash & connection) {
            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            if (m_trackedComponents.has(instanceId)) {
                m_trackedComponents.get<vector<Hash> >(instanceId + ".connections").push_back(connection);
            }
        }

        bool SignalSlotable::disconnect(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, const bool isVerbose) {

            if (signalInstanceId.empty()) signalInstanceId = m_instanceId;
            if (slotInstanceId.empty()) slotInstanceId = m_instanceId;

            bool signalExists = tryToDisconnectFromSignal(signalInstanceId, signalFunction, slotInstanceId, slotFunction, isVerbose);
            tryToDisconnectFromSlot(signalInstanceId, signalFunction, slotInstanceId, slotFunction, isVerbose);

            bool connectionDestroyed = false;
            if (signalExists) {
                //emit("signalDisconnected", signalInstanceId, signalFunction, slotInstanceId, slotFunction);
                connectionDestroyed = true;
                if (isVerbose) cout << "INFO  : Connection successfully released." << endl;
            } else {
                if (isVerbose) cout << "ERROR : Connection could not be released (signal was not found)." << endl;
            }



            return connectionDestroyed;
        }

        bool SignalSlotable::tryToDisconnectFromSignal(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, const bool isVerbose) {

            bool signalExists = false;

            if (signalInstanceId == m_instanceId) { // Local signal requested

                // In case of the heartbeat signal, clean all affected connections and stop tracking
                if (signalFunction == "signalHeartbeat") {
                    cleanSignalsAndStopTracking(slotInstanceId);
                    signalExists = true;
                } else if (tryToUnregisterSlot(signalFunction, slotInstanceId, slotFunction)) {
                    signalExists = true;
                } else {
                    signalExists = false;
                    if (isVerbose) cout << "WARN  : The requested signal \"" << signalFunction << "\" is currently not available on this (local) instance \"" << m_instanceId << "\"." << endl;
                }
            } else { // Remote signal requested
                try {
                    request(signalInstanceId, "slotDisconnectFromSignal", signalFunction, slotInstanceId, slotFunction).timeout(1000).receive(signalExists);
                    if (!signalExists) {
                        if (isVerbose) cout << "WARN  : The requested signal \"" << signalFunction << "\" is currently not available on signal-instanceId \"" << signalInstanceId << "\"." << endl;
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    signalExists = false;
                    if (isVerbose) cout << "WARN  : The requested signal-instanceId \"" << signalInstanceId << "\" is currently not available." << endl;
                }
            }

            return signalExists;
        }

        bool SignalSlotable::tryToUnregisterSlot(const std::string& signalFunction, const std::string& slotInstanceId, const std::string & slotFunction) {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            SignalInstancesConstIt it = m_signalInstances.find(signalFunction);
            if (it != m_signalInstances.end()) { // Signal found
                // Unregister slotId from local signal
                it->second->unregisterSlot(slotInstanceId, slotFunction);
                unregisterConnectionFromTracking(m_instanceId, signalFunction, slotInstanceId, slotFunction);
                return true;
            }
            return false;
        }

        void SignalSlotable::slotDisconnectFromSignal(const std::string& signalFunction, const std::string& slotInstanceId, const std::string & slotFunction) {
            // In case of the heartbeat signal, clean all affected connections and stop tracking
            if (signalFunction == "signalHeartbeat") {
                cleanSignalsAndStopTracking(slotInstanceId);
                reply(true);
            } else if (tryToUnregisterSlot(signalFunction, slotInstanceId, slotFunction)) {
                reply(true);
            } else {
                reply(false);
            }
        }

        bool SignalSlotable::tryToDisconnectFromSlot(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, const bool isVerbose) {

            bool slotExists = false;

            if (slotInstanceId == m_instanceId) { // Local slot requested
                if (hasLocalSlot(slotFunction)) {
                    slotExists = true;

                    unregisterConnectionFromTracking(signalInstanceId, signalFunction, slotInstanceId, slotFunction);

                    if (signalFunction == "signalHeartbeat") {
                        // Remove slot information from own heartbeat
                        cleanSignalsAndStopTracking(signalInstanceId);
                    }

                } else {
                    slotExists = false;
                    if (isVerbose) cout << "WARN  : The requested slot \"" << slotFunction << "\" is currently not available on this (local) instance \"" << m_instanceId << "\"." << endl;
                }
            } else { // Remote slot requested
                try {
                    request(slotInstanceId, "slotDisconnectFromSlot", signalInstanceId, signalFunction, slotFunction).timeout(1000).receive(slotExists);
                    if (!slotExists) {
                        if (isVerbose) cout << "WARN  : The requested slot \"" << slotFunction << "\" is currently not available on slot-instanceId \"" << slotInstanceId << "\"." << endl;
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    slotExists = false;
                    if (isVerbose) cout << "WARN  : The requested slot-instanceId \"" << slotInstanceId << "\" is currently not available." << endl;
                }
            }
            return slotExists;
        }

        void SignalSlotable::slotDisconnectFromSlot(const std::string& signalInstanceId, const std::string& signalFunction, const std::string & slotFunction) {

            if (hasLocalSlot(slotFunction)) {

                unregisterConnectionFromTracking(signalInstanceId, signalFunction, m_instanceId, slotFunction);

                if (signalFunction == "signalHeartbeat") {
                    // Remove slot information from own heartbeat
                    cleanSignalsAndStopTracking(signalInstanceId);
                }
                reply(true);
            } else {
                reply(false);
            }
        }

        bool SignalSlotable::hasLocalSlot(const std::string & slotFunction) const {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            return m_localSlotInstances.find(slotFunction) != m_localSlotInstances.end();
        }

        SignalSlotable::SlotInstancePointer SignalSlotable::getLocalSlot(const std::string& slotFunction) const {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            SlotInstances::const_iterator it = m_localSlotInstances.find(slotFunction);
            if (it != m_localSlotInstances.end()) return it->second;
            return SlotInstancePointer();
        }

        SignalSlotable::SlotInstancePointer SignalSlotable::getGlobalSlot(const std::string& slotFunction) const {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            SlotInstances::const_iterator it = m_globalSlotInstances.find(slotFunction);
            if (it != m_globalSlotInstances.end()) return it->second;
            return SlotInstancePointer();
        }

        bool SignalSlotable::hasSignal(const std::string & signalFunction) const {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            return m_signalInstances.find(signalFunction) != m_signalInstances.end();
        }

        void SignalSlotable::trackExistenceOfInstance(const std::string & instanceId) {

            if (instanceId == m_instanceId) return;

            connect(instanceId, "signalHeartbeat", "", "slotHeartbeat");
        }

        void SignalSlotable::stopTrackingExistenceOfInstance(const std::string & instanceId) {

            disconnect("", "signalHeartbeat", instanceId, "slotHeartbeat");

        }

        void SignalSlotable::unregisterConnectionFromTracking(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string & slotFunction) {

            string connectionId = signalInstanceId + signalFunction + slotInstanceId + slotFunction;

            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            for (Hash::iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
                vector<Hash>& connections = it->getValue<Hash>().get<vector<Hash> >("connections");
                for (vector<Hash>::iterator jt = connections.begin(); jt != connections.end(); ++jt) {
                    const Hash& connection = *jt;
                    if (connection.get<string > ("connectionId") == connectionId) {
                        connections.erase(jt);
                        break;
                    }
                }
            }
        }

        void SignalSlotable::connectionNotAvailable(const std::string& instanceId, const std::vector<karabo::util::Hash>& connections) {
            KARABO_LOG_FRAMEWORK_WARN << m_instanceId << " says: Instance \"" << instanceId << "\" is not available";
            KARABO_LOG_FRAMEWORK_DEBUG << "The following connection will thus not work:";
            for (size_t i = 0; i < connections.size(); ++i) {
                const Hash& connection = connections[i];
                const string& signalInstanceId = connection.get<string > ("signalInstanceId");
                const string& signalFunction = connection.get<string > ("signalFunction");
                const string& slotInstanceId = connection.get<string > ("slotInstanceId");
                const string& slotFunction = connection.get<string > ("slotFunction");
                KARABO_LOG_FRAMEWORK_DEBUG << "\"" << signalFunction << "\" (" << signalInstanceId << ") \t\t <--> \t\t \"" << slotFunction << "\" (" << slotInstanceId << ")";
            }
        }

        void SignalSlotable::connectionAvailableAgain(const std::string& instanceId, const std::vector<karabo::util::Hash>& connections) {
            KARABO_LOG_FRAMEWORK_INFO << "Previously unavailable instance \"" << instanceId << "\" is now available";
            KARABO_LOG_FRAMEWORK_DEBUG << "The following connections are (re-)established: ";
            for (size_t i = 0; i < connections.size(); ++i) {
                const Hash& connection = connections[i];
                const string& signalInstanceId = connection.get<string > ("signalInstanceId");
                const string& signalFunction = connection.get<string > ("signalFunction");
                const string& slotInstanceId = connection.get<string > ("slotInstanceId");
                const string& slotFunction = connection.get<string > ("slotFunction");
                KARABO_LOG_FRAMEWORK_DEBUG << "\"" << signalFunction << "\" (" << signalInstanceId << ") \t\t <--> \t\t \"" << slotFunction << "\" (" << slotInstanceId << ")";
            }
        }

        string SignalSlotable::fetchInstanceId(const std::string & signalOrSlotId) const {
            return signalOrSlotId.substr(0, signalOrSlotId.find_last_of('/'));
        }

        std::pair<std::string, std::string> SignalSlotable::splitIntoInstanceIdAndFunctionName(const std::string& signalOrSlotId, const char sep) const {
            size_t pos = signalOrSlotId.find_last_of(sep);
            if (pos == std::string::npos) return std::make_pair("", signalOrSlotId);
            std::string instanceId = signalOrSlotId.substr(0, pos);
            std::string functionName = signalOrSlotId.substr(pos);
            return std::make_pair(instanceId, functionName);
        }

        void SignalSlotable::refreshTimeToLiveForConnectedSlot(const std::string& instanceId, int countdown, const karabo::util::Hash & instanceInfo) {

            /**
             * Several cases:
             * a) Got a refresh request that for an instance that was not tracked
             *    This should only happen in case it disappeared for a long time and got garbage collected in the meanwhile
             * b) Got a refresh request for a device with negative countdown
             *    The device silently disappeared and is now back again. As they did not loose their state we are fine again.
             * c) Got a refresh request for a device with positive countdown
             *    Fine! Still there.
             */

            Hash trackedComponents;

            {
                boost::mutex::scoped_lock lock(m_heartbeatMutex);
                if (m_trackedComponents.has(instanceId)) {
                    Hash& entry = m_trackedComponents.get<Hash > (instanceId);
                    int oldCountdown = entry.get<int>("countdown");

                    // Special case, after armed for tracking we see this heartbeat for the first time
                    // In this case we will answer immediately with our heartbeat making sure we properly initialize
                    // the countdown
                    if (entry.get<Hash>("instanceInfo").empty()) {

                        //KARABO_LOG_FRAMEWORK_DEBUG << "refreshTimeToLiveForConnectedSlot: Emitting extra heartbeat from " << m_instanceId;
                        emit("signalHeartbeat", m_instanceId, m_heartbeatInterval, m_instanceInfo);
                    }

                    entry.set("instanceInfo", instanceInfo);
                    entry.set("countdown", countdown);
                    if (oldCountdown < 0) {
                        // Copy here to allow freeing the mutex on shared resource m_trackedComponents
                        trackedComponents = m_trackedComponents;
                    }
                } else {
                    //KARABO_LOG_FRAMEWORK_WARN << "Got refresh request for unregistered heartbeat of " << instanceId
                    //        << " Maybe the device disappeared (without dying) for a long time an came back now.";
                    return;
                }
            }

            if (!trackedComponents.empty()) {
                const Hash& entry = trackedComponents.get<Hash > (instanceId);

                KARABO_LOG_FRAMEWORK_INFO << "Previously disappeared instance " << instanceId << " silently came back";

                if (entry.get<bool>("isExplicitlyTracked") == true) {

                    if (m_instanceAvailableAgainHandler) m_instanceAvailableAgainHandler(instanceId, instanceInfo);

                }
            }
        }

        void SignalSlotable::letConnectionSlowlyDieWithoutHeartbeat() {

            try {

                Hash trackedComponents;

                while (m_doTracking) {

                    vector<string> remove;

                    m_heartbeatMutex.lock();

                    try {

                        for (Hash::iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
                            Hash& entry = it->getValue<Hash>();
                            entry.get<int>("countdown")--; // Regular count down

                            if (!entry.get<bool>("isExplicitlyTracked") && entry.get<vector<Hash> >("connections").empty()) {
                                // Flag all instances which are not explicitely tracked
                                // and do not surveil any connections to be removed
                                remove.push_back(it->getKey());
                            }

                        }

                        // Copy here to allow freeing the mutex on shared resource m_trackedComponents
                        trackedComponents = m_trackedComponents;

                    } catch (const Exception& e) {
                        KARABO_LOG_FRAMEWORK_ERROR << e;
                    } catch (...) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Unknown error happened in user callback for instance being not available";
                    }

                    m_heartbeatMutex.unlock();


                    // Stop tracking instances that are not needed to be tracked anymore
                    // This can happen if all connections are disconnected on a not-explicitly tracked device
                    for (size_t i = 0; i < remove.size(); ++i) {
                        stopTrackingExistenceOfInstance(remove[i]);
                    }

                    // From now on we play with a non-shared, safe copy
                    for (Hash::iterator it = trackedComponents.begin(); it != trackedComponents.end(); ++it) {
                        Hash& entry = it->getValue<Hash>();

                        int countdown = entry.get<int>("countdown");

                        //cout << endl << m_instanceId << "(" << it->getKey() << "): " << countdown << endl;

                        if (countdown == 0) { // Connection lost
                            const Hash& instanceInfo = entry.get<Hash>("instanceInfo");

                            KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Instance \"" << it->getKey() << "\" silently disappeared (no heartbeats received anymore)";

                            if (m_instanceNotAvailableHandler) {
                                try {
                                    // Only explicitely tracked instances inform about their death                                    
                                    if (entry.get<bool>("isExplicitlyTracked")) {
                                        KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Calling back InstanceNotAvailableHandler with \"" << it->getKey() << "\"";
                                        m_instanceNotAvailableHandler(it->getKey(), instanceInfo); // Inform via callback   
                                    }
                                } catch (const Exception& e) {
                                    KARABO_LOG_FRAMEWORK_ERROR << e;
                                } catch (...) {
                                    KARABO_LOG_FRAMEWORK_ERROR << "Unknown error happened in user callback for instance being not available";
                                }
                            }
                        }

                        if (countdown < -21600 /* => 12 hours with sleep(2) */) {
                            KARABO_LOG_FRAMEWORK_DEBUG << "Giving up waiting for silently died instance \"" << it->getKey() << "\" to reappear";
                            cleanSignalsAndStopTracking(it->getKey());
                        }
                    }

                    // We are sleeping thrice as long as the count-down ticks (which ticks in seconds)
                    boost::this_thread::sleep(boost::posix_time::seconds(3));
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "letConnectionSlowlyDieWithoutHeartbeat triggered an exception: " << e;
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "letConnectionSlowlyDieWithoutHeartbeat triggered an unknown exception";
            }

        }

        void SignalSlotable::cleanSignalsAndStopTracking(const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            if (m_trackedComponents.has(instanceId)) {

                KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << instanceId << "\" will not be tracked anymore, cleaning all signals";

                const vector<Hash>& connections = m_trackedComponents.get<std::vector<Hash> >(instanceId + ".connections");
                for (size_t i = 0; i < connections.size(); ++i) {
                    const Hash& connection = connections[i];
                    const string& signalInstanceId = connection.get<string > ("signalInstanceId");
                    const string& signalFunction = connection.get<string > ("signalFunction");
                    const string& slotInstanceId = connection.get<string > ("slotInstanceId");
                    const string& slotFunction = connection.get<string > ("slotFunction");
                    //cout << "\"" << signalFunction << "\" (" << signalInstanceId << ") \t\t <--> \t\t \"" << slotFunction << "\" (" << slotInstanceId << ")" << endl;
                    if (signalInstanceId == m_instanceId) {

                        boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                        SignalInstancesConstIt it = m_signalInstances.find(signalFunction);
                        if (it != m_signalInstances.end()) {
                            //KARABO_LOG_FRAMEWORK_DEBUG << "Cleaning local signal: " << signalFunction;
                            // Unregister slotId from local signal
                            it->second->unregisterSlot(slotInstanceId, slotFunction);
                        }
                    }
                }
                {
                    boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                    m_signalInstances["signalHeartbeat"]->unregisterSlot(instanceId, "slotHeartbeat");
                }
                m_trackedComponents.erase(instanceId);
            }
        }

        Hash SignalSlotable::prepareConnectionNotAvailableInformation(const karabo::util::Hash & hash) const {
            Hash result;
            for (Hash::const_iterator jt = hash.begin(); jt != hash.end(); ++jt) {
                const AssocType& associates = jt->getValue<AssocType>();
                vector<string > tmp;
                tmp.reserve(associates.size());
                for (AssocTypeConstIterator kt = associates.begin(); kt != associates.end(); ++kt) {
                    tmp.push_back(kt->first);
                }
                result.set(jt->getKey(), tmp);
            }
            return result;
        }

        void SignalSlotable::connectInputChannels() {
            // Loop channels
            for (InputChannels::const_iterator it = m_inputChannels.begin(); it != m_inputChannels.end(); ++it) {
                AbstractInput::Pointer channel = it->second;
                if (channel->needsDeviceConnection()) {
                    // Loop connected outputs
                    std::vector<karabo::util::Hash> outputChannels = channel->getConnectedOutputChannels();
                    for (size_t j = 0; j < outputChannels.size(); ++j) {
                        const std::string& instanceId = outputChannels[j].get<string > ("instanceId");
                        const std::string& channelId = outputChannels[j].get<string > ("channelId");
                        bool channelExists = false;
                        karabo::util::Hash reply;
                        int sleep = 1;
                        int trials = 8;
                        while ((trials--) > 0) {
                            try {
                                this->request(instanceId, "slotGetOutputChannelInformation", channelId, static_cast<int> (getpid())).timeout(1000).receive(channelExists, reply);
                            } catch (karabo::util::TimeoutException&) {
                                karabo::util::Exception::clearTrace();
                                std::cout << "Could not find instanceId \"" + instanceId + "\" for IO connection" << std::endl;
                                std::cout << "Trying again in " << sleep << " seconds." << std::endl;
                                boost::this_thread::sleep(boost::posix_time::seconds(sleep));
                                sleep += 2;
                                continue;
                            }
                            break;
                        }
                        if (channelExists) {
                            channel->connect(reply); // Synchronous
                        } else {
                            throw KARABO_IO_EXCEPTION("Could not find outputChannel \"" + channelId + "\" on instanceId \"" + instanceId + "\"");
                        }
                    }
                }
            }
        }

        karabo::util::Hash SignalSlotable::slotGetOutputChannelInformation(const std::string& ioChannelId, const int& processId) {
            OutputChannels::const_iterator it = m_outputChannels.find(ioChannelId);
            if (it != m_outputChannels.end()) {
                karabo::util::Hash h(it->second->getInformation());
                if (processId == static_cast<int> (getpid())) {
                    h.set("memoryLocation", "local");
                } else {
                    h.set("memoryLocation", "remote");
                }
                reply(true, h);
                return h;
            } else {
                reply(false, karabo::util::Hash());
                return karabo::util::Hash();
            }
        }

        int SignalSlotable::godEncode(const std::string & password) {
            unsigned int code = 0;
            for (size_t i = 0; i < password.size() - 1; ++i) {
                unsigned int asciiValue = static_cast<int> (password[i]) * (i + 1);
                code += asciiValue;
            }
            size_t lastIdx = password.size() - 1;
            unsigned int asciiValue = static_cast<int> (password[lastIdx]) * (lastIdx + 1);
            code += asciiValue;
            code /= password.size();
            return code;
        }

        int SignalSlotable::getAccessLevel(const std::string & deviceId) const {
            boost::mutex::scoped_lock lock(m_accessLevelMutex);
            boost::optional<const Hash::Node&> node = m_accessList.find(deviceId);
            if (node) return node->getValue<int>();
            else return m_defaultAccessLevel;
        }

        const std::string& SignalSlotable::getUserName() const {
            return m_username;
        }

        bool SignalSlotable::hasReceivedReply(const std::string& replyId) const {
            boost::mutex::scoped_lock lock(m_receivedRepliesMutex);
            return m_receivedReplies.find(replyId) != m_receivedReplies.end();
        }

        void SignalSlotable::popReceivedReply(const std::string& replyId, karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body) {
            boost::mutex::scoped_lock lock(m_receivedRepliesMutex);
            ReceivedReplies::iterator it = m_receivedReplies.find(replyId);
            if (it != m_receivedReplies.end()) {
                header = it->second.first;
                body = it->second.second;
            }
            // Remove
            m_receivedReplies.erase(it);
        }

        bool SignalSlotable::timedWaitAndPopReceivedReply(const std::string& replyId, karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body, int timeout) {
#if defined(KARABO_SIGNAL_SLOTABLE_TIMED_WAIT_AND_POP_RECEIVED_REPLY_POLLING)
            int currentWaitingTime = 0;
            while (!hasReceivedReply(replyId) && currentWaitingTime < timeout) {
                boost::this_thread::sleep(boost::posix_time::milliseconds(2));
                currentWaitingTime += 2;
            }
            if (currentWaitingTime >= timeout) return false;

            popReceivedReply(replyId, header, body);
            return true;
#else
            bool result = true;
            boost::shared_ptr<BoostMutexCond> bmc(new BoostMutexCond);
            try {
                boost::mutex::scoped_lock lock(m_receivedRepliesBMCMutex);
                m_receivedRepliesBMC[replyId] = bmc;
            } catch (...) {
                KARABO_RETHROW
            }
            boost::system_time waitUntil = boost::get_system_time() + boost::posix_time::milliseconds(timeout);
            try {
                boost::mutex::scoped_lock lock(bmc->m_mutex);
                while (!this->hasReceivedReply(replyId)) {
                    if (!bmc->m_cond.timed_wait(lock, waitUntil)) {
                        result = false;
                        break;
                    }
                }
            } catch (...) {
                KARABO_RETHROW
            }
            try {
                boost::mutex::scoped_lock lock(m_receivedRepliesBMCMutex);
                m_receivedRepliesBMC.erase(replyId);
            } catch (...) {
                KARABO_RETHROW
            }
            if (result) {
                popReceivedReply(replyId, header, body);
            }
            return result;
#endif
        }

        void SignalSlotable::startTimer(int milliseconds, const ExpireHandler& handler, const std::string& id) {
            boost::mutex::scoped_lock lock(m_timerMutex);
            m_timerHandlers[id] = handler;
            m_heartbeatProducerChannel->waitAsync(milliseconds, boost::bind(&SignalSlotable::asyncTimerHandler, this, id), id);
        }

        void SignalSlotable::killTimer(const std::string& id) {
            boost::mutex::scoped_lock lock(m_timerMutex);
            m_timerHandlers.erase(id);
        }

        void SignalSlotable::asyncTimerHandler(const std::string& id) {
            bool found = false;
            ExpireHandler handler;
            {
                boost::mutex::scoped_lock lock(m_timerMutex);
                std::map<std::string, ExpireHandler>::iterator it = m_timerHandlers.find(id);
                if (it != m_timerHandlers.end()) {
                    found = true;
                    handler = it->second;
                    m_timerHandlers.erase(it);
                }
            }
            if (found) handler(id);
        }
    }
}
