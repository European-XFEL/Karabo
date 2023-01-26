/*
 * $Id$
 *
 * File:   RedisBroker.cc
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 * Created on May 11, 2021, 14:47 PM
 */

#include "karabo/net/RedisBroker.hh"

#include <chrono>

#include "karabo/log/Logger.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/SimpleElement.hh"


using namespace karabo::util;


#define REDIS_CLIENT_CLASS "RedisClient"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Broker, karabo::net::RedisBroker)

namespace karabo {
    namespace net {


        void RedisBroker::expectedParameters(karabo::util::Schema& s) {
            UINT32_ELEMENT(s)
                  .key("subscribeTimeout")
                  .displayedName("Subscribe timeout")
                  .description("Max. time awaiting broker response for 'subscribe/unsubsribe' request")
                  .assignmentOptional()
                  .defaultValue(5)
                  .unit(Unit::SECOND)
                  .commit();
        }


        RedisBroker::RedisBroker(const karabo::util::Hash& config)
            : Broker(config),
              m_client(),
              m_handlerStrand(boost::make_shared<Strand>(EventLoop::getIOService())),
              m_messageHandler(),
              m_errorNotifier(),
              m_handlerMutex(),
              m_producerMap(),
              m_producerMapMutex(),
              m_consumerMap(),
              m_consumerTimestamp(),
              m_consumerMapMutex(),
              m_store(),
              m_subscribeTimeout(config.get<unsigned int>("subscribeTimeout")),
              m_timestamp(double(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count())) {
            Hash redisConfig("brokers", m_availableBrokerUrls);
            redisConfig.set("instanceId", m_instanceId);
            redisConfig.set("domain", m_topic);
            m_client = Configurator<RedisClient>::create(REDIS_CLIENT_CLASS, redisConfig);
        }


        RedisBroker::~RedisBroker() {}


        RedisBroker::RedisBroker(const RedisBroker& o, const std::string& newInstanceId)
            : Broker(o, newInstanceId),
              m_client(Configurator<RedisClient>::create(
                    REDIS_CLIENT_CLASS,
                    Hash("brokers", m_availableBrokerUrls, "instanceId", newInstanceId, "domain", m_topic))),
              m_handlerStrand(boost::make_shared<Strand>(EventLoop::getIOService())),
              m_messageHandler(),
              m_errorNotifier(),
              m_handlerMutex(),
              m_producerMap(),
              m_producerMapMutex(),
              m_consumerMap(),
              m_consumerTimestamp(),
              m_consumerMapMutex(),
              m_store(),
              m_timestamp(double(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count())) {}


        Broker::Pointer RedisBroker::clone(const std::string& instanceId) {
            return Broker::Pointer(new RedisBroker(*this, instanceId));
        }


        void RedisBroker::connect() {
            if (!m_client->isConnected()) {
                boost::system::error_code ec = m_client->connect();
                if (ec) {
                    std::ostringstream oss;
                    oss << "Failed to connect to REDIS server: code #" << ec.value() << " -- " << ec.message();
                    throw KARABO_NETWORK_EXCEPTION(oss.str());
                }
            }
        }


        void RedisBroker::disconnect() {
            if (m_client && m_client->isConnected()) m_client->disconnect();
        }


        bool RedisBroker::isConnected() const {
            return (m_client && m_client->isConnected());
        }


        std::string RedisBroker::getBrokerUrl() const {
            return m_client->getBrokerUrl();
        }


        void RedisBroker::redisReadHashHandler(const boost::system::error_code& ec, const std::string& topic,
                                               const Hash::Pointer& msg, const consumer::MessageHandler& handler,
                                               const consumer::ErrorNotifier& errorNotifier) {
            if (m_client && m_client->isConnected() && !ec) {
                checkOrder(topic, msg, handler); // call success handler
            } else {
                // Error ...
                std::ostringstream oss;
                oss << "Topic \"" << topic << "\" : Error code #" << ec.value() << " -- " << ec.message();
                if (errorNotifier) {
                    errorNotifier(consumer::Error::drop, oss.str()); // call error handler
                } else {
                    throw KARABO_NETWORK_EXCEPTION(oss.str());
                }
            }
        }


