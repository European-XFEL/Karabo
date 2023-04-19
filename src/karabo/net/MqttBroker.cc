/*
 * $Id$
 *
 * File:   MqttBroker.cc
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 * Created on June 27, 2020, 9:23 PM
 */

#include "karabo/net/MqttBroker.hh"

#include <chrono>

#include "karabo/log/Logger.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/SimpleElement.hh"


using namespace karabo::util;
using namespace boost::placeholders;


#define MQTT_CLIENT_CLASS "MqttCppClient"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Broker, karabo::net::MqttBroker)

namespace karabo {
    namespace net {


        void MqttBroker::expectedParameters(karabo::util::Schema& s) {
            UINT32_ELEMENT(s)
                  .key("deadline")
                  .displayedName("Deadline timeout")
                  .description("Deadline timeout in milliseconds")
                  .assignmentOptional()
                  .defaultValue(100)
                  .unit(Unit::SECOND)
                  .metricPrefix(MetricPrefix::MILLI)
                  .commit();
        }


        MqttBroker::MqttBroker(const karabo::util::Hash& config)
            : Broker(config),
              m_client(),
              m_handlerStrand(boost::make_shared<Strand>(EventLoop::getIOService())),
              m_messageHandler(),
              m_errorNotifier(),
              m_producerMap(),
              m_producerMapMutex(),
              m_consumerMap(),
              m_consumerTimestamp(),
              m_consumerMapMutex(),
              m_store(),
              m_deadlines(),
              m_deadlineTimeout(config.get<unsigned int>("deadline")),
              m_timestamp(double(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count())) {
            Hash mqttConfig("brokers", m_availableBrokerUrls);
            mqttConfig.set("instanceId", m_instanceId);
            mqttConfig.set("domain", m_topic);
            m_client = Configurator<MqttClient>::create(MQTT_CLIENT_CLASS, mqttConfig);
        }


        MqttBroker::~MqttBroker() {
            stopReading();
            m_client.reset();
        }


        MqttBroker::MqttBroker(const MqttBroker& o, const std::string& newInstanceId)
            : Broker(o, newInstanceId),
              m_client(Configurator<MqttClient>::create(
                    MQTT_CLIENT_CLASS,
                    Hash("brokers", m_availableBrokerUrls, "instanceId", newInstanceId, "domain", m_topic))),
              m_handlerStrand(boost::make_shared<Strand>(EventLoop::getIOService())),
              m_messageHandler(),
              m_errorNotifier(),
              m_producerMap(),
              m_producerMapMutex(),
              m_consumerMap(),
              m_consumerTimestamp(),
              m_consumerMapMutex(),
              m_store(),
              m_deadlines(),
              m_deadlineTimeout(o.m_deadlineTimeout),
              m_timestamp(double(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count())) {}


        Broker::Pointer MqttBroker::clone(const std::string& instanceId) {
            return Broker::Pointer(new MqttBroker(*this, instanceId));
        }


        void MqttBroker::connect() {
            if (!m_client->isConnected()) {
                boost::system::error_code ec = m_client->connect();
                if (ec) {
                    std::ostringstream oss;
                    oss << "Failed to connect to MQTT broker: code #" << ec.value() << " -- " << ec.message();
                    throw KARABO_NETWORK_EXCEPTION(oss.str());
                }
            }
        }


        void MqttBroker::disconnect() {
            if (m_client && m_client->isConnected()) m_client->disconnect();
        }


        bool MqttBroker::isConnected() const {
            return (m_client && m_client->isConnected());
        }


        std::string MqttBroker::getBrokerUrl() const {
            return m_client->getBrokerUrl();
        }


        std::string MqttBroker::getClientId() const {
            return m_client->getClientId();
        }


