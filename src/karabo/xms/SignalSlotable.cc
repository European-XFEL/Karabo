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

#include "SignalSlotable.hh"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>

#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/Validator.hh"
#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/Version.hh"

using namespace std::chrono;
using namespace std::literals::chrono_literals;
using std::placeholders::_1;
using std::placeholders::_2;

namespace {
    /**
     * Check need for slot name mangling (i.e. replacing dots from slots under node by `_`)
     *
     * @param unmangledSlotFunction string to mangle
     *
     * @return a pair - if its 'first' is true', use its 'second' as mangled function
     *                  if its 'first' is false, no need to mangle, use unmangledSlotFunction
     */
    std::pair<bool, std::string> mangleSlotFunction(const std::string& unmangledSlotFunction) {
        const char cStringSep[] = {karabo::data::Hash::k_defaultSep, '\0'};

        if (unmangledSlotFunction.find(karabo::data::Hash::k_defaultSep) == std::string::npos) {
            return {false, std::string()}; // no need for mangling
        } else {
            return {true, boost::algorithm::replace_all_copy(unmangledSlotFunction, cStringSep, "_")};
        }
    }
} // namespace

namespace karabo {
    namespace xms {

        using namespace std;
        using namespace karabo::data;
        using namespace karabo::util;
        using namespace karabo::data;
        using namespace karabo::net;

        static Hash getHeartbeatInfo(const Hash& instanceInfo);

        /// Milliseconds of timeout when asking for validity of my id at startup:
        const int msPingTimeoutInIsValidInstanceId = 1000;

        const int channelReconnectIntervalSec = 6; // seconds  - significantly longer than getOutChannelInfoTimeoutMsec
        const int getOutChannelInfoTimeoutMsec = 3000; // milliseconds - this plus time of few other async actions in
                                                       // the input channel reconnection cycle should be smaller than
                                                       // channelReconnectIntervalSec to avoid interference between
                                                       // reconnection cycles
        // Static initializations
        std::mutex SignalSlotable::m_uuidGeneratorMutex;
        boost::uuids::random_generator SignalSlotable::m_uuidGenerator;
        std::unordered_map<std::string, SignalSlotable::WeakPointer> SignalSlotable::m_sharedInstanceMap;
        std::shared_mutex SignalSlotable::m_sharedInstanceMapMutex;


        bool SignalSlotable::tryToCallDirectly(const std::string& instanceId, const karabo::data::Hash::Pointer& header,
                                               const karabo::data::Hash::Pointer& body) const {
            // Global calls must go via the broker
            if (instanceId == "*") return false;
            SignalSlotable::Pointer ptr = getInstanceForShortcut(instanceId);
            if (!ptr) return false;

            ptr->processEvent(header, body);
            return true;
        }


        void SignalSlotable::Requestor::receiveAsync(const std::function<void()>& replyCallback,
                                                     const SignalSlotable::Requestor::AsyncErrorHandler& errorHandler) {
            m_signalSlotable->registerSlot(replyCallback, m_replyId);
            registerErrorHandler(errorHandler);
            sendRequest();
        }


        void SignalSlotable::Requestor::registerErrorHandler(const AsyncErrorHandler& errorHandler) {
            // If timeout is not explicitely specified (default is zero), use twice the maximum travel time
            // to the broker as default. Do not allow negative values either.
            // Otherwise we have a little memory leak if the slotInstanceId is never responding, e.g.
            // since no such instance exists...
            const int timeout = (m_timeout > 0 ? m_timeout : m_defaultAsyncTimeout);

            // Register a timer and error handler into map
            auto timer = std::make_shared<boost::asio::steady_timer>(EventLoop::getIOService());
            timer->expires_after(milliseconds(timeout));
            timer->async_wait(bind_weak(&SignalSlotable::receiveAsyncTimeoutHandler, m_signalSlotable,
                                        boost::asio::placeholders::error, m_replyId, errorHandler));
            m_signalSlotable->addReceiveAsyncErrorHandles(m_replyId, timer, errorHandler);
        }

        void SignalSlotable::Requestor::getSignalInstanceId(const karabo::data::Hash::Pointer& header,
                                                            std::string& result) {
            if (header && header->has("signalInstanceId") && header->is<std::string>("signalInstanceId")) {
                header->get("signalInstanceId", result);
            }
        };