        boost::system::error_code RedisBroker::subscribeToRemoteSignal(const std::string& signalInstanceId,
                                                                       const std::string& signalFunction) {
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto fut = prom->get_future();
            subscribeToRemoteSignalAsync(signalInstanceId, signalFunction,
                                         [prom](const boost::system::error_code& ec) { prom->set_value(ec); });
            auto status = fut.wait_for(std::chrono::seconds(m_subscribeTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return fut.get();
        }


        void RedisBroker::subscribeToRemoteSignalAsync(const std::string& signalInstanceId,
                                                       const std::string& signalFunction,
                                                       const AsyncHandler& completionHandler) {
            if (!m_client || !m_client->isConnected()) {
                if (completionHandler) {
                    m_handlerStrand->post(boost::bind(completionHandler, KARABO_ERROR_CODE_NOT_CONNECTED));
                }
                return;
            }
            std::string topic =
                  m_topic + "/signals/" + boost::replace_all_copy(signalInstanceId, "/", "|") + "/" + signalFunction;
            {
                boost::mutex::scoped_lock lock(m_handlerMutex);
                if (!m_messageHandler || !m_errorNotifier) {
                    std::ostringstream oss;
                    oss << "Attempt to subscribe to \"" << topic << "\" before startReading is called!";
                    if (completionHandler) {
                        KARABO_LOG_FRAMEWORK_ERROR << oss.str();
                        m_handlerStrand->post(boost::bind(completionHandler, KARABO_ERROR_CODE_IO_ERROR));
                        return;
                    } else {
                        throw KARABO_LOGIC_EXCEPTION(oss.str());
                    }
                }
                auto readHandler = bind_weak(&RedisBroker::redisReadHashHandler, this, _1, _2, _3, m_messageHandler,
                                             m_errorNotifier);
                m_client->subscribeAsync(topic, readHandler, completionHandler);
            }
        }


        boost::system::error_code RedisBroker::unsubscribeFromRemoteSignal(const std::string& signalInstanceId,
                                                                           const std::string& signalFunction) {
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto fut = prom->get_future();
            unsubscribeFromRemoteSignalAsync(signalInstanceId, signalFunction,
                                             [prom](const boost::system::error_code& ec) { prom->set_value(ec); });
            auto status = fut.wait_for(std::chrono::seconds(m_subscribeTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return fut.get();
        }


        void RedisBroker::unsubscribeFromRemoteSignalAsync(const std::string& signalInstanceId,
                                                           const std::string& signalFunction,
                                                           const AsyncHandler& completionHandler) {
            if (!m_client || !m_client->isConnected()) {
                if (completionHandler) {
                    m_handlerStrand->post(boost::bind(completionHandler, KARABO_ERROR_CODE_NOT_CONNECTED));
                }
                return;
            }
            std::string topic =
                  m_topic + "/signals/" + boost::replace_all_copy(signalInstanceId, "/", "|") + "/" + signalFunction;
            if (m_client->isSubscribed(topic)) {
                KARABO_LOG_FRAMEWORK_DEBUG << "RedisBroker::unsubscribeFromRemoteSignalAsync topic=" << topic;
                m_client->unsubscribeAsync(topic, completionHandler);
            } else {
                if (completionHandler) {
                    m_client->post(boost::bind(completionHandler, KARABO_ERROR_CODE_SUCCESS));
                }
            }
        }


        void RedisBroker::setOrderNumbers(const std::string& consumers, const karabo::util::Hash::Pointer& header) {
            std::vector<std::string> consumerIds;
            boost::split(consumerIds, consumers, boost::is_any_of("|"), boost::token_compress_on);
            std::vector<long long> v;
            // NOTE:  Rely on external mutex protection: m_producerMapMutex
            for (const auto& id : consumerIds) {
                // If 'id' not yet in map, m_producerMap[id] creates an entry and zero-initializes it (POD map value).
                v.push_back(++m_producerMap[id]);
            }
            header->set("orderNumbers", toString(v));
            // Set instance timestamp in milliseconds since epoch as a string
            header->set("producerTimestamp", m_timestamp);
        }


        void RedisBroker::write(const std::string& target, const karabo::util::Hash::Pointer& header,
                                const karabo::util::Hash::Pointer& body, const int priority, const int timeToLive) {
            if (!m_client || !m_client->isConnected()) {
                std::ostringstream oss;
                oss << "no broker connection.";
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
            if (!header) {
                throw KARABO_PARAMETER_EXCEPTION("header pointer is null");
            }

            KARABO_LOG_FRAMEWORK_TRACE << "*** write TARGET = \"" << target << "\", topic=\"" << m_topic
                                       << "\"...\n... and HEADER is \n"
                                       << *header;

            boost::mutex::scoped_lock lock(m_producerMapMutex);

            // If orderNumbers is already here ... we are going to re-evaluate it...
            header->erase("orderNumbers");

            std::string topic = "";

            if (target == m_topic + "_beats") {
                topic = m_topic + "/signals/" + boost::replace_all_copy(m_instanceId, "/", "|") + "/signalHeartbeat";

            } else if (target == "karaboGuiDebug") {
                topic = "karaboGuiDebug";

            } else if (target == m_topic) {
                if (!header->has("signalFunction")) {
                    throw KARABO_LOGIC_EXCEPTION("Header has to define \"signalFunction\"");
                }
                if (!header->has("slotInstanceIds")) {
                    throw KARABO_LOGIC_EXCEPTION("Header has to define \"slotInstanceIds\"");
                }

                const std::string& signalInstanceId = header->get<std::string>("signalInstanceId");
                const std::string& signalFunction = header->get<std::string>("signalFunction");
                if (signalInstanceId != m_instanceId) {
                    std::ostringstream oss;
                    oss << "Cannot publish \"" << signalFunction << "\" from \"" << m_instanceId << "\": "
                        << "the signalInstanceId should be \"" << signalInstanceId << "\"!";
                    throw KARABO_LOGIC_EXCEPTION(oss.str());
                }
                std::string slotInstanceIds = header->get<std::string>("slotInstanceIds");
                // strip possible vertical lines ...   ("__none__" is without '|')
                if (slotInstanceIds.at(0) == '|' && slotInstanceIds.at(slotInstanceIds.size() - 1) == '|') {
                    slotInstanceIds = slotInstanceIds.substr(1, slotInstanceIds.size() - 2); // trim | (vertical line)
                }

                if (signalFunction == "__call__" && slotInstanceIds == "*") {
                    // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                    // 'signalFunction' => __call__ STRING
                    // 'slotInstanceIds' => |*| STRING
                    // 'slotFunctions' => |*:slotInstanceNew| STRING

                    // NOTE: broadcast messages are not used for serial number counting...
                    topic = m_topic + "/global_slots";

                } else if (signalFunction == "__request__" ||
                           signalFunction == "__requestNoWait__"
                           // ************************** request **************************
                           // 'replyTo' => 38184c31-6a5a-4f9d-bc81-4d9ae754a16c STRING
                           // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                           // 'signalFunction' => __request__ STRING
                           // 'slotInstanceIds' => |Karabo_GuiServer_0| STRING
                           // 'slotFunctions' => |Karabo_GuiServer_0:slotPing| STRING
                           // ...  or ...
                           // 'replyInstanceIds' => |Karabo_GuiServer_0| STRING
                           // 'replyFunctions' => |Karabo_GuiServer_0:slotLoggerMap| STRING
                           // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                           // 'signalFunction' => __requestNoWait__ STRING
                           // 'slotInstanceIds' => |Karabo_DataLoggerManager_0| STRING
                           // 'slotFunctions' => |Karabo_DataLoggerManager_0:slotGetLoggerMap| STRING

                           || signalFunction == "__reply__" ||
                           signalFunction == "__replyNoWait__"
                           // ************************** reply **************************
                           // 'replyFrom' => 10c91a8f-abbf-47bd-82f5-b8201057e0e2 STRING
                           // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                           // 'signalFunction' => __reply__ STRING
                           // 'slotInstanceIds' => |Karabo_AlarmService| STRING
                           // ... or ...
                           // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                           // 'signalFunction' => __replyNoWait__ STRING
                           // 'slotInstanceIds' => |DataLogger-karabo/dataLogger| STRING
                           // 'slotFunctions' => |DataLogger-karabo/dataLogger:slotChanged| STRING

                           || signalFunction == "__call__") {
                    // ************************** call **************************
                    // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                    // 'signalFunction' => __call__ STRING
                    // 'slotInstanceIds' => |Karabo_AlarmService| STRING
                    // 'slotFunctions' => |Karabo_AlarmService:slotPingAnswer| STRING

                    const std::string& slotInstanceId = slotInstanceIds;
                    if (signalFunction == "__call__" && slotInstanceId.find("|") != std::string::npos) {
                        throw KARABO_LOGIC_EXCEPTION("Unexpected vertical line(|) in slotInstanceId=" + slotInstanceId);
                    }
                    topic = m_topic + "/slots/" + boost::replace_all_copy(slotInstanceId, "/", "|");

                    setOrderNumbers(slotInstanceId, header);

                } else {
                    // ************************** emit **************************
                    // signalFunction == "signalSomething"
                    // Example:
                    // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                    // 'signalFunction' => signalChanged STRING
                    // 'slotInstanceIds' => |DataLogger-karabo/dataLogger||dataAggregator1| STRING
                    // 'slotFunctions' => |DataLogger-karabo/dataLogger:slotChanged||dataAggregator1:slotData| STRING //
                    // 'slotInstanceIds' => |DataLogger-karabo/dataLogger| STRING
                    // ...

                    topic = m_topic + "/signals/" + boost::replace_all_copy(signalInstanceId, "/", "|") + "/" +
                            signalFunction;

                    // slotInstanceIds here is stripped: DataLogger-karabo/dataLogger||dataAggregator1
                    setOrderNumbers(slotInstanceIds, header);
                }
            }

            if (topic.empty()) {
                throw KARABO_LOGIC_EXCEPTION("Attempt to 'write' to unknown target: \"" + target + "\"");
            }

            auto msg = boost::make_shared<Hash>("header", *header, "body", *body);

            try {
                this->publish(topic, msg);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        // This method is protected and virtual ... can be overridden in derived class
        void RedisBroker::publish(const std::string& t, const karabo::util::Hash::Pointer& m) {
            boost::system::error_code ec = m_client->publish(t, m);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to publish to \"" << t << "\""
                    << " : code #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void RedisBroker::registerRedisTopic(const std::string& topic, const consumer::MessageHandler& handler,
                                             const consumer::ErrorNotifier& errorNotifier) {
            auto readHandler = bind_weak(&RedisBroker::redisReadHashHandler, this, _1, _2, _3, handler, errorNotifier);
            boost::system::error_code ec = m_client->subscribe(topic, readHandler);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to subscribe to topic \"" << topic << "\""
                    << " : code #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void RedisBroker::unregisterRedisTopic(const std::string& topic) {
            boost::system::error_code ec;
            ec = m_client->unsubscribe(topic);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to unsubscribe from topic \"" << topic << "\": code #" << ec.value() << " -- "
                    << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void RedisBroker::registerRedisTopics(const std::vector<std::string>& topics,
                                              const consumer::MessageHandler& handler,
                                              const consumer::ErrorNotifier& errorNotifier) {
            auto readHandler = bind_weak(&RedisBroker::redisReadHashHandler, this, _1, _2, _3, handler, errorNotifier);
            RedisTopicSubOptions params;
            for (size_t i = 0; i < topics.size(); i++) {
                params.push_back(std::make_tuple(topics[i], readHandler));
            }
            boost::system::error_code ec = m_client->subscribe(params);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to subscribe to topics \"" << toString(topics) << "\": code #" << ec.value() << " -- "
                    << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void RedisBroker::unregisterRedisTopics(const std::vector<std::string>& topics) {
            boost::system::error_code ec;
            ec = m_client->unsubscribe(topics);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to unsubscribe from topics \"" << toString(topics) << "\": code #" << ec.value()
                    << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void RedisBroker::startReading(const consumer::MessageHandler& handler,
                                       const consumer::ErrorNotifier& errorNotifier) {
            std::string id = boost::replace_all_copy(m_instanceId, "/", "|");
            std::vector<std::string> topics;
            topics.push_back(m_topic + "/slots/" + id);
            if (m_consumeBroadcasts) {
                topics.push_back(m_topic + "/global_slots");
            }
            {
                boost::mutex::scoped_lock lock(m_handlerMutex);
                if (handler) m_messageHandler = handler;
                if (errorNotifier) m_errorNotifier = errorNotifier;
            }
            registerRedisTopics(topics, handler, errorNotifier);
        }


        void RedisBroker::stopReading() {
            if (m_topic.empty() || m_instanceId.empty()) return;
            m_client->unsubscribeAll();
            // Reset for symmetry
            boost::mutex::scoped_lock lock(m_handlerMutex);
            m_messageHandler = consumer::MessageHandler();
            m_errorNotifier = consumer::ErrorNotifier();
        }


        void RedisBroker::checkOrder(const std::string& topic, const karabo::util::Hash::Pointer& msg,
                                     const consumer::MessageHandler& handler) {
            boost::mutex::scoped_lock lock(m_consumerMapMutex);

            auto header = boost::make_shared<Hash>(msg->get<Hash>("header"));
            auto body = boost::make_shared<Hash>(msg->get<Hash>("body"));
            auto callback = boost::bind(handler, header, body);

            if (header->empty() || !header->has("signalInstanceId") || !header->has("slotInstanceIds") ||
                !header->has("orderNumbers") || header->get<std::string>("slotInstanceIds") == "|*|") {
                m_handlerStrand->post(callback);
                return;
            }

            // The producer identity is 'producerId' + 'producerTimestamp' (incarnation) ...
            // because remote producer might be restarted and we can know that by timestamp.
            auto producerId = header->get<std::string>("signalInstanceId");
            // The message has to have "produceTimestamp"
            if (!header->has("producerTimestamp")) {
                throw KARABO_LOGIC_EXCEPTION("Message lacks \"producerTimestamp\"");
            }
            auto producerTimestamp = header->get<double>("producerTimestamp");

            // Check if producer was known before.
            if (m_consumerMap.find(producerId) == m_consumerMap.end()) {
                // (Re-)initialize customer counters
                m_consumerMap.emplace(producerId, 0LL);
                m_consumerTimestamp[producerId] = 0.0; // set invalid timestamp
                // NOTE: consumer is just restarted and m_store[producerId] is not valid
                m_store[producerId] =                                   // producer instanceId
                      std::map<long long,                               // producer order number
                               std::pair<double,                        // producer timestamp
                                         boost::function<void()> > >(); // callback
            }

            auto slotInstanceIds = header->get<std::string>("slotInstanceIds");
            // Convert 'slotInstanceIds' to vector of consumers
            std::vector<std::string> consumerIds; // vector of consumerId
            // strip '|'
            auto stripIds = slotInstanceIds.substr(1, slotInstanceIds.size() - 2);
            // split by "||"
            boost::split(consumerIds, stripIds, boost::is_any_of("|"), boost::token_compress_on);

            // Decode 'orderNumbers' to vector of serial numbers ...
            auto orderNums = fromString<long long, std::vector>(header->get<std::string>("orderNumbers"));

            // Validity check: compare sizes of two vectors: consumer names and serial numbers
            if (orderNums.size() != consumerIds.size()) {
                // Looks like orderNums is not correct, so we cannot trust it ...
                // It can result in desynchronization between producer and consumer and
                // points to logic problems!
                std::ostringstream oss;
                oss << "Length of orderNums=[" << toString(orderNums) << "] > consumerIds=[" << toString(consumerIds)
                    << "] ... m_consumerMap[" << producerId << "]=" << m_consumerMap[producerId] << " ...  header=..\n"
                    << *header;
                throw KARABO_LOGIC_EXCEPTION(oss.str());
            }

            // Find in 2 parallel arrays of equal 'size' the producer's serial number ...
            long long recvNumber = 0LL;
            for (size_t n = 0; n < consumerIds.size(); ++n) {
                if (consumerIds[n] == m_instanceId) {
                    recvNumber = orderNums[n];
                    break;
                }
            }

            if (recvNumber == 0) {
                // subscribed therefore received this message... but slot is not yet registered ...
                return;
            }

            if (m_consumerTimestamp[producerId] != producerTimestamp) {
                // Producer is of another incarnation (restarted)
                m_consumerTimestamp[producerId] = producerTimestamp;
                cleanObsolete(producerId, producerTimestamp); // clean old messages
                m_consumerMap[producerId] = 0;                // synchronize consumer counter
            }

            // Expect the message received in order: recvNumber == (m_consumerMap[producerId] + 1)

            if (recvNumber < (m_consumerMap[producerId] + 1)) {
                return; // duplicated message

            } else if (recvNumber > (m_consumerMap[producerId] + 1)) {
                // special case ... 1st message is out-of-order? No... previous incarnation ...
                if (m_consumerMap[producerId] == 0) {
                    m_consumerMap[producerId] = recvNumber;
                    m_handlerStrand->post(callback);
                    return;
                }
                // Put to the 'store' of pending messages for reordering
                m_store[producerId][recvNumber] = std::make_pair(producerTimestamp, callback);

            } else {
                // Message received in order!
                m_handlerStrand->post(callback);
                m_consumerMap[producerId] = recvNumber; // just synchronize
            }

            handleStore(producerId, recvNumber);
        }


        void RedisBroker::handleStore(const std::string& producerId, long long recvNumber) {
            for (auto it = m_store[producerId].begin(); it != m_store[producerId].end();) {
                // Check if timestamp is valid ...
                if (it->second.first == m_consumerTimestamp[producerId]) {
                    long long currentNumber = it->first;
                    if (currentNumber > (m_consumerMap[producerId] + 1)) {
                        // Maximal number in store
                        long long maxNumber = m_store[producerId].rbegin()->first;
                        // Last inserted number is not max number ...
                        if (maxNumber != recvNumber) break;
                        size_t size = m_store[producerId].size();
                        if (size < 2) {
                            KARABO_LOG_FRAMEWORK_WARN << "*** JAM in \"" << m_instanceId << "\" for \"" << producerId
                                                      << "\", store size: " << size << ", low #" << currentNumber
                                                      << ", high #" << maxNumber
                                                      << ", awaited order number=" << (m_consumerMap[producerId] + 1);
                            break;
                        }
                        m_consumerMap[producerId] = currentNumber - 1;
                    }
                    if (currentNumber == (m_consumerMap[producerId] + 1)) {
                        m_consumerMap[producerId] = currentNumber;
                        m_handlerStrand->post(std::move(it->second.second)); // dispatch callback
                    }
                }
                it = m_store[producerId].erase(it);
            }
        }


        void RedisBroker::cleanObsolete(const std::string& producerId, const double validTimestamp) {
            for (auto it = m_store[producerId].begin(); it != m_store[producerId].end();) {
                if (it->second.first == validTimestamp) {
                    ++it; // keep "valid" message
                } else {
                    it = m_store[producerId].erase(it);
                }
            }
        }


        void RedisBroker::startReadingHeartbeats(const consumer::MessageHandler& handler,
                                                 const consumer::ErrorNotifier& errorNotifier) {
            std::string topic = m_topic + "/signals/*/signalHeartbeat";
            registerRedisTopic(topic, handler, errorNotifier);
        }

    } // namespace net
} // namespace karabo
