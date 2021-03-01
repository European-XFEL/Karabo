/*
 * $Id$
 *
 * File:   MqttBroker.cc
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 * Created on June 27, 2020, 9:23 PM
 */

#include "karabo/net/EventLoop.hh"
#include "karabo/net/MqttBroker.hh"
#include "karabo/net/utils.hh"
#include "karabo/log/Logger.hh"


using namespace karabo::util;


#define MQTT_CLIENT_CLASS "MqttCppClient"


namespace karabo {
    namespace net {


        KARABO_REGISTER_FOR_CONFIGURATION(Broker, MqttBroker)


        void MqttBroker::expectedParameters(karabo::util::Schema& s) {
        }


        MqttBroker::MqttBroker(const karabo::util::Hash& config)
            : Broker(config)
            , m_client()
            , m_handlerStrand(boost::make_shared<Strand>(EventLoop::getIOService()))
            , m_messageHandler()
            , m_errorNotifier() {
            Hash mqttConfig("brokers", m_availableBrokerUrls);
            mqttConfig.set("instanceId", m_instanceId);
            mqttConfig.set("domain", m_topic);
            m_client = Configurator<MqttClient>::create(MQTT_CLIENT_CLASS, mqttConfig);
        }


        MqttBroker::~MqttBroker() {
            stopReading();
            m_client.reset();
        }


        MqttBroker::MqttBroker(const MqttBroker& o, const std::string& newInstanceId) :
            Broker(o, newInstanceId),
            m_client(Configurator<MqttClient>::create(MQTT_CLIENT_CLASS,
                                                      Hash("brokers", m_availableBrokerUrls,
                                                           "instanceId", newInstanceId,
                                                           "domain", m_topic))),
            m_handlerStrand(boost::make_shared<Strand>(EventLoop::getIOService())),
            m_messageHandler(),
            m_errorNotifier() {
        }


        Broker::Pointer MqttBroker::clone(const std::string& instanceId) {
            return Broker::Pointer(new MqttBroker(*this, instanceId));
        }