        std::pair<karabo::data::Hash::Pointer, karabo::data::Hash::Pointer>
        SignalSlotable::Requestor::receiveResponseHashes() {
            karabo::data::Hash::Pointer header, body;
            try {
                receiveResponse(header, body); // sends request and blocks until reply arrives or times out
                const boost::optional<karabo::data::Hash::Node&> errorNode = header->find("error");
                if (errorNode && errorNode->is<bool>() && errorNode->getValue<bool>()) {
                    // Handling an error, so double check that input is as expected, i.e. body has key "a1":
                    const boost::optional<karabo::data::Hash::Node&> textNode = body->find("a1");
                    const std::string text(textNode && textNode->is<std::string>()
                                                 ? textNode->getValue<std::string>()
                                                 : "Error signaled, but body without string at key \"a1\"");
                    const boost::optional<karabo::data::Hash::Node&> detailsNode = body->find("a2"); // since 2.14.0
                    const std::string details(detailsNode && detailsNode->is<std::string>()
                                                    ? detailsNode->getValue<std::string>()
                                                    : std::string());
                    throw karabo::data::RemoteException(text, header->get<std::string>("signalInstanceId"), details);
                }

            } catch (const karabo::data::TimeoutException&) {
                // rethrow as same type without message: detailedMsg() will show the full trace, userFriendlyMsg() just
                // one line
                KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION(std::string()));
            } catch (const karabo::data::RemoteException&) {
                throw; // Do not change the type, just throw again.
            } catch (const karabo::data::CastException&) {
                std::string signalInstanceId("unknown");
                getSignalInstanceId(header, signalInstanceId);
                const std::string msg("'" + m_signalSlotable->getInstanceId() +
                                      "' received incompatible response "
                                      "from '" +
                                      signalInstanceId + "'");
                KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION(msg));
            } catch (const karabo::data::Exception& e) {
                std::string signalInstanceId("unknown");
                getSignalInstanceId(header, signalInstanceId);
                const std::string msg("Error while '" + m_signalSlotable->getInstanceId() +
                                      "' received following reply from '" + signalInstanceId +
                                      "': " + (body ? toString(*body) : std::string()));
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION(msg));
            }

            return std::make_pair(header, body);
        }


        /**
         * Register a new slot function for a slot. A new slot is generated
         * if so necessary. It is checked that the signature of the new
         * slot is the same as an already registered one.
         */
        void SignalSlotable::registerSlot(const std::function<void()>& slot, const std::string& funcName) {
            // If the same slot name was registered under a different signature before,
            // the dynamic_pointer_cast will return a NULL pointer and finally registerNewSlot
            // will throw an exception.
            auto s = std::dynamic_pointer_cast<SlotN<void>>(findSlot(funcName));
            if (!s) {
                s = std::make_shared<SlotN<void>>(funcName);
                registerNewSlot(funcName, std::static_pointer_cast<Slot>(s));
            }
            s->registerSlotFunction(slot);
        }


        void SignalSlotable::doSendMessage(const std::string& instanceId, const karabo::data::Hash::Pointer& header,
                                           const karabo::data::Hash::Pointer& body, const std::string& topic,
                                           bool forceViaBroker) const {
            // Timestamp added to be able to measure latencies
            header->set("MQTimestamp", getEpochMillis());
            if (!forceViaBroker) {
                if (tryToCallDirectly(instanceId, header, body)) return;
            }

            const std::string& t = (topic.empty() ? m_topic : topic);
            m_connection->write(t, header, body);
        }


        void SignalSlotable::registerBroadcastHandler(
              std::function<void(const karabo::data::Hash::Pointer& header, const karabo::data::Hash::Pointer& body)>
                    handler) {
            m_broadCastHandler = handler;
        }


        karabo::data::Hash::Pointer SignalSlotable::prepareCallHeader(const std::string& slotInstanceId,
                                                                      const std::string& slotFunction) const {
            auto header = std::make_shared<Hash>();
            header->set("signalInstanceId", m_instanceId);
            header->set("signalFunction", "__call__");
            header->set("slotInstanceIds", "|" + slotInstanceId + "|");
            header->set("slotFunctions", "|" + slotInstanceId + ":" + slotFunction + "|");
            header->set("userName", m_username);
            return header;
        }


        SignalSlotable::Requestor::Requestor(SignalSlotable* signalSlotable)
            : m_signalSlotable(signalSlotable), m_replyId(SignalSlotable::generateUUID()), m_timeout(0) {}


        SignalSlotable::Requestor::~Requestor() {}


        SignalSlotable::Requestor& SignalSlotable::Requestor::timeout(const int& milliseconds) {
            m_timeout = milliseconds;
            return *this;
        }


        karabo::data::Hash::Pointer SignalSlotable::Requestor::prepareRequestHeader(const std::string& slotInstanceId,
                                                                                    const std::string& slotFunction) {
            karabo::data::Hash::Pointer header(new karabo::data::Hash);
            header->set("replyTo", m_replyId);
            header->set("signalInstanceId", m_signalSlotable->getInstanceId());
            header->set("signalFunction", "__request__");
            header->set("slotInstanceIds", "|" + slotInstanceId + "|");
            header->set("slotFunctions", "|" + slotInstanceId + ":" + slotFunction + "|");
            header->set("userName", m_signalSlotable->getUserName());
            return header;
        }


        karabo::data::Hash::Pointer SignalSlotable::Requestor::prepareRequestNoWaitHeader(
              const std::string& requestSlotInstanceId, const std::string& requestSlotFunction,
              const std::string& replySlotInstanceId, const std::string& replySlotFunction) {
            karabo::data::Hash::Pointer header(new karabo::data::Hash);
            // TODO Rename replyInstanceIds and replyFunctions to replyInstanceId and replyFunction
            header->set("replyInstanceIds", "|" + replySlotInstanceId + "|");
            header->set("replyFunctions", "|" + replySlotInstanceId + ":" + replySlotFunction + "|");
            header->set("signalInstanceId", m_signalSlotable->getInstanceId());
            header->set("signalFunction", "__requestNoWait__");
            header->set("slotInstanceIds", "|" + requestSlotInstanceId + "|");
            header->set("slotFunctions", "|" + requestSlotInstanceId + ":" + requestSlotFunction + "|");
            header->set("userName", m_signalSlotable->getUserName());
            return header;
        }


        void SignalSlotable::Requestor::registerRequest(const std::string& slotInstanceId,
                                                        const karabo::data::Hash::Pointer& header,
                                                        const karabo::data::Hash::Pointer& body) {
            m_slotInstanceId = slotInstanceId;
            m_header = header;
            m_body = body;
        }


        void SignalSlotable::Requestor::sendRequest() const {
            try {
                m_signalSlotable->doSendMessage(m_slotInstanceId, m_header, m_body);
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Problems sending request"));
            }
        }


        void SignalSlotable::Requestor::receiveResponse(karabo::data::Hash::Pointer& header,
                                                        karabo::data::Hash::Pointer& body) {
            m_signalSlotable->registerSynchronousReply(m_replyId);
            sendRequest();
            if (!m_signalSlotable->timedWaitAndPopReceivedReply(m_replyId, header, body, m_timeout)) {
                // FIXME: Add slotInstanceId and info from m_header etc
                throw KARABO_TIMEOUT_EXCEPTION("Reply timed out");
            }
        }

        std::vector<std::any> SignalSlotable::Requestor::receiveAsVecOfAny() {
            karabo::data::Hash::Pointer replyBodyPtr = receiveResponseHashes().second;
            std::set<std::string> paths;
            std::vector<std::any> replyValues;
            replyValues.reserve(replyBodyPtr->size());
            for (auto it = replyBodyPtr->mbegin(); it != replyBodyPtr->mend(); ++it) {
                replyValues.push_back(it->second.getValueAsAny());
            }
            return replyValues;
        }

        SignalSlotable::SignalSlotable()
            : m_randPing(rand() + 2),
              m_broadcastEventStrand(std::make_shared<karabo::net::Strand>(EventLoop::getIOService())),
              m_trackAllInstances(false),
              m_heartbeatInterval(10),
              m_trackingTimer(EventLoop::getIOService()),
              m_heartbeatTimer(EventLoop::getIOService()),
              m_performanceTimer(EventLoop::getIOService()),
              m_channelConnectTimer(EventLoop::getIOService()) {
            // TODO: Consider to move setTopic() to init(..) and inside it set topic from connection (instead of from
            // environment).
            //       Caveat: Ensure that Signals registered in device constructors get the correct topic!
            setTopic();
            EventLoop::addThread(); // possibly needed e.g. for ensureInstanceIdIsValid, see note therein
        }


        SignalSlotable::SignalSlotable(const string& instanceId, const Broker::Pointer& connection,
                                       const int heartbeatInterval, const karabo::data::Hash& instanceInfo)
            : SignalSlotable() {
            init(instanceId, connection, heartbeatInterval, instanceInfo);
        }


        SignalSlotable::SignalSlotable(const std::string& instanceId, const karabo::data::Hash& brokerConfiguration,
                                       const int heartbeatInterval, const karabo::data::Hash& instanceInfo)
            : SignalSlotable() {
            // Create Broker and call init(..)
            Broker::Pointer connection =
                  Configurator<Broker>::create(Broker::brokerTypeFromEnv(), Hash("instanceId", instanceId));
            init(instanceId, connection, heartbeatInterval, instanceInfo);
        }


        SignalSlotable::~SignalSlotable() {
            if (!m_randPing) {
                deregisterFromShortcutMessaging(); // registered after setting m_rand = 0
                stopTrackingSystem();
                m_heartbeatTimer.cancel();

                KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << m_instanceId << "\" shuts cleanly down";
                std::shared_lock<std::shared_mutex> lock(m_instanceInfoMutex);
                call("*", "slotInstanceGone", m_instanceId, m_instanceInfo);
            }
            EventLoop::removeThread();
        }


        void SignalSlotable::deregisterFromShortcutMessaging() {
            std::unique_lock<std::shared_mutex> lock(m_sharedInstanceMapMutex);
            // Just erase the weak pointer - cannot promote to shared pointer and check whether it is really 'this'
            // since this method is called from destructor, so shared pointer to this is already gone.
            if (m_sharedInstanceMap.erase(m_instanceId) == 0) {
                KARABO_LOG_FRAMEWORK_WARN << m_instanceId
                                          << " failed to deregisterFromShortcutMessaging: not registered";
            }
            // Clear local map - with same mutex lock order as in registerForShortcutMessaging()
            std::lock_guard<std::mutex> lock2(m_myInstanceMapMutex);
            m_myInstanceMap.clear();
        }


        void SignalSlotable::registerForShortcutMessaging() {
            std::unique_lock<std::shared_mutex> lock(m_sharedInstanceMapMutex);
            auto itAndSuccess = m_sharedInstanceMap.insert(std::make_pair(m_instanceId, weak_from_this()));
            if (!itAndSuccess.second) {
                KARABO_LOG_FRAMEWORK_WARN
                      << m_instanceId << ": Failed to register for short-cut "
                      << "messaging since there is already another instance, pointer "
                      << m_sharedInstanceMap[m_instanceId].lock().get(); // just to see whether it is non zero..
            }
            // Copy over who is present so far - need both mutex locks to ensure consistency
            std::lock_guard<std::mutex> lock2(m_myInstanceMapMutex);
            m_myInstanceMap = m_sharedInstanceMap;
        }


        void SignalSlotable::init(const std::string& instanceId, const karabo::net::Broker::Pointer& connection,
                                  const int heartbeatInterval, const karabo::data::Hash& instanceInfo,
                                  bool consumeBroadcasts) {
            m_instanceId = instanceId;
            m_connection = connection;
            // Inject actual instanceId into Broker connection object (new/old) if needed
            if (m_connection->getInstanceId() != m_instanceId) {
                std::ostringstream oss;
                oss << "*** The instanceId in connection: \"" << m_connection->getInstanceId()
                    << "\" doesn't match the requested \"" << m_instanceId << "\"";
                throw KARABO_SIGNALSLOT_EXCEPTION(oss.str());
            }
            // Set the flag defining the way how to consume broadcast messages
            m_connection->setConsumeBroadcasts(consumeBroadcasts);

            if (heartbeatInterval <= 0) {
                throw KARABO_SIGNALSLOT_EXCEPTION("Non-positive heartbeat interval: " + toString(heartbeatInterval));
            }
            m_heartbeatInterval = heartbeatInterval;
            // Threading not yet established for this instance, so no mutex lock needed
            m_instanceInfo = instanceInfo;

            if (!m_connection->isConnected()) {
                m_connection->connect();
            }

            registerDefaultSignalsAndSlots();

            // No mutex lock needed yet, see above
            m_instanceInfo.set("heartbeatInterval", m_heartbeatInterval);
            m_instanceInfo.set("karaboVersion", karabo::util::Version::getVersion());
            if (!m_instanceInfo.has("type")) {
                m_instanceInfo.set("type", "unknown");
            }
        }


        void SignalSlotable::start() {
            ensureInstanceIdIsValid(m_instanceId);
            m_connection->startReading(bind_weak(&SignalSlotable::processEvent, this, _1, _2),
                                       bind_weak(&SignalSlotable::consumerErrorNotifier, this, std::string(), _1, _2));
            ensureInstanceIdIsUnique(m_instanceId);
            KARABO_LOG_FRAMEWORK_INFO << "Instance starts up in topic '" << getTopic() << "' as '" << m_instanceId
                                      << "' - Karabo " << karabo::util::Version::getVersion();
            m_randPing = 0; // Allows to answer on slotDiscover.
            registerForShortcutMessaging();
            startPerformanceMonitor();
            {
                std::shared_lock<std::shared_mutex> lock(m_instanceInfoMutex);
                call("*", "slotInstanceNew", m_instanceId, m_instanceInfo);
            }
            // Start emitting heartbeats
            delayedSendHeartbeat(m_heartbeatInterval);
        }


        void SignalSlotable::ensureInstanceIdIsValid(const std::string& instanceId) {
            // space ' ' causes problem in xml serialisaton
            // dot '.' (i.e. Hash::k_defaultSep) is bad if id used as key in Hash
            // colon ':' separates instanceId and pipeline channel name
            const char* allowedCharacters =
                  "0123456789_/-" // not std::string to save dynamic memory allocation
                  "abcdefghijklmnopqrstuvwxyz"
                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            if (instanceId.empty() || instanceId.find_first_not_of(allowedCharacters) != std::string::npos) {
                const std::string msg("Instance id '" + instanceId +
                                      "' invalid - must contain one ore more "
                                      "letters, digits, '_', '/', and '-'.");
                throw KARABO_SIGNALSLOT_EXCEPTION(msg);
            }
        }


        void SignalSlotable::ensureInstanceIdIsUnique(const std::string& instanceId) {
            {
                // It is important to check first for local conflicts, else
                // shortcut messaging (enabled by the conflicting instance) will trick slotPing request
                // (m_myInstanceMap not yet filled, so use shared one)
                std::shared_lock<std::shared_mutex> lock(m_sharedInstanceMapMutex);
                if (m_sharedInstanceMap.count(instanceId)) {
                    // Note: "Another instance with ID" is assumed in DataLoggerManager::loggerInstantiationHandler
                    throw KARABO_SIGNALSLOT_EXCEPTION("Another instance with ID '" + instanceId +
                                                      "' is already online in this process (localhost)");
                }
            }
            // Ping any instance with my id. If there is one, it will answer, if not, we timeout.
            // slotPing takes care that I do not answer myself before timeout...
            // Note: To process the reply may require the thread added to the event loop in SignalSlotable constructor
            Hash instanceInfo;
            try {
                request(instanceId, "slotPing", m_randPing)
                      .timeout(msPingTimeoutInIsValidInstanceId)
                      .receive(instanceInfo);
            } catch (const karabo::data::TimeoutException&) {
                // Receiving this timeout is the expected behavior
                Exception::clearTrace();
                return;
            }
            string foreignHost("unknown");
            if (instanceInfo.has("host")) instanceInfo.get("host", foreignHost);
            // Note: See above about "Another instance with ID"!
            throw KARABO_SIGNALSLOT_EXCEPTION("Another instance with ID '" + instanceId +
                                              "' is already online (on host: " + foreignHost + ")");
        }


        void SignalSlotable::consumerErrorNotifier(const std::string& consumer, karabo::net::consumer::Error ec,
                                                   const std::string& message) {
            const std::string fullMsg("Error " + toString(static_cast<int>(ec)) + " from consumer '" + consumer +
                                      "': " + message);
            std::lock_guard<std::mutex> lock(m_brokerErrorHandlerMutex);
            if (m_brokerErrorHandler) {
                try {
                    m_brokerErrorHandler(fullMsg);
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << ": Exception in broker error handler when "
                                               << "handling '" << fullMsg << "':" << e.what();
                }
            } else {
                // log an extra error only if no handler
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << ": " << fullMsg;
            }
        }


        long long SignalSlotable::getEpochMillis() const {
            return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        }


        void SignalSlotable::handleReply(const karabo::data::Hash::Pointer& header,
                                         const karabo::data::Hash::Pointer& body, long long whenPostedEpochMs) {
            // Collect performance statistics
            if (m_updatePerformanceStatistics) {
                updateLatencies(header, whenPostedEpochMs);
            }
            boost::optional<Hash::Node&> signalIdNode = header->find("signalInstanceId");
            const std::string& signalId =
                  (signalIdNode && signalIdNode->is<std::string>() ? signalIdNode->getValue<std::string>()
                                                                   : "unspecified sender");
            KARABO_LOG_FRAMEWORK_TRACE << m_instanceId << ": Injecting reply from: " << signalId << *header << *body;

            const string& replyId = header->get<string>("replyFrom");
            std::pair<std::shared_ptr<boost::asio::steady_timer>, Requestor::AsyncErrorHandler> timerAndHandler =
                  getReceiveAsyncErrorHandles(replyId);
            // Check if a timer was registered for the reply and cancel it since message handled here before expiration
            if (timerAndHandler.first && timerAndHandler.first->cancel() == 0) {
                // Cancelling failed, error handler was already put to event loop and will handle timeout
                return;
            }

            // Check whether the reply is an error
            bool asyncErrorHandlerCalled = false;
            boost::optional<Hash::Node&> errorNode = header->find("error");
            if (errorNode && errorNode->is<bool>() && errorNode->getValue<bool>()) {
                // Handling an error, so double check that input is as expected, i.e. body has key "a1":
                const boost::optional<karabo::data::Hash::Node&> textNode = body->find("a1");
                const std::string text(textNode && textNode->is<std::string>()
                                             ? textNode->getValue<std::string>()
                                             : "Error signaled, but body without string at key 'a1'");
                const boost::optional<karabo::data::Hash::Node&> detailsNode = body->find("a2"); // since Karabo 2.14.0
                const std::string details(detailsNode && detailsNode->is<std::string>()
                                                ? detailsNode->getValue<std::string>()
                                                : std::string());
                if (timerAndHandler.second) {
                    try {
                        throw karabo::data::RemoteException(text, signalId, details);
                    } catch (const std::exception&) {
                        try {
                            // Handler can do: try {throw;} catch(const karabo::data::RemoteException&) {...;}
                            asyncErrorHandlerCalled = true;
                            timerAndHandler.second();
                        } catch (const std::exception& e) {
                            KARABO_LOG_FRAMEWORK_WARN << getInstanceId() << ": Received error from '" << signalId
                                                      << "' for request id '" << replyId
                                                      << "', but error handler throws exception:\n"
                                                      << e.what();
                        }
                    }
                    Exception::clearTrace();
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << getInstanceId() << ": Received error from '" << signalId
                                              << "': " << text
                                              << (details.empty() ? std::string() : "\ndetails: " + details);
                }
            }

            // Check whether a callback (temporary slot) was registered for the reply
            // (if it timed out before, the slot will already be gone)
            SlotInstancePointer slot = getSlot(replyId);
            try {
                if (!asyncErrorHandlerCalled && slot) {
                    // Do not check (false) arity-nargs equality for reply slots
                    slot->callRegisteredSlotFunctions(*header, *body, false);
                }
            } catch (const std::exception& e) {
                if (timerAndHandler.second) {
                    try {
                        // Handler can do: try {throw;} catch(const karabo::data::CastException&) {...;} catch (..){
                        timerAndHandler.second();
                    } catch (const std::exception& e) {
                        KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << ": Exception when handling reply from '"
                                                   << signalId << "', but error handler throws exception:\n"
                                                   << e.what() << "\nmessage body: " << *body;
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << ": Exception when handling reply from '" << signalId
                                               << "': " << e.what() << "\nmessage body: " << *body;
                }
            }
            removeSlot(replyId);
            // Now check whether someone is synchronously waiting for us and if yes wake him up
            std::shared_ptr<BoostMutexCond> bmc;
            {
                std::lock_guard<std::mutex> lock(m_receivedRepliesBMCMutex);
                ReceivedRepliesBMC::iterator it = m_receivedRepliesBMC.find(replyId);
                if (it != m_receivedRepliesBMC.end()) bmc = it->second;
            }
            // Insert reply and notify only if it is expected
            if (!bmc) return;
            {
                // Caveat: Need to lock two mutices - and in that order:
                //         - The first one protects changing the condition that bmc->m_cond is waiting for.
                //         - The second protects adding reply to m_receivedReplies.
                //         - Order of locks must be as in SignalSlotable::timedWaitAndPopReceivedReply(...)
                std::lock_guard<std::mutex> lock0(bmc->m_mutex);
                std::lock_guard<std::mutex> lock(m_receivedRepliesMutex);
                m_receivedReplies[replyId] = std::make_pair(header, body);
            }
            bmc->m_cond.notify_one(); // notify only if
        }


        void SignalSlotable::onHeartbeatMessage(const karabo::data::Hash::Pointer& header,
                                                const karabo::data::Hash::Pointer& body) {
            try {
                SlotInstancePointer slot = getSlot("slotHeartbeat");
                if (slot) {
                    // Synchronously call the slot - no Strand or so needed since JmsConsumer guarantees ordering
                    slot->callRegisteredSlotFunctions(*header, *body);
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << ": Exception in onHeartbeatMessage: " << e.what();
            }
        }


        void SignalSlotable::startTrackingSystem() {
            // Countdown and finally timeout registered heartbeats
            m_trackingTimer.expires_after(10ms);
            m_trackingTimer.async_wait(bind_weak(&karabo::xms::SignalSlotable::letInstanceSlowlyDieWithoutHeartbeat,
                                                 this, boost::asio::placeholders::error));
        }


        void SignalSlotable::stopTrackingSystem() {
            m_trackingTimer.cancel();
        }


        void SignalSlotable::startPerformanceMonitor() {
            m_performanceTimer.expires_after(10ms);
            m_performanceTimer.async_wait(bind_weak(&karabo::xms::SignalSlotable::updatePerformanceStatistics, this,
                                                    boost::asio::placeholders::error));
        }


        void SignalSlotable::stopPerformanceMonitor() {
            m_performanceTimer.cancel();
        }


        void SignalSlotable::updatePerformanceStatistics(const boost::system::error_code& e) {
            if (e) return;
            try {
                if (m_updatePerformanceStatistics) {
                    std::unique_lock<std::mutex> lock(m_latencyMutex);
                    const Hash::Pointer performanceMeasures = std::make_shared<Hash>(
                          "processingLatency", m_processingLatency.average(), "maxProcessingLatency",
                          m_processingLatency.maximum, "numMessages", m_processingLatency.counts, "maxEventLoopLatency",
                          m_eventLoopLatency.maximum);
                    // Reset statistics
                    m_processingLatency.clear();
                    m_eventLoopLatency.clear();

                    // Call handler synchronously - no need to keep lock for that.
                    lock.unlock();
                    m_updatePerformanceStatistics(performanceMeasures);
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Exception in updatePerformanceStatistics: " << e.what();
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Unknown exception in updatePerformanceStatistics";
            }
            m_performanceTimer.expires_after(5s);
            m_performanceTimer.async_wait(bind_weak(&karabo::xms::SignalSlotable::updatePerformanceStatistics, this,
                                                    boost::asio::placeholders::error));
        }


        void SignalSlotable::updateLatencies(const karabo::data::Hash::Pointer& header, long long whenPostedEpochMs) {
            boost::optional<Hash::Node&> timestampNode = header->find("MQTimestamp");
            if (timestampNode && timestampNode->is<long long>()) {
                const long long nowInEpochMillis = getEpochMillis();
                const long long latency = nowInEpochMillis - timestampNode->getValue<long long>();
                const unsigned int posLatency = static_cast<unsigned int>(std::max(latency, 0ll));

                const long long evtLoopLatency = nowInEpochMillis - whenPostedEpochMs;
                const unsigned int posEvtLoopLatency = static_cast<unsigned int>(std::max(evtLoopLatency, 0ll));

                std::lock_guard<std::mutex> lock(m_latencyMutex);
                m_processingLatency.add(posLatency);
                m_eventLoopLatency.add(posEvtLoopLatency);
            }
        }


        void SignalSlotable::processEvent(const karabo::data::Hash::Pointer& header,
                                          const karabo::data::Hash::Pointer& body) {
            try {
                // If it is a broadcast message and a handler registered for that, call it:
                if (m_broadCastHandler) {
                    boost::optional<Hash::Node&> allInstanceIds = header->find("slotInstanceIds");
                    if (allInstanceIds && allInstanceIds->is<std::string>() // properly formed header...
                        &&
                        allInstanceIds->getValue<std::string>().find("|*|") != std::string::npos) { // ...for broadcast
                        m_broadCastHandler(header, body);
                    }
                }

                // Retrieve the signalInstanceId
                const std::string& signalInstanceId = (header->has("signalInstanceId") ? // const ref is essential!
                                                             header->get<std::string>("signalInstanceId")
                                                                                       : std::string("unknown"));
                // Check whether this message is an async reply
                if (header->has("replyFrom")) {
                    // replies are never broadcasted, so use normal event strand
                    karabo::net::Strand::Pointer strand(getUnicastEventStrand(signalInstanceId));
                    strand->post(bind_weak(&SignalSlotable::handleReply, this, header, body, getEpochMillis()));
                    return;
                }
                // TODO: To check for remote errors reported after requestNoWait, do e.g.
                //
                // boost::optional<Hash::Node&> errorNode = header->find("error");
                // if (errorNode && errorNode->is<bool>() && errorNode->getValue<bool>()) {
                //   ...
                // }
                // as in handleReply (or better find a unified solution)

                /* The header of each event (message)
                 * should contain all slotFunctions that must be called
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

                const boost::optional<Hash::Node&> signalFuncNode = header->find("signalFunction");
                if (signalFuncNode && !signalFuncNode->getValue<std::string>().starts_with("__") &&
                    allSlotsNode->getValue<std::string>() == "__none__") {
                    // A remote signal - find the slot (theoretically there are more than one)
                    std::vector<std::string> slotFunctions;
                    { // Look for slot in stored connections
                        std::lock_guard<std::mutex> lock(m_signalSlotConnectionsMutex);
                        auto connectionIt = m_signalSlotConnections.find(signalInstanceId);
                        if (connectionIt != m_signalSlotConnections.end()) {
                            for (const SignalSlotConnection& connection : connectionIt->second) {
                                if (connection.signal == signalFuncNode->getValue<std::string>()) {
                                    slotFunctions.push_back(connection.slot);
                                }
                            }
                        }
                    }
                    for (const std::string& slot : slotFunctions) {
                        Strand::Pointer strand(getUnicastEventStrand(signalInstanceId));
                        strand->post(bind_weak(&SignalSlotable::processSingleSlot, this, slot, false, signalInstanceId,
                                               header, body, getEpochMillis()));
                    }
                    if (!slotFunctions.empty()) return;
                }


                std::string slotFunctions = allSlotsNode->getValue<string>(); // by value since trimmed later
                KARABO_LOG_FRAMEWORK_TRACE << this->getInstanceId() << ": Process event for slotFunctions '"
                                           << slotFunctions
                                           << "'"; // << "\n Header: " << header << "\n Body: " << body;

                // Trim and split on the "|" string, avoid empty entries
                std::vector<string> allSlots;
                boost::trim_if(slotFunctions, boost::is_any_of("|"));
                boost::split(allSlots, slotFunctions, boost::is_any_of("|"), boost::token_compress_on);

                for (const string& instanceSlots : allSlots) {
                    const size_t pos = instanceSlots.find_first_of(":");
                    if (pos == std::string::npos) {
                        KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Badly shaped message header, instanceSlots '"
                                                  << instanceSlots << "' lack a ':'.";
                        continue;
                    }
                    const string instanceId(instanceSlots.substr(0, pos));
                    // We should call only functions defined for our instanceId or broadcasted ("*") ones
                    const bool isBroadcast = (instanceId == "*");
                    if (!isBroadcast && instanceId != m_instanceId) continue;

                    vector<string> slotFunctions =
                          karabo::data::fromString<string, vector>(instanceSlots.substr(pos + 1));
                    if (slotFunctions.empty()) slotFunctions.push_back(std::string()); // empty slot name, fails later
                    for (const string& slotFunction : slotFunctions) {
                        // Broadcasted calls in their own Strand: massive 'attacks' of instanceNew/Gone etc. should
                        // not introduce latencies for normal slot calls - and ordering between broadcast and normal
                        // calls is anyway not guaranteed since broadcasts never use inner process short cut.
                        // To avoid that the same slot called in parallel with itself (e.g. from different senders),
                        // there is an undesired mutex lock in Slot::callRegisteredSlotFunctions...
                        Strand::Pointer strand(isBroadcast ? m_broadcastEventStrand
                                                           : getUnicastEventStrand(signalInstanceId));
                        strand->post(bind_weak(&SignalSlotable::processSingleSlot, this, slotFunction, isBroadcast,
                                               signalInstanceId, header, body, getEpochMillis()));
                    }
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << ": Exception while processing event: " << e.what();
            }
        }


        karabo::net::Strand::Pointer SignalSlotable::getUnicastEventStrand(const std::string& signalInstanceId) {
            // processEvent which calls getUnicastEventStrand can be processed in parallel (for messages from broker
            // or inner process shortcut), so we have to protect:
            std::lock_guard<std::mutex> lock(m_unicastEventStrandsMutex);
            karabo::net::Strand::Pointer& strand = m_unicastEventStrands[signalInstanceId];
            if (!strand) {
                // First message of that sender - initialise the strand:
                strand = std::make_shared<karabo::net::Strand>(EventLoop::getIOService());
            }

            return strand;
        }

        void SignalSlotable::processSingleSlot(const std::string& slotFunction, bool globalCall,
                                               const std::string& signalInstanceId,
                                               const karabo::data::Hash::Pointer& header,
                                               const karabo::data::Hash::Pointer& body, long long whenPostedEpochMs) {
            // Collect performance statistics
            if (m_updatePerformanceStatistics) {
                updateLatencies(header, whenPostedEpochMs);
            }
            // Helper for log message in case of exception
            auto slotCaller = [](const Hash::Pointer& header) -> std::string {
                if (header) {
                    auto node = header->find("signalInstanceId");
                    if (node) return node->getValue<std::string>();
                }
                return "<unknown>"; // no header or no "signalInstanceId" key in it
            };

            try {
                // Check whether slot is callable
                if (m_slotCallGuardHandler) {
                    // This function will throw an exception in case the slot is not callable
                    m_slotCallGuardHandler(slotFunction, signalInstanceId);
                }

                SlotInstancePointer slot = getSlot(slotFunction);
                if (slot) {
                    { // Store slot for asyncReply
                        std::lock_guard<std::mutex> lock(m_currentSlotsMutex);
                        m_currentSlots[std::this_thread::get_id()] = std::make_pair(slotFunction, globalCall);
                    }
                    // TODO: callRegisteredSlotFunctions copies header since it is passed by value :-(.
                    slot->callRegisteredSlotFunctions(*header, *body);
                    sendPotentialReply(*header, slotFunction, globalCall);
                    { // Clean again
                        std::lock_guard<std::mutex> lock(m_currentSlotsMutex);
                        m_currentSlots.erase(std::this_thread::get_id());
                    }
                } else if (!globalCall) {
                    // Warn on non-existing slot, but only if directly addressed:
                    KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Received a message from '" << signalInstanceId
                                              << "' to non-existing slot \"" << slotFunction << "\"";
                    // To trigger call of replyException below, i.e. give an answer and do not timeout
                    throw KARABO_SIGNALSLOT_EXCEPTION("'" + getInstanceId() += "' has no slot '" + slotFunction + "'");
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Miss globally called slot " << slotFunction;
                }
            } catch (const karabo::data::Exception& e) {
                const std::string friendlyMsg(e.userFriendlyMsg(false));
                const std::string details(e.detailedMsg());
                KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << ": Exception in slot '" << slotFunction << "' called by '"
                                           << slotCaller(header) << "': " << details;
                replyException(*header, friendlyMsg, details);
            } catch (const std::exception& e) {
                const std::string msg(e.what());
                KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << ": Exception in slot '" << slotFunction << "' called by '"
                                           << slotCaller(header) << "': " << msg;
                replyException(*header, msg, std::string());
            }
        }


        void SignalSlotable::registerReply(const karabo::data::Hash::Pointer& reply) {
            std::lock_guard<std::mutex> lock(m_replyMutex);
            m_replies[std::this_thread::get_id()] = reply;
        }


        std::tuple<karabo::data::Hash::Pointer, std::string, bool> SignalSlotable::registerAsyncReply() {
            // Get name of current slot (sometimes referred to as 'slotFunction'):
            std::pair<std::string, bool> slotName_calledGlobally;
            {
                std::lock_guard<std::mutex> lock(m_currentSlotsMutex);
                const auto it = m_currentSlots.find(std::this_thread::get_id());
                if (it != m_currentSlots.end()) {
                    slotName_calledGlobally = it->second;
                }
                // else { // slot is not called via processEvent and thus any reply does not matter!}
            }
            std::tuple<karabo::data::Hash::Pointer, std::string, bool> result;
            const std::string& slotName = slotName_calledGlobally.first;
            // If no slotName placed, reply does not matter (see above) - we mark this with non-existing header pointer.
            if (!slotName.empty()) {
                result = std::make_tuple(getSenderInfo(slotName)->getHeaderOfSender(), slotName,
                                         slotName_calledGlobally.second);

                // Place an invalid reply to avoid a default reply to be sent (see sendPotentialReply):
                registerReply(karabo::data::Hash::Pointer());
            }

            return result;
        }


        void SignalSlotable::AsyncReply::error(const std::string& message, const std::string& details) const {
            // See SignalSlotable::registerAsyncReply() about non-existing header
            const data::Hash::Pointer& header = std::get<0>(m_slotInfo);
            if (header) {
                m_signalSlotable->replyException(*header, message, details);
            }
        }


        std::string SignalSlotable::generateUUID() {
            // The generator is not thread safe, but we rely on real uniqueness!
            std::lock_guard<std::mutex> lock(m_uuidGeneratorMutex);
            return boost::uuids::to_string(m_uuidGenerator());
        }


        void SignalSlotable::replyException(const karabo::data::Hash& header, const std::string& message,
                                            const std::string& details) {
            if (header.has("replyTo")) {
                const std::string targetInstanceId = header.get<string>("signalInstanceId");
                Hash::Pointer replyHeader = std::make_shared<Hash>();
                replyHeader->set("error", true);
                replyHeader->set("replyFrom", header.get<std::string>("replyTo"));
                replyHeader->set("signalInstanceId", m_instanceId);
                replyHeader->set("signalFunction", "__reply__");
                replyHeader->set("slotInstanceIds", "|" + targetInstanceId + "|");

                Hash::Pointer replyBody = std::make_shared<Hash>("a1", message, "a2", details);
                doSendMessage(targetInstanceId, replyHeader, replyBody);
            }
            // else {
            // TODO: care about header.has("replyInstanceIds") (i.e. requestNoWait) as well!
            //}
        }


        void SignalSlotable::sendPotentialReply(const karabo::data::Hash& header, const std::string& slotFunction,
                                                bool global) {
            // We could be requested in two different ways.
            // TODO: Get rid of requestNoWait code path once receiveAsync is everywhere.
            // GF: But currently there is a difference: requestNoWait allows to get answers from
            //     everybody if called globally whereas a global request's reply will be refused below.
            const bool caseRequest = header.has("replyTo"); // with receive or receiveAsync
            const bool caseRequestNoWait = header.has("replyInstanceIds");

            std::lock_guard<std::mutex> lock(m_replyMutex);
            Replies::iterator replyIter = m_replies.find(std::this_thread::get_id());
            const bool replyPlaced = (replyIter != m_replies.end());
            if (!caseRequest && !caseRequestNoWait) {
                // Not requested, so nothing to reply, but we have to remove the
                // reply that may have been placed in the slot.
                if (replyPlaced) m_replies.erase(replyIter);
                return;
            }
            // The reply of a slot requested globally ("*") should be ignored.
            // If not, all but the first reply reaching the requesting instance
            // would anyways be ignored. So we just remove the reply.
            // Note that a global requestNoWait will work instead: All answers
            // will call the given slot.
            if (global && caseRequest) { // NOT: || caseRequestNoWait) {
                if (replyPlaced) {
                    m_replies.erase(replyIter);
                    // But it is fishy if the slot was requested instead of simply called!
                    KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Refusing to reply to "
                                              << header.get<std::string>("signalInstanceId") << " since it request-ed '"
                                              << slotFunction << "' (i.e. globally).";
                }
                return;
            }

            // For caseRequestNoWait it does not make sense to send an empty reply if
            // the called slot did not provide a reply itself (possible argument mismatch for reply slot).
            if (caseRequestNoWait && !replyPlaced) {
                KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Slot '" << slotFunction << "' did not place a "
                                          << "reply, but was called via requestNoWait";
                return;
            }

            // We are left with valid requests/requestNoWaits. For requests, we send an empty
            // reply if the slot did not place one. That tells the caller at least that
            // the slot finished - i.e. a synchronous request stops blocking.
            // An invalid reply (null pointer) means that registerAsyncReply was called and the
            // reply will be handled later.
            Hash::Pointer replyBody;
            if (replyPlaced) {
                replyBody = replyIter->second;
                m_replies.erase(replyIter);
                if (!replyBody) { // empty pointer as placed by registerAsyncReply
                    return;
                }
            } else {
                replyBody = std::make_shared<Hash>();
            }
            Hash::Pointer replyHeader = std::make_shared<Hash>();

            std::string targetInstanceId;
            replyHeader->set("signalInstanceId", m_instanceId);
            if (caseRequest) {
                targetInstanceId = header.get<string>("signalInstanceId");
                replyHeader->set("replyFrom", header.get<std::string>("replyTo"));
                replyHeader->set("signalFunction", "__reply__");
                replyHeader->set("slotInstanceIds", "|" + targetInstanceId + "|");
            } else { // i.e. caseRequestNoWait with a reply properly placed
                targetInstanceId = header.get<string>("replyInstanceIds");
                replyHeader->set("signalFunction", "__replyNoWait__");
                replyHeader->set("slotInstanceIds", header.get<string>("replyInstanceIds"));
                replyHeader->set("slotFunctions", header.get<string>("replyFunctions"));
            }

            // Our answer to slotPing may interest someone remote trying to come up with our instanceId,
            // so we must not bypass the broker.
            const bool viaBroker = (slotFunction == "slotPing");
            doSendMessage(targetInstanceId, replyHeader, replyBody, m_topic, viaBroker);
        }


        void SignalSlotable::registerDefaultSignalsAndSlots() {
            // Listener for heartbeats
            KARABO_SLOT(slotHeartbeat, string /*instanceId*/, Hash /*heartbeatInfo*/)

            KARABO_SIGNAL("signalInstanceNew", string, Hash);

            KARABO_SIGNAL("signalInstanceGone", string, Hash);

            KARABO_SIGNAL("signalInstanceUpdated", string, Hash);

            // Global instance new notification
            KARABO_SLOT(slotInstanceNew, string /*instanceId*/, Hash /*instanceInfo*/)

            // Global slot instance gone
            KARABO_SLOT(slotInstanceGone, string /*instanceId*/, Hash /*instanceInfo*/)

            // Global slot instance updated
            KARABO_SLOT(slotInstanceUpdated, string /*instanceId*/, Hash /*instanceInfo*/)

            // Listener for other instances' ping
            KARABO_SLOT(slotPing, int /*replyIfSame*/)

            // Listener for topology discovery
            KARABO_SLOT(slotDiscover, string /*requestorId*/)

            // Listener for answers to discovery
            KARABO_SLOT(slotDiscoverAnswer, string /*instanceId*/, Hash /*instanceInfo*/)

            // Connects signal to slot from signal instance side
            KARABO_SLOT(slotConnectToSignal, string /*signalFunction*/, string /*slotInstanceId*/,
                        string /*slotFunction*/)

            // Replies whether slot exists on this instance
            KARABO_SLOT(slotHasSlot, string /*slotFunction*/)

            // Subscribe from slot instance side
            KARABO_SLOT(slotSubscribeRemoteSignal, string /*signalInstanceId*/, string /*signalFunction*/)
            KARABO_SLOT(slotUnsubscribeRemoteSignal, string /*signalInstanceId*/, string /*signalFunction*/)

            // Disconnects signal from slot
            KARABO_SLOT(slotDisconnectFromSignal, string /*signalFunction*/, string /*slotInstanceId*/,
                        string /*slotFunction*/)

            // Function request
            KARABO_SLOT(slotGetAvailableFunctions, string /*functionType*/)

            // Provides information about pipeline connectivity
            KARABO_SLOT(slotGetOutputChannelInformation,
                        karabo::data::Hash /*hash*/) // wrapper for generic calls, that encapsulate arguments in Hash

            KARABO_SLOT(slotGetOutputChannelNames)
        }


        void SignalSlotable::trackAllInstances() {
            m_trackAllInstances = true;
            m_connection->startReadingHeartbeats(
                  bind_weak(&SignalSlotable::onHeartbeatMessage, this, _1, _2),
                  bind_weak(&SignalSlotable::consumerErrorNotifier, this, std::string("heartbeats"), _1, _2));
            startTrackingSystem();
        }


        void SignalSlotable::slotInstanceNew(const std::string& instanceId, const karabo::data::Hash& instanceInfo) {
            // Check if new instance in static map of local instances - if yes, copy to non-static one for usage
            // (Before 'if (instanceId == m_instanceId)' below to shortcut also self-messaging.)
            {
                std::shared_lock<std::shared_mutex> lock(m_sharedInstanceMapMutex);
                auto it = m_sharedInstanceMap.find(instanceId);
                if (it != m_sharedInstanceMap.end()) {
                    WeakPointer localOther(it->second);
                    // 2nd mutex lock in proper order as everywhere else
                    std::lock_guard<std::mutex> lock2(m_myInstanceMapMutex);
                    m_myInstanceMap[instanceId] = localOther;
                }
            }

            if (instanceId == m_instanceId) return;

            // In the past, we called cleanSignals(instanceId) here to ensure that all old connections (maintained as
            // part of the signal) are erased. But since instanceNew broadcasts have no order guarantee with other slot
            // calls, 'instanceId' might have already connected to one of our signals before we process its instanceNew
            // here. So the cleaning can wrongly erase a connection that has just been created.

            if (m_trackAllInstances) {
                // If it was already tracked, this call will overwrite it (= reset countdown)
                addTrackedInstance(instanceId, instanceInfo);
            }

            emit("signalInstanceNew", instanceId, instanceInfo);

            reconnectSignals(instanceId);

            reconnectInputChannels(instanceId);
        }


        void SignalSlotable::slotInstanceGone(const std::string& instanceId, const karabo::data::Hash& instanceInfo) {
            { // Erase again from shortcut messaging if it took part in that
                std::lock_guard<std::mutex> lock(m_myInstanceMapMutex);
                if (m_myInstanceMap.erase(instanceId) > 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << " erased " << instanceId << " from shared local map";
                }
            }

            if (instanceId == m_instanceId) return;

            cleanSignals(instanceId);

            if (m_trackAllInstances) {
                eraseTrackedInstance(instanceId);
            }

            emit("signalInstanceGone", instanceId, instanceInfo);
        }


        void SignalSlotable::slotInstanceUpdated(const std::string& instanceId,
                                                 const karabo::data::Hash& instanceInfo) {
            if (instanceId == m_instanceId) return;

            if (m_trackAllInstances) {
                // Merge the new instanceInfo (ignoring case of a shrunk instanceInfo...)
                addTrackedInstance(instanceId, instanceInfo);
            }
            emit("signalInstanceUpdated", instanceId, instanceInfo); // after addTrackedInstance(..)?
        }


        Broker::Pointer SignalSlotable::getConnection() const {
            return m_connection;
        }


        void SignalSlotable::sendHeartbeat(const boost::system::error_code& e) {
            if (e) return;
            try {
                Hash heartBeatInfo;
                {
                    std::shared_lock<std::shared_mutex> lock(m_instanceInfoMutex);
                    heartBeatInfo = getHeartbeatInfo(m_instanceInfo);
                }
                call("*", "slotHeartbeat", getInstanceId(), heartBeatInfo);
            } catch (std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << ": sendHeartbeat triggered an exception: " << e.what();
            }
            delayedSendHeartbeat(m_heartbeatInterval);
        }


        void SignalSlotable::delayedSendHeartbeat(int delayInSeconds) {
            // Protect against any bad interval that causes spinning:
            delayInSeconds = (delayInSeconds ? std::abs(delayInSeconds) : 10);
            m_heartbeatTimer.expires_after(seconds(delayInSeconds));
            m_heartbeatTimer.async_wait(
                  bind_weak(&karabo::xms::SignalSlotable::sendHeartbeat, this, boost::asio::placeholders::error));
        }


        Hash SignalSlotable::getAvailableInstances(bool /*unused*/) {
            KARABO_LOG_FRAMEWORK_DEBUG << "getAvailableInstances";
            if (!m_trackAllInstances) {
                std::lock_guard<std::mutex> lock(m_trackedInstancesMutex);
                m_trackedInstances.clear();
            }
            call("*", "slotDiscover", m_instanceId);
            // The function slotDiscoverAnswer will be called by all instances available now
            // Lets wait a fair amount of time - huaaah this is bad isn't it :-(
            // Since we block here for a long time, add a thread to ensure that all slotDiscoverAnswer can be processed.
            EventLoop::addThread();
            std::this_thread::sleep_for(2000ms);
            EventLoop::removeThread();
            std::lock_guard<std::mutex> lock(m_trackedInstancesMutex);
            KARABO_LOG_FRAMEWORK_DEBUG << "Available instances: " << m_trackedInstances;
            return m_trackedInstances;
        }


        std::pair<bool, std::string> SignalSlotable::exists(const std::string& instanceId) {
            string hostname;
            Hash instanceInfo;
            try {
                this->request(instanceId, "slotPing", 1).timeout(200).receive(instanceInfo);
            } catch (const karabo::data::TimeoutException&) {
                Exception::clearTrace();
                return std::make_pair(false, hostname);
            }
            if (instanceInfo.has("host")) instanceInfo.get("host", hostname);
            return std::make_pair(true, hostname);
        }


        void SignalSlotable::slotPing(int rand) {
            if (m_randPing == 0 || rand != m_randPing) {
                // I am fully up (m_randPing == 0) or I get a ping from another (rand != m_randPing) instance:
                // I reply my existence, providing my instance info.
                // NOTE: m_randPing of a booting instance is always > 1, i.e. rand == 1 will always get this reply.
                std::shared_lock<std::shared_mutex> lock(m_instanceInfoMutex);
                reply(m_instanceInfo);
            } else {
                // I am not yet fully up (0 != m_randPing) and the foreign rand(Ping) is the same as mine.
                // So I ping myself and must not answer.
                // Therefore, place an invalid reply to avoid a default reply to be sent (see sendPotentialReply):
                registerReply(karabo::data::Hash::Pointer());
            }
        }

        void SignalSlotable::slotDiscover(const std::string& requestorId) {
            // Only if m_randPing was set to zero, we are a full member of the topology with a valid instanceId.
            // So we answer the discovery request only then.
            if (0 == m_randPing) {
                std::shared_lock<std::shared_mutex> lock(m_instanceInfoMutex);
                call(requestorId, "slotDiscoverAnswer", m_instanceId, m_instanceInfo);
            }
        }

        std::vector<string> SignalSlotable::getAvailableSignals(const std::string& instanceId, int timeout) {
            std::vector<string> signals;
            try {
                request(instanceId, "slotGetAvailableFunctions", "signals").timeout(timeout).receive(signals);
            } catch (const karabo::data::TimeoutException&) {
                karabo::data::Exception::clearTrace();
                cout << "ERROR:  The requested instanceId \"" << instanceId << "\" is currently not available." << endl;
            }
            return signals;
        }


        std::vector<string> SignalSlotable::getAvailableSlots(const std::string& instanceId, int timeout) {
            std::vector<string> slots;
            try {
                request(instanceId, "slotGetAvailableFunctions", "slots").timeout(timeout).receive(slots);
            } catch (const karabo::data::TimeoutException&) {
                karabo::data::Exception::clearTrace();
                cout << "ERROR:  The requested instanceId \"" << instanceId << "\" is currently not available." << endl;
            }
            return slots;
        }


        const SignalSlotable::SlotInstancePointer& SignalSlotable::getSenderInfo(
              const std::string& unmangledSlotFunction) {
            const std::pair<bool, std::string> needMangle = mangleSlotFunction(unmangledSlotFunction);
            const std::string& mangledSlotFunction = (needMangle.first ? needMangle.second : unmangledSlotFunction);

            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            auto it = m_slotInstances.find(mangledSlotFunction);
            if (it == m_slotInstances.end())
                throw KARABO_SIGNALSLOT_EXCEPTION("No slot-object could be found for slotFunction \"" +
                                                  mangledSlotFunction + "\"");
            return it->second;
        }


        void SignalSlotable::slotGetAvailableFunctions(const std::string& type) {
            std::vector<string> functions;
            if (type == "signals") {
                std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
                for (auto it = m_signalInstances.cbegin(); it != m_signalInstances.cend(); ++it) {
                    const string& function = it->first;
                    functions.push_back(function);
                }
            } else if (type == "slots") {
                std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
                for (auto it = m_slotInstances.cbegin(); it != m_slotInstances.cend(); ++it) {
                    const string& function = it->first;
                    // Filter out service slots // TODO finally update to last set of those
                    if (function == "slotConnectToSignal" || function == "slotDisconnectFromSignal" ||
                        function == "slotGetAvailableFunctions" || function == "slotHasSlot" ||
                        function == "slotHeartbeat" || function == "slotPing" || function == "slotDiscover" ||
                        function == "slotDiscoverAnswer") {
                        continue;
                    }
                    functions.push_back(it->first);
                }
            }
            reply(functions);
        }


        void SignalSlotable::slotDiscoverAnswer(const std::string& instanceId, const karabo::data::Hash& instanceInfo) {
            if (!hasTrackedInstance(instanceId)) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Got ping answer from instanceId " << instanceId;
                emit("signalInstanceNew", instanceId, instanceInfo);
            } else {
                KARABO_LOG_FRAMEWORK_DEBUG << "Got ping answer from instanceId (but already tracked) " << instanceId;
            }
            addTrackedInstance(instanceId, instanceInfo);
        }


        void SignalSlotable::slotHeartbeat(const std::string& instanceId, const Hash& heartbeatInfo) {
            if (m_trackAllInstances) {
                if (!hasTrackedInstance(instanceId)) {
                    KARABO_LOG_FRAMEWORK_INFO << "Tracking instances, but received heart beat from unknown '"
                                              << instanceId << "'";
                    // Resurrected after not sending heartbeats anymore, or heartbeat arrived before
                    // slotDiscoverAnswer/instanceNew.

                    // HeartbeatInfo is a partial instanceInfo only, so discover the resurrected instance which
                    // then will call our slotDiscoverAnswer (that cares about tracking) with the full instanceInfo.
                    call(instanceId, "slotDiscover", m_instanceId);
                } else {
                    // Merge the potentially partial instanceInfo and re-set the countdown
                    addTrackedInstance(instanceId, heartbeatInfo);
                }
            }
        }


        const std::string& SignalSlotable::getInstanceId() const {
            return m_instanceId;
        }


        void SignalSlotable::updateInstanceInfo(const karabo::data::Hash& update, bool remove) {
            std::unique_lock<std::shared_mutex> lock(m_instanceInfoMutex);
            if (remove) {
                m_instanceInfo.subtract(update);
            } else {
                m_instanceInfo.merge(update);
            }
            call("*", "slotInstanceUpdated", m_instanceId, m_instanceInfo);
        }


        karabo::data::Hash SignalSlotable::getInstanceInfo() const {
            std::shared_lock<std::shared_mutex> lock(m_instanceInfoMutex);
            return m_instanceInfo;
        }


        void SignalSlotable::registerSlotCallGuardHandler(const SlotCallGuardHandler& slotCallGuardHandler) {
            m_slotCallGuardHandler = slotCallGuardHandler;
        }


        //************************** Connect **************************//


        bool SignalSlotable::connect(const std::string& signalInstanceIdIn, const std::string& signalFunction,
                                     const std::string& slotFunction) {
            const std::string& signalInstanceId = (signalInstanceIdIn.empty() ? m_instanceId : signalInstanceIdIn);

            // Keep track of what we connect - or at least try to:
            this->storeConnection(signalInstanceId, signalFunction, slotFunction);

            if (hasSlot(slotFunction)) {
                if (this->tryToConnectToSignal(signalInstanceId, signalFunction, slotFunction)) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Successfully connected slot '" << slotFunction << "' to signal '"
                                               << signalInstanceId << "." << signalFunction << "'.";
                    return true;
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "Could not connect slot '" << slotFunction << "' to (non-existing?) "
                                              << "signal '" << signalInstanceId << "." << signalFunction << "'. Will "
                                              << "try again if '" << signalInstanceId << "' sends signalInstanceNew.";
                }
            } else {
                KARABO_LOG_FRAMEWORK_WARN << m_instanceId << " has no slot '" << slotFunction << "', so cannot connect"
                                          << " to signal '" << signalInstanceId << "." << signalFunction
                                          << "'. Will try again if '" << signalInstanceId
                                          << "' sends signalInstanceNew.";
            }

            return false;
        }


        void SignalSlotable::storeConnection(const std::string& signalInstanceId, const std::string& signalFunction,
                                             const std::string& slotFunction) {
            const SignalSlotConnection connection(signalInstanceId, signalFunction, slotFunction);
            std::lock_guard<std::mutex> lock(m_signalSlotConnectionsMutex);
            // Register to re-connect if signal instance comes back.
            // (We might skip to register for signalInstanceId == m_instanceId, but then "reconnectSignals"
            //  looses its genericity.)
            m_signalSlotConnections[signalInstanceId].insert(connection);
        }


        bool SignalSlotable::removeStoredConnection(const std::string& signalInstanceId,
                                                    const std::string& signalFunction,
                                                    const std::string& slotFunction) {
            bool connectionWasKnown = false;
            const SignalSlotConnection connection(signalInstanceId, signalFunction, slotFunction);

            std::lock_guard<std::mutex> lock(m_signalSlotConnectionsMutex);
            auto it = m_signalSlotConnections.find(signalInstanceId);
            if (it != m_signalSlotConnections.end()) {
                if (it->second.erase(connection) >= 1) {
                    connectionWasKnown = true;
                    if (it->second.empty()) m_signalSlotConnections.erase(it);
                }
            }

            return connectionWasKnown;
        }

        SignalSlotable::Pointer SignalSlotable::getInstanceForShortcut(const std::string& instanceId) const {
            SignalSlotable::Pointer localInstance;

            std::lock_guard<std::mutex> lock(m_myInstanceMapMutex);
            auto it = m_myInstanceMap.find(instanceId);
            if (it != m_myInstanceMap.end()) localInstance = it->second.lock();

            return localInstance;
        }

        bool SignalSlotable::tryToConnectToSignal(const std::string& signalInstanceId,
                                                  const std::string& signalFunction, const std::string& slotFunction) {
            bool success = false;

            // Figure out whether we have a local signal, i.e. with instance in same process
            SignalSlotable::Pointer localSignalInstance = getInstanceForShortcut(signalInstanceId);

            // If local, we subscribe on the signal, otherwise on broker
            if (localSignalInstance) { // Could even be myself
                std::lock_guard<std::mutex> lock(localSignalInstance->m_signalSlotInstancesMutex);
                auto it = localSignalInstance->m_signalInstances.find(signalFunction);
                if (it != localSignalInstance->m_signalInstances.end()) { // Signal found
                    // Register slotFunction to signal
                    it->second->registerSlot(m_instanceId, slotFunction);
                    success = true;
                } else {
                    KARABO_LOGGING_DEBUG("Local signalInstance '", signalInstanceId, "' has no signal '",
                                         signalFunction);
                }
            } else {
                boost::system::error_code ec = m_connection->subscribeToRemoteSignal(signalInstanceId, signalFunction);
                if (ec) {
                    KARABO_LOG_FRAMEWORK_WARN << m_instanceId << " : Failed to subscribe to remote signal \""
                                              << signalInstanceId << ":" << signalFunction << "\": #" << ec.value()
                                              << " -- " << ec.message();
                } else {
                    success = true;
                }
            }

            return success;
        }


        SignalSlotable::SlotInstancePointer SignalSlotable::findSlot(const std::string& funcName) {
            SlotInstancePointer ret;
            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            SlotInstances::const_iterator it = m_slotInstances.find(funcName);
            if (it != m_slotInstances.end()) {
                ret = it->second;
            }
            return ret;
        }


        void SignalSlotable::registerNewSlot(const std::string& funcName, SlotInstancePointer instance) {
            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            SlotInstancePointer& newinstance = m_slotInstances[funcName];
            if (newinstance) {
                throw KARABO_SIGNALSLOT_EXCEPTION("The slot \"" + funcName +
                                                  "\" has been registered with two different signatures");
            } else {
                newinstance = instance;
            }
        }


        void SignalSlotable::slotConnectToSignal(const std::string& signalFunction, const std::string& slotInstanceId,
                                                 const std::string& slotFunction) {
            bool result = false;
            {
                std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
                auto it = m_signalInstances.find(signalFunction);
                if (it != m_signalInstances.end()) {
                    it->second->registerSlot(slotInstanceId, slotFunction);
                    result = true;
                }
            }

            reply(result);
        }


        void SignalSlotable::slotHasSlot(const std::string& unmangledSlotFunction) {
            // handle noded slots
            const std::pair<bool, std::string> needMangle = mangleSlotFunction(unmangledSlotFunction);
            const std::string& mangledSlotFunction = (needMangle.first ? needMangle.second : unmangledSlotFunction);

            bool result = false;
            {
                std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
                if (m_slotInstances.find(mangledSlotFunction) != m_slotInstances.end()) {
                    result = true;
                }
            }
            reply(result);
        }


        void SignalSlotable::slotSubscribeRemoteSignal(const std::string& signalInstanceId,
                                                       const std::string& signalFunction) {
            AsyncReply aReply(this);
            m_connection->subscribeToRemoteSignalAsync(
                  signalInstanceId, signalFunction,
                  [weakSelf{weak_from_this()}, aReply{std::move(aReply)}](const boost::system::error_code& ec) {
                      auto self = weakSelf.lock();
                      if (!self) return; // If we are (going) down, one must not use an AsyncReply of us anymore
                      if (ec) {
                          std::ostringstream oss;
                          oss << "Connect signal-slot failed: #" << ec.value() << " -- " << ec.message();
                          aReply.error(oss.str());
                      } else {
                          aReply(true);
                      }
                  });
        }


        void SignalSlotable::slotUnsubscribeRemoteSignal(const std::string& signalInstanceId,
                                                         const std::string& signalFunction) {
            AsyncReply aReply(this);
            m_connection->unsubscribeFromRemoteSignalAsync(
                  signalInstanceId, signalFunction,
                  [weakSelf{weak_from_this()}, aReply{std::move(aReply)}](const boost::system::error_code& ec) {
                      auto self = weakSelf.lock();
                      if (!self) return; // If we are (going) down, one must not use an AsyncReply of us anymore
                      if (ec) {
                          std::ostringstream oss;
                          oss << "Disconnect signal-slot failed: #" << ec.value() << " -- " << ec.message();
                          aReply.error(oss.str());
                      } else {
                          aReply(true);
                      }
                  });
        }


        void SignalSlotable::asyncConnect(const std::string& signalInstanceIdIn, const std::string& signalSignature,
                                          const std::string& slotSignature, const std::function<void()>& successHandler,
                                          const std::function<void()>& failureHandler) {
            const std::string& signalInstanceId = (signalInstanceIdIn.empty() ? m_instanceId : signalInstanceIdIn);

            // Keep track of what we connect - or at least try to:
            storeConnection(signalInstanceId, signalSignature, slotSignature);

            if (!hasSlot(slotSignature)) {
                EventLoop::post(
                      std::bind(callErrorHandler, failureHandler, "Have no slot '" + slotSignature + "'.", false));
                return;
            }

            // Figure out whether we have a local signal, i.e. with instance in same process
            SignalSlotable::Pointer localSignalInstance = getInstanceForShortcut(signalInstanceId);

            if (localSignalInstance) {
                // Local, so we need to subscribe on the signal.
                std::lock_guard<std::mutex> lock(localSignalInstance->m_signalSlotInstancesMutex);
                auto it = localSignalInstance->m_signalInstances.find(signalSignature);
                if (it != localSignalInstance->m_signalInstances.end()) { // Signal found
                    // Register slotFunction to signal
                    it->second->registerSlot(m_instanceId, slotSignature);
                    if (successHandler) EventLoop::post(successHandler);
                } else {
                    KARABO_LOGGING_DEBUG("Local signalInstance '", signalInstanceId, "' has no signal '",
                                         signalSignature, "'.");
                    EventLoop::post(std::bind(
                          callErrorHandler, failureHandler,
                          "Local signalInstance '" + signalInstanceId + "' has no signal '" + signalSignature + "'.",
                          false));
                }
            } else {
                // We have to subscribe to the broker
                auto subscriptionCallback = [signalInstanceId, signalSignature, failureHandler,
                                             successHandler](const boost::system::error_code& ec) {
                    if (ec) {
                        std::ostringstream oss;
                        oss << "Broker connection to '" << signalInstanceId << "." << signalSignature
                            << "' failed: " << "code #" << ec.value() << " -- " << ec.message();
                        callErrorHandler(failureHandler, oss.str());
                    } else {
                        if (successHandler) successHandler();
                    }
                };
                m_connection->subscribeToRemoteSignalAsync(signalInstanceId, signalSignature, subscriptionCallback);
            }
        }


        void SignalSlotable::callErrorHandler(const SignalSlotable::AsyncErrorHandler& handler,
                                              const std::string& message, bool isTimeout) {
            if (handler) {
                try {
                    if (isTimeout) {
                        throw KARABO_TIMEOUT_EXCEPTION(message);
                    } else {
                        throw KARABO_SIGNALSLOT_EXCEPTION(message);
                    }
                } catch (const std::exception&) {
                    Exception::clearTrace();
                    // handler can now do 'try { throw; } catch (const SignalSlotException& e) { ... }'
                    try {
                        handler();
                    } catch (const std::exception& e) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Error handler for message '" << message
                                                   << "' threw exception: " << e.what();
                    }
                }
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << message;
            }
        }


        void SignalSlotable::asyncConnect(const std::vector<SignalSlotConnection>& signalSlotConnections,
                                          const std::function<void()>& successHandler,
                                          const SignalSlotable::AsyncErrorHandler& failureHandler) {
            if (signalSlotConnections.empty()) {
                // Nothing to do, so don't create book keeping structure that can never be cleared.
                if (successHandler) EventLoop::post(successHandler); // But give feedback (in another thread)
                return;
            }

            auto singleConnectDone =
                  [doneFlags{std::make_shared<std::vector<bool>>(signalSlotConnections.size(), false)},
                   commonFailMsgs{std::make_shared<std::string>()}, mutex{std::make_shared<std::mutex>()},
                   successHandler, failureHandler](size_t i, bool succeeded) {
                      // Protect access to data behind pointer "members" of the lambda which are shared between
                      // its individual "instances" created by std::bind below.
                      std::lock_guard<std::mutex> lock(*mutex);

                      if (!succeeded) {
                          std::ostringstream oss;
                          oss << (commonFailMsgs->empty() ? "" : "\n") << "Connection " << i << " failed: ";
                          try {
                              throw; // We are called within a 'catch' block and can rethrow
                          } catch (const karabo::data::Exception& e) {
                              oss << e.userFriendlyMsg();
                          } catch (const std::exception& e) {
                              oss << e.what();
                          }
                          // Keep track of failure info
                          commonFailMsgs->append(oss.str());
                      }

                      (*doneFlags)[i] = true;
                      for (bool flag : *doneFlags) {
                          if (!flag) return; // Still at least one to go
                      }

                      // This is the last connection that reported its success
                      if (commonFailMsgs->empty()) {
                          if (successHandler) successHandler();
                      } else {
                          // Any timeout has precedence
                          callErrorHandler(failureHandler, *commonFailMsgs);
                      }
                  };

            // Send individual requests
            for (size_t i = 0; i < signalSlotConnections.size(); ++i) {
                const SignalSlotConnection& con = signalSlotConnections[i];
                asyncConnect(con.signalInstanceId, con.signal, con.slot, std::bind(singleConnectDone, i, true),
                             std::bind(singleConnectDone, i, false));
            }
        }


        void SignalSlotable::reconnectSignals(const std::string& newInstanceId) {
            std::set<SignalSlotConnection> connections;
            {
                std::lock_guard<std::mutex> lock(m_signalSlotConnectionsMutex);
                auto it = m_signalSlotConnections.find(newInstanceId);

                if (it != m_signalSlotConnections.end()) {
                    connections = it->second;
                }
            }

            const bool newIsLocal = static_cast<bool>(getInstanceForShortcut(newInstanceId));
            for (std::set<SignalSlotConnection>::const_iterator it = connections.begin(), iEnd = connections.end();
                 it != iEnd; ++it) {
                KARABO_LOG_FRAMEWORK_INFO << this->getInstanceId() << " tries to reconnect signal '"
                                          << it->signalInstanceId << "." << it->signal << "' to its slot '" << it->slot
                                          << "'.";
                if (newIsLocal) {
                    // We might have subscribed on the broker before when newInstanceId was offline.
                    // So first unsubscribe to avoid duplicated delivery via shortcut _and_ broker.
                    auto onComplete = [weakThis{weak_from_this()}, connection{*it}](const boost::system::error_code&) {
                        if (auto self = weakThis.lock()) {
                            // No matter whether broker unsubscription failed or not, we want to connect now
                            self->asyncConnect(connection.signalInstanceId, connection.signal, connection.slot);
                        }
                    };
                    m_connection->unsubscribeFromRemoteSignalAsync(it->signalInstanceId, it->signal, onComplete);
                } else {
                    // No success (nor failure) handler needed - there will be log error messages anyway.
                    asyncConnect(it->signalInstanceId, it->signal, it->slot);
                }
            }
        }


        void SignalSlotable::addTrackedInstance(const std::string& instanceId, const karabo::data::Hash& instanceInfo) {
            const boost::optional<const Hash::Node&> beatsNode = instanceInfo.find("heartbeatInterval");
            if (!beatsNode) {
                KARABO_LOG_FRAMEWORK_ERROR << "Cannot track '" << instanceId << "' since its instanceInfo lacks the "
                                           << "'heartbeatInterval': " << instanceInfo;
                return;
            }
            const int countdown = beatsNode->getValue<int>();

            std::lock_guard<std::mutex> lock(m_trackedInstancesMutex);
            boost::optional<Hash::Node&> node = m_trackedInstances.find(instanceId);
            if (node) {
                // A known instance:
                // We might be here from a heartbeat that sends incomplete instanceInfo, so merge what is available.
                Hash& instanceHash = node->getValue<Hash>();
                instanceHash.get<Hash>("instanceInfo").merge(instanceInfo);
                instanceHash.set("countdown", countdown);
            } else {
                // A new instance to be added
                Hash h("instanceInfo", instanceInfo, "countdown", countdown);
                m_trackedInstances.set(instanceId, std::move(h));
            }
        }


        bool SignalSlotable::hasTrackedInstance(const std::string& instanceId) {
            std::lock_guard<std::mutex> lock(m_trackedInstancesMutex);
            return m_trackedInstances.has(instanceId);
        }


        bool SignalSlotable::eraseTrackedInstance(const std::string& instanceId) {
            bool wasTracked = false;
            std::lock_guard<std::mutex> lock(m_trackedInstancesMutex);
            if (m_trackedInstances.erase(instanceId)) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << instanceId << "\" will not be tracked anymore";
                wasTracked = true;
            }
            return wasTracked;
        }


        void SignalSlotable::updateTrackedInstanceInfo(const std::string& instanceId,
                                                       const karabo::data::Hash& instanceInfo) {
            std::lock_guard<std::mutex> lock(m_trackedInstancesMutex);
            if (m_trackedInstances.has(instanceId)) {
                Hash& h = m_trackedInstances.get<Hash>(instanceId);
                h.set("instanceInfo", instanceInfo);
                h.set("countdown", instanceInfo.get<int>("heartbeatInterval"));
            }
        }


        bool SignalSlotable::disconnect(const std::string& signalInstanceIdIn, const std::string& signalFunction,
                                        const std::string& slotFunction) {
            const std::string& signalInstanceId = (signalInstanceIdIn.empty() ? m_instanceId : signalInstanceIdIn);

            // Remove from list of connections that this SignalSlotable established.
            removeStoredConnection(signalInstanceId, signalFunction, slotFunction);

            if (!hasSlot(slotFunction)) {
                KARABO_LOG_FRAMEWORK_INFO << m_instanceId << ": Failed to disconnect slot '" << slotFunction
                                          << "' from signal '" << signalInstanceId << "." << signalFunction
                                          << "': slot does not exist.";
                return false;
            }

            // Figure out whether we have a local signal, i.e. with instance in same process
            SignalSlotable::Pointer localSignalInstance = getInstanceForShortcut(signalInstanceId);

            // If local, we undo subscription from the signal, otherwise from broker
            if (localSignalInstance) { // Could even be myself
                std::lock_guard<std::mutex> lock(localSignalInstance->m_signalSlotInstancesMutex);
                auto it = localSignalInstance->m_signalInstances.find(signalFunction);
                if (it != localSignalInstance->m_signalInstances.end()) { // Signal found
                    if (it->second->unregisterSlot(m_instanceId, slotFunction)) {
                        KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << ": Successfully disconnected slot '"
                                                   << slotFunction << "' from signal '" << signalInstanceId << "."
                                                   << signalFunction << "'.";
                        return true;
                    } else {
                        KARABO_LOG_FRAMEWORK_INFO << m_instanceId << ": Failed to disconnect slot '" << slotFunction
                                                  << "' from signal '" << signalInstanceId << "." << signalFunction
                                                  << "': slot not registered.";
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_INFO << m_instanceId << ": Failed to disconnect slot '" << slotFunction
                                              << "' from signal '" << signalInstanceId << "." << signalFunction
                                              << "': signal does not exist.";
                }
                return false;
            } else {
                boost::system::error_code ec = m_connection->subscribeToRemoteSignal(signalInstanceId, signalFunction);
                if (ec) {
                    KARABO_LOG_FRAMEWORK_INFO << m_instanceId << ": Failed broker unsubscription from signal '"
                                              << signalInstanceId << "." << signalFunction << "': #" << ec.value()
                                              << " -- " << ec.message();
                    return false;
                } else {
                    return true;
                }
            }
        }


        bool SignalSlotable::tryToUnregisterSlot(const std::string& signalFunction, const std::string& slotInstanceId,
                                                 const std::string& slotFunction) {
            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            auto it = m_signalInstances.find(signalFunction);
            if (it != m_signalInstances.end()) { // Signal found
                // Unregister slotId from local signal
                return it->second->unregisterSlot(slotInstanceId, slotFunction);
            }
            return false;
        }


        void SignalSlotable::asyncDisconnect(const std::string& signalInstanceIdIn, const std::string& signalFunction,
                                             const std::string& slotFunction,
                                             const std::function<void()>& successHandler,
                                             const SignalSlotable::AsyncErrorHandler& failureHandler) {
            const std::string& signalInstanceId = (signalInstanceIdIn.empty() ? m_instanceId : signalInstanceIdIn);

            // Remove from list of connections that this SignalSlotable established.
            removeStoredConnection(signalInstanceId, signalFunction, slotFunction);

            if (!hasSlot(slotFunction)) {
                KARABO_LOG_FRAMEWORK_INFO << m_instanceId << ": Failed to disconnect slot '" << slotFunction
                                          << "' from signal '" << signalInstanceId << "." << signalFunction
                                          << "': slot does not exist.";
                EventLoop::post(
                      std::bind(callErrorHandler, failureHandler, "Have no slot '" + slotFunction + "'.", false));
                return;
            }

            const std::string errorMsg(m_instanceId + " failed to disconnect slot '" + slotFunction +
                                       "' from signal '" + signalInstanceId + "." + signalFunction + "'");
            SignalSlotable::Pointer localSignalInstance = getInstanceForShortcut(signalInstanceId);
            if (localSignalInstance) { // Could even be myself
                if (localSignalInstance->tryToUnregisterSlot(signalFunction, m_instanceId, slotFunction)) {
                    if (successHandler) EventLoop::post(successHandler);
                } else {
                    callErrorHandler(failureHandler, errorMsg + " -- was not connected locally");
                }
            } else {
                auto onComplete = [successHandler, failureHandler, // not moving from since const references
                                   errorMsg](const boost::system::error_code& ec) {
                    if (ec) {
                        std::ostringstream oss;
                        oss << errorMsg << ": code #" << ec.value() << " -- " << ec.message();
                        callErrorHandler(failureHandler, oss.str());
                    } else {
                        if (successHandler) successHandler();
                    }
                };

                m_connection->unsubscribeFromRemoteSignalAsync(signalInstanceId, signalFunction, onComplete);
            }
        }


        void SignalSlotable::slotDisconnectFromSignal(const std::string& signalFunction,
                                                      const std::string& slotInstanceId,
                                                      const std::string& slotFunction) {
            if (tryToUnregisterSlot(signalFunction, slotInstanceId, slotFunction)) {
                reply(true);
            } else {
                reply(false);
            }
        }


        bool SignalSlotable::hasSlot(const std::string& unmangledSlotFunction) const {
            // handle noded slots
            const std::pair<bool, std::string> needMangle = mangleSlotFunction(unmangledSlotFunction);
            const std::string& mangledSlotFunction = (needMangle.first ? needMangle.second : unmangledSlotFunction);

            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            return m_slotInstances.find(mangledSlotFunction) != m_slotInstances.end();
        }


        SignalSlotable::SlotInstancePointer SignalSlotable::getSlot(const std::string& unmangledSlotFunction) const {
            // handle noded slots
            const std::pair<bool, std::string> needMangle = mangleSlotFunction(unmangledSlotFunction);
            const std::string& mangledSlotFunction = (needMangle.first ? needMangle.second : unmangledSlotFunction);

            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            SlotInstances::const_iterator it = m_slotInstances.find(mangledSlotFunction);
            if (it != m_slotInstances.end()) return it->second;
            return SlotInstancePointer();
        }


        void SignalSlotable::removeSlot(const std::string& unmangledSlotFunction) {
            // handle noded slots
            const std::pair<bool, std::string> needMangle = mangleSlotFunction(unmangledSlotFunction);
            const std::string& mangledSlotFunction = (needMangle.first ? needMangle.second : unmangledSlotFunction);

            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            m_slotInstances.erase(mangledSlotFunction);
            // Will clean any associated timers to this slot
            m_receiveAsyncErrorHandles.erase(mangledSlotFunction);
        }


        bool SignalSlotable::hasSignal(const std::string& signalFunction) const {
            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            return m_signalInstances.find(signalFunction) != m_signalInstances.end();
        }


        SignalSlotable::SignalInstancePointer SignalSlotable::getSignal(const std::string& signalFunction) const {
            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            auto it = m_signalInstances.find(signalFunction);
            if (it != m_signalInstances.end()) return it->second;
            return SignalInstancePointer();
        }


        string SignalSlotable::fetchInstanceId(const std::string& signalOrSlotId) const {
            return signalOrSlotId.substr(0, signalOrSlotId.find_last_of('/'));
        }


        void SignalSlotable::letInstanceSlowlyDieWithoutHeartbeat(const boost::system::error_code& e) {
            if (e) return;

            try {
                if (m_trackAllInstances) {
                    vector<pair<string, Hash>> deadOnes;

                    decreaseCountdown(deadOnes);

                    for (size_t i = 0; i < deadOnes.size(); ++i) {
                        KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Instance \"" << deadOnes[i].first
                                                  << "\" silently disappeared (no heartbeats received anymore)";
                        emit("signalInstanceGone", deadOnes[i].first, deadOnes[i].second);
                        eraseTrackedInstance(deadOnes[i].first);
                    }
                }

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "letInstanceSlowlyDieWithoutHeartbeat triggered an exception: "
                                           << e.what();
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "letInstanceSlowlyDieWithoutHeartbeat triggered an unknown exception";
            }

            // We are sleeping five times as long as the count-down ticks (which ticks in seconds)
            m_trackingTimer.expires_after(5s);
            m_trackingTimer.async_wait(bind_weak(&karabo::xms::SignalSlotable::letInstanceSlowlyDieWithoutHeartbeat,
                                                 this, boost::asio::placeholders::error));
        }


        void SignalSlotable::decreaseCountdown(std::vector<std::pair<std::string, karabo::data::Hash>>& deadOnes) {
            std::lock_guard<std::mutex> lock(m_trackedInstancesMutex);

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
            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);

            KARABO_LOG_FRAMEWORK_TRACE << m_instanceId << " says : Cleaning all signals for instance \"" << instanceId
                                       << "\"";

            for (SignalInstances::iterator it = m_signalInstances.begin(); it != m_signalInstances.end(); ++it) {
                it->second->unregisterSlot(instanceId);
            }
        }


        bool SignalSlotable::SignalSlotConnection::operator<(const SignalSlotConnection& other) const {
            // Compare members in sequence. Since arrays of references are not allowed, so use pointers.
            const std::string* const mine[] = {&signalInstanceId, &signal, &slot};
            const std::string* const others[] = {&other.signalInstanceId, &other.signal, &other.slot};
            const size_t numMembers = sizeof(mine) / sizeof(mine[0]);

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


        InputChannel::Pointer SignalSlotable::createInputChannel(
              const std::string& channelName, const karabo::data::Hash& config,
              const DataHandler& onDataAvailableHandler, const InputHandler& onInputAvailableHandler,
              const InputHandler& onEndOfStreamEventHandler, const InputChannel::ConnectionTracker& connectTracker) {
            if (!config.has(channelName))
                throw KARABO_PARAMETER_EXCEPTION(
                      "The provided configuration must contain the channel name as key in the configuration");
            Hash channelConfig = config.get<Hash>(channelName);
            if (channelConfig.has("schema")) channelConfig.erase("schema");
            InputChannel::Pointer channel = Configurator<InputChannel>::create("InputChannel", channelConfig);
            channel->setInstanceId(m_instanceId + ":" + channelName);
            channel->registerDataHandler(onDataAvailableHandler);
            channel->registerInputHandler(onInputAvailableHandler);
            channel->registerEndOfStreamEventHandler(onEndOfStreamEventHandler);
            channel->registerConnectionTracker(connectTracker);
            {
                // Add to container after setting all handlers - can immediately take part in (auto re-)connection then
                std::lock_guard<std::mutex> lock(m_pipelineChannelsMutex);
                m_inputChannels[channelName] = channel;
            }
            return channel;
        }


        bool SignalSlotable::removeInputChannel(const std::string& channelName) {
            std::lock_guard<std::mutex> lock(m_pipelineChannelsMutex);
            return (m_inputChannels.erase(channelName) > 0);
        }


        OutputChannel::Pointer SignalSlotable::createOutputChannel(const std::string& channelName,
                                                                   const karabo::data::Hash& config,
                                                                   const OutputHandler& onOutputPossibleHandler) {
            if (!config.has(channelName))
                throw KARABO_PARAMETER_EXCEPTION(
                      "The provided configuration must contain the channel name as key in the configuration");
            Hash channelConfig = config.get<Hash>(channelName);
            if (channelConfig.has("schema")) channelConfig.erase("schema");
            // The recommended extra '0' argument requires to call initialize() afterwards:
            OutputChannel::Pointer channel = Configurator<OutputChannel>::create("OutputChannel", channelConfig, 0);
            channel->setInstanceIdAndName(m_instanceId, channelName);
            channel->initialize();

            if (onOutputPossibleHandler) {
                channel->registerIOEventHandler(onOutputPossibleHandler);
            }
            {
                std::lock_guard<std::mutex> lock(m_pipelineChannelsMutex);
                OutputChannel::Pointer& previous = m_outputChannels[channelName];
                if (previous) {
                    // Disable and thus disconnect other ends - in case user code still holds a shared pointer somewhere
                    previous->disable();
                }
                previous = channel; // overwrite what is in m_outputChannels
            }
            return channel;
        }


        bool SignalSlotable::removeOutputChannel(const std::string& channelName) {
            std::lock_guard<std::mutex> lock(m_pipelineChannelsMutex);
            auto it = m_outputChannels.find(channelName);
            if (it == m_outputChannels.end()) {
                return false;
            } else {
                // Disable and thus disconnect other ends - in case user code still holds a shared pointer somewhere.
                it->second->disable();
                m_outputChannels.erase(it);
                return true;
            }
        }


        SignalSlotable::InputChannels SignalSlotable::getInputChannels() const {
            std::lock_guard<std::mutex> lock(m_pipelineChannelsMutex);
            return m_inputChannels;
        }


        SignalSlotable::OutputChannels SignalSlotable::getOutputChannels() const {
            std::lock_guard<std::mutex> lock(m_pipelineChannelsMutex);
            return m_outputChannels;
        }


        std::vector<std::string> SignalSlotable::getOutputChannelNames() const {
            vector<string> names;
            {
                std::lock_guard<std::mutex> lock(m_pipelineChannelsMutex);
                for (const auto& x : m_outputChannels) names.push_back(x.first);
            }
            return names;
        }


        void SignalSlotable::slotGetOutputChannelNames() {
            const std::vector<std::string>& ret = getOutputChannelNames();
            reply(ret);
        }


        OutputChannel::Pointer SignalSlotable::getOutputChannelNoThrow(const std::string& name) {
            std::lock_guard<std::mutex> lock(m_pipelineChannelsMutex);
            OutputChannels::const_iterator it = m_outputChannels.find(name);
            if (it != m_outputChannels.end()) {
                return it->second;
            }
            return OutputChannel::Pointer();
        }


        OutputChannel::Pointer SignalSlotable::getOutputChannel(const std::string& name) {
            auto result = getOutputChannelNoThrow(name);
            if (!result) {
                throw KARABO_PARAMETER_EXCEPTION("OutputChannel \"" + name + " \" does not exist");
            }
            return result;
        }


        InputChannel::Pointer SignalSlotable::getInputChannelNoThrow(const std::string& name) {
            std::lock_guard<std::mutex> lock(m_pipelineChannelsMutex);
            InputChannels::const_iterator it = m_inputChannels.find(name);
            if (it != m_inputChannels.end()) {
                return it->second;
            }
            return InputChannel::Pointer();
        }


        InputChannel::Pointer SignalSlotable::getInputChannel(const std::string& name) {
            auto result = getInputChannelNoThrow(name);
            if (!result) {
                throw KARABO_PARAMETER_EXCEPTION("InputChannel \"" + name + "\" does not exist");
            }
            return result;
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


        void SignalSlotable::connectInputChannels(const boost::system::error_code& e) {
            if (e) return; // cancelled

            // Loop channels
            std::vector<std::string> channelsToCheck;
            std::lock_guard<std::mutex> lock(m_pipelineChannelsMutex);
            using karabo::net::AsyncStatus;
            auto status = std::make_shared<std::vector<AsyncStatus>>(m_inputChannels.size(), AsyncStatus::PENDING);
            auto aMutex = std::make_shared<std::mutex>(); // protects access to above vector of status
            size_t counter = 0;
            size_t fullyConnected = 0;
            for (InputChannels::const_iterator it = m_inputChannels.begin(); it != m_inputChannels.end();
                 ++it, ++counter) {
                const std::string& channelName = it->first;
                channelsToCheck.push_back(channelName);
                const InputChannel::Pointer& channel = it->second;

                // Treat only connections that are disconnected
                std::vector<std::string> outputsToIgnore;
                using karabo::net::ConnectionStatus;
                const std::unordered_map<std::string, ConnectionStatus> connectStatus(channel->getConnectionStatus());
                for (auto mapIt = connectStatus.begin(); mapIt != connectStatus.end(); ++mapIt) {
                    const std::string& outputChannel = mapIt->first;
                    const ConnectionStatus status = mapIt->second;
                    if (status == ConnectionStatus::DISCONNECTED) {
                        KARABO_LOG_FRAMEWORK_DEBUG << getInstanceId() << " Try connecting '" << channelName << "' to '"
                                                   << outputChannel << "'";
                    } else {
                        if (status != ConnectionStatus::CONNECTED) { // log the rare cases
                            const std::string statusTxt(status == ConnectionStatus::CONNECTING
                                                              ? "CONNECTING"
                                                              : (status == ConnectionStatus::DISCONNECTING
                                                                       ? "DISCONNECTING"
                                                                       : toString(static_cast<int>(status))));
                            KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << " Skip connecting '" << channelName
                                                      << "' to '" << outputChannel << "' since connection status is '"
                                                      << statusTxt << "'";
                        }
                        outputsToIgnore.push_back(outputChannel);
                    }
                }
                if (outputsToIgnore.size() == connectStatus.size()) {
                    ++fullyConnected;
                    // Could skip asyncConnectInputChannel, but then take care of counter and size of 'status'
                }
                asyncConnectInputChannel(channel,
                                         bind_weak(&SignalSlotable::handleInputConnected, this, _1, channelName, aMutex,
                                                   status, counter, outputsToIgnore.size()),
                                         outputsToIgnore);
            }
            KARABO_LOG_FRAMEWORK_DEBUG << getInstanceId() << " Channel reconnection cycle cares about '"
                                       << toString(channelsToCheck) << "' (" << fullyConnected
                                       << " of them do not need reconnection)";

            m_channelConnectTimer.expires_after(seconds(channelReconnectIntervalSec));
            m_channelConnectTimer.async_wait(
                  bind_weak(&SignalSlotable::connectInputChannels, this, boost::asio::placeholders::error));
        }

        void SignalSlotable::handleInputConnected(bool success, const std::string& channelName,
                                                  const std::shared_ptr<std::mutex>& mut,
                                                  const std::shared_ptr<std::vector<karabo::net::AsyncStatus>>& status,
                                                  size_t i, size_t numOutputsToIgnore) {
            const InputChannel::Pointer input = getInputChannelNoThrow(channelName);
            const size_t numOutputs = (input ? input->getConnectedOutputChannels().size() : 0ul);
            if (success) {
                if (numOutputs > numOutputsToIgnore) { // avoid spam in logs
                    KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << " connected InputChannel '" << channelName
                                              << "' to " << numOutputs - numOutputsToIgnore << " output channel(s)";
                }
            } else {
                std::string msg;
                try {
                    throw;
                } catch (const karabo::data::TimeoutException&) {
                    msg = "timeout";
                } catch (const std::exception& e) {
                    msg = e.what();
                }
                KARABO_LOG_FRAMEWORK_DEBUG << getInstanceId() << " failed to connect InputChannel '" << channelName
                                           << "' to some of its " << numOutputs - numOutputsToIgnore
                                           << " output channels: " << msg;
            }

            std::lock_guard<std::mutex> lock(*mut);
            (*status)[i] = (success ? AsyncStatus::DONE : AsyncStatus::FAILED);
            size_t nSucceeded = 0;
            for (size_t j = 0; j < status->size(); ++j) {
                if ((*status)[j] == AsyncStatus::PENDING) return;
                if ((*status)[j] == AsyncStatus::DONE) ++nSucceeded;
            }

            KARABO_LOG_FRAMEWORK_DEBUG << getInstanceId() << ": Finished input channel reconnection attempts - "
                                       << nSucceeded << " out of " << status->size() << " succeeded";
            // If this handler would be reliably called, could charge here the m_channelConnectTimer
            // (instead of at the end of connectInputChannels
            //  [where it would then be needed for the m_inputChannels.empty() case only]).
        }

        void SignalSlotable::reconnectInputChannels(const std::string& instanceId) {
            // Loop channels
            InputChannels inputChannels(getInputChannels()); // copy to avoid need for locking mutex while iterating
            for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
                const std::string& channelName = it->first;
                const InputChannel::Pointer& channel = it->second;
                const std::map<std::string, karabo::data::Hash>& outputChannels = channel->getConnectedOutputChannels();
                for (std::map<std::string, karabo::data::Hash>::const_iterator ii = outputChannels.begin();
                     ii != outputChannels.end(); ++ii) {
                    const string& outputChannelString = ii->first;
                    if (instanceId != outputChannelString.substr(0, instanceId.size())) {
                        continue; // instanceId ~ instanceId@output
                    }
                    KARABO_LOG_FRAMEWORK_DEBUG << "reconnectInputChannels for '" << m_instanceId
                                               << "' to output channel '" << outputChannelString << "'";
                    const std::string myInstanceId(getInstanceId());
                    auto handler = [outputChannelString, myInstanceId, channelName](bool success) {
                        if (success) {
                            KARABO_LOG_FRAMEWORK_INFO << myInstanceId << " Successfully reconnected InputChannel '"
                                                      << channelName << "' to '" << outputChannelString << "'";
                        } else {
                            try {
                                throw;
                            } catch (const std::exception& e) {
                                std::string why(e.what());
                                bool bad = true;
                                if (why.find("Transport endpoint is already connected") != std::string::npos) {
                                    why = "Already connected"; // Do not spam log with exception printout
                                    bad = false;
                                }
                                (bad ? KARABO_LOG_FRAMEWORK_WARN : KARABO_LOG_FRAMEWORK_INFO)
                                      << myInstanceId << " Failed to reconnect InputChannel '" << channelName
                                      << "' to '" << outputChannelString << "': " << why;
                            }
                        }
                    };

                    connectInputToOutputChannel(channel, outputChannelString, handler);
                }
            }
        }


        void SignalSlotable::connectInputChannel(const InputChannel::Pointer& channel, int trails) {
            // Called from - pcLayer package for DAQ: DataAggregator::connectToDataSources(..) (Nov. 2018)
            // Loop connected outputs
            const std::map<std::string, karabo::data::Hash>& outputChannels = channel->getConnectedOutputChannels();
            for (std::map<std::string, karabo::data::Hash>::const_iterator it = outputChannels.begin();
                 it != outputChannels.end(); ++it) {
                const string& outputChannelString = it->first;
                connectInputToOutputChannel_old(channel, outputChannelString, trails);
            }
        }


        void SignalSlotable::asyncConnectInputChannel(const InputChannel::Pointer& channel,
                                                      const std::function<void(bool)>& handler,
                                                      const std::vector<std::string>& outputChannelsToIgnore) {
            std::map<std::string, karabo::data::Hash> outputChannels(channel->getConnectedOutputChannels());
            for (const std::string& outputToIgnore : outputChannelsToIgnore) {
                outputChannels.erase(outputToIgnore);
            }

            // Nothing to do except informing that this 'nothing' succeeded ;-)
            if (outputChannels.empty()) {
                // To avoid surprises with mutex locks, do not call directly, but leave context by posting to event loop
                if (handler) EventLoop::post(std::bind(handler, true));
                return;
            }

            // Book-keeping structure for all individual requests, kept alive until last "inner" handler finishes:
            // a mutex to protect against parallel execution of inner handlers, vector of individual statuses,
            // and the outer handler.
            // Note: With map of outputChannelStrings and AsyncStatus (instead of vector<AsyncStatus>) we would be in
            // trouble if someone configured the InputChannel to connect twice to the same OutputChannel - strange, but
            // possible...
            using karabo::net::AsyncStatus;
            auto status =
                  std::make_shared<std::tuple<std::mutex, std::vector<AsyncStatus>, std::function<void(bool)>>>();
            std::get<1>(*status).resize(outputChannels.size(), AsyncStatus::PENDING);
            std::get<2>(*status) = handler; // As direct argument of 'innerHandler' below it would be copied more often.

            // Loop connected outputs
            unsigned int counter = 0;
            for (std::map<std::string, karabo::data::Hash>::const_iterator it = outputChannels.begin();
                 it != outputChannels.end(); ++it, ++counter) {
                const string& outputChannelString = it->first;
                auto innerHandler = bind_weak(&SignalSlotable::connectSingleInputHandler, this, status, counter,
                                              outputChannelString, _1);
                connectInputToOutputChannel(channel, outputChannelString, innerHandler);
            }
        }


        void SignalSlotable::connectSingleInputHandler(
              std::shared_ptr<std::tuple<std::mutex, std::vector<karabo::net::AsyncStatus>, std::function<void(bool)>>>
                    status,
              unsigned int counter, const std::string& outputChannelString, bool singleSuccess) {
            if (!singleSuccess) {
                std::string msg;
                try {
                    throw;
                } catch (const karabo::data::TimeoutException&) {
                    msg = "timeout";
                    Exception::clearTrace();
                } catch (const std::exception& e) {
                    msg = e.what();
                    if (msg.find("Transport endpoint is already connected") != std::string::npos) {
                        msg = "Already connected"; // Do not spam log with exception printout
                    }
                }
                KARABO_LOG_FRAMEWORK_DEBUG << getInstanceId() << " failed to connect InputChannel to '"
                                           << outputChannelString << "': " << msg;
            }

            const std::function<void(bool)>& allReadyHandler = std::get<2>(*status);
            if (!allReadyHandler) {
                // Nobody cares about overall success or failure - so logging above warning is enough.
                return;
            }

            std::vector<karabo::net::AsyncStatus>& singleStatuses = std::get<1>(*status);
            // Lock the mutex:
            std::lock_guard<std::mutex> lock(std::get<0>(*status));
            using karabo::net::AsyncStatus;
            singleStatuses.at(counter) = (singleSuccess ? AsyncStatus::DONE : AsyncStatus::FAILED);

            unsigned int numFailed = 0;
            for (const AsyncStatus aStatus : singleStatuses) {
                switch (aStatus) {
                    case AsyncStatus::PENDING:
                        return; // just wait for the others
                    case AsyncStatus::FAILED:
                        ++numFailed;
                    case AsyncStatus::DONE:;
                }
            }

            // None pending anymore - so report!
            if (0 == numFailed) {
                allReadyHandler(true);
            } else {
                try {
                    throw KARABO_SIGNALSLOT_EXCEPTION(
                          (boost::format("Failed to create %d out of %d connections of an InputChannel") % numFailed %
                           singleStatuses.size())
                                .str());
                } catch (const std::exception&) {
                    Exception::clearTrace();
                    allReadyHandler(false);
                }
            }
        }

        // where is this used?
        void SignalSlotable::connectInputToOutputChannel_old(const InputChannel::Pointer& channel,
                                                             const std::string& outputChannelString, int trials) {
            KARABO_LOG_FRAMEWORK_DEBUG << "connectInputToOutputChannel  on \"" << m_instanceId
                                       << "\"  : outputChannelString is \"" << outputChannelString << "\"";

            std::map<std::string, karabo::data::Hash> outputChannels = channel->getConnectedOutputChannels();
            std::map<std::string, karabo::data::Hash>::const_iterator it = outputChannels.find(outputChannelString);
            if (it == outputChannels.end()) return;

            // it->first => outputChannelString (STRING)
            // it->second => outputChannelInfo  (HASH)  with connection parameters

            bool channelExists = !it->second.empty();
            if (!channelExists) {
                std::vector<std::string> v;
                boost::split(v, outputChannelString, boost::is_any_of("@:"));

                const std::string& instanceId = v[0];
                const std::string& channelId = v[1];

                const unsigned int timeout = 1000; // in ms
                auto successHandler = util::bind_weak(&SignalSlotable::connectInputChannelHandler_old, this, channel,
                                                      outputChannelString, _1, _2);
                auto timeoutHandler = util::bind_weak(&SignalSlotable::connectInputChannelErrorHandler_old, this,
                                                      channel, outputChannelString, trials, timeout + 2000);
                this->request(instanceId, "slotGetOutputChannelInformation", channelId, static_cast<int>(getpid()))
                      .timeout(timeout)
                      .receiveAsync<bool, karabo::data::Hash>(successHandler, timeoutHandler);
            } else {
                // Everything is already there!
                // GF Nov. 2019: But who guarantees that the information is still valid? The other end could have
                //               restarted and have another port - and maybe even another host name!
                channel->connect(it->second); // asynchronous
            }
        }

        void SignalSlotable::connectInputChannelHandler_old(const InputChannel::Pointer& inChannel,
                                                            const std::string& outputChannelString,
                                                            bool outChannelExists,
                                                            const karabo::data::Hash& outChannelInfo) {
            if (outChannelExists) {
                Hash allInfo(outChannelInfo);
                allInfo.set("outputChannelString", outputChannelString);
                inChannel->updateOutputChannelConfiguration(outputChannelString, allInfo);
                inChannel->connect(allInfo); // asynchronous
            } else {
                // Could happen from reconnectInputChannels (from slotInstanceNew) if the new instance registers its
                // OutputChannel after sending its "I am alive!" signal. But that is a bug on that side!
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << " could not connect outputChannel '"
                                           << outputChannelString << "' since its instance has no such channel.";
            }
        }


        void SignalSlotable::connectInputChannelErrorHandler_old(const InputChannel::Pointer& inChannel,
                                                                 const std::string& outputChannelString, int trials,
                                                                 unsigned int nextTimeout) {
            // We do not care whether it was a timeout or another (likely remote) error, so we do not do:
            // try { throw; } catch (TimeoutError&) { _action_;} catch (RemoteException&) {_other action_;}
            if (trials-- > 0) {
                KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << " did not find instance of channel '"
                                          << outputChannelString << "', try again for " << nextTimeout
                                          << " ms (and then " << trials << " more times).";

                std::vector<std::string> v;
                boost::split(v, outputChannelString, boost::is_any_of("@:"));

                const std::string& instanceId = v[0];
                const std::string& channelId = v[1];

                auto successHandler = util::bind_weak(&SignalSlotable::connectInputChannelHandler_old, this, inChannel,
                                                      outputChannelString, _1, _2);
                auto timeoutHandler = util::bind_weak(&SignalSlotable::connectInputChannelErrorHandler_old, this,
                                                      inChannel, outputChannelString, trials, nextTimeout + 2000);
                this->request(instanceId, "slotGetOutputChannelInformation", channelId, static_cast<int>(getpid()))
                      .timeout(nextTimeout)
                      .receiveAsync<bool, karabo::data::Hash>(successHandler, timeoutHandler);
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << " finally gives up to find instance of channel '"
                                           << outputChannelString
                                           << "', maybe it is not yet online (will try again if it gets online).";
            }
        }


        void SignalSlotable::connectInputToOutputChannel(const InputChannel::Pointer& channel,
                                                         const std::string& outputChannelString,
                                                         const std::function<void(bool)>& handler) {
            KARABO_LOG_FRAMEWORK_DEBUG << getInstanceId() << ": connectInputToOutputChannel with "
                                       << "outputChannelString '" << outputChannelString << "'";

            const std::map<std::string, karabo::data::Hash> outputChannels = channel->getConnectedOutputChannels();
            const std::map<std::string, karabo::data::Hash>::const_iterator it =
                  outputChannels.find(outputChannelString);
            if (it == outputChannels.end()) {
                const std::string msg("Cannot connect InputChannel to '" + outputChannelString +
                                      "' since not configured for it.");
                // Log even if handler exists:
                KARABO_LOG_FRAMEWORK_WARN << getInstanceId() << msg;
                if (handler) {
                    try {
                        throw KARABO_SIGNALSLOT_EXCEPTION(msg);
                    } catch (const std::exception&) {
                        Exception::clearTrace();
                        handler(false);
                    }
                }
                return;
            }
            // it->first => outputChannelString (STRING)
            // it->second => outputChannelInfo  (HASH)  with connection parameters

            std::vector<std::string> v;
            boost::split(v, outputChannelString, boost::is_any_of("@:"));

            const std::string& instanceId = v[0];
            const std::string& channelId = (v.size() >= 2 ? v[1] : "");
            auto successHandler = util::bind_weak(&SignalSlotable::connectInputChannelHandler, this, channel,
                                                  outputChannelString, handler, _1);
            const std::string myInstanceId(getInstanceId()); // prepare to capture in lambda without capture of 'this'
            auto failureHandler = [outputChannelString, handler, myInstanceId]() {
                try {
                    throw;
                } catch (const std::exception& e) {
                    // Could directly call handler, but want to add info about path of problem
                    std::string msg("Cannot connect InputChannel to '" + outputChannelString +
                                    "' since failed to receive output channel information");
                    if (handler) {
                        try {
                            KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION(msg += "."));
                        } catch (const std::exception&) {
                            if (handler) handler(false);
                            Exception::clearTrace();
                        }
                    } else {
                        KARABO_LOG_FRAMEWORK_WARN << myInstanceId << " " << msg << ": " << e.what();
                    }
                }
            };
            this->request(instanceId, "slotGetOutputChannelInformation",
                          Hash("channelId", channelId, "processId", static_cast<int>(getpid()), "hostname",
                               boost::asio::ip::host_name()))
                  .timeout(getOutChannelInfoTimeoutMsec)
                  .receiveAsync<karabo::data::Hash>(successHandler, failureHandler);
        }


        void SignalSlotable::connectInputChannelHandler(const InputChannel::Pointer& inChannel,
                                                        const std::string& outputChannelString,
                                                        const std::function<void(bool)>& handler,
                                                        const karabo::data::Hash& outputChannelInfo) {
            const bool outChannelExists = outputChannelInfo.get<bool>("success");
            const karabo::data::Hash& outChannelInfo = outputChannelInfo.get<karabo::data::Hash>("info");
            if (outChannelExists) {
                Hash allInfo(outChannelInfo);
                allInfo.set("outputChannelString", outputChannelString);
                inChannel->updateOutputChannelConfiguration(outputChannelString, allInfo);
                auto connectHandler = [handler, outputChannelString](const karabo::net::ErrorCode& ec) {
                    if (!ec) {
                        if (handler) handler(true);
                    } else {
                        const std::string msg("Cannot connect InputChannel to '" + outputChannelString +
                                              "' since: " + ec.message());
                        if (handler) {
                            try {
                                throw KARABO_SIGNALSLOT_EXCEPTION(msg);
                            } catch (const std::exception&) {
                                Exception::clearTrace();
                                handler(false);
                            }
                        } else {
                            KARABO_LOG_FRAMEWORK_WARN << msg;
                        }
                    }
                };
                inChannel->connect(allInfo, connectHandler); // Asynchronous
            } else {
                const std::string msg("Cannot connect InputChannel to '" + outputChannelString +
                                      "' since instance has no such channel.");
                if (handler) {
                    try {
                        throw KARABO_SIGNALSLOT_EXCEPTION(msg);
                    } catch (const std::exception&) {
                        Exception::clearTrace();
                        handler(false);
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << msg;
                }
            }
        }


        void SignalSlotable::slotGetOutputChannelInformation(const karabo::data::Hash& hash) {
            const std::string& channelId = hash.get<std::string>("channelId");
            // The MDL DeviceClient doesn't provide this processId or hostname information in the hash
            const int processId = (hash.has("processId") ? hash.get<int>("processId") : -1);
            const std::string hostname = (hash.has("hostname") ? hash.get<std::string>("hostname") : std::string());

            std::lock_guard<std::mutex> lock(m_pipelineChannelsMutex);
            OutputChannels::const_iterator it = m_outputChannels.find(channelId);
            if (it != m_outputChannels.end()) {
                karabo::data::Hash h(it->second->getInformation());
                // Almost always the receiver is remote, i.e. in another process:
                h.set("memoryLocation", "remote");
                // But better check whether it is not
                if (processId == static_cast<int>(getpid()) && !std::getenv("KARABO_NO_PIPELINE_SHORTCUT")) {
                    // HACK/workaround:
                    // The host name should be given as slot argument. But for now let's stay backward compatible...
                    const SlotInstancePointer slot(getSlot("slotGetOutputChannelInformation"));
                    if (slot) {
                        if (!hostname.empty() && hostname == boost::asio::ip::host_name()) {
                            // Same pid on same host means same process:
                            h.set("memoryLocation", "local");
                        }
                    }
                }
                reply(karabo::data::Hash("success", true, "info", h));
            } else {
                reply(karabo::data::Hash("success", false, "info", karabo::data::Hash()));
            }
        }


        const std::string& SignalSlotable::getUserName() const {
            return m_username;
        }


        bool SignalSlotable::hasReceivedReply(const std::string& replyId) const {
            std::lock_guard<std::mutex> lock(m_receivedRepliesMutex);
            return m_receivedReplies.find(replyId) != m_receivedReplies.end();
        }


        void SignalSlotable::popReceivedReply(const std::string& replyId, karabo::data::Hash::Pointer& header,
                                              karabo::data::Hash::Pointer& body) {
            std::lock_guard<std::mutex> lock(m_receivedRepliesMutex);
            ReceivedReplies::iterator it = m_receivedReplies.find(replyId);
            if (it != m_receivedReplies.end()) {
                header = it->second.first;
                body = it->second.second;
            }
            // Remove
            m_receivedReplies.erase(it);
        }


        void SignalSlotable::registerSynchronousReply(const std::string& replyId) {
            std::shared_ptr<BoostMutexCond> bmc = std::make_shared<BoostMutexCond>();
            {
                std::lock_guard<std::mutex> lock(m_receivedRepliesBMCMutex);
                m_receivedRepliesBMC[replyId] = bmc;
            }
        }


        bool SignalSlotable::timedWaitAndPopReceivedReply(const std::string& replyId,
                                                          karabo::data::Hash::Pointer& header,
                                                          karabo::data::Hash::Pointer& body, int timeout) {
            bool result = true;
            std::shared_ptr<BoostMutexCond> bmc;
            {
                std::lock_guard<std::mutex> lock(m_receivedRepliesBMCMutex);
                bmc = m_receivedRepliesBMC[replyId];
            }
            try {
                std::unique_lock<std::mutex> lock(bmc->m_mutex);
                while (!this->hasReceivedReply(replyId)) {
                    if (bmc->m_cond.wait_for(lock, milliseconds(timeout)) == std::cv_status::timeout) {
                        result = false;
                        break;
                    }
                }
            } catch (...) {
                KARABO_RETHROW
            }
            try {
                std::lock_guard<std::mutex> lock(m_receivedRepliesBMCMutex);
                m_receivedRepliesBMC.erase(replyId);
            } catch (...) {
                KARABO_RETHROW
            }
            if (result) {
                popReceivedReply(replyId, header, body);
            }
            return result;
        }


        void SignalSlotable::registerPerformanceStatisticsHandler(
              const UpdatePerformanceStatisticsHandler& updatePerformanceStatisticsHandler) {
            m_updatePerformanceStatistics = updatePerformanceStatisticsHandler;
        }


        void SignalSlotable::registerBrokerErrorHandler(const BrokerErrorHandler& errorHandler) {
            std::lock_guard<std::mutex> lock(m_brokerErrorHandlerMutex);
            m_brokerErrorHandler = errorHandler;
        }


        void SignalSlotable::setTopic(const std::string& topic) {
            // Set topic as given as argument.
            // If empty, deduce from the environment.
            if (topic.empty()) {
                m_topic = karabo::net::Broker::brokerDomainFromEnv();
            } else {
                m_topic = topic;
            }
        }


        void SignalSlotable::receiveAsyncTimeoutHandler(const boost::system::error_code& e, const std::string& replyId,
                                                        const SignalSlotable::AsyncErrorHandler& errorHandler) {
            if (e) return; // Timer got cancelled.

            // Remove the slot with function name replyId, as the message took too long
            removeSlot(replyId);
            std::string msg("Timeout of asynchronous request with id '" + replyId + "'");
            if (errorHandler) {
                try {
                    throw KARABO_TIMEOUT_EXCEPTION(msg);
                } catch (const std::exception&) {
                    Exception::clearTrace();
                    try {
                        // Now the errorHandler can do try { throw; } catch (catch karabo::data::TimeoutException& e)
                        // {...;}
                        errorHandler();
                        return;
                    } catch (const std::exception& e) {
                        (msg += ", but error handler throws exception: ") += e.what();
                    }
                }
            }
            KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << ": " << msg;
        }


        void SignalSlotable::addReceiveAsyncErrorHandles(const std::string& replyId,
                                                         const std::shared_ptr<boost::asio::steady_timer>& timer,
                                                         const SignalSlotable::AsyncErrorHandler& errorHandler) {
            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            m_receiveAsyncErrorHandles[replyId] = std::make_pair(timer, errorHandler);
        }


        std::pair<std::shared_ptr<boost::asio::steady_timer>, SignalSlotable::AsyncErrorHandler>
        SignalSlotable::getReceiveAsyncErrorHandles(const std::string& replyId) const {
            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            auto it = m_receiveAsyncErrorHandles.find(replyId);
            if (it != m_receiveAsyncErrorHandles.end()) {
                return it->second;
            }
            return std::make_pair(std::shared_ptr<boost::asio::steady_timer>(), AsyncErrorHandler());
        }


        SignalSlotable::LatencyStats::LatencyStats() : sum(0), counts(0), maximum(0) {}


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
            return counts > 0 ? sum / static_cast<float>(counts) : 0.f;
        }


        Hash getHeartbeatInfo(const Hash& instanceInfo) {
            return Hash("type", instanceInfo.get<std::string>("type"), //
                        "heartbeatInterval", instanceInfo.get<int>("heartbeatInterval"));
        }


    } // namespace xms
} // namespace karabo
