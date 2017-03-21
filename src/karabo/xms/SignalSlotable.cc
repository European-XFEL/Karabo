/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "SignalSlotable.hh"
#include "karabo/util/Version.hh"
#include "karabo/net/EventLoop.hh"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/time_duration.hpp>

#include <string>
#include <algorithm>
#include <vector>

namespace karabo {
    namespace xms {

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::net;

        /// Milliseconds of timeout when asking for validity of my id at startup:
        const int msPingTimeoutInIsValidInstanceId = 1000;

        // Static initializations
        boost::uuids::random_generator SignalSlotable::Requestor::m_uuidGenerator;
        std::unordered_map<std::string, SignalSlotable*> SignalSlotable::m_instanceMap;
        boost::shared_mutex SignalSlotable::m_instanceMapMutex;
        std::map<std::string, std::string> SignalSlotable::m_connectionStrings;
        boost::mutex SignalSlotable::m_connectionStringsMutex;
        PointToPoint::Pointer SignalSlotable::m_pointToPoint;


        bool SignalSlotable::tryToCallDirectly(const std::string& instanceId,
                                               const karabo::util::Hash::Pointer& header,
                                               const karabo::util::Hash::Pointer& body) const {

            // Global signals must go via the broker
            if (instanceId == "*") return false;
            {
                boost::shared_lock<boost::shared_mutex> lock(m_instanceMapMutex);
                auto it = m_instanceMap.find(instanceId);
                if (it != m_instanceMap.end()) {
                    EventLoop::getIOService().post(bind_weak(&karabo::xms::SignalSlotable::processEvent,
                                                             it->second, header, body));
                    return true;
                } else {
                    return false;
                }
            }
        }


        void SignalSlotable::Requestor::receiveAsync(const boost::function<void () >& replyCallback, const boost::function<void()>& timeoutHandler) {
            m_signalSlotable->registerSlot(replyCallback, m_replyId);
            registerDeadlineTimer(timeoutHandler);
            sendRequest();
        }

        void SignalSlotable::Requestor::registerDeadlineTimer(const boost::function<void()>& timeoutHandler) {
            if (m_timeout > 0) {
                // Register a deadline timer into map
                auto timer = boost::make_shared<boost::asio::deadline_timer>(EventLoop::getIOService());
                timer->expires_from_now(boost::posix_time::milliseconds(m_timeout));
                timer->async_wait(bind_weak(&SignalSlotable::receiveAsyncTimeoutHandler, m_signalSlotable,
                                            boost::asio::placeholders::error, m_replyId, timeoutHandler));
                m_signalSlotable->addReceiveAsyncTimer(m_replyId, timer);
            }
        }


        /**
         * Register a new slot function for a slot. A new slot is generated
         * if so necessary. It is checked that the signature of the new
         * slot is the same as an already registered one.
         */
        void SignalSlotable::registerSlot(const boost::function<void ()>& slot, const std::string& funcName) {
            // If the same slot name was registered under a different signature before,
            // the dynamic_pointer_cast will return a NULL pointer and finally registerNewSlot
            // will throw an exception.
            auto s = boost::dynamic_pointer_cast < SlotN<void> >(findSlot(funcName));
            if (!s) {
                s = boost::make_shared < SlotN <void> >(funcName);
                registerNewSlot(funcName, boost::static_pointer_cast<Slot>(s));
            }
            s->registerSlotFunction(slot);
        }


        bool SignalSlotable::tryToCallP2P(const std::string& slotInstanceId, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body, int prio) const {
            if (slotInstanceId == "*" || slotInstanceId.empty() || !m_pointToPoint) return false;
            return m_pointToPoint->publish(slotInstanceId, header, body, prio);
        }


        void SignalSlotable::doSendMessage(const std::string& instanceId, const karabo::util::Hash::Pointer& header,
                                           const karabo::util::Hash::Pointer& body, int prio, int timeToLive,
                                           const std::string& topic, bool forceViaBroker) const {

            if (!forceViaBroker) {
                if (tryToCallDirectly(instanceId, header, body)) return;
                if (tryToCallP2P(instanceId, header, body, prio)) return;
            }

            const std::string& t = topic.empty() ? m_topic : topic;
            m_producerChannel->write(t, header, body, prio, timeToLive);
        }


        karabo::util::Hash::Pointer SignalSlotable::prepareCallHeader(const std::string& slotInstanceId,
                                                                      const std::string& slotFunction) const {
            auto header = boost::make_shared<Hash>();
            header->set("signalInstanceId", m_instanceId);
            header->set("signalFunction", "__call__");
            header->set("slotInstanceIds", "|" + slotInstanceId + "|");
            header->set("slotFunctions", "|" + slotInstanceId + ":" + slotFunction + "|");
            header->set("hostName", boost::asio::ip::host_name());
            header->set("userName", m_username);
            // Timestamp added to be able to measure latencies even if broker is by-passed
            header->set("MQTimestamp", getEpochMillis());
            return header;
        }


        SignalSlotable::Requestor::Requestor(SignalSlotable* signalSlotable) :
            m_signalSlotable(signalSlotable), m_replyId(generateUUID()), m_timeout(0) {
        }


        SignalSlotable::Requestor::~Requestor() {
        }


        SignalSlotable::Requestor& SignalSlotable::Requestor::timeout(const int& milliseconds) {
            m_timeout = milliseconds;
            return *this;
        }


        karabo::util::Hash::Pointer
        SignalSlotable::Requestor::prepareRequestHeader(const std::string& slotInstanceId,
                                                        const std::string& slotFunction) {
            karabo::util::Hash::Pointer header(new karabo::util::Hash);
            header->set("replyTo", m_replyId);
            header->set("signalInstanceId", m_signalSlotable->getInstanceId());
            header->set("signalFunction", "__request__");
            header->set("slotInstanceIds", "|" + slotInstanceId + "|");
            header->set("slotFunctions", "|" + slotInstanceId + ":" + slotFunction + "|");
            header->set("hostName", boost::asio::ip::host_name());
            header->set("userName", m_signalSlotable->getUserName());
            // Timestamp added to be able to measure latencies even if broker is by-passed
            header->set("MQTimestamp", m_signalSlotable->getEpochMillis());
            return header;
        }


        karabo::util::Hash::Pointer
        SignalSlotable::Requestor::prepareRequestNoWaitHeader(const std::string& requestSlotInstanceId,
                                                              const std::string& requestSlotFunction,
                                                              const std::string& replySlotInstanceId,
                                                              const std::string& replySlotFunction) {
            karabo::util::Hash::Pointer header(new karabo::util::Hash);
            // TODO Rename replyInstanceIds and replyFunctions to replyInstanceId and replyFunction
            header->set("replyInstanceIds", "|" + replySlotInstanceId + "|");
            header->set("replyFunctions", "|" + replySlotInstanceId + ":" + replySlotFunction + "|");
            header->set("signalInstanceId", m_signalSlotable->getInstanceId());
            header->set("signalFunction", "__requestNoWait__");
            header->set("slotInstanceIds", "|" + requestSlotInstanceId + "|");
            header->set("slotFunctions", "|" + requestSlotInstanceId + ":" + requestSlotFunction + "|");
            header->set("hostName", boost::asio::ip::host_name());
            header->set("userName", m_signalSlotable->getUserName());
            // Timestamp added to be able to measure latencies even if broker is by-passed
            header->set("MQTimestamp", m_signalSlotable->getEpochMillis());
            return header;
        }