        void MqttBroker::connect() {
            if (!m_client->isConnected()) {
                boost::system::error_code ec = m_client->connect();
                if (ec) {
                    std::ostringstream oss;
                    oss << "Failed to connect to MQTT broker: code #"
                            << ec.value() << " -- " << ec.message();
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


        void MqttBroker::mqttReadHashHandler(const boost::system::error_code& ec,
                                             const std::string& topic,
                                             const Hash::Pointer & msg,
                                             const consumer::MessageHandler& handler,
                                             const consumer::ErrorNotifier& errorNotifier) {
            if (!ec) {
                auto hdr = boost::make_shared<Hash>(msg->get<Hash>("header"));
                auto body = boost::make_shared<Hash>(msg->get<Hash>("body"));
                checkOrder(handler, hdr, body); // call success handler
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
            subscribeToRemoteSignalAsync(signalInstanceId,
                                         signalFunction,
                                         [&]
                                         (const boost::system::error_code & ec) {
                                             prom.set_value(ec);
                                         });
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
            std::string topic = m_topic + "/signals/" + boost::replace_all_copy(signalInstanceId, "/", "|") + "/" + signalFunction;
            auto readHandler = bind_weak(&MqttBroker::mqttReadHashHandler, this, _1, _2, _3, m_messageHandler, m_errorNotifier);
            constexpr SubOpts subopts = SubQos::AtLeastOnce;
            m_client->subscribeAsync(topic, subopts, readHandler, completionHandler);
        }


        boost::system::error_code MqttBroker::unsubscribeFromRemoteSignal(const std::string& signalInstanceId,
                                                                          const std::string& signalFunction) {

            std::promise<boost::system::error_code> prom;
            auto fut = prom.get_future();
            unsubscribeFromRemoteSignalAsync(signalInstanceId,
                                             signalFunction,
                                             [&]
                                             (const boost::system::error_code & ec) {
                                                 prom.set_value(ec);
                                             });
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
            std::string topic = m_topic + "/signals/"
                    + boost::replace_all_copy(signalInstanceId, "/", "|")
                    + "/" + signalFunction;
            if (m_client->isSubscribed(topic)) {
                KARABO_LOG_FRAMEWORK_DEBUG << "MqttBroker::unsubscribeFromRemoteSignalAsync topic=" << topic;
                m_client->unsubscribeAsync(topic, completionHandler);
            } else {
                if (completionHandler) {
                    completionHandler(KARABO_ERROR_CODE_SUCCESS);
                }
            }
        }


        void MqttBroker::write(const std::string& target,
                               const karabo::util::Hash::Pointer& header,
                               const karabo::util::Hash::Pointer& body,
                               const int priority, const int timeToLive) {

            if (!m_client || !m_client->isConnected()) {
                std::ostringstream oss;
                oss << "MqttBroker.write: no broker connection.";
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
            if (!header) {
                throw KARABO_PARAMETER_EXCEPTION("MqttBroker.write: header pointer is null");
            }

            PubOpts pubopts = PubQos::AtMostOnce;
            if (priority >= 4) pubopts = PubQos::AtLeastOnce;

            KARABO_LOG_FRAMEWORK_TRACE << "*** write TARGET = \"" << target << "\", topic=\"" << m_topic
                    << "\"...\n... and HEADER is \n" << *header;

            std::string topic = "";

            if (target == m_topic + "_beats") {

                topic = m_topic + "/signals/" + boost::replace_all_copy(m_instanceId, "/", "|") + "/signalHeartbeat";

            } else if (target == "karaboGuiDebug") {

                topic = "karaboGuiDebug";

            } else if (target == m_topic && header->has("target") && header->get<std::string>("target") == "log") {

                topic = m_topic + "/log";

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
                slotInstanceIds = slotInstanceIds.substr(1, slotInstanceIds.size() - 2); // trim | (vertical line)

                if (signalFunction == "__request__" || signalFunction == "__requestNoWait__") {
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

                    const std::string& slotInstanceId = slotInstanceIds;
                    topic = m_topic + "/slots/" + boost::replace_all_copy(slotInstanceId, "/", "|");

                } else if (signalFunction == "__reply__" || signalFunction == "__replyNoWait__") {
                    // 'replyFrom' => 10c91a8f-abbf-47bd-82f5-b8201057e0e2 STRING
                    // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                    // 'signalFunction' => __reply__ STRING
                    // 'slotInstanceIds' => |Karabo_AlarmService| STRING
                    // ... or ...
                    // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                    // 'signalFunction' => __replyNoWait__ STRING
                    // 'slotInstanceIds' => |DataLogger-karabo/dataLogger| STRING
                    // 'slotFunctions' => |DataLogger-karabo/dataLogger:slotChanged| STRING

                    const std::string& slotInstanceId = slotInstanceIds;
                    topic = m_topic + "/slots/" + boost::replace_all_copy(slotInstanceId, "/", "|");

                } else if (signalFunction == "__call__") {

                    if (slotInstanceIds == "*") {
                        // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                        // 'signalFunction' => __call__ STRING
                        // 'slotInstanceIds' => |*| STRING
                        // 'slotFunctions' => |*:slotInstanceNew| STRING

                        if (!header->has("slotFunctions")) {
                            throw KARABO_LOGIC_EXCEPTION("Header has to define \"slotFunctions\"");
                        }
                        const std::string& slotFunctions = header->get<std::string>("slotFunctions");
                        std::string slotFunction = slotFunctions.substr(1, slotFunctions.size() - 2); // trim |
                        slotFunction = slotFunction.substr(2);
                        topic = m_topic + "/global_slots/" + slotFunction;

                    } else {
                        // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                        // 'signalFunction' => __call__ STRING
                        // 'slotInstanceIds' => |Karabo_AlarmService| STRING
                        // 'slotFunctions' => |Karabo_AlarmService:slotPingAnswer| STRING

                        const std::string& slotInstanceId = slotInstanceIds;
                        if (slotInstanceId.find("|") != std::string::npos) {
                            throw KARABO_LOGIC_EXCEPTION("Unexpected vertical line(|) in slotInstanceId=" + slotInstanceId);
                        }
                        topic = m_topic + "/slots/" + boost::replace_all_copy(slotInstanceId, "/", "|");

                    }

                } else {
                    // signalFunction == "signalSomething"
                    // Example:
                    // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                    // 'signalFunction' => signalChanged STRING
                    // 'slotInstanceIds' => |DataLogger-karabo/dataLogger| STRING
                    // 'slotFunctions' => |DataLogger-karabo/dataLogger:slotChanged| STRING
                    // ...

                    topic = m_topic + "/signals/" + boost::replace_all_copy(signalInstanceId, "/", "|") + "/" + signalFunction;

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


        void MqttBroker::publish(const std::string& t,
                                 const karabo::util::Hash::Pointer& m,
                                 PubOpts o) {

            boost::system::error_code ec = m_client->publish(t, m, o);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to publish to \"" << t << "\", pubopts="
                        << o << " : code #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void MqttBroker::registerMqttTopic(const std::string& topic,
                                           const karabo::net::SubOpts& subopts,
                                           const consumer::MessageHandler& handler,
                                           const consumer::ErrorNotifier& errorNotifier) {

            auto readHandler = bind_weak(&MqttBroker::mqttReadHashHandler, this, _1, _2, _3, handler, errorNotifier);
            boost::system::error_code ec = m_client->subscribe(topic, subopts, readHandler);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to subscribe to topic \"" << topic << "\", " << subopts
                        << " : code #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void MqttBroker::unregisterMqttTopic(const std::string& topic) {
            boost::system::error_code ec;
            ec = m_client->unsubscribe(topic);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to unsubscribe to topic \"" << topic << "\": code #"
                        << ec.value() << " -- " << ec.message();
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
                oss << "Failed to subscribe to topics \"" << toString(topics)
                        << "\": code #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void MqttBroker::unregisterMqttTopics(const std::vector<std::string>& topics) {
            boost::system::error_code ec;
            ec = m_client->unsubscribe(topics);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to unsubscribe from topics \"" << toString(topics)
                        << "\": code #" << ec.value() << " -- " << ec.message();
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
            options.push_back(SubQos::AtLeastOnce);
            if (m_consumeBroadcasts) {
                topics.push_back(m_topic + "/global_slots/+");
                options.push_back(SubQos::AtLeastOnce);
            }
            registerMqttTopics(topics, options,
                               bind_weak(&MqttBroker::checkOrder, this,
                                         handler, _1, _2, false), errorNotifier);
        }


        void MqttBroker::stopReading() {
            if (m_topic.empty() || m_instanceId.empty()) return;
            // TODO:  Check m_store for pending messages ...
            // ... and if any pass them to m_messageHandler
            m_client->unsubscribeAll();
            m_messageHandler = consumer::MessageHandler();
            m_errorNotifier = consumer::ErrorNotifier();
        }


        void MqttBroker::writeLocal(const consumer::MessageHandler& handler,
                                    const karabo::util::Hash::Pointer& header,
                                    const karabo::util::Hash::Pointer& body) {
            this->checkOrder(handler, header, body, true);
        }


        void MqttBroker::checkOrder(const consumer::MessageHandler& handler,
                                    const karabo::util::Hash::Pointer& header,
                                    const karabo::util::Hash::Pointer& body,
                                    bool local) {

            if (handler) {
                m_handlerStrand->post(boost::bind(handler, header, body));
            }
            return;
        }


        void MqttBroker::startReadingHeartbeats(const consumer::MessageHandler& handler,
                                                const consumer::ErrorNotifier& errorNotifier) {
            std::string id = boost::replace_all_copy(m_instanceId, "/", "|");
            std::string topic = m_topic + "/signals/+/signalHeartbeat";
            registerMqttTopic(topic, SubQos::AtMostOnce, handler, errorNotifier);
        }


        void MqttBroker::startReadingLogs(const consumer::MessageHandler& handler,
                                          const consumer::ErrorNotifier& errorNotifier) {
            std::string topic = m_topic + "/log";
            registerMqttTopic(topic, SubQos::AtMostOnce, handler, errorNotifier);
        }

    }
}
