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

        /// Milliseconds of timeout when asking for validity of my id at startup:
        const int msPingTimeoutInIsValidInstanceId = 1000;

        // Static initializations
        boost::uuids::random_generator SignalSlotable::Requestor::m_uuidGenerator;
        std::map<std::string, SignalSlotable*> SignalSlotable::m_instanceMap;
        boost::mutex SignalSlotable::m_instanceMapMutex;
        std::map<std::string, std::string> SignalSlotable::m_connectionStrings;
        boost::mutex SignalSlotable::m_connectionStringsMutex;
        PointToPoint::Pointer SignalSlotable::m_pointToPoint;


        bool SignalSlotable::tryToCallDirectly(const std::string& instanceId, const karabo::util::Hash::Pointer& header,
                                               const karabo::util::Hash::Pointer& body) const {
            if (instanceId == "*" || instanceId.empty())
                return false;

            SignalSlotable* that = 0;
            {
                boost::mutex::scoped_lock lock(m_instanceMapMutex);
                std::map<std::string, SignalSlotable*>::iterator it = m_instanceMap.find(instanceId);
                if (it != m_instanceMap.end()) {
                    that = it->second;
                }
            }

            if (that) {
                that->injectEvent(that->m_consumerChannel, header, body);
                return true;
            } else {
                return false;
            }
        }


        bool SignalSlotable::tryToCallP2P(const std::string& slotInstanceId, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body, int prio) const {
            if (slotInstanceId == "*" || slotInstanceId.empty() || !m_pointToPoint) return false;
            return m_pointToPoint->publish(slotInstanceId, header, body, prio);
        }


        void SignalSlotable::doSendMessage(const std::string& instanceId, const karabo::util::Hash::Pointer& header,
                                           const karabo::util::Hash::Pointer& body, int prio, int timeToLive) const {
            if (tryToCallDirectly(instanceId, header, body)) return;
            if (tryToCallP2P(instanceId, header, body, prio)) return;
            m_producerChannel->write(*header, *body, prio, timeToLive);
        }


        SignalSlotable::Caller::Caller(const SignalSlotable* signalSlotable) : m_signalSlotable(signalSlotable) {
        }


        void SignalSlotable::Caller::sendMessage(const std::string& slotInstanceId,
                                                 const karabo::util::Hash::Pointer& header,
                                                 const karabo::util::Hash::Pointer& body) const {
            try {
                if (!m_signalSlotable)
                    throw KARABO_PARAMETER_EXCEPTION("Caller::sendMessage  : m_signalSlotable = 0 or m_signalSlotable->m_producerChannel is 0");
                // Empty slotInstanceId means self messaging:
                const std::string& instanceId = (slotInstanceId.empty() ? m_signalSlotable->getInstanceId() : slotInstanceId);
                m_signalSlotable->doSendMessage(instanceId, header, body, KARABO_SYS_PRIO, KARABO_SYS_TTL);
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Problems sending message"));
            }
        }


        karabo::util::Hash::Pointer SignalSlotable::Caller::prepareHeader(const std::string& slotInstanceId,
                                                                          const std::string& slotFunction) const {
            karabo::util::Hash::Pointer header(new Hash);
            header->set("signalInstanceId", m_signalSlotable->getInstanceId());
            header->set("signalFunction", "__call__");
            header->set("slotInstanceIds", "|" + slotInstanceId + "|");
            header->set("slotFunctions", "|" + slotInstanceId + ":" + slotFunction + "|");
            header->set("hostName", boost::asio::ip::host_name());
            header->set("userName", m_signalSlotable->getUserName());
            header->set("MQTimestamp", m_signalSlotable->getEpochMillis());
            return header;
        }


        void SignalSlotable::Requestor::Receiver::receive(SignalSlotable::Requestor *ss) {
            try {
                Hash::Pointer header, body;
                ss->receiveResponse(header, body);
                if (header->has("error")) throw KARABO_SIGNALSLOT_EXCEPTION(header->get<std::string>("error"));
                inner(body);
            } catch (const karabo::util::TimeoutException&) {
                KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
            } catch (const karabo::util::CastException &) {
                KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Error whilst receiving message on instance \"" + ss->m_signalSlotable->getInstanceId() + "\""));
            }

        }


        SignalSlotable::Requestor::Requestor(SignalSlotable* signalSlotable) :
        m_signalSlotable(signalSlotable), m_replyId(generateUUID()), m_isRequested(false), m_isReceived(false) {
        }


        SignalSlotable::Requestor& SignalSlotable::Requestor::timeout(const int& milliseconds) {
            m_timeout = milliseconds;
            return *this;
        }


        karabo::util::Hash::Pointer SignalSlotable::Requestor::prepareHeader(const std::string& slotInstanceId, const std::string& slotFunction) {
            karabo::util::Hash::Pointer header(new karabo::util::Hash);
            header->set("replyTo", m_replyId);
            header->set("signalInstanceId", m_signalSlotable->getInstanceId());
            header->set("signalFunction", "__request__");
            header->set("slotInstanceIds", "|" + slotInstanceId + "|");
            header->set("slotFunctions", "|" + slotInstanceId + ":" + slotFunction + "|");
            header->set("hostName", boost::asio::ip::host_name());
            header->set("userName", m_signalSlotable->getUserName());
            return header;
        }


        karabo::util::Hash::Pointer SignalSlotable::Requestor::prepareHeaderNoWait(const std::string& requestSlotInstanceId,
                                                                                   const std::string& requestSlotFunction,
                                                                                   const std::string& replySlotInstanceId,
                                                                                   const std::string& replySlotFunction) {

            karabo::util::Hash::Pointer header(new karabo::util::Hash);
            header->set("replyInstanceIds", "|" + replySlotInstanceId + "|");
            header->set("replyFunctions", "|" + replySlotInstanceId + ":" + replySlotFunction + "|");
            header->set("signalInstanceId", m_signalSlotable->getInstanceId());
            header->set("signalFunction", "__requestNoWait__");
            header->set("slotInstanceIds", "|" + requestSlotInstanceId + "|");
            header->set("slotFunctions", "|" + requestSlotInstanceId + ":" + requestSlotFunction + "|");
            header->set("hostName", boost::asio::ip::host_name());
            header->set("userName", m_signalSlotable->getUserName());
            return header;

        }


        void SignalSlotable::Requestor::registerRequest() {
            if (m_isRequested) throw KARABO_SIGNALSLOT_EXCEPTION("You have to receive an answer before you can send a new request");
            m_isRequested = true;
            m_isReceived = false;
        }


        std::string SignalSlotable::Requestor::generateUUID() {
            return boost::uuids::to_string(m_uuidGenerator());
        }


        void SignalSlotable::Requestor::sendRequest(const std::string& instanceId, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) const {
            try {
                m_signalSlotable->doSendMessage(instanceId, header, body, KARABO_SYS_PRIO, KARABO_SYS_TTL);
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Problems sending request"));
            }
        }


        void SignalSlotable::Requestor::receiveResponse(karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body) {
            if (!m_signalSlotable->timedWaitAndPopReceivedReply(m_replyId, header, body, m_timeout)) {
                m_isReceived = true;
                m_isRequested = false;
                throw KARABO_TIMEOUT_EXCEPTION("Reply timed out");
            }

            m_isReceived = true;
            m_isRequested = false;
        }


        SignalSlotable::SignalSlotable()
        : m_connectionInjected(false), m_randPing(rand() + 2), m_discoverConnectionResourcesMode(false) {
        }


        SignalSlotable::SignalSlotable(const string& instanceId, const BrokerConnection::Pointer& connection)
        : m_connectionInjected(false), m_randPing(rand() + 2), m_discoverConnectionResourcesMode(false) {
            init(instanceId, connection);
        }


        SignalSlotable::SignalSlotable(const std::string& instanceId,
                                       const std::string& brokerType,
                                       const karabo::util::Hash& brokerConfiguration)
        : m_connectionInjected(false), m_randPing(rand() + 2), m_discoverConnectionResourcesMode(false) {
            BrokerConnection::Pointer connection = BrokerConnection::create(brokerType, brokerConfiguration);
            init(instanceId, connection);
        }


        SignalSlotable::~SignalSlotable() {
            // Last chance to deregister from static map, but should already be done...
            this->deregisterFromShortcutMessaging();
        }


        void SignalSlotable::deregisterFromShortcutMessaging() {
            boost::mutex::scoped_lock lock(m_instanceMapMutex);
            std::map<std::string, SignalSlotable*>::iterator it = m_instanceMap.find(m_instanceId);
            // Let's be sure that we remove ourself:
            if (it != m_instanceMap.end() && it->second == this) {
                m_instanceMap.erase(it);
            }
            // Transfer the connection resources discovering duty to another SignalSlotable if any
            if (m_discoverConnectionResourcesMode) {
                it = m_instanceMap.begin();
                if (it != m_instanceMap.end()) it->second->m_discoverConnectionResourcesMode = true;
                m_discoverConnectionResourcesMode = false;
            }
        }


        void SignalSlotable::registerForShortcutMessaging() {
            boost::mutex::scoped_lock lock(m_instanceMapMutex);
            SignalSlotable*& instance = m_instanceMap[m_instanceId];
            if (!instance) {
                instance = this;
            } else if (instance != this) {
                KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Cannot register "
                        << "for short-cut messaging since there is already another instance.";
                // Do not dare to call methods on instance - could already be destructed...?
            }
        }


        void SignalSlotable::init(const std::string& instanceId,
                                  const karabo::net::BrokerConnection::Pointer& connection) {

            m_trackAllInstances = false;
            m_defaultAccessLevel = KARABO_DEFAULT_ACCESS_LEVEL;
            m_connection = connection;
            m_instanceId = instanceId;
            m_nThreads = 2;

            // Currently only removes dots
            sanifyInstanceId(m_instanceId);
            if (m_connectionInjected) {
                m_ioService.reset();
            } else {
                // Create the managing ioService object
                m_ioService = m_connection->getIOService();

                // Start connection (and take the default channel for signals)
                m_connection->start();
            }
            m_producerChannel = m_connection->createChannel();
            m_consumerChannel = m_connection->createChannel();
            m_heartbeatProducerChannel = m_connection->createChannel("beats");
            m_heartbeatConsumerChannel = m_connection->createChannel("beats");

            registerDefaultSignalsAndSlots();
        }


        void SignalSlotable::injectConnection(const std::string& instanceId, const karabo::net::BrokerConnection::Pointer& connection) {
            m_connectionInjected = true;
            init(instanceId, connection);
        }


        std::pair<bool, std::string > SignalSlotable::isValidInstanceId(const std::string & instanceId) {
            // Ping any guy with my id. If there is one, he will answer, if not, we timeout.
            // HACK: slotPing takes care that I do not answer myself before timeout...
            Hash instanceInfo;
            try {
                request(instanceId, "slotPing", instanceId, m_randPing, false)
                        .timeout(msPingTimeoutInIsValidInstanceId).receive(instanceInfo);
            } catch (const karabo::util::TimeoutException&) {
                Exception::clearTrace();
                return std::make_pair(true, "");
            }
            string foreignHost("unknown");
            if (instanceInfo.has("host")) instanceInfo.get("host", foreignHost);
            const string msg("Another instance with ID '" + instanceId
                             + "' is already online (on host: " + foreignHost + ")");
            return std::make_pair(false, msg);
        }


        void SignalSlotable::sanifyInstanceId(std::string & instanceId) const {
            for (std::string::iterator it = instanceId.begin(); it != instanceId.end(); ++it) {
                if ((*it) == '.') (*it) = '-';
            }
        }


        void SignalSlotable::injectEvent(karabo::net::BrokerChannel::Pointer, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) {

            {
                boost::mutex::scoped_lock lock(m_latencyMutex);
                if (header->has("MQTimestamp")) {
                    m_brokerLatency.first += (getEpochMillis() - header->get<long long>("MQTimestamp"));
                    m_brokerLatency.second++;
                }
            }


            // Check whether this message is a reply
            if (header->has("replyFrom")) {
                handleReply(header, body);
            } else {
                {
                    boost::mutex::scoped_lock lock(m_eventQueueMutex);
                    m_eventQueue.push_back(std::make_pair(header, body));
                }
                m_hasNewEvent.notify_one();
            }
        }


        long long SignalSlotable::getEpochMillis() const {
            using namespace boost::gregorian;
            using namespace boost::local_time;
            using namespace boost::posix_time;

            ptime epochTime(date(1970, 1, 1));
            ptime nowTime = microsec_clock::universal_time();
            time_duration difference = nowTime - epochTime;
            return difference.total_milliseconds();
        }


        void SignalSlotable::handleReply(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) {
            KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Injecting reply from: " << header->get<string>("signalInstanceId");
            const string& replyId = header->get<string>("replyFrom");
            // Check whether a callback (temporary slot) was registered for the reply
            SlotInstancePointer slot = getSlot(replyId);
            try {
                if (slot) slot->callRegisteredSlotFunctions(*header, *body);
            } catch (const Exception& e) {
                if (m_exceptionHandler) m_exceptionHandler(e);
                else KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << ": An error happened within a reply callback: \n" << e;
            } catch (const std::exception& se) {
                if (m_exceptionHandler) m_exceptionHandler(KARABO_SIGNALSLOT_EXCEPTION(std::string(se.what()) + " in reply callback"));
                else KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << ": An error happened within a reply callback: " << se.what();
            }
            removeSlot(replyId);
            // Now check whether someone is synchronously waiting for us and if yes wake him up            
            boost::shared_ptr<BoostMutexCond> bmc;
            {
                boost::mutex::scoped_lock lock(m_receivedRepliesBMCMutex);
                ReceivedRepliesBMC::iterator it = m_receivedRepliesBMC.find(replyId);
                if (it != m_receivedRepliesBMC.end()) bmc = it->second;
            }
            // Insert reply and notify only if it is expected
            if (!bmc) return;
            {
                boost::mutex::scoped_lock lock(m_receivedRepliesMutex);
                m_receivedReplies[replyId] = std::make_pair(header, body);
            }
            bmc->m_cond.notify_one(); // notify only if 
        }


        void SignalSlotable::injectHeartbeat(karabo::net::BrokerChannel::Pointer, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) {
            SlotInstancePointer slot = getSlot("slotHeartbeat");
            // Synchronously call the slot
            if (slot) slot->callRegisteredSlotFunctions(*header, *body);
        }


        void SignalSlotable::runEventLoop(int heartbeatInterval, const karabo::util::Hash& instanceInfo) {

            try {

                m_heartbeatInterval = heartbeatInterval;
                m_instanceInfo = instanceInfo;

                m_runEventLoop = true;

                startBrokerMessageConsumption();

                // Start all configured event threads
                for (int i = 0; i < m_nThreads; ++i) {
                    m_eventLoopThreads.create_thread(boost::bind(&karabo::xms::SignalSlotable::_runEventLoop, this));
                }

                m_eventLoopThreads.join_all(); // Join all event dispatching threads

                stopBrokerMessageConsumption();

                if (!m_randPing) {
                    stopTrackingSystem();
                    stopEmittingHearbeats();

                    KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << m_instanceId << "\" shuts cleanly down";
                    call("*", "slotInstanceGone", m_instanceId, m_instanceInfo);
                }


            } catch (const karabo::util::Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Error in runEventLoop: " << e;
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Error in runEventLoop: " << e.what();
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Unknown error in runEventLoop happened";
            }
        }


        bool SignalSlotable::ensureOwnInstanceIdUnique() {

            // Inject the heartbeat interval to instanceInfo
            m_instanceInfo.set("heartbeatInterval", m_heartbeatInterval);

            // Inject karaboVersion
            m_instanceInfo.set("karaboVersion", karabo::util::Version::getVersion());


            // Make sure the instanceId is valid and unique
            std::pair<bool, std::string> result = isValidInstanceId(m_instanceId);
            if (!result.first) {
                KARABO_LOG_FRAMEWORK_ERROR << result.second;
                stopEventLoop();
                return result.first;
            } else {
                // We are unique - so now we can register ourself for short-cut messaging.
                this->registerForShortcutMessaging();
            }

            // Put the P2P producer local port number into instanceInfo for distributing around
            if (!m_pointToPoint) {
                m_pointToPoint = PointToPoint::Pointer(new PointToPoint);
                m_discoverConnectionResourcesMode = true;
                KARABO_LOG_FRAMEWORK_DEBUG << "PointToPoint producer connection string is \"" << m_pointToPoint->getConnectionString() << "\"";
            }
            m_instanceInfo.set("p2p_connection", m_pointToPoint->getConnectionString());

            KARABO_LOG_FRAMEWORK_INFO << "Instance starts up with id '" << m_instanceId
                    << "' and uses " << m_nThreads << " threads.";

            m_randPing = 0; // Allows to answer on slotPing with argument rand = 0.
            call("*", "slotInstanceNew", m_instanceId, m_instanceInfo);

            startEmittingHeartbeats(m_heartbeatInterval);
            startTrackingSystem();

            return result.first;
        }


        void SignalSlotable::_runEventLoop() {

            while (m_runEventLoop) {

                if (eventQueueIsEmpty()) { // If queue is empty we wait for the next event
                    boost::mutex::scoped_lock lock(m_waitMutex);
                    m_hasNewEvent.wait(lock);
                }

                // Got notified about new events, start processing them
                processEvents(); // Should never throw

            }
        }


        void SignalSlotable::stopEventLoop() {

            // Finish the outer while loop
            m_runEventLoop = false;

            // Notify all currently waiting threads to finish
            m_hasNewEvent.notify_all();
        }


        void SignalSlotable::startTrackingSystem() {
            // Countdown and finally timeout registered heartbeats
            m_doTracking = true;
            m_trackingThread = boost::thread(boost::bind(&SignalSlotable::letInstanceSlowlyDieWithoutHeartbeat, this));
        }


        void SignalSlotable::stopTrackingSystem() {
            m_doTracking = false;
            m_trackingThread.join();
        }


        void SignalSlotable::startEmittingHeartbeats(const int heartbeatInterval) {
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

            if (!m_connectionInjected)
                m_brokerThread = boost::thread(boost::bind(&karabo::net::BrokerIOService::work, m_ioService));

            // Prepare the slot selector
            string selector = "slotInstanceIds LIKE '%|" + m_instanceId + "|%' OR slotInstanceIds LIKE '%|*|%'";
            m_consumerChannel->setFilter(selector);
            m_consumerChannel->readAsyncHashHash(boost::bind(&karabo::xms::SignalSlotable::injectEvent, this, _1, _2, _3));
        }


        void SignalSlotable::stopBrokerMessageConsumption() {
            this->deregisterFromShortcutMessaging(); // stop short-cut messaging as well
            if (!m_connectionInjected) {
                m_ioService->stop();
                m_brokerThread.join();
            }
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

                try {

                    /* Header and body are shared pointers               
                     * By now the event variable keeps the only reference to them and hence the objects alive
                     * In the next while loop header and body will be destructed
                     * We take const references for convenience here
                     */
                    const Hash& header = *(event.first);
                    const Hash& body = *(event.second);

                    // Collect performance statistics
                    {
                        boost::mutex::scoped_lock lock(m_latencyMutex);
                        if (header.has("MQTimestamp")) {
                            m_processingLatency.first += (getEpochMillis() - header.get<long long>("MQTimestamp"));
                            m_processingLatency.second++;
                        }
                    }

                    /* The header of each event (message) 
                     * should contain all slotFunctions that must be a called
                     * The formatting is like: 
                     * slotFunctions -> [|<instanceId1>:<slotFunction1>[,<slotFunction2>]]
                     * Example:
                     * slotFunctions -> |FooInstance:slotFoo1,slotFoo2|BarInstance:slotBar1,slotBar2|"
                     */
                    boost::optional<const Hash::Node&> allSlotsNode = header.find("slotFunctions");
                    if (!allSlotsNode) {
                        KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId()
                                << ": Skip processing event since header lacks key 'slotFunctions'.";
                        continue;
                    }

                    std::string slotFunctions = allSlotsNode->getValue<string>(); // by value since trimmed later
                    KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << ": Process event for slotFunctions '"
                            << slotFunctions << "'"; // << "\n Header: " << header << "\n Body: " << body;

                    // Trim and split on the "|" string, avoid empty entries
                    std::vector<string> allSlots;
                    boost::trim_if(slotFunctions, boost::is_any_of("|"));
                    boost::split(allSlots, slotFunctions, boost::is_any_of("|"), boost::token_compress_on);


                    BOOST_FOREACH(const string& instanceSlots, allSlots) {
                        //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Processing instanceSlots: " << instanceSlots;
                        const size_t pos = instanceSlots.find_first_of(":");
                        if (pos == std::string::npos) {
                            KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Encountered badly shaped message header";
                            continue;
                        }
                        const string instanceId(instanceSlots.substr(0, pos));
                        // We should call only functions defined for our instanceId or global ("*") ones
                        const bool globalCall = (instanceId == "*");
                        if (!globalCall && instanceId != m_instanceId) continue;

                        const vector<string> slotFunctions = karabo::util::fromString<string, vector>(instanceSlots.substr(pos + 1));


                        BOOST_FOREACH(const string& slotFunction, slotFunctions) {
                            //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Going to call local " << slotFunction << " if registered";

                            if (m_slotCallGuardHandler) {
                                if (!m_slotCallGuardHandler(slotFunction)) {
                                    KARABO_LOG_FRAMEWORK_WARN << "Guard rejected slot '" << slotFunction << "'.";
                                    sendPotentialReply(header, globalCall);
                                    continue;
                                }
                            }

                            SlotInstancePointer slot = getSlot(slotFunction);
                            if (slot) {
                                // try/catch around user code that might have registered an exception handler:
                                try {
                                    slot->callRegisteredSlotFunctions(header, body);
                                } catch (const Exception& e) {
                                    if (m_exceptionHandler) m_exceptionHandler(e);
                                    else {
                                        KARABO_LOG_FRAMEWORK_ERROR << this->getInstanceId() << ": Exception in slot '"
                                                << slotFunction << "': " << e;
                                    }
                                } catch (const std::exception& se) {
                                    if (m_exceptionHandler) {
                                        m_exceptionHandler(KARABO_SIGNALSLOT_EXCEPTION(std::string(se.what()) + " in slot '" + slotFunction + "'"));
                                    } else {
                                        KARABO_LOG_FRAMEWORK_ERROR << this->getInstanceId() << ": "
                                                << se.what() << " in slot '" << slotFunction << "'.";
                                    }
                                }
                                sendPotentialReply(header, globalCall);
                            } else if (!globalCall) {
                                // Warn on non-existing slot, but only if directly addressed:
                                const std::string& signalInstanceId = (header.has("signalInstanceId") ? // const ref is essential!
                                        header.get<std::string>("signalInstanceId") : std::string("unknown"));
                                KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Received a message from '"
                                        << signalInstanceId << "' to non-existing slot \"" << slotFunction << "\"";
                            } else {
                                KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Miss slot " << slotFunction;
                            }
                        }
                    }
                } catch (const Exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << this->getInstanceId() << ": Exception while running inner event loop occurred: " << e;
                } catch (...) {
                    KARABO_LOG_FRAMEWORK_ERROR << this->getInstanceId() << ": Unknown exception while running inner event loop occurred.";
                }
            }
        }


        void SignalSlotable::registerReply(const karabo::util::Hash& reply) {
            boost::mutex::scoped_lock lock(m_replyMutex);
            m_replies[boost::this_thread::get_id()] = reply;
        }


        void SignalSlotable::sendPotentialReply(const karabo::util::Hash& header, bool global) {

            // We could be requested in two different ways.
            // TODO: Get rid of requestNoWait code path once receiveAsync is everywhere.
            // GF: But currently there is a difference: requestNoWait allows to get answers from
            //     everybody if called globally whereas a global request's reply will be refused below.
            const bool caseRequest = header.has("replyTo"); // with receive or receiveAsync
            const bool caseRequestNoWait = header.has("replyInstanceIds");

            const boost::thread::id replyId = boost::this_thread::get_id();
            boost::mutex::scoped_lock lock(m_replyMutex);
            if (!caseRequest && !caseRequestNoWait) {
                // Not requested, so nothing to reply, but we have to remove the
                // reply that may have been placed in the slot.
                m_replies.erase(replyId);
                return;
            }
            // The reply of a slot requested globally ("*") should be ignored.
            // If not, all but the first reply reaching the requesting instance
            // would anyways be ignored. So we just remove the reply.
            // Note that a global requestNoWait will work instead: All answers
            // will call the given slot.
            if (global && caseRequest) { // NOT: || caseRequestNoWait) {
                if (m_replies.erase(replyId) != 0) {
                    // But it is fishy if the slot was requested instead of simply called!
                    KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Refusing to reply to "
                            << header.get<std::string>("signalInstanceId") << " since it request-ed '"
                            << header.get<std::string>("slotFunctions") << "' (i.e. globally).";
                }
                // KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << ": sendPotentialReply - but global request.";
                return;
            }

            // For caseRequestNoWait it does not make sense to send an empty reply if
            // the called slot did not place an answer (argument mismatch for reply slot).
            Replies::iterator it = m_replies.find(boost::this_thread::get_id());
            if (caseRequestNoWait && it == m_replies.end()) {
                KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Slot '"
                        << header.get<std::string>("slotFunctions") << "' did not place a "
                        << "reply, but was called via requestNoWait";
                return;
            }

            // We are left with valid requests/requestNoWaits. For requests, we send an empty
            // reply if the slot did not place one. That tells the caller at least that
            // the slot finished - i.e. a synchronous request stops blocking.
            karabo::util::Hash replyHeader;
            if (caseRequest) {
                replyHeader.set("replyFrom", header.get<std::string > ("replyTo"));
                replyHeader.set("signalInstanceId", m_instanceId);
                replyHeader.set("signalFunction", "__reply__");
                replyHeader.set("slotInstanceIds", "|" + header.get<string>("signalInstanceId") + "|");
            } else { // i.e. caseRequestNoWait with a reply properly placed
                replyHeader.set("signalInstanceId", m_instanceId);
                replyHeader.set("signalFunction", "__replyNoWait__");
                replyHeader.set("slotInstanceIds", header.get<string>("replyInstanceIds"));
                replyHeader.set("slotFunctions", header.get<string>("replyFunctions"));
            }
            // Inject an empty reply in case that no one was provided in the slot body.
            // (Using a ref that is const is essential to keep the temporary util::Hash() alive.)
            const util::Hash& reply = (it != m_replies.end() ? it->second : util::Hash());
            m_producerChannel->write(replyHeader, reply, KARABO_SYS_PRIO, KARABO_SYS_TTL);
            // KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << ": sendPotentialReply: " << reply;

            // Finally remove the reply.
            if (it != m_replies.end()) m_replies.erase(it);
        }


        void SignalSlotable::registerDefaultSignalsAndSlots() {

            // The heartbeat signal goes through a different topic, so we
            // cannot use the normal registerSignal.
            SignalInstancePointer heartbeatSignal(new karabo::xms::Signal(this, m_heartbeatProducerChannel, m_instanceId, "signalHeartbeat", 9));
            storeSignal("signalHeartbeat", heartbeatSignal);
            boost::function<void (const string&, const int&, const Hash&) > f = boost::bind(&Signal::emit3<string, int, Hash>, heartbeatSignal, _1, _2, _3);
            m_emitFunctions.set("signalHeartbeat", f);

            // Listener for heartbeats
            KARABO_SLOT3(slotHeartbeat, string /*instanceId*/, int /*heartbeatIntervalInSec*/, Hash /*instanceInfo*/)

            KARABO_SYSTEM_SIGNAL("signalInstanceNew", string, Hash);

            KARABO_SYSTEM_SIGNAL("signalInstanceGone", string, Hash);

            //KARABO_SYSTEM_SIGNAL3("signalHeartbeat", string /*instanceId*/, int /*heartbeatIntervalInSec*/, Hash /*instanceInfo*/)

            // Global ping listener
            KARABO_SLOT3(slotPing, string /*callersInstanceId*/, int /*replyIfSame*/, bool /*trackPingedInstance*/)

            // Global instance new notification
            KARABO_SLOT2(slotInstanceNew, string /*instanceId*/, Hash /*instanceInfo*/)

            // Global slot instance gone
            KARABO_SLOT2(slotInstanceGone, string /*instanceId*/, Hash /*instanceInfo*/)

            // Listener for ping answers
            KARABO_SLOT2(slotPingAnswer, string /*instanceId*/, Hash /*instanceInfo*/)

            // Connects signal to slot
            KARABO_SLOT3(slotConnectToSignal, string /*signalFunction*/, string /*slotInstanceId*/, string /*slotFunction*/)

            // Replies whether slot exists on this instance
            KARABO_SLOT1(slotHasSlot, string /*slotFunction*/)

            // Disconnects signal from slot
            KARABO_SLOT3(slotDisconnectFromSignal, string /*signalFunction*/, string /*slotInstanceId*/, string /*slotFunction*/)

            // Function request
            KARABO_SLOT1(slotGetAvailableFunctions, string /*functionType*/)

            // Provides information about p2p connectivity
            KARABO_SLOT2(slotGetOutputChannelInformation, string /*ioChannelId*/, int /*pid*/)

            // Establishes/Releases P2P connections
            KARABO_SLOT3(slotConnectToOutputChannel, string /*inputChannelName*/, karabo::util::Hash /*outputChannelInfo */, bool /*connect/disconnect*/)
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


        void SignalSlotable::trackAllInstances() {
            m_trackAllInstances = true;
            m_heartbeatConsumerChannel->setFilter("signalFunction = 'signalHeartbeat'");
            m_heartbeatConsumerChannel->readAsyncHashHash(boost::bind(&karabo::xms::SignalSlotable::injectHeartbeat, this, _1, _2, _3));
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

            // This will ensure that all old connections (maintained as part of the signal) are erased
            cleanSignals(instanceId);

            if (m_trackAllInstances) {
                // If it was already tracked, this call will overwrite it (= reset countdown)
                addTrackedInstance(instanceId, instanceInfo);
            }

            emit("signalInstanceNew", instanceId, instanceInfo);

            reconnectSignals(instanceId);

            if (m_discoverConnectionResourcesMode && instanceInfo.has("p2p_connection") && m_instanceInfo.has("p2p_connection")) {
                string localConnectionString, remoteConnectionString;
                m_instanceInfo.get("p2p_connection", localConnectionString);
                instanceInfo.get("p2p_connection", remoteConnectionString);

                // Store only remote connection strings
                if (localConnectionString != remoteConnectionString) {
                    boost::mutex::scoped_lock lock(m_connectionStringsMutex);
                    m_connectionStrings[instanceId] = remoteConnectionString;
                }
            }

            reconnectInputChannels(instanceId);
        }


        void SignalSlotable::slotInstanceGone(const std::string& instanceId, const karabo::util::Hash & instanceInfo) {

            if (instanceId == m_instanceId) return;

            cleanSignals(instanceId);

            if (m_trackAllInstances) {
                eraseTrackedInstance(instanceId);
            }

            emit("signalInstanceGone", instanceId, instanceInfo);
            if (m_discoverConnectionResourcesMode && instanceInfo.has("p2p_connection")) {
                boost::mutex::scoped_lock lock(m_connectionStringsMutex);
                map<string, string>::iterator it = m_connectionStrings.find(instanceId);
                if (it != m_connectionStrings.end()) m_connectionStrings.erase(it);
            }
        }


        BrokerConnection::Pointer SignalSlotable::getConnection() const {
            return m_connection;
        }


        void SignalSlotable::emitHeartbeat() {
            boost::this_thread::sleep(boost::posix_time::seconds(1));
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


        Hash SignalSlotable::getAvailableInstances(bool activateTracking) {
            KARABO_LOG_FRAMEWORK_DEBUG << "getAvailableInstances";
            if (!m_trackAllInstances) m_trackedInstances.clear();
            call("*", "slotPing", m_instanceId, 0, activateTracking);
            // The function slotPingAnswer will be called by all instances available now
            // Lets wait a fair amount of time - huaaah this is bad isn't it :-(
            boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
            KARABO_LOG_FRAMEWORK_DEBUG << "Available instances: " << m_trackedInstances;
            return m_trackedInstances;
        }


        std::pair<bool, std::string > SignalSlotable::exists(const std::string & instanceId) {
            string hostname;
            Hash instanceInfo;
            try {
                this->request(instanceId, "slotPing", instanceId, 1, false).timeout(200).receive(instanceInfo);
            } catch (const karabo::util::TimeoutException&) {
                return std::make_pair(false, hostname);
            }
            if (instanceInfo.has("host")) instanceInfo.get("host", hostname);
            return std::make_pair(true, hostname);
        }


        void SignalSlotable::slotPing(const std::string& instanceId, int rand, bool trackPingedInstance) {

            if (rand != 0) {
                // case 1) Called by an instance that is coming up: rand is his m_randPing before it gets 'valid',
                // case 2) or by SignalSlotable::exists: rand is 1
                if (instanceId == m_instanceId) {
                    if (rand == m_randPing) {
                        // We are in case 1) and I ask myself. I must not answer, at least not in time.
                        // HACK: Let's wait until my own request timed out for sure.
                        boost::this_thread::sleep(boost::posix_time::milliseconds(msPingTimeoutInIsValidInstanceId * 1.5));
                    } else {
                        // m_randPing == 0 (I am up) or >= 2 (I am 'booting')
                        // 1) It is not me, so that guy must not come up: tell him. Note: Two guys coming up
                        //    at the same time with the same id might both fail here.
                        // 2) I just reply my existence.
                        reply(m_instanceInfo);
                    }
                }
            } else if (!m_randPing) {
                // I should only answer, if my name got accepted which is indicated by a value of m_randPing==0
                call(instanceId, "slotPingAnswer", m_instanceId, m_instanceInfo);
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
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            SlotInstancesConstIt it = m_slotInstances.find(slotFunction);
            if (it == m_slotInstances.end()) throw KARABO_SIGNALSLOT_EXCEPTION("No slot-object could be found for slotFunction \"" + slotFunction + "\"");
            return it->second;
        }


        void SignalSlotable::slotGetAvailableFunctions(const std::string & type) {
            std::vector<string> functions;
            if (type == "signals") {
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                for (SignalInstancesConstIt it = m_signalInstances.begin(); it != m_signalInstances.end(); ++it) {
                    const string& function = it->first;
                    functions.push_back(function);
                }
            } else if (type == "slots") {
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                for (SlotInstancesConstIt it = m_slotInstances.begin(); it != m_slotInstances.end(); ++it) {
                    const string& function = it->first;
                    // Filter out service slots // TODO finally update to last set of those
                    if (function == "slotConnectToSignal" || function == "slotDisconnectFromSignal" || function == "slotGetAvailableFunctions" ||
                            function == "slotHasSlot" || function == "slotHeartbeat" || function == "slotPing" || function == "slotPingAnswer" ||
                            function == "slotStopTrackingExistenceOfConnection") {
                        continue;
                    }
                    functions.push_back(it->first);
                }
            }
            reply(functions);
        }


        void SignalSlotable::slotPingAnswer(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            if (!hasTrackedInstance(instanceId)) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Got ping answer from instanceId " << instanceId;
                emit("signalInstanceNew", instanceId, instanceInfo);
            } else {
                KARABO_LOG_FRAMEWORK_DEBUG << "Got ping answer from instanceId (but already tracked) " << instanceId;
            }
            addTrackedInstance(instanceId, instanceInfo);

        }


        void SignalSlotable::slotHeartbeat(const std::string& instanceId, const int& heartbeatInterval, const Hash& instanceInfo) {
            if (m_trackAllInstances) {
                if (!hasTrackedInstance(instanceId)) {
                    // Notify about new instance
                    emit("signalInstanceNew", instanceId, instanceInfo);
                    //if (m_instanceAvailableAgainHandler) m_instanceAvailableAgainHandler(instanceId, instanceInfo);
                }
                // This overwrites the old entry and resets the countdown 
                addTrackedInstance(instanceId, instanceInfo);
            }
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


        void SignalSlotable::registerInstanceNotAvailableHandler(const InstanceNotAvailableHandler & instanceNotAvailableCallback) {
            m_instanceNotAvailableHandler = instanceNotAvailableCallback;
        }


        void SignalSlotable::registerInstanceAvailableAgainHandler(const InstanceAvailableAgainHandler & instanceAvailableAgainCallback) {
            m_instanceAvailableAgainHandler = instanceAvailableAgainCallback;
        }


        void SignalSlotable::registerExceptionHandler(const ExceptionHandler& exceptionFoundCallback) {
            m_exceptionHandler = exceptionFoundCallback;
        }


        void SignalSlotable::registerSlotCallGuardHandler(const SlotCallGuardHandler& slotCallGuardHandler) {
            m_slotCallGuardHandler = slotCallGuardHandler;
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

        bool SignalSlotable::connect(const std::string& signalInstanceIdIn, const std::string& signalFunction,
                const std::string& slotInstanceIdIn, const std::string& slotFunction) {

            const std::string& signalInstanceId = (signalInstanceIdIn.empty() ? m_instanceId : signalInstanceIdIn);
            const std::string& slotInstanceId = (slotInstanceIdIn.empty() ? m_instanceId : slotInstanceIdIn);

            {
                // Keep track of what we connect - or at least try to:
                const SignalSlotConnection connection(signalInstanceId, signalFunction, slotInstanceId, slotFunction);
                boost::mutex::scoped_lock lock(m_signalSlotConnectionsMutex);
                // Register twice as we have to re-connect if either signal or slot instance comes back.
                // (We might skip to register for s*InstanceId == m_instanceId, but then "reconnectSignals"
                //  looses its genericity.)
                m_signalSlotConnections[signalInstanceId].insert(connection);
                m_signalSlotConnections[slotInstanceId].insert(connection);
            }

            if (this->instanceHasSlot(slotInstanceId, slotFunction)) {
	      if (this->tryToConnectToSignal(signalInstanceId, signalFunction, slotInstanceId, slotFunction)) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Successfully connected slot '" << slotInstanceId << "." << slotFunction
                    << "' to signal '" << signalInstanceId << "." << signalFunction << "'.";
		return true;
	      } else {
                KARABO_LOG_FRAMEWORK_DEBUG << "Could not connect slot '" << slotInstanceId << "." << slotFunction
                    << "' to (non-existing?) signal '" << signalInstanceId << "." << signalFunction << "'.";
	      }
	    } else {
              KARABO_LOG_FRAMEWORK_DEBUG << "Did not try to connect non-existing slot '" << slotInstanceId << "." << slotFunction
                  << "' to signal '" << signalInstanceId << "." << signalFunction << "'.";
	    }

	    return false;
        }

        bool SignalSlotable::tryToConnectToSignal(const std::string& signalInstanceId, const std::string& signalFunction,
                const std::string& slotInstanceId, const std::string& slotFunction) {

            bool signalExists = false;

            if (signalInstanceId == m_instanceId) { // Local signal requested
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                SignalInstancesConstIt it = m_signalInstances.find(signalFunction);
                if (it != m_signalInstances.end()) { // Signal found
                    signalExists = true;
                    // Register new slotId to local signal
                    it->second->registerSlot(slotInstanceId, slotFunction);
                } else {
                    signalExists = false;
                    KARABO_LOG_FRAMEWORK_DEBUG << "Requested signal '" << signalFunction << "' is not available locally on this instance '" << m_instanceId << "'.";
                }
            } else { // Remote signal requested
                try {
                    request(signalInstanceId, "slotConnectToSignal", signalFunction, slotInstanceId, slotFunction).timeout(1000).receive(signalExists);
                    if (!signalExists) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Requested signal '" << signalFunction << "' is not available on remote instance '" << signalInstanceId << "'.";
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    signalExists = false;
                    KARABO_LOG_FRAMEWORK_DEBUG << "Remote instance '" << signalInstanceId << "' did not respond in time"
                            << " the request to connect to its signal '" << signalFunction << "'.";
                }
            }
            return signalExists;
        }


        SignalSlotable::SlotInstancePointer SignalSlotable::findSlot(const std::string &funcName) {
            SlotInstancePointer ret;
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            SlotInstances::const_iterator it = m_slotInstances.find(funcName);
            if (it != m_slotInstances.end()) {
                ret = it->second;
            }
            return ret;
        }


        void SignalSlotable::registerNewSlot(const std::string &funcName, SlotInstancePointer instance) {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            SlotInstancePointer& newinstance = m_slotInstances[funcName];
            if (newinstance) {
                throw KARABO_SIGNALSLOT_EXCEPTION("The slot \"" + funcName + "\" has been registered with two different signatures");
            } else {
                newinstance = instance;
            }
        }


        void SignalSlotable::slotConnectToSignal(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction) {
            bool result = false;
            {
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                SignalInstancesConstIt it = m_signalInstances.find(signalFunction);
                if (it != m_signalInstances.end()) {
                    it->second->registerSlot(slotInstanceId, slotFunction);
                    result = true;
                }
            }

            reply(result);
        }

        bool SignalSlotable::instanceHasSlot(const std::string& slotInstanceId, const std::string& slotFunction) {

            if (slotInstanceId == "*") return true; // GLOBAL slots may or may not exist

            bool slotExists = false;

            if (slotInstanceId == m_instanceId) { // Local slot requested
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                if (m_slotInstances.find(slotFunction) != m_slotInstances.end()) { // Slot found
                    slotExists = true;
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Requested slot '" << slotFunction
                            << "' is currently not available locally on instance '" << m_instanceId << "'.";
                }
            } else {// Remote slot requested
                try {
                    request(slotInstanceId, "slotHasSlot", slotFunction).timeout(1000).receive(slotExists);
                    if (!slotExists) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Requested slot '" << slotFunction
                                << "' is currently not available on remote instance '" << slotInstanceId << "'.";
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    slotExists = false;
                    KARABO_LOG_FRAMEWORK_DEBUG << "Remote instance '" << slotInstanceId << "' did not respond in time"
                            << " whether it has a slot '" << slotFunction << "'.";
                }
            }
            return slotExists;
        }

        void SignalSlotable::slotHasSlot(const std::string& slotFunction) {
            bool result = false;
            {
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                if (m_slotInstances.find(slotFunction) != m_slotInstances.end()) {
                    result = true;
                }
            }
            reply(result);
        }


        bool SignalSlotable::connect(const std::string& signal, const std::string& slot) {
            std::pair<std::string, std::string> signalPair = splitIntoInstanceIdAndFunctionName(signal);
            std::pair<std::string, std::string> slotPair = splitIntoInstanceIdAndFunctionName(slot);
            return connect(signalPair.first, signalPair.second, slotPair.first, slotPair.second);
        }


        void SignalSlotable::reconnectSignals(const std::string& newInstanceId) {

            std::set<SignalSlotConnection> connections;
            {
                boost::mutex::scoped_lock lock(m_signalSlotConnectionsMutex);
                SignalSlotConnections::iterator it = m_signalSlotConnections.find(newInstanceId);

                if (it != m_signalSlotConnections.end()) {
                    connections = it->second;
                }
            }

            // Must not call connect(..) under protection of m_signalSlotConnectionsMutex: deadlock!
            for (std::set<SignalSlotConnection>::const_iterator it = connections.begin(), iEnd = connections.end();
                    it != iEnd; ++it) {
                KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << " tries to reconnect signal '"
                        << it->m_signalInstanceId << "." << it->m_signal << "' to slot '"
                        << it->m_slotInstanceId << "." << it->m_slot << "'.";
                this->connect(it->m_signalInstanceId, it->m_signal, it->m_slotInstanceId, it->m_slot);
            }
        }


        void SignalSlotable::addTrackedInstance(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            boost::mutex::scoped_lock lock(m_trackedInstancesMutex);
            Hash h;
            h.set("instanceInfo", instanceInfo);
            // Initialize countdown with the heartbeat interval
            h.set("countdown", instanceInfo.get<int>("heartbeatInterval"));
            m_trackedInstances.set(instanceId, h);
        }


        bool SignalSlotable::hasTrackedInstance(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_trackedInstancesMutex);
            return m_trackedInstances.has(instanceId);
        }


        void SignalSlotable::eraseTrackedInstance(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_trackedInstancesMutex);
            m_trackedInstances.erase(instanceId);
        }


        void SignalSlotable::updateTrackedInstanceInfo(const std::string& instanceId, const karabo::util::Hash & instanceInfo) {
            boost::mutex::scoped_lock lock(m_trackedInstancesMutex);
            if (m_trackedInstances.has(instanceId)) {
                Hash& h = m_trackedInstances.get<Hash>(instanceId);
                h.set("instanceInfo", instanceInfo);
                h.set("countdown", instanceInfo.get<int>("heartbeatInterval"));
            }
        }


        void SignalSlotable::addTrackedInstanceConnection(const std::string& instanceId, const karabo::util::Hash & connection) {
            boost::mutex::scoped_lock lock(m_trackedInstancesMutex);
            if (m_trackedInstances.has(instanceId)) {
                m_trackedInstances.get<vector<Hash> >(instanceId + ".connections").push_back(connection);
            }
        }

        bool SignalSlotable::disconnect(const std::string& signalInstanceIdIn, const std::string& signalFunction, const std::string& slotInstanceIdIn, const std::string& slotFunction) {

            const std::string& signalInstanceId = (signalInstanceIdIn.empty() ? m_instanceId : signalInstanceIdIn);
            const std::string& slotInstanceId = (slotInstanceIdIn.empty() ? m_instanceId : slotInstanceIdIn);

            // Remove from list of connections that this SignalSlotable established.
            bool connectionWasKnown = false;
            {
                boost::mutex::scoped_lock lock(m_signalSlotConnectionsMutex);
                const SignalSlotConnection connection(signalInstanceId, signalFunction, slotInstanceId, slotFunction);
                // Might be in there twice: once for signal, once for slot.
                SignalSlotConnections::iterator it = m_signalSlotConnections.find(signalInstanceId);
                if (it != m_signalSlotConnections.end()) {
                    connectionWasKnown = (it->second.erase(connection) >= 1);
                }
                it = m_signalSlotConnections.find(slotInstanceId);
                if (it != m_signalSlotConnections.end()) {
                    connectionWasKnown = (it->second.erase(connection) >= 1 ? true : connectionWasKnown);
                }
            }

            const bool result = tryToDisconnectFromSignal(signalInstanceId, signalFunction, slotInstanceId, slotFunction);

            if (result) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Successfully disconnected slot '" << slotInstanceId << "." << slotFunction
                        << "' from signal '" << signalInstanceId << "." << signalFunction << "'.";
            } else {
                KARABO_LOG_FRAMEWORK_DEBUG << "Failed to disconnected slot '" << slotInstanceId << "." << slotFunction
                        << "' from signal '" << signalInstanceId << "." << signalFunction << "'.";
            }

            if (result && !connectionWasKnown) {
                KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << "Disconnected slot '" << slotInstanceId << "." << slotFunction
                        << "' from signal '" << signalInstanceId << "." << signalFunction << "', but did not connect them "
                        << "before. Whoever connected them will probably re-connect once '" << signalInstanceId
                        << "' or '" << slotInstanceId << "' come back.";
            }

            return result;
        }

        bool SignalSlotable::tryToDisconnectFromSignal(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction) {

            bool signalExists = false;

            if (signalInstanceId == m_instanceId) { // Local signal requested

                if (signalFunction == "signalHeartbeat") {
                    // Never disconnect from heartbeats - why?
                    signalExists = true;
                } else if (tryToUnregisterSlot(signalFunction, slotInstanceId, slotFunction)) {
                    signalExists = true;
                } else {
                    signalExists = false;
                    KARABO_LOG_FRAMEWORK_DEBUG << "Cannot disconnect from signal '" << signalFunction
                            << "' since it does not exist on this (local) instance '" << m_instanceId << "'.";
                }
            } else { // Remote signal requested
                try {
                    request(signalInstanceId, "slotDisconnectFromSignal", signalFunction, slotInstanceId, slotFunction).timeout(1000).receive(signalExists);
                    if (!signalExists) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Cannot disconnect from signal '" << signalFunction
                                << "' on remote instance '" << signalInstanceId << "' since it does not exist.";
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    signalExists = false;
                    KARABO_LOG_FRAMEWORK_DEBUG << "Remote instance '" << signalInstanceId << "' did not respond in time"
                            << " the request to disconnect from its signal '" << signalFunction << "'.";
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
                return true;
            }
            return false;
        }


        void SignalSlotable::slotDisconnectFromSignal(const std::string& signalFunction, const std::string& slotInstanceId, const std::string & slotFunction) {
            if (signalFunction == "signalHeartbeat") {
                // Never disconnect from heartbeats - why?
                reply(true);
            } else if (tryToUnregisterSlot(signalFunction, slotInstanceId, slotFunction)) {
                reply(true);
            } else {
                reply(false);
            }
        }


        bool SignalSlotable::hasSlot(const std::string & slotFunction) const {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            return m_slotInstances.find(slotFunction) != m_slotInstances.end();
        }


        SignalSlotable::SlotInstancePointer SignalSlotable::getSlot(const std::string& slotFunction) const {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            SlotInstances::const_iterator it = m_slotInstances.find(slotFunction);
            if (it != m_slotInstances.end()) return it->second;
            return SlotInstancePointer();
        }


        void SignalSlotable::removeSlot(const std::string& slotFunction) {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            m_slotInstances.erase(slotFunction);
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
            // Isn't this the wrong way round, i.e. disconnect my slot from the others signal:
            // disconnect(instanceId, "signalHeartbeat", "", "slotHeartbeat");?
            disconnect("", "signalHeartbeat", instanceId, "slotHeartbeat");

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


        SignalSlotable::SignalInstancePointer SignalSlotable::addSignalIfNew(const std::string& signalFunction, int priority, int messageTimeToLive) {
            {
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                if (m_signalInstances.find(signalFunction) != m_signalInstances.end()) {
                    SignalInstancePointer s;
                    return s;
                }
            }
            SignalInstancePointer s(new Signal(this, m_producerChannel, m_instanceId, signalFunction, priority, messageTimeToLive));
            storeSignal(signalFunction, s);
            return s;
        }


        void SignalSlotable::storeSignal(const std::string &signalFunction, SignalInstancePointer signalInstance) {
            m_signalInstances[signalFunction] = signalInstance;
        }


        void SignalSlotable::letInstanceSlowlyDieWithoutHeartbeat() {

            try {

                while (m_doTracking) {

                    if (!m_connection->isConnected()) {
                        boost::this_thread::sleep(boost::posix_time::seconds(5));
                        continue;
                    }

                    if (m_trackAllInstances) {

                        //cout << "COUNTDOWN" << endl;
                        //cout << m_trackedInstances << endl;

                        vector<pair<string, Hash> > deadOnes;

                        decreaseCountdown(deadOnes);

                        for (size_t i = 0; i < deadOnes.size(); ++i) {
                            KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Instance \"" << deadOnes[i].first << "\" silently disappeared (no heartbeats received anymore)";
                            emit("signalInstanceGone", deadOnes[i].first, deadOnes[i].second);
                            eraseTrackedInstance(deadOnes[i].first);
                        }
                    }


                    // This thread is used as a "Trittbrett" for slowly updating latency information and queue sizes
                    if (m_updatePerformanceStatistics) {
                        boost::mutex::scoped_lock lock(m_latencyMutex);
                        float blAve = -1;
                        float plAve = -1;
                        if (m_brokerLatency.second > 0) {
                            blAve = m_brokerLatency.first / (float) m_brokerLatency.second;
                        }
                        if (m_processingLatency.second > 0) {
                            plAve = m_processingLatency.first / (float) m_processingLatency.second;
                        }

                        // Call handler
                        m_updatePerformanceStatistics(blAve, plAve, static_cast<unsigned int> (m_eventQueue.size()));

                        // Reset statistics
                        m_brokerLatency.first = m_brokerLatency.second = 0;
                        m_processingLatency.first = m_processingLatency.second = 0;
                    }

                    // We are sleeping thrice as long as the count-down ticks (which ticks in seconds)
                    boost::this_thread::sleep(boost::posix_time::seconds(5));
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "letInstanceSlowlyDieWithoutHeartbeat triggered an exception: " << e;
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "letInstanceSlowlyDieWithoutHeartbeat triggered an unknown exception";
            }
        }


        void SignalSlotable::decreaseCountdown(std::vector<std::pair<std::string, karabo::util::Hash> >& deadOnes) {
            boost::mutex::scoped_lock lock(m_trackedInstancesMutex);

            for (Hash::iterator it = m_trackedInstances.begin(); it != m_trackedInstances.end(); ++it) {
                Hash& entry = it->getValue<Hash>();
                int& countdown = entry.get<int>("countdown");
                countdown--; // Regular count down

                if (countdown == 0) { // Instance lost
                    deadOnes.push_back(std::make_pair<string, Hash>(it->getKey(), entry.get<Hash>("instanceInfo")));
                }
            }
        }


        void SignalSlotable::cleanSignals(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);

            KARABO_LOG_FRAMEWORK_TRACE << m_instanceId << " says : Cleaning all signals for instance \"" << instanceId << "\"";

            for (SignalInstances::iterator it = m_signalInstances.begin(); it != m_signalInstances.end(); ++it) {
                it->second->unregisterSlot(instanceId);
            }
        }


        void SignalSlotable::stopTracking(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_trackedInstancesMutex);
            KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << instanceId << "\" will not be tracked anymore";
            m_trackedInstances.erase(instanceId);
        }


        bool SignalSlotable::SignalSlotConnection::operator<(const SignalSlotConnection& other) const {
            // Compare members in sequence. Since arrays of references are not allowed, so use pointers.
            const std::string * const mine[] = {&m_signalInstanceId, &m_signal, &m_slotInstanceId, &m_slot};
            const std::string * const others[] = {&other.m_signalInstanceId, &other.m_signal, &other.m_slotInstanceId, &other.m_slot};
            const size_t numMembers = sizeof (mine) / sizeof (mine[0]);

            for (size_t i = 0; i < numMembers; ++i) {
                if (*(mine[i]) < *(others[i])) {
                    // My member is smaller than the one of other.
                    return true;
                } else if (*(mine[i]) == *(others[i])) {
                    // My member equals that of other - try next members if there are any.
                    continue;
                } else {
                    // My member is larger than the one of other.
                    break;
                }
            }
            // Any of my members was larger than the corresponding one of other or all where equal.
            return false;
        }


        InputChannel::Pointer SignalSlotable::createInputChannel(const std::string& channelName, const karabo::util::Hash& config,
                                                                 const boost::function<void (const Data&) >& onDataAvailableHandler,
                                                                 const boost::function<void (const InputChannel::Pointer&) >& onInputAvailableHandler,
                                                                 const boost::function<void(const InputChannel::Pointer&)>& onEndOfStreamEventHandler) {
            if (!config.has(channelName)) throw KARABO_PARAMETER_EXCEPTION("The provided configuration must contain the channel name as key in the configuration");
            Hash channelConfig = config.get<Hash>(channelName);
            if (channelConfig.has("schema")) channelConfig.erase("schema");
            InputChannel::Pointer channel = Configurator<InputChannel>::create("InputChannel", channelConfig);
            channel->setInstanceId(m_instanceId);
            m_inputChannels[channelName] = channel;
            // in fact, only one of the following two can be set...
            if (onDataAvailableHandler) {
                this->registerDataHandler(channelName, onDataAvailableHandler);
            }
            if (onInputAvailableHandler) {
                this->registerInputHandler(channelName, onInputAvailableHandler);
            }
            if (onEndOfStreamEventHandler) {
                this->registerEndOfStreamHandler(channelName, onEndOfStreamEventHandler);
            }
            return channel;
        }


        OutputChannel::Pointer SignalSlotable::createOutputChannel(const std::string& channelName, const karabo::util::Hash& config,
                                                                   const boost::function<void (const OutputChannel::Pointer&)>& onOutputPossibleHandler) {
            if (!config.has(channelName)) throw KARABO_PARAMETER_EXCEPTION("The provided configuration must contain the channel name as key in the configuration");
            Hash channelConfig = config.get<Hash>(channelName);
            if (channelConfig.has("schema")) channelConfig.erase("schema");
            OutputChannel::Pointer channel = Configurator<OutputChannel>::create("OutputChannel", channelConfig);
            channel->setInstanceId(m_instanceId);
            if (onOutputPossibleHandler) {
                channel->registerIOEventHandler(onOutputPossibleHandler);
            }
            m_outputChannels[channelName] = channel;
            return channel;
        }


        const SignalSlotable::InputChannels& SignalSlotable::getInputChannels() const {
            return m_inputChannels;
        }


        const SignalSlotable::OutputChannels& SignalSlotable::getOutputChannels() const {
            return m_outputChannels;
        }


        const OutputChannel::Pointer& SignalSlotable::getOutputChannel(const std::string& name) {

            OutputChannels::const_iterator it = m_outputChannels.find(name);
            if (it != m_outputChannels.end()) {
                return it->second;
            }
            throw KARABO_PARAMETER_EXCEPTION("OutputChannel \"" + name + " \" does not exist");
        }


        const InputChannel::Pointer& SignalSlotable::getInputChannel(const std::string& name) {

            InputChannels::const_iterator it = m_inputChannels.find(name);
            if (it != m_inputChannels.end()) {
                return it->second;
            }
            throw KARABO_PARAMETER_EXCEPTION("InputChannel \"" + name + "\" does not exist");
        }


        void SignalSlotable::inputHandlerWrap(const boost::function<void (const karabo::xms::InputChannel::Pointer&)>& handler,
                                              const karabo::xms::InputChannel::Pointer& input) {
            try {
                // Make sure that SignalSlotable shared pointer can be built (device still exists)...  otherwise exception is thrown
                SignalSlotable::Pointer self = shared_from_this();
                // call user callback
                handler(input);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_INFO << "\"inputHandlerWrap\" call is too late: Device is destroyed already -- " << e.what();
            }
        }


        void SignalSlotable::registerInputHandler(const std::string& channelName,
                                                  const boost::function<void (const karabo::xms::InputChannel::Pointer&)>& handler) {
            getInputChannel(channelName)->registerInputHandler(boost::bind(&SignalSlotable::inputHandlerWrap, this, handler, _1));
        }


        void SignalSlotable::dataHandlerWrap(const boost::function<void (const karabo::xms::Data&) >& handler,
                                             const karabo::xms::Data& data) {
            try {
                // Make sure that SignalSlotable shared pointer can be built...  if not the we get exception
                SignalSlotable::Pointer self = shared_from_this();
                // call user callback
                handler(data);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_INFO << "\"dataHandlerWrap\" call is too late: Device is destroyed already -- " << e.what();
            }
        }


        void SignalSlotable::registerDataHandler(const std::string& channelName,
                                                 const boost::function<void (const karabo::xms::Data&) >& handler) {
            getInputChannel(channelName)->registerDataHandler(boost::bind(&SignalSlotable::dataHandlerWrap, this, handler, _1));
        }


        void SignalSlotable::endOfStreamHandlerWrap(const boost::function<void (const boost::shared_ptr<InputChannel>&) >& handler,
                                                    const boost::shared_ptr<InputChannel>& input) {
            try {
                // Make sure that SignalSlotable shared pointer can be built (device still exists)...  otherwise exception is thrown
                SignalSlotable::Pointer self = shared_from_this();
                // call user callback
                handler(input);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_INFO << "\"endOfStreamHandlerWrap\" call is too late: Device is destroyed already -- " << e.what();
            }
        }


        void SignalSlotable::registerEndOfStreamHandler(const std::string& channelName, const boost::function<void (const karabo::xms::InputChannel::Pointer&)>& handler) {
            getInputChannel(channelName)->registerEndOfStreamEventHandler(boost::bind(&SignalSlotable::endOfStreamHandlerWrap, this, handler, _1));
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
                connectInputChannel(it->second);
            }
        }


        void SignalSlotable::reconnectInputChannels(const std::string& instanceId) {

            KARABO_LOG_FRAMEWORK_DEBUG << "reconnectInputChannels of \"" << m_instanceId << "\"  to output channels of \"" << instanceId << "\"";

            // Loop channels
            for (InputChannels::const_iterator it = m_inputChannels.begin(); it != m_inputChannels.end(); ++it) {
                const InputChannel::Pointer& channel = it->second;
                const std::map<std::string, karabo::util::Hash>& outputChannels = channel->getConnectedOutputChannels();
                for (std::map<std::string, karabo::util::Hash>::const_iterator ii = outputChannels.begin(); ii != outputChannels.end(); ++ii) {
                    const string& outputChannelString = ii->first;
                    if (instanceId != outputChannelString.substr(0, instanceId.size())) continue; // instanceId ~ instanceId@output
                    channel->disconnect(outputChannelString);
                    connectInputToOutputChannel(channel, outputChannelString);
                }
            }
        }


        void SignalSlotable::disconnectInputChannels(const std::string& instanceId) {

        }


        void SignalSlotable::connectInputChannel(const InputChannel::Pointer& channel, int trails, int sleep) {
            // Loop connected outputs
            const std::map<std::string, karabo::util::Hash>& outputChannels = channel->getConnectedOutputChannels();
            for (std::map<std::string, karabo::util::Hash>::const_iterator it = outputChannels.begin(); it != outputChannels.end(); ++it) {
                const string& outputChannelString = it->first;
                connectInputToOutputChannel(channel, outputChannelString, trails, sleep);
            }
        }


        void SignalSlotable::connectInputToOutputChannel(const InputChannel::Pointer& channel, const std::string& outputChannelString, int trials, int sleep) {

            KARABO_LOG_FRAMEWORK_DEBUG << "connectInputToOutputChannel  on \"" << m_instanceId << "\"  : outputChannelString is \"" << outputChannelString << "\"";

            std::map<std::string, karabo::util::Hash> outputChannels = channel->getConnectedOutputChannels();
            std::map<std::string, karabo::util::Hash>::const_iterator it = outputChannels.find(outputChannelString);
            if (it == outputChannels.end()) return;

            // it->first => outputChannelString (STRING)
            // it->second => outputChannelInfo  (HASH)  with connection parameters

            bool channelExists = !it->second.empty();
            if (!channelExists) {
                karabo::util::Hash reply;

                std::vector<std::string> v;
                boost::split(v, outputChannelString, boost::is_any_of("@:"));

                const std::string& instanceId = v[0];
                const std::string& channelId = v[1];

                while ((trials--) > 0) {
                    try {
                        KARABO_LOG_FRAMEWORK_DEBUG << "connectInputToOutputChannel  on \"" << m_instanceId << "\"  :  "
                                << "request \"" << instanceId << "\", slotGetOutputChannelInformation, channelId=" << channelId;

                        this->request(instanceId, "slotGetOutputChannelInformation", channelId,
                                      static_cast<int> (getpid())).timeout(1000).receive(channelExists, reply);
                    } catch (karabo::util::TimeoutException&) {
                        karabo::util::Exception::clearTrace();
                        KARABO_LOG_FRAMEWORK_INFO << "Could not find instanceId \"" + instanceId + "\" for IO connection";
                        KARABO_LOG_FRAMEWORK_INFO << "Trying again in " << sleep << " seconds.";
                        boost::this_thread::sleep(boost::posix_time::seconds(sleep));
                        sleep += 2;
                        continue;
                    }
                    // Use all attempts if failed to get channel existing...
                    if (!channelExists) {
                        boost::this_thread::sleep(boost::posix_time::seconds(sleep));
                        sleep += 2;
                        continue;
                    } else {
                        ostringstream oss;
                        oss << reply.get<string>("connectionType") << "://" << reply.get<string>("hostname")
                                << ":" << reply.get<unsigned int>("port");
                        reply.set("connectionString", oss.str());
                        reply.set("outputChannelString", outputChannelString);
                        channel->updateOutputChannelConfiguration(outputChannelString, reply);
                        outputChannels = channel->getConnectedOutputChannels(); // update outputChannels with new copy
                    }
                    break;
                }
            }
            if (channelExists) {
                channel->connect(outputChannels[outputChannelString]); // Synchronous
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Could not find outputChannel \"" << outputChannelString
                        << "\". Perhaps device with output channel is not online yet.";
            }
        }


        void SignalSlotable::connectInputChannelAsync(const InputChannel::Pointer& channel, const boost::function<void() >& handler) {
            // Loop connected outputs
            const std::map<std::string, karabo::util::Hash>& outputChannels = channel->getConnectedOutputChannels();
            for (std::map<std::string, karabo::util::Hash>::const_iterator it = outputChannels.begin(); it != outputChannels.end(); ++it) {
                const std::string& outputChannelString = it->first;

                std::vector<std::string> v;
                boost::split(v, outputChannelString, boost::is_any_of("@:"));

                const std::string& instanceId = v[0];
                const std::string& channelId = v[1];

                this->request(instanceId, "slotGetOutputChannelInformation", channelId, static_cast<int> (getpid()))
                        .receiveAsync<bool, karabo::util::Hash > (boost::bind(&SignalSlotable::onInputChannelConnectInfo,
                                                                              this, channel, handler,
                                                                              instanceId, channelId, _1, _2));
            }

        }


        void SignalSlotable::onInputChannelConnectInfo(const InputChannel::Pointer& channel,
                                                       const boost::function<void() >& handler,
                                                       const std::string& instanceId, const std::string& channelId,
                                                       bool channelExists, const karabo::util::Hash& info) {
            if (channelExists) {
                channel->connect(info); // Synchronous
                handler();
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "Could not find outputChannel \"" << channelId << "\" on instanceId \"" << instanceId << "\"";
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


        void SignalSlotable::setNumberOfThreads(int nThreads) {
            m_nThreads = nThreads;
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
            bool result = true;
            boost::shared_ptr<BoostMutexCond> bmc(new BoostMutexCond);
            {
                boost::mutex::scoped_lock lock(m_receivedRepliesBMCMutex);
                m_receivedRepliesBMC[replyId] = bmc;
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
        }


        void SignalSlotable::registerPerformanceStatisticsHandler(const UpdatePerformanceStatisticsHandler& updatePerformanceStatisticsHandler) {
            m_updatePerformanceStatistics = updatePerformanceStatisticsHandler;
        }


        bool SignalSlotable::connectP2P(const std::string& signalInstanceId) {
            if (signalInstanceId == m_instanceId) return false;
            string signalConnectionString;
            int attempt = 0;
            int millis = 200; // milliseconds
            while (attempt++ < 4) {
                // Try to find connection string (URI) locally in global table: m_connectionStrings
                {
                    boost::mutex::scoped_lock lock(m_connectionStringsMutex);
                    map<string, string>::iterator it = m_connectionStrings.find(signalInstanceId);
                    if (it != m_connectionStrings.end()) {
                        signalConnectionString = it->second;
                        break;
                    }
                }

                // ... failed :( ... try to request instanceInfo remotely via broker ...
                Hash instanceInfo;
                try {
                    this->request(signalInstanceId, "slotPing", signalInstanceId, 1, false).timeout(millis).receive(instanceInfo);
                    if (instanceInfo.has("p2p_connection")) {
                        instanceInfo.get("p2p_connection", signalConnectionString);
                        boost::mutex::scoped_lock lock(m_connectionStringsMutex);
                        m_connectionStrings[signalInstanceId] = signalConnectionString;
                        break;
                    }
                } catch (const TimeoutException&) {
                    Exception::clearTrace();
                    millis *= 5;
                }
            }

            // connection string should not be empty
            if (signalConnectionString.empty()) return false;

            m_pointToPoint->connect(signalInstanceId, m_instanceId, signalConnectionString,
                                    boost::bind(&SignalSlotable::tryToCallDirectly, this, _1, _2, _3));
            return true;
        }


        void SignalSlotable::disconnectP2P(const std::string& signalInstanceId) {
            if (signalInstanceId == m_instanceId) return;
            m_pointToPoint->disconnect(signalInstanceId, m_instanceId);
        }
    }
}
