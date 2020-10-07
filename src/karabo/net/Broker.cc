#include "karabo/net/Broker.hh"
#include "karabo/net/utils.hh"
#include "karabo/util.hpp"
#include "karabo/log.hpp"

using namespace karabo::util;


namespace karabo {
    namespace net {


        void Broker::expectedParameters(karabo::util::Schema& s) {

            VECTOR_STRING_ELEMENT(s).key("brokers")
                    .displayedName("Brokers")
                    .description("Brokers must be provided as URLs of format: "
                                 "tcp://<host>:<port>. Extra URLs serve as fallback.")
                    .assignmentOptional().defaultValue(brokersFromEnv())
                    .init()
                    .commit();

            STRING_ELEMENT(s).key("instanceId")
                    .displayedName("Instance ID")
                    .description("Instance ID")
                    .assignmentOptional().defaultValue("__none__")
                    .init()
                    .commit();

            STRING_ELEMENT(s).key("domain")
                    .displayedName("Domain")
                    .description("Domain or root topic like SPB, FXE, MID, ...")
                    .assignmentOptional().defaultValue(Broker::brokerDomainFromEnv())
                    .init()
                    .commit();
        }


        Broker::Broker(const karabo::util::Hash& config)
                : m_availableBrokerUrls(config.get<std::vector<std::string> >("brokers"))
                , m_topic(config.get<std::string>("domain"))
                , m_instanceId(config.get<std::string>("instanceId"))
                , m_consumeBroadcasts(true)
                , m_messageHandler()
                , m_errorNotifier() {
        }


        Broker::Broker(const Broker& o)
                : m_availableBrokerUrls(o.m_availableBrokerUrls)
                , m_topic(o.m_topic)
                , m_consumeBroadcasts(true) {
        }


        Broker::~Broker() {
        }


        std::vector<std::string> Broker::brokersFromEnv() {
            const char* env = getenv("KARABO_BROKER");
            return karabo::util::fromString<std::string, std::vector>(env ? env : "tcp://exfl-broker.desy.de:7777,tcp://localhost:7777");
        }


        std::string Broker::brokerTypeFromEnv() {

            std::string type;
            for (const std::string& address : brokersFromEnv()) {
                const size_t pos = address.find("://");
                if (pos == std::string::npos || pos == 0u) {
                    throw KARABO_LOGIC_EXCEPTION("Broker address '" + address + "' does not specify protocol.");
                }
                const std::string protocol(address.substr(0, pos));
                if (type.empty()) {
                    type = protocol;
                } else if (type != protocol) {
                    throw KARABO_LOGIC_EXCEPTION("Inconsistent broker types in " + karabo::util::toString(brokersFromEnv()));
                }
            }
            // For backward compatibility: jms uses a tcp connection, specified in KARABO_BROKER...
            if (type == "tcp") {
                type = "jms";
            }
            return type;
        }

        std::string Broker::brokerDomainFromEnv() {
            // look for environment variables KARABO_BROKER_TOPIC
            // as a fall back the environment variables
            // LOGNAME, USER, LNAME and USERNAME, in order.
            // This implementation is inspired by python's getpass.getuser

            const std::vector<std::string> varNames = {
                                                       "KARABO_BROKER_TOPIC",
                                                       "LOGNAME",
                                                       "USER",
                                                       "LNAME",
                                                       "USERNAME"
            };
            for (const std::string& varName : varNames) {
                const char* env = getenv(varName.c_str());
                if (env && strlen(env) > 0) {
                    return std::string(env);
                }
            }
            return "karabo";
        }

    }
}