        void SignalSlotable::Requestor::registerRequest(const std::string& slotInstanceId,
                                                        const karabo::util::Hash::Pointer& header,
                                                        const karabo::util::Hash::Pointer& body) {
            m_slotInstanceId = slotInstanceId;
            m_header = header;
            m_body = body;
        }


        std::string SignalSlotable::Requestor::generateUUID() {
            return boost::uuids::to_string(m_uuidGenerator());
        }


        void SignalSlotable::Requestor::sendRequest() const {
            try {
                m_signalSlotable->doSendMessage(m_slotInstanceId, m_header, m_body, KARABO_SYS_PRIO, KARABO_SYS_TTL);
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Problems sending request"));
            }
        }


        void SignalSlotable::Requestor::receiveResponse(karabo::util::Hash::Pointer& header,
                                                        karabo::util::Hash::Pointer& body) {
            m_signalSlotable->registerSynchronousReply(m_replyId);
            sendRequest();
            if (!m_signalSlotable->timedWaitAndPopReceivedReply(m_replyId, header, body, m_timeout)) {
                throw KARABO_TIMEOUT_EXCEPTION("Reply timed out");
            }
        }


        SignalSlotable::SignalSlotable() :
            m_randPing(rand() + 2),
            m_trackAllInstances(false),
            m_heartbeatInterval(10),
            m_trackingTimer(EventLoop::getIOService()),
            m_heartbeatTimer(EventLoop::getIOService()),
            m_performanceTimer(EventLoop::getIOService()),
            m_discoverConnectionResourcesMode(false) {
            setTopic();
            EventLoop::addThread();
        }


        SignalSlotable::SignalSlotable(const string& instanceId, const JmsConnection::Pointer& connection,
                                       const int heartbeatInterval, const karabo::util::Hash& instanceInfo) :
            SignalSlotable() {
            init(instanceId, connection, heartbeatInterval, instanceInfo);
        }


        SignalSlotable::SignalSlotable(const std::string& instanceId, const std::string& connectionClass,
                                       const karabo::util::Hash& brokerConfiguration,
                                       const int heartbeatInterval, const karabo::util::Hash& instanceInfo) :
            SignalSlotable() {
            JmsConnection::Pointer connection = Configurator<JmsConnection>::create(connectionClass, brokerConfiguration);
            init(instanceId, connection, heartbeatInterval, instanceInfo);
        }


        SignalSlotable::~SignalSlotable() {
            // Last chance to deregister from static map, but should already be done...
            this->deregisterFromShortcutMessaging();

            if (!m_randPing) {
                stopTrackingSystem();
                stopEmittingHearbeats();

                KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << m_instanceId << "\" shuts cleanly down";
                call("*", "slotInstanceGone", m_instanceId, m_instanceInfo);
            }
            EventLoop::removeThread();
        }


        void SignalSlotable::deregisterFromShortcutMessaging() {
            boost::unique_lock<boost::shared_mutex> lock(m_instanceMapMutex);
            auto it = m_instanceMap.find(m_instanceId);
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
            m_instanceInfo.erase("p2p_connection");
        }


        void SignalSlotable::registerForShortcutMessaging() {
            boost::unique_lock<boost::shared_mutex> lock(m_instanceMapMutex);
            SignalSlotable*& instance = m_instanceMap[m_instanceId];
            if (!instance) {
                instance = this;
            } else if (instance != this) {
                // Do not dare to call methods on instance - could already be destructed...?
                KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Cannot register "
                        << "for short-cut messaging since there is already another instance.";
            }
            if (!m_pointToPoint) {
                m_pointToPoint = boost::make_shared<PointToPoint>();
                m_discoverConnectionResourcesMode = true;
                KARABO_LOG_FRAMEWORK_DEBUG << "PointToPoint producer connection string is \""
                        << m_pointToPoint->getConnectionString() << "\"";
            }
            m_instanceInfo.set("p2p_connection", m_pointToPoint->getConnectionString());
        }


        void SignalSlotable::init(const std::string& instanceId,
                                  const karabo::net::JmsConnection::Pointer& connection,
                                  const int heartbeatInterval, const karabo::util::Hash& instanceInfo) {

            m_instanceId = instanceId;
            m_connection = connection;
            m_heartbeatInterval = heartbeatInterval;
            m_instanceInfo = instanceInfo;

            // Currently only removes dots
            sanifyInstanceId(m_instanceId);

            if (!m_connection->isConnected()) {
                m_connection->connect();
            }

            // Create producers and consumers
            m_producerChannel = m_connection->createProducer();
            // This will select messages addressed to me
            const string selector("slotInstanceIds LIKE '%|" + m_instanceId + "|%' OR slotInstanceIds LIKE '%|*|%'");
            m_consumerChannel = m_connection->createConsumer(m_topic, selector);
            m_heartbeatProducerChannel = m_connection->createProducer();

            registerDefaultSignalsAndSlots();

            m_instanceInfo.set("heartbeatInterval", m_heartbeatInterval);
            m_instanceInfo.set("karaboVersion", karabo::util::Version::getVersion());
        }


        void SignalSlotable::start() {
            m_consumerChannel->readAsync(bind_weak(&karabo::xms::SignalSlotable::onBrokerMessage, this, _1, _2));
            ensureInstanceIdIsValid(m_instanceId);
            KARABO_LOG_FRAMEWORK_INFO << "Instance starts up with id " << m_instanceId;
            m_randPing = 0; // Allows to answer on slotPing with argument rand = 0.
            registerForShortcutMessaging();
            startEmittingHeartbeats();
            startPerformanceMonitor();
            call("*", "slotInstanceNew", m_instanceId, m_instanceInfo);
        }


        void SignalSlotable::ensureInstanceIdIsValid(const std::string& instanceId) {
            {
                // It is important to check first for local conflicts, else
                // shortcut messaging (enabled by the conflicting instance) will trick slotPing request
                boost::shared_lock<boost::shared_mutex> lock(m_instanceMapMutex);
                if (m_instanceMap.count(instanceId)) {
                    throw KARABO_SIGNALSLOT_EXCEPTION("Another instance with ID '" + instanceId +
                                                      "' is already online in this process (localhost)");
                }
            }
            // Ping any guy with my id. If there is one, he will answer, if not, we timeout.
            // HACK: slotPing takes care that I do not answer myself before timeout...
            Hash instanceInfo;
            try {
                request(instanceId, "slotPing", instanceId, m_randPing, false)
                        .timeout(msPingTimeoutInIsValidInstanceId).receive(instanceInfo);
            } catch (const karabo::util::TimeoutException&) {
                // Receiving this timeout is the expected behavior
                Exception::clearTrace();
                return;
            }
            string foreignHost("unknown");
            if (instanceInfo.has("host")) instanceInfo.get("host", foreignHost);
            throw KARABO_SIGNALSLOT_EXCEPTION("Another instance with ID '" + instanceId +
                                              "' is already online (on host: " + foreignHost + ")");
        }