        void MqttBroker::mqttReadHashHandler(const boost::system::error_code& ec, const std::string& topic,
                                             const Hash::Pointer& msg, const consumer::MessageHandler& handler,
                                             const consumer::ErrorNotifier& errorNotifier) {
            if (!ec) {
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


        boost::system::error_code MqttBroker::subscribeToRemoteSignal(const std::string& signalInstanceId,
                                                                      const std::string& signalFunction) {
            std::promise<boost::system::error_code> prom;
            auto fut = prom.get_future();
            subscribeToRemoteSignalAsync(signalInstanceId, signalFunction,
                                         [&](const boost::system::error_code& ec) { prom.set_value(ec); });
            return fut.get();
        }


        void MqttBroker::subscribeToRemoteSignalAsync(const std::string& signalInstanceId,
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
            auto readHandler =
                  bind_weak(&MqttBroker::mqttReadHashHandler, this, _1, _2, _3, m_messageHandler, m_errorNotifier);
            // SubQos::AtLeastOnce results in performance drop.
            constexpr SubOpts subopts = SubQos::AtMostOnce;
            m_client->subscribeAsync(topic, subopts, readHandler, completionHandler);
        }


        boost::system::error_code MqttBroker::unsubscribeFromRemoteSignal(const std::string& signalInstanceId,
                                                                          const std::string& signalFunction) {
            std::promise<boost::system::error_code> prom;
            auto fut = prom.get_future();
            unsubscribeFromRemoteSignalAsync(signalInstanceId, signalFunction,
                                             [&](const boost::system::error_code& ec) { prom.set_value(ec); });
            return fut.get();
        }


        void MqttBroker::unsubscribeFromRemoteSignalAsync(const std::string& signalInstanceId,
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
                KARABO_LOG_FRAMEWORK_DEBUG << "MqttBroker::unsubscribeFromRemoteSignalAsync topic=" << topic;
                m_client->unsubscribeAsync(topic, completionHandler);
            } else {
                if (completionHandler) {
                    completionHandler(KARABO_ERROR_CODE_SUCCESS);
                }
            }
        }


        void MqttBroker::setOrderNumbers(const std::string& consumers, const karabo::util::Hash::Pointer& header) {
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


        void MqttBroker::write(const std::string& target, const karabo::util::Hash::Pointer& header,
                               const karabo::util::Hash::Pointer& body, const int priority, const int timeToLive) {
            if (!m_client || !m_client->isConnected()) {
                std::ostringstream oss;
                oss << "MqttBroker.write: no broker connection.";
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
            if (!header) {
                throw KARABO_PARAMETER_EXCEPTION("MqttBroker.write: header pointer is null");
            }

            PubOpts pubopts = PubQos::AtMostOnce;
            if (priority >= 4) pubopts = PubQos::AtLeastOnce; // Strange but it has no influence onto performance

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

                    setOrderNumbers(slotInstanceIds, header);
                }
            }

            if (topic.empty()) {
                throw KARABO_LOGIC_EXCEPTION("Attempt to 'write' to unknown target: \"" + target + "\"");
            }

            auto msg = boost::make_shared<Hash>("header", *header, "body", *body);

            try {
                this->publish(topic, msg, pubopts);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        // This method is protected and virtual ... can be overridden in derived class
        void MqttBroker::publish(const std::string& t, const karabo::util::Hash::Pointer& m, PubOpts o) {
            boost::system::error_code ec = m_client->publish(t, m, o);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to publish to \"" << t << "\", pubopts=" << o << " : code #" << ec.value() << " -- "
                    << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void MqttBroker::registerMqttTopic(const std::string& topic, const karabo::net::SubOpts& subopts,
                                           const consumer::MessageHandler& handler,
                                           const consumer::ErrorNotifier& errorNotifier) {
            auto readHandler = bind_weak(&MqttBroker::mqttReadHashHandler, this, _1, _2, _3, handler, errorNotifier);
            boost::system::error_code ec = m_client->subscribe(topic, subopts, readHandler);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to subscribe to topic \"" << topic << "\", " << subopts << " : code #" << ec.value()
                    << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void MqttBroker::unregisterMqttTopic(const std::string& topic) {
            boost::system::error_code ec;
            ec = m_client->unsubscribe(topic);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to unsubscribe to topic \"" << topic << "\": code #" << ec.value() << " -- "
                    << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void MqttBroker::registerMqttTopics(const std::vector<std::string>& topics,
                                            const std::vector<karabo::net::SubOpts>& options,
                                            const consumer::MessageHandler& handler,
                                            const consumer::ErrorNotifier& errorNotifier) {
            if (topics.size() != options.size()) {
                throw KARABO_LOGIC_EXCEPTION("The topics vector size not the same as options size");
            }
            auto readHandler = bind_weak(&MqttBroker::mqttReadHashHandler, this, _1, _2, _3, handler, errorNotifier);
            TopicSubOptions params;
            for (size_t i = 0; i < topics.size(); i++) {
                params.push_back(std::make_tuple(topics[i], options[i], readHandler));
            }
            boost::system::error_code ec = m_client->subscribe(params);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to subscribe to topics \"" << toString(topics) << "\": code #" << ec.value() << " -- "
                    << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void MqttBroker::unregisterMqttTopics(const std::vector<std::string>& topics) {
            boost::system::error_code ec;
            ec = m_client->unsubscribe(topics);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to unsubscribe from topics \"" << toString(topics) << "\": code #" << ec.value()
                    << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void MqttBroker::startReading(const consumer::MessageHandler& handler,
                                      const consumer::ErrorNotifier& errorNotifier) {
            m_messageHandler = handler;
            m_errorNotifier = errorNotifier;
            std::string id = boost::replace_all_copy(m_instanceId, "/", "|");
            std::vector<std::string> topics;
            std::vector<SubOpts> options;
            topics.push_back(m_topic + "/slots/" + id);
            options.push_back(SubQos::AtMostOnce);
            if (m_consumeBroadcasts) {
                topics.push_back(m_topic + "/global_slots");
                options.push_back(SubQos::ExactlyOnce);
            }
            registerMqttTopics(topics, options, handler, errorNotifier);
        }


        void MqttBroker::stopReading() {
            if (m_topic.empty() || m_instanceId.empty()) return;
            m_client->unsubscribeAll();
            m_messageHandler = consumer::MessageHandler();
            m_errorNotifier = consumer::ErrorNotifier();
        }


        void MqttBroker::checkOrder(const std::string& topic, const karabo::util::Hash::Pointer& msg,
                                    const consumer::MessageHandler& handler) {
            boost::mutex::scoped_lock lock(m_consumerMapMutex);

            auto header = boost::make_shared<Hash>(msg->get<Hash>("header"));
            auto body = boost::make_shared<Hash>(msg->get<Hash>("body"));
            auto callback = boost::bind(handler, header, body);

            // orderNumbers in header
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

                // Put to 'm_store' of pending messages for reordering
                m_store[producerId][recvNumber] = std::make_pair(producerTimestamp, callback);

            } else {
                // Message received in order!
                m_handlerStrand->post(callback);
                m_consumerMap[producerId] = recvNumber; // just synchronize
            }

            handleStore(producerId, recvNumber);
        }


        void MqttBroker::handleStore(const std::string& producerId, long long recvNumber) {
            // Try to resolve possible ordering problem
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


        void MqttBroker::cleanObsolete(const std::string& producerId, const double validTimestamp) {
            for (auto it = m_store[producerId].begin(); it != m_store[producerId].end();) {
                if (it->second.first == validTimestamp) {
                    ++it; // keep "valid" message
                } else {
                    it = m_store[producerId].erase(it);
                }
            }
        }


        void MqttBroker::startReadingHeartbeats(const consumer::MessageHandler& handler,
                                                const consumer::ErrorNotifier& errorNotifier) {
            std::string id = boost::replace_all_copy(m_instanceId, "/", "|");
            std::string topic = m_topic + "/signals/+/signalHeartbeat";
            registerMqttTopic(topic, SubQos::AtMostOnce, handler, errorNotifier);
        }

    } // namespace net
} // namespace karabo
