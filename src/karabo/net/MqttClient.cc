/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#include "MqttClient.hh"

#include "karabo/net/EventLoop.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/VectorElement.hh"

using namespace karabo::util;
// using namespace karabo::io;

namespace karabo {
    namespace net {


        void MqttClient::expectedParameters(Schema& expected) {
            VECTOR_STRING_ELEMENT(expected)
                  .key("brokers")
                  .displayedName("Broker URLs")
                  .description("Vector of URLs {\"mqtt://hostname:port\",...}")
                  .assignmentMandatory()
                  .minSize(1)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("instanceId")
                  .displayedName("Instance ID")
                  .description("Instance ID")
                  .assignmentOptional()
                  .defaultValue("none")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("domain")
                  .displayedName("Domain")
                  .description("Domain is root topic (former JMS topic)")
                  .assignmentMandatory()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("cleanSession")
                  .displayedName("Clean session")
                  .description("Declare non-persistent connection")
                  .assignmentOptional()
                  .defaultValue(true)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("username")
                  .displayedName("User name")
                  .description("User name")
                  .assignmentOptional()
                  .defaultValue(getenv("USER") ? getenv("USER") : "")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("password")
                  .displayedName("Password")
                  .description("Password")
                  .assignmentOptional()
                  .defaultValue("")
                  .commit();

            UINT16_ELEMENT(expected)
                  .key("keepAliveSec")
                  .displayedName("Keep alive")
                  .description("Max. number of seconds that client connection considered being alive")
                  .assignmentOptional()
                  .defaultValue(120)
                  .unit(Unit::SECOND)
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("skipFlag")
                  .displayedName("Skip body deserialization")
                  .description("Skip body deserialization, i.e. keep message body as a binary blob")
                  .assignmentOptional()
                  .defaultValue(false)
                  .commit();
        }


        MqttClient::MqttClient(const karabo::util::Hash& input)
            : m_ios(boost::make_shared<boost::asio::io_context>()),
              m_thread(),
              m_brokerUrls(),
              m_domain(),
              m_instanceId(),
              m_skipFlag(input.get<bool>("skipFlag")) {
            input.get("domain", m_domain);
            input.get("instanceId", m_instanceId);
            run();
        }


        MqttClient::~MqttClient() {
            m_ios->stop();
            if (m_thread->get_id() != boost::this_thread::get_id()) {
                m_thread->join();
            }
        }


        void MqttClient::run() {
            // NOTE from Gero:
            // In case that instead of using this single threaded io-service, the
            // Karabo event loop is used, the mqtt client tests have to be
            // refactored, i.e. the CPPUNIT_ASSERT have to be moved from handlers
            // to the test function (since the Karabo event loop threads catch exceptions).

            m_thread = boost::make_shared<boost::thread>([this]() {
                boost::asio::io_context::work work(*m_ios);
                m_ios->run();
            });
        }


        std::string MqttClient::getUuidAsString() {
            static std::atomic<unsigned int> counter(0);
            return bareHostName() + "_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
        }


        namespace mqtttools {


            bool topicHasWildcard(const std::string& topic) {
                return topic.find_last_of("+#") != std::string::npos;
            }


            bool topicMatches(const std::string& sub, const std::string& topic) {
                size_t spos = 0, tpos = 0;
                size_t topicLen = topic.size();
                size_t subLen = sub.size();
                bool multilevelWildcard = false;

                if (subLen == 0 || topicLen == 0) {
                    return false;
                }

                while (spos < subLen && tpos <= topicLen) {
                    if (sub[spos] == topic[tpos]) {
                        if (tpos == topicLen - 1) {
                            /* Check for e.g. foo matching foo/# */
                            if (spos == subLen - 3 && sub[spos + 1] == '/' && sub[spos + 2] == '#') {
                                return true;
                            }
                        }
                        spos++;
                        tpos++;
                        if (spos == subLen && tpos == topicLen) {
                            return true;
                        } else if (tpos == topicLen && spos == subLen - 1 && sub[spos] == '+') {
                            if (spos > 0 && sub[spos - 1] != '/') {
                                return false;
                            }
                            return true;
                        }
                    } else {
                        if (sub[spos] == '+') {
                            /* Check for bad "+foo" or "a/+foo" subscription */
                            if (spos > 0 && sub[spos - 1] != '/') {
                                return false;
                            }
                            /* Check for bad "foo+" or "foo+/a" subscription */
                            if (spos < subLen - 1 && sub[spos + 1] != '/') {
                                return false;
                            }
                            spos++;
                            while (tpos < topicLen && topic[tpos] != '/') {
                                tpos++;
                            }
                            if (tpos == topicLen && spos == subLen) {
                                return true;
                            }
                        } else if (sub[spos] == '#') {
                            if (spos > 0 && sub[spos - 1] != '/') {
                                return false;
                            }
                            multilevelWildcard = true;
                            if (spos + 1 != subLen) {
                                return false;
                            } else {
                                return true;
                            }
                        } else {
                            /* Check for e.g. foo/bar matching foo/+/# */
                            if (spos > 0 && spos + 2 == subLen && tpos == topicLen && sub[spos - 1] == '+' &&
                                sub[spos] == '/' && sub[spos + 1] == '#') {
                                return true;
                            } else {
                                return false;
                            }
                        }
                    }
                }
                if (multilevelWildcard == false && (tpos < topicLen || spos < subLen)) {
                    return false;
                }
                return true;
            }
        } // namespace mqtttools

    } // namespace net
} // namespace karabo