        void SignalSlotable::sanifyInstanceId(std::string& instanceId) const {
            for (std::string::iterator it = instanceId.begin(); it != instanceId.end(); ++it) {
                if ((*it) == '.') (*it) = '-';
            }
        }


        void SignalSlotable::onBrokerMessage(const karabo::util::Hash::Pointer& header,
                                             const karabo::util::Hash::Pointer& body) {
            // This emulates the behavior of older karabo versions which called processEvent concurrently
            EventLoop::getIOService().post(bind_weak(&karabo::xms::SignalSlotable::processEvent, this, header, body));
            m_consumerChannel->readAsync(bind_weak(&karabo::xms::SignalSlotable::onBrokerMessage, this, _1, _2));
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
            KARABO_LOG_FRAMEWORK_TRACE << m_instanceId << ": Injecting reply from: "
                    << header->get<string>("signalInstanceId") << *header << *body;
            const string& replyId = header->get<string>("replyFrom");
            // Check if the timer was registered for the reply ... and cancel it
            boost::shared_ptr<boost::asio::deadline_timer> timer = getReceiveAsyncTimer(replyId);
            if (timer) timer->cancel(); // A timer was set, but the message arrived before expiration -> cancel
            // Check whether a callback (temporary slot) was registered for the reply
            SlotInstancePointer slot = getSlot(replyId);
            try {
                if (slot) {
                    slot->callRegisteredSlotFunctions(*header, *body);
                }
            } catch (const std::exception& e) {
                boost::optional<Hash::Node&> signalIdNode = header->find("signalInstanceId");
                const std::string& signalId = (signalIdNode && signalIdNode->is<std::string>() ?
                                              "'" + signalIdNode->getValue<std::string>() + "'" :
                                              " unspecified sender");
                KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << ": Exception when handling reply from "
                        << signalId << ": " << e.what();
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


        void SignalSlotable::onHeartbeatMessage(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) {
            SlotInstancePointer slot = getSlot("slotHeartbeat");
            // Synchronously call the slot
            if (slot) slot->callRegisteredSlotFunctions(*header, *body);
            // Re-register
            m_heartbeatConsumerChannel->readAsync(bind_weak(&karabo::xms::SignalSlotable::onHeartbeatMessage, this, _1, _2));
        }


        void SignalSlotable::startTrackingSystem() {
            // Countdown and finally timeout registered heartbeats
            m_trackingTimer.expires_from_now(boost::posix_time::milliseconds(10));
            m_trackingTimer.async_wait(bind_weak(&karabo::xms::SignalSlotable::letInstanceSlowlyDieWithoutHeartbeat,
                                                 this, boost::asio::placeholders::error));
        }


        void SignalSlotable::stopTrackingSystem() {
            m_trackingTimer.cancel();
        }


        void SignalSlotable::startPerformanceMonitor() {
            // Countdown and finally timeout registered heartbeats
            m_performanceTimer.expires_from_now(boost::posix_time::milliseconds(10));
            m_performanceTimer.async_wait(bind_weak(&karabo::xms::SignalSlotable::updatePerformanceStatistics,
                                                    this, boost::asio::placeholders::error));
        }


        void SignalSlotable::stopPerformanceMonitor() {
            m_performanceTimer.cancel();
        }


        void SignalSlotable::updatePerformanceStatistics(const boost::system::error_code& e) {
            if (e) return;
            try {
                if (m_updatePerformanceStatistics) {
                    boost::mutex::scoped_lock lock(m_latencyMutex);
                    // Call handler synchronously
                    m_updatePerformanceStatistics(m_processingLatency.average(), m_processingLatency.maximum);
                    // Reset statistics
                    m_processingLatency.clear();
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << e.userFriendlyMsg();
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Unknown exception happened";
            }
            m_performanceTimer.expires_from_now(boost::posix_time::seconds(5));
            m_performanceTimer.async_wait(bind_weak(&karabo::xms::SignalSlotable::updatePerformanceStatistics,
                                                    this, boost::asio::placeholders::error));
        }


        void SignalSlotable::startEmittingHeartbeats() {
            m_heartbeatTimer.expires_from_now(boost::posix_time::milliseconds(10));
            m_heartbeatTimer.async_wait(bind_weak(&karabo::xms::SignalSlotable::emitHeartbeat, this,
                                                  boost::asio::placeholders::error));
        }


        void SignalSlotable::stopEmittingHearbeats() {
            m_heartbeatTimer.cancel();
        }


        void SignalSlotable::processEvent(const karabo::util::Hash::Pointer& header,
                                          const karabo::util::Hash::Pointer& body) {
            try {

                // Collect performance statistics
                if (m_updatePerformanceStatistics) {
                    if (header->has("MQTimestamp")) {
                        boost::mutex::scoped_lock lock(m_latencyMutex);
                        const long long latency = getEpochMillis() - header->get<long long>("MQTimestamp");
                        const unsigned int posLatency = static_cast<unsigned int> (std::max(latency, 0ll));
                        m_processingLatency.add(posLatency);
                    }
                }

                // Check whether this message is a reply
                if (header->has("replyFrom")) {
                    handleReply(header, body);
                    return;
                }

                /* The header of each event (message)
                 * should contain all slotFunctions that must be a called
                 * The formatting is like:
                 * slotFunctions -> [|<instanceId1>:<slotFunction1>[,<slotFunction2>]]
                 * Example:
                 * slotFunctions -> |FooInstance:slotFoo1,slotFoo2|BarInstance:slotBar1,slotBar2|"
                 */
                boost::optional<Hash::Node&> allSlotsNode = header->find("slotFunctions");
                if (!allSlotsNode) {
                    KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId()
                            << ": Skip processing event since header lacks key 'slotFunctions'.";
                    return;
                }

                std::string slotFunctions = allSlotsNode->getValue<string>(); // by value since trimmed later
                KARABO_LOG_FRAMEWORK_TRACE << this->getInstanceId() << ": Process event for slotFunctions '"
                        << slotFunctions << "'"; // << "\n Header: " << header << "\n Body: " << body;

                // Trim and split on the "|" string, avoid empty entries
                std::vector<string> allSlots;
                boost::trim_if(slotFunctions, boost::is_any_of("|"));
                boost::split(allSlots, slotFunctions, boost::is_any_of("|"), boost::token_compress_on);

                // Retrieve the signalInstanceId
                const std::string& signalInstanceId = (header->has("signalInstanceId") ? // const ref is essential!
                                                       header->get<std::string>("signalInstanceId") :
                                                       std::string("unknown"));


                BOOST_FOREACH(const string& instanceSlots, allSlots) {
                    //KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Processing instanceSlots: " << instanceSlots;
                    const size_t pos = instanceSlots.find_first_of(":");
                    if (pos == std::string::npos) {
                        KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Badly shaped message header, instanceSlots '"
                                << instanceSlots << "' lack a ':'.";
                        continue;
                    }
                    const string instanceId(instanceSlots.substr(0, pos));
                    // We should call only functions defined for our instanceId or global ("*") ones
                    const bool globalCall = (instanceId == "*");
                    if (!globalCall && instanceId != m_instanceId) continue;

                    const vector<string> slotFunctions = karabo::util::fromString<string, vector>(instanceSlots.substr(pos + 1));


                    BOOST_FOREACH(const string& slotFunction, slotFunctions) {
                        try {
                            // Check whether slot is callable
                            if (m_slotCallGuardHandler) {
                                // This function will throw an exception in case the slot is not callable
                                m_slotCallGuardHandler(slotFunction, signalInstanceId);
                            }

                            SlotInstancePointer slot = getSlot(slotFunction);
                            if (slot) {
                                slot->callRegisteredSlotFunctions(*header, *body);
                                sendPotentialReply(*header, slotFunction, globalCall);
                            } else if (!globalCall) {
                                // Warn on non-existing slot, but only if directly addressed:
                                KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Received a message from '"
                                        << signalInstanceId << "' to non-existing slot \"" << slotFunction << "\"";
                            } else {
                                KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Miss globally called slot " << slotFunction;
                            }
                        } catch (const std::exception& e) {
                            const std::string msg(e.what());
                            KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << ": Exception in slot '"
                                    << slotFunction << "': " << msg;
                            replyException(*header, msg);
                        } catch (...) {
                            KARABO_LOG_FRAMEWORK_ERROR << this->getInstanceId() << ": Unknown exception in slot '"
                                    << slotFunction << " happened";
                            replyException(*header, "unknown exception");
                        }
                    }
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << ": Exception while processing slot call: " << e;
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << ": Unknown exception while processing slot call.";
            }
        }


        void SignalSlotable::registerReply(const karabo::util::Hash::Pointer& reply) {
            boost::mutex::scoped_lock lock(m_replyMutex);
            m_replies[boost::this_thread::get_id()] = reply;
        }


        void SignalSlotable::replyException(const karabo::util::Hash& header, const std::string& message) {
            if (header.has("replyTo")) {
                const std::string targetInstanceId = header.get<string>("signalInstanceId");
                Hash::Pointer replyHeader = boost::make_shared<Hash>();
                replyHeader->set("error", true);
                replyHeader->set("replyFrom", header.get<std::string > ("replyTo"));
                replyHeader->set("signalInstanceId", m_instanceId);
                replyHeader->set("signalFunction", "__reply__");
                replyHeader->set("slotInstanceIds", "|" + targetInstanceId + "|");
                Hash::Pointer replyBody = boost::make_shared<Hash>("a1", message);
                doSendMessage(targetInstanceId, replyHeader, replyBody, KARABO_SYS_PRIO, KARABO_SYS_TTL);
            }
        }


        void SignalSlotable::sendPotentialReply(const karabo::util::Hash& header,
                                                const std::string& slotFunction, bool global) {

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
                            << slotFunction << "' (i.e. globally).";
                }
                // KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << ": sendPotentialReply - but global request.";
                return;
            }

            // For caseRequestNoWait it does not make sense to send an empty reply if
            // the called slot did not place an answer (argument mismatch for reply slot).
            Replies::iterator it = m_replies.find(boost::this_thread::get_id());
            if (caseRequestNoWait && it == m_replies.end()) {
                KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Slot '"
                        << slotFunction << "' did not place a "
                        << "reply, but was called via requestNoWait";
                return;
            }

            // We are left with valid requests/requestNoWaits. For requests, we send an empty
            // reply if the slot did not place one. That tells the caller at least that
            // the slot finished - i.e. a synchronous request stops blocking.
            Hash::Pointer replyHeader = boost::make_shared<Hash>();

            std::string targetInstanceId;
            if (caseRequest) {
                targetInstanceId = header.get<string>("signalInstanceId");
                replyHeader->set("replyFrom", header.get<std::string > ("replyTo"));
                replyHeader->set("signalInstanceId", m_instanceId);
                replyHeader->set("signalFunction", "__reply__");
                replyHeader->set("slotInstanceIds", "|" + targetInstanceId + "|");
            } else { // i.e. caseRequestNoWait with a reply properly placed
                targetInstanceId = header.get<string>("replyInstanceIds");
                replyHeader->set("signalInstanceId", m_instanceId);
                replyHeader->set("signalFunction", "__replyNoWait__");
                replyHeader->set("slotInstanceIds", header.get<string>("replyInstanceIds"));
                replyHeader->set("slotFunctions", header.get<string>("replyFunctions"));
            }
            // Inject an empty reply in case that no one was provided in the slot body.
            Hash::Pointer replyBody;
            if (it != m_replies.end()) {
                replyBody = it->second;
                m_replies.erase(it);
            } else {
                replyBody = boost::make_shared<Hash>();
            }
            // Our answer to slotPing may interest someone remote trying to come up with our instanceId,
            // so we must not bypass the broker.
            const bool viaBroker = (slotFunction == "slotPing");
            doSendMessage(targetInstanceId, replyHeader, replyBody, KARABO_SYS_PRIO, KARABO_SYS_TTL,
                          m_topic, viaBroker);
        }


        void SignalSlotable::registerDefaultSignalsAndSlots() {

            // The heartbeat signal goes through a different topic, so we
            // cannot use the normal registerSignal.
            Signal::Pointer heartbeatSignal = boost::make_shared<Signal>(this, m_heartbeatProducerChannel,
                                                                         m_instanceId, "signalHeartbeat", KARABO_SYS_PRIO,
                                                                         KARABO_SYS_TTL);
            heartbeatSignal->setTopic(m_topic + "_beats");
            storeSignal("signalHeartbeat", heartbeatSignal);


            // Listener for heartbeats
            KARABO_SLOT3(slotHeartbeat, string /*instanceId*/, int /*heartbeatIntervalInSec*/, Hash /*instanceInfo*/)

            KARABO_SYSTEM_SIGNAL("signalInstanceNew", string, Hash);

            KARABO_SYSTEM_SIGNAL("signalInstanceGone", string, Hash);

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

            KARABO_SLOT0(slotGetOutputChannelNames)
        }


        void SignalSlotable::trackAllInstances() {
            m_trackAllInstances = true;
            m_heartbeatConsumerChannel = m_connection->createConsumer(m_topic + "_beats",
                                                                      "signalFunction = 'signalHeartbeat'");
            m_heartbeatConsumerChannel->readAsync(bind_weak(&karabo::xms::SignalSlotable::onHeartbeatMessage, this, _1, _2));
            startTrackingSystem();
        }


        void SignalSlotable::slotInstanceNew(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {

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


        void SignalSlotable::slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {

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


        JmsConnection::Pointer SignalSlotable::getConnection() const {
            return m_connection;
        }


        void SignalSlotable::emitHeartbeat(const boost::system::error_code& e) {
            if (e) return;
            try {
                emit("signalHeartbeat", getInstanceId(), m_heartbeatInterval, m_instanceInfo);
            } catch (Exception &e) {
                KARABO_LOG_FRAMEWORK_ERROR << "emitHeartbeat triggered an exception: " << e;
            } catch (std::exception &e) {
                KARABO_LOG_FRAMEWORK_ERROR << "emitHeartbeat triggered a standard exception: " << e.what();
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "emitHeartbeat triggered an unknown exception";
            }
            m_heartbeatTimer.expires_from_now(boost::posix_time::seconds(m_heartbeatInterval));
            m_heartbeatTimer.async_wait(bind_weak(&karabo::xms::SignalSlotable::emitHeartbeat, this,
                                                  boost::asio::placeholders::error));
        }


        Hash SignalSlotable::getAvailableInstances(bool activateTracking) {
            KARABO_LOG_FRAMEWORK_DEBUG << "getAvailableInstances";
            if (!m_trackAllInstances) {
                boost::mutex::scoped_lock lock(m_trackedInstancesMutex);
                m_trackedInstances.clear();
            }
            call("*", "slotPing", m_instanceId, 0, activateTracking);
            // The function slotPingAnswer will be called by all instances available now
            // Lets wait a fair amount of time - huaaah this is bad isn't it :-(
            // Since we block here for a long time, add a thread to ensure that all slotPingAnswer can be processed.
            EventLoop::addThread();
            boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
            EventLoop::removeThread();
            boost::mutex::scoped_lock lock(m_trackedInstancesMutex);
            KARABO_LOG_FRAMEWORK_DEBUG << "Available instances: " << m_trackedInstances;
            return m_trackedInstances;
        }


        std::pair<bool, std::string > SignalSlotable::exists(const std::string& instanceId) {
            string hostname;
            Hash instanceInfo;
            try {
                this->request(instanceId, "slotPing", instanceId, 1, false).timeout(200).receive(instanceInfo);
            } catch (const karabo::util::TimeoutException&) {
                Exception::clearTrace();
                return std::make_pair(false, hostname);
            }
            if (instanceInfo.has("host")) instanceInfo.get("host", hostname);
            return std::make_pair(true, hostname);
        }


        void SignalSlotable::slotPing(const std::string& instanceId, int rand, bool trackPingedInstance) {

            if (rand != 0) {
                // case 1) Called by an instance that is coming up: rand is his m_randPing before it gets 'valid',
                // case 2) or by SignalSlotable::exists or SignalSlotable::connectP2P: rand is 1
                if (instanceId == m_instanceId) {
                    if (rand == m_randPing) {
                        // We are in case 1) and I ask myself. I must not answer, at least not in time.
                        // HACK: Let's wait until my own request timed out for sure.
                        boost::this_thread::sleep(boost::posix_time::milliseconds(msPingTimeoutInIsValidInstanceId * 1.5));
                    }
                    // else:
                    // m_randPing == 0 (I am up) or >= 2 (I am 'booting')
                    // 1) It is not me, so that guy must not come up: tell him. Note: Two guys coming up
                    //    at the same time with the same id might both fail here.
                    // 2) I just reply my existence.
                    reply(m_instanceInfo);
                }
            } else if (!m_randPing) {
                // I should only answer, if my name got accepted which is indicated by a value of m_randPing==0
                call(instanceId, "slotPingAnswer", m_instanceId, m_instanceInfo);
            }
        }


        std::vector<string> SignalSlotable::getAvailableSignals(const std::string& instanceId) {
            std::vector<string> signals;
            try {
                request(instanceId, "slotGetAvailableFunctions", "signals").timeout(100).receive(signals);
            } catch (const karabo::util::TimeoutException&) {
                karabo::util::Exception::clearTrace();
                cout << "ERROR:  The requested instanceId \"" << instanceId << "\" is currently not available." << endl;
            }
            return signals;
        }


        std::vector<string> SignalSlotable::getAvailableSlots(const std::string& instanceId) {
            std::vector<string> slots;
            try {
                request(instanceId, "slotGetAvailableFunctions", "slots").timeout(100).receive(slots);
            } catch (const karabo::util::TimeoutException&) {
                karabo::util::Exception::clearTrace();
                cout << "ERROR:  The requested instanceId \"" << instanceId << "\" is currently not available." << endl;
            }
            return slots;
        }


        const SignalSlotable::SlotInstancePointer& SignalSlotable::getSenderInfo(const std::string& slotFunction) {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            auto it = m_slotInstances.find(slotFunction);
            if (it == m_slotInstances.end()) throw KARABO_SIGNALSLOT_EXCEPTION("No slot-object could be found for slotFunction \"" + slotFunction + "\"");
            return it->second;
        }


        void SignalSlotable::slotGetAvailableFunctions(const std::string& type) {
            std::vector<string> functions;
            if (type == "signals") {
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                for (auto it = m_signalInstances.cbegin(); it != m_signalInstances.cend(); ++it) {
                    const string& function = it->first;
                    functions.push_back(function);
                }
            } else if (type == "slots") {
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                for (auto it = m_slotInstances.cbegin(); it != m_slotInstances.cend(); ++it) {
                    const string& function = it->first;
                    // Filter out service slots // TODO finally update to last set of those
                    if (function == "slotConnectToSignal" || function == "slotDisconnectFromSignal" || function == "slotGetAvailableFunctions" ||
                        function == "slotHasSlot" || function == "slotHeartbeat" || function == "slotPing" || function == "slotPingAnswer") {
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


        const std::string& SignalSlotable::getInstanceId() const {
            return m_instanceId;
        }


        void SignalSlotable::updateInstanceInfo(const karabo::util::Hash& update) {
            m_instanceInfo.merge(update);
            call("*", "slotInstanceUpdated", m_instanceId, m_instanceInfo);
        }


        const karabo::util::Hash& SignalSlotable::getInstanceInfo() const {
            return m_instanceInfo;
        }


        void SignalSlotable::registerInstanceNewHandler(const InstanceInfoHandler& instanceNewHandler) {
            m_instanceNewHandler = instanceNewHandler;
        }


        void SignalSlotable::registerInstanceGoneHandler(const InstanceInfoHandler& instanceGoneHandler) {
            m_instanceGoneHandler = instanceGoneHandler;
        }


        void SignalSlotable::registerInstanceUpdatedHandler(const InstanceInfoHandler& instanceUpdatedHandler) {
            m_instanceUpdatedHandler = instanceUpdatedHandler;
        }


        void SignalSlotable::registerSlotCallGuardHandler(const SlotCallGuardHandler& slotCallGuardHandler) {
            m_slotCallGuardHandler = slotCallGuardHandler;
        }


        bool SignalSlotable::connectChannels(const std::string& outputInstanceId, const std::string& outputName,
                                             const std::string& inputInstanceId, const std::string& inputName) {

            const std::string& outputId = outputInstanceId.empty() ? m_instanceId : outputInstanceId;
            const std::string& inputId = inputInstanceId.empty() ? m_instanceId : inputInstanceId;

            bool outputChannelExists = false;
            karabo::util::Hash outputChannelInfo;
            try {
                this->request(outputId, "slotGetOutputChannelInformation", outputName, static_cast<int> (getpid()))
                        .timeout(1000).receive(outputChannelExists, outputChannelInfo);
            } catch (const karabo::util::TimeoutException&) {
                karabo::util::Exception::clearTrace();
                outputChannelExists = false;
            }

            bool inputChannelExists = false;
            if (outputChannelExists) {
                try {
                    this->request(inputId, "slotConnectToOutputChannel", inputName, outputChannelInfo, true)
                            .timeout(1000).receive(inputChannelExists);
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    inputChannelExists = false;
                }
            }

            if (outputChannelExists && inputChannelExists) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Successfully connected '" << inputId << '.' << inputName
                        << "' to '" << outputId << '.' << outputName << '.';
                return true;
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Failed to connect '" << inputId << '.' << inputName
                        << "' to '" << outputId << '.' << outputName << '.';
                return false;
            }
        }


        void SignalSlotable::slotConnectToOutputChannel(const std::string& inputName, const karabo::util::Hash& outputChannelInfo, bool connect) {
            // Loop channels
            InputChannels::const_iterator it = m_inputChannels.find(inputName);
            if (it != m_inputChannels.end()) {
                if (connect) it->second->connect(outputChannelInfo); // Synchronous
                else it->second->disconnect(outputChannelInfo);
                reply(true);
            }

            reply(false);
        }


        bool SignalSlotable::disconnectChannels(const std::string& outputInstanceId, const std::string& outputName,
                                                const std::string& inputInstanceId, const std::string& inputName) {

            const std::string& outputId = outputInstanceId.empty() ? m_instanceId : outputInstanceId;
            const std::string& inputId = inputInstanceId.empty() ? m_instanceId : inputInstanceId;

            bool outputChannelExists = false;

            // Need to get the outputChannelInfo (containing amongst others, port and host)
            karabo::util::Hash outputChannelInfo;

            try {
                this->request(outputId, "slotGetOutputChannelInformation", outputName, static_cast<int> (getpid()))
                        .timeout(1000).receive(outputChannelExists, outputChannelInfo);
            } catch (const karabo::util::TimeoutException&) {
                karabo::util::Exception::clearTrace();
                outputChannelExists = false;
            }

            bool inputChannelExists = false;
            if (outputChannelExists) {
                try {
                    this->request(inputId, "slotConnectToOutputChannel", inputName, outputChannelInfo, false)
                            .timeout(1000).receive(inputChannelExists);
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    inputChannelExists = false;
                }
            }

            if (outputChannelExists && inputChannelExists) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Successfully disconnected '" << inputId << '.' << inputName
                        << "' from '" << outputId << '.' << outputName << '.';
                return true;
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Failed to disconnect '" << inputId << '.' << inputName
                        << "' from '" << outputId << '.' << outputName << '.';
                return false;
            }
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
                    KARABO_LOG_FRAMEWORK_WARN << "Could not connect slot '" << slotInstanceId << "." << slotFunction
                            << "' to (non-existing?) signal '" << signalInstanceId << "." << signalFunction << "'. Will try"
                            << " again if '" << slotInstanceId << "' or '" << signalInstanceId << "' send signalInstanceNew.";
                }
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Did not try to connect non-existing slot '" << slotInstanceId << "." << slotFunction
                        << "' to signal '" << signalInstanceId << "." << signalFunction << "'. Will try"
                        << " again if '" << slotInstanceId << "' or '" << signalInstanceId << "' send signalInstanceNew.";
            }

            return false;
        }


        bool SignalSlotable::tryToConnectToSignal(const std::string& signalInstanceId, const std::string& signalFunction,
                                                  const std::string& slotInstanceId, const std::string& slotFunction) {

            bool signalExists = false;

            if (signalInstanceId == m_instanceId) { // Local signal requested
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                auto it = m_signalInstances.find(signalFunction);
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
                    KARABO_LOG_FRAMEWORK_WARN << "Remote instance '" << signalInstanceId << "' did not respond in time"
                            << " the request to connect to its signal '" << signalFunction << "'.";
                }
            }
            return signalExists;
        }


        SignalSlotable::SlotInstancePointer SignalSlotable::findSlot(const std::string& funcName) {
            SlotInstancePointer ret;
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            SlotInstances::const_iterator it = m_slotInstances.find(funcName);
            if (it != m_slotInstances.end()) {
                ret = it->second;
            }
            return ret;
        }


        void SignalSlotable::registerNewSlot(const std::string&funcName, SlotInstancePointer instance) {
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
                auto it = m_signalInstances.find(signalFunction);
                if (it != m_signalInstances.end()) {
                    it->second->registerSlot(slotInstanceId, slotFunction);
                    result = true;
                }
            }

            reply(result);
        }


        bool SignalSlotable::instanceHasSlot(const std::string& slotInstanceId, const std::string& unmangledSlotFunction) {

            if (slotInstanceId == "*") return true; // GLOBAL slots may or may not exist
            
            //convert noded slots to follow underscore representation
            const  std::string& mangledSlotFunction = (unmangledSlotFunction.find('.') == std::string::npos ? unmangledSlotFunction 
                                                      : boost::algorithm::replace_all_copy(unmangledSlotFunction, ".", "_"));
            
            bool slotExists = false;

            if (slotInstanceId == m_instanceId) { // Local slot requested
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                if (m_slotInstances.find(mangledSlotFunction) != m_slotInstances.end()) { // Slot found
                    slotExists = true;
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Requested slot '" << mangledSlotFunction
                            << "' is currently not available locally on instance '" << m_instanceId << "'.";
                }
            } else {// Remote slot requested
                try {
                    request(slotInstanceId, "slotHasSlot", mangledSlotFunction).timeout(1000).receive(slotExists);
                    if (!slotExists) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Requested slot '" << mangledSlotFunction
                                << "' is currently not available on remote instance '" << slotInstanceId << "'.";
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    slotExists = false;
                    KARABO_LOG_FRAMEWORK_WARN << "Remote instance '" << slotInstanceId << "' did not respond in time"
                            << " whether it has a slot '" << mangledSlotFunction << "'.";
                }
            }
            return slotExists;
        }


        void SignalSlotable::slotHasSlot(const std::string& unmangledSlotFunction) {
            //handle noded slots
            const  std::string& mangledSlotFunction = (unmangledSlotFunction.find('.') == std::string::npos ? unmangledSlotFunction 
                                                      : boost::algorithm::replace_all_copy(unmangledSlotFunction, ".", "_"));
            
            bool result = false;
            {
                boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
                if (m_slotInstances.find(mangledSlotFunction) != m_slotInstances.end()) {
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
            const boost::optional<const Hash::Node&> beatsNode = instanceInfo.find("heartbeatInterval");
            std::string sanifiedInstanceId(instanceId);
            sanifyInstanceId(sanifiedInstanceId);
            if (!beatsNode || sanifiedInstanceId != instanceId) {
                KARABO_LOG_FRAMEWORK_ERROR << "Cannot track '" << instanceId << "' since its instanceId is invalid or "
                        << "its instanceInfo lacks the 'heartbeatInterval': " << instanceInfo;
                return;
            }

            boost::mutex::scoped_lock lock(m_trackedInstancesMutex);
            Hash h;
            h.set("instanceInfo", instanceInfo);
            // Initialize countdown with the heartbeat interval
            h.set("countdown", beatsNode->getValue<int>());
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


        void SignalSlotable::updateTrackedInstanceInfo(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            boost::mutex::scoped_lock lock(m_trackedInstancesMutex);
            if (m_trackedInstances.has(instanceId)) {
                Hash& h = m_trackedInstances.get<Hash>(instanceId);
                h.set("instanceInfo", instanceInfo);
                h.set("countdown", instanceInfo.get<int>("heartbeatInterval"));
            }
        }


        void SignalSlotable::addTrackedInstanceConnection(const std::string& instanceId, const karabo::util::Hash& connection) {
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

            bool disconnected = false;

            if (signalInstanceId == m_instanceId) { // Local signal requested

                if (signalFunction == "signalHeartbeat") {
                    // Never disconnect from heartbeats - why?
                    disconnected = true;
                } else {
                    disconnected = tryToUnregisterSlot(signalFunction, slotInstanceId, slotFunction);
                }
                if (!disconnected) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Could not disconnect slot '" << slotInstanceId << "." << slotFunction
                            << "' from local signal '" << m_instanceId << "." << signalFunction << "'.";
                }
            } else { // Remote signal requested
                try {
                    request(signalInstanceId, "slotDisconnectFromSignal", signalFunction, slotInstanceId, slotFunction).timeout(1000).receive(disconnected);
                    if (!disconnected) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Could not disconnect slot '" << slotInstanceId << "." << slotFunction
                                << "' from remote signal '" << m_instanceId << "." << signalFunction << "'.";
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    disconnected = false;
                    KARABO_LOG_FRAMEWORK_WARN << "Remote instance '" << signalInstanceId << "' did not respond in time"
                            << " the request to disconnect slot '" << slotInstanceId << "." << slotFunction << "' from"
                            << " its signal '" << signalFunction << "'.";
                }
            }

            return disconnected;
        }


        bool SignalSlotable::tryToUnregisterSlot(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction) {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            auto it = m_signalInstances.find(signalFunction);
            if (it != m_signalInstances.end()) { // Signal found
                // Unregister slotId from local signal
                return it->second->unregisterSlot(slotInstanceId, slotFunction);
            }
            return false;
        }


        void SignalSlotable::slotDisconnectFromSignal(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction) {
            if (signalFunction == "signalHeartbeat") {
                // Never disconnect from heartbeats - why?
                reply(true);
            } else if (tryToUnregisterSlot(signalFunction, slotInstanceId, slotFunction)) {
                reply(true);
            } else {
                reply(false);
            }
        }


        bool SignalSlotable::hasSlot(const std::string& unmangledSlotFunction) const {
            //handle noded slots
            const  std::string& mangledSlotFunction = (unmangledSlotFunction.find('.') == std::string::npos ? unmangledSlotFunction 
                                                      : boost::algorithm::replace_all_copy(unmangledSlotFunction, ".", "_"));
            
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            return m_slotInstances.find(mangledSlotFunction) != m_slotInstances.end();
        }


        SignalSlotable::SlotInstancePointer SignalSlotable::getSlot(const std::string& unmangledSlotFunction) const {
            //handle noded slots
            const  std::string& mangledSlotFunction = (unmangledSlotFunction.find('.') == std::string::npos ? unmangledSlotFunction 
                                                      : boost::algorithm::replace_all_copy(unmangledSlotFunction, ".", "_"));
            
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            SlotInstances::const_iterator it = m_slotInstances.find(mangledSlotFunction);
            if (it != m_slotInstances.end()) return it->second;
            return SlotInstancePointer();
        }


        void SignalSlotable::removeSlot(const std::string& unmangledSlotFunction) {
            //handle noded slots
            const  std::string& mangledSlotFunction = (unmangledSlotFunction.find('.') == std::string::npos ? unmangledSlotFunction 
                                                      : boost::algorithm::replace_all_copy(unmangledSlotFunction, ".", "_"));
            
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            m_slotInstances.erase(mangledSlotFunction);
            // Will clean any associated timers to this slot
            m_receiveAsyncTimeoutHandlers.erase(mangledSlotFunction);
        }


        bool SignalSlotable::hasSignal(const std::string& signalFunction) const {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            return m_signalInstances.find(signalFunction) != m_signalInstances.end();
        }


        SignalSlotable::SignalInstancePointer SignalSlotable::getSignal(const std::string& signalFunction) const {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            auto it = m_signalInstances.find(signalFunction);
            if (it != m_signalInstances.end()) return it->second;
            return SignalInstancePointer();
        }


        string SignalSlotable::fetchInstanceId(const std::string& signalOrSlotId) const {
            return signalOrSlotId.substr(0, signalOrSlotId.find_last_of('/'));
        }


        std::pair<std::string, std::string> SignalSlotable::splitIntoInstanceIdAndFunctionName(const std::string& signalOrSlotId, const char sep) const {
            size_t pos = signalOrSlotId.find_last_of(sep);
            if (pos == std::string::npos) return std::make_pair("", signalOrSlotId);
            std::string instanceId = signalOrSlotId.substr(0, pos);
            std::string functionName = signalOrSlotId.substr(pos);
            return std::make_pair(instanceId, functionName);
        }


        void SignalSlotable::storeSignal(const std::string&signalFunction, SignalInstancePointer signalInstance) {
            m_signalInstances[signalFunction] = signalInstance;
        }


        void SignalSlotable::letInstanceSlowlyDieWithoutHeartbeat(const boost::system::error_code& e) {

            if (e) return;

            try {

                if (m_trackAllInstances) {

                    vector<pair<string, Hash> > deadOnes;

                    decreaseCountdown(deadOnes);

                    for (size_t i = 0; i < deadOnes.size(); ++i) {
                        KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Instance \"" << deadOnes[i].first <<
                                "\" silently disappeared (no heartbeats received anymore)";
                        emit("signalInstanceGone", deadOnes[i].first, deadOnes[i].second);
                        eraseTrackedInstance(deadOnes[i].first);
                    }
                }

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "letInstanceSlowlyDieWithoutHeartbeat triggered an exception: " << e.what();
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "letInstanceSlowlyDieWithoutHeartbeat triggered an unknown exception";
            }

            // We are sleeping five times as long as the count-down ticks (which ticks in seconds)
            m_trackingTimer.expires_from_now(boost::posix_time::seconds(5));
            m_trackingTimer.async_wait(bind_weak(&karabo::xms::SignalSlotable::letInstanceSlowlyDieWithoutHeartbeat,
                                                 this, boost::asio::placeholders::error));
        }


        void SignalSlotable::decreaseCountdown(std::vector<std::pair<std::string, karabo::util::Hash> >& deadOnes) {
            boost::mutex::scoped_lock lock(m_trackedInstancesMutex);

            for (Hash::iterator it = m_trackedInstances.begin(); it != m_trackedInstances.end(); ++it) {
                Hash& entry = it->getValue<Hash>();
                int& countdown = entry.get<int>("countdown");
                countdown--; // Regular count down

                if (countdown == 0) { // Instance lost
                    deadOnes.push_back(std::pair<string, Hash>(it->getKey(), entry.get<Hash>("instanceInfo")));
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
                                                                 const DataHandler& onDataAvailableHandler,
                                                                 const InputHandler& onInputAvailableHandler,
                                                                 const InputHandler& onEndOfStreamEventHandler) {
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


        OutputChannel::Pointer SignalSlotable::createOutputChannel(const std::string& channelName,
                                                                   const karabo::util::Hash& config,
                                                                   const OutputHandler& onOutputPossibleHandler) {
            if (!config.has(channelName)) throw KARABO_PARAMETER_EXCEPTION("The provided configuration must contain the channel name as key in the configuration");
            Hash channelConfig = config.get<Hash>(channelName);
            if (channelConfig.has("schema")) channelConfig.erase("schema");
            OutputChannel::Pointer channel = Configurator<OutputChannel>::create("OutputChannel", channelConfig);
            channel->setInstanceIdAndName(m_instanceId, channelName);
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


        std::vector<std::string> SignalSlotable::getOutputChannelNames() const {
            vector<string> names;
            for (const auto& x : m_outputChannels) names.push_back(x.first);
            return names;
        }


        std::vector<std::string> SignalSlotable::slotGetOutputChannelNames() {
            reply(getOutputChannelNames());
            return getOutputChannelNames();
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


        void SignalSlotable::registerInputHandler(const std::string& channelName, const InputHandler& handler) {
            getInputChannel(channelName)->registerInputHandler(handler);
        }


        void SignalSlotable::registerDataHandler(const std::string& channelName, const DataHandler& handler) {
            getInputChannel(channelName)->registerDataHandler(handler);
        }


        void SignalSlotable::registerEndOfStreamHandler(const std::string& channelName, const InputHandler& handler) {
            getInputChannel(channelName)->registerEndOfStreamEventHandler(handler);
        }


        void SignalSlotable::connectInputChannels() {
            // Loop channels
            for (InputChannels::const_iterator it = m_inputChannels.begin(); it != m_inputChannels.end(); ++it) {
                connectInputChannel(it->second);
            }
        }


        void SignalSlotable::reconnectInputChannels(const std::string& instanceId) {

            // Loop channels
            for (InputChannels::const_iterator it = m_inputChannels.begin(); it != m_inputChannels.end(); ++it) {
                const InputChannel::Pointer& channel = it->second;
                const std::map<std::string, karabo::util::Hash>& outputChannels = channel->getConnectedOutputChannels();
                for (std::map<std::string, karabo::util::Hash>::const_iterator ii = outputChannels.begin(); ii != outputChannels.end(); ++ii) {
                    const string& outputChannelString = ii->first;
                    if (instanceId != outputChannelString.substr(0, instanceId.size())) continue; // instanceId ~ instanceId@output
                    KARABO_LOG_FRAMEWORK_DEBUG << "reconnectInputChannels for '" << m_instanceId
                            << "' to output channel '" << outputChannelString << "'";
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


        void SignalSlotable::slotGetOutputChannelInformation(const std::string& ioChannelId, const int& processId) {
            OutputChannels::const_iterator it = m_outputChannels.find(ioChannelId);
            if (it != m_outputChannels.end()) {
                karabo::util::Hash h(it->second->getInformation());
                if (processId == static_cast<int> (getpid())) {
                    h.set("memoryLocation", "local");
                } else {
                    h.set("memoryLocation", "remote");
                }
                reply(true, h);
            } else {
                reply(false, karabo::util::Hash());
            }
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


        void SignalSlotable::registerSynchronousReply(const std::string& replyId) {
            boost::shared_ptr<BoostMutexCond> bmc = boost::make_shared<BoostMutexCond>();
            {
                boost::mutex::scoped_lock lock(m_receivedRepliesBMCMutex);
                m_receivedRepliesBMC[replyId] = bmc;
            }
        }


        bool SignalSlotable::timedWaitAndPopReceivedReply(const std::string& replyId, karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body, int timeout) {
            bool result = true;
            boost::shared_ptr<BoostMutexCond> bmc;
            {
                boost::mutex::scoped_lock lock(m_receivedRepliesBMCMutex);
                bmc = m_receivedRepliesBMC[replyId];
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
                                    bind_weak(&SignalSlotable::onP2pMessage, this, _1, _2));
            return true;
        }


        void SignalSlotable::onP2pMessage(const karabo::util::Hash::Pointer& header,
                                          const karabo::util::Hash::Pointer& body) {

            EventLoop::getIOService().post(bind_weak(&SignalSlotable::processEvent, this, header, body));
            // To be equivalent to onBrokerMessage, we would have to re-register for the next p2p message.
            // Currently this is done in PointToPoint::Consumer::consume(...) and cannot easily be moved here.
        }


        void SignalSlotable::disconnectP2P(const std::string& signalInstanceId) {
            if (signalInstanceId == m_instanceId) return;
            m_pointToPoint->disconnect(signalInstanceId, m_instanceId);
        }


        void SignalSlotable::setTopic(const std::string& topic) {
            if (topic.empty()) {
                m_topic = "karabo";
                char* env = getenv("USER");
                if (env != 0) m_topic = string(env);
                env = getenv("KARABO_BROKER_TOPIC");
                if (env != 0) m_topic = string(env);
            }
        }


        void SignalSlotable::receiveAsyncTimeoutHandler(const boost::system::error_code& e, const std::string& replyId,
                                                        const boost::function<void()>& timeoutCallback) {
            if (e) return;
            // Remove the slot with function name replyId, as the message took too long
            removeSlot(replyId);
            if (timeoutCallback) {
                timeoutCallback();
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "Asynchronous request with id \"" << replyId << "\" timed out";
            }
        }


        void SignalSlotable::addReceiveAsyncTimer(const std::string& replyId, const boost::shared_ptr<boost::asio::deadline_timer>& timer) {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            m_receiveAsyncTimeoutHandlers[replyId] = timer;
        }


        boost::shared_ptr<boost::asio::deadline_timer> SignalSlotable::getReceiveAsyncTimer(const std::string& replyId) const {
            boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
            auto it = m_receiveAsyncTimeoutHandlers.find(replyId);
            if (it != m_receiveAsyncTimeoutHandlers.end()) {
                return it->second;
            }
            return boost::shared_ptr<boost::asio::deadline_timer>();
        }


        SignalSlotable::LatencyStats::LatencyStats() : sum(0), counts(0), maximum(0) {
        }


        void SignalSlotable::LatencyStats::add(unsigned int latency) {
            sum += latency;
            ++counts;
            if (latency > maximum) {
                maximum = latency;
            }
        }


        void SignalSlotable::LatencyStats::clear() {
            sum = counts = maximum = 0;
        }


        float SignalSlotable::LatencyStats::average() const {
            return counts > 0 ? sum / static_cast<float> (counts) : -1.f;
        }
    }
}
