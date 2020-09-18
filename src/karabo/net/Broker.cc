#include "karabo/net/Broker.hh"
#include "karabo/net/utils.hh"
#include "karabo/util.hpp"
#include "karabo/log.hpp"

using namespace karabo::util;


#define DEFAULT_BROKER_STRING "tcp://exfl-broker.desy.de:7777,tcp://localhost:7777"


namespace karabo {
    namespace net {


        void Broker::expectedParameters(karabo::util::Schema& s) {

            VECTOR_STRING_ELEMENT(s).key("brokers")
                    .displayedName("Brokers")
                    .description("Brokers must be provided as URLs of format: "
                                 "tcp://<host>:<port>. Extra URLs serve as fallback.")
                    .assignmentOptional().defaultValueFromString("tcp://localhost:7777")
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
                    .assignmentOptional().defaultValue("KARABO")
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

    }
}
