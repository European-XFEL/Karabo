/*
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
#include "karabo/net/Broker.hh"

#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/net/utils.hh"

using namespace karabo::data;


namespace karabo {
    namespace net {

        const std::vector<std::string> Broker::m_broadcastSlots{"slotInstanceNew", "slotInstanceUpdated",
                                                                "slotInstanceGone", "slotDiscover"};

        void Broker::expectedParameters(karabo::data::Schema& s) {
            VECTOR_STRING_ELEMENT(s)
                  .key("brokers")
                  .displayedName("Brokers")
                  .description(
                        "Brokers must be provided as URLs of format: "
                        "tcp://<host>:<port>. Extra URLs serve as fallback.")
                  .assignmentOptional()
                  .defaultValue(brokersFromEnv())
                  .init()
                  .commit();

            STRING_ELEMENT(s)
                  .key("instanceId")
                  .displayedName("Instance ID")
                  .description("Instance ID")
                  .assignmentOptional()
                  .defaultValue("__none__")
                  .init()
                  .commit();

            STRING_ELEMENT(s)
                  .key("domain")
                  .displayedName("Domain")
                  .description("Domain or root topic like SPB, FXE, MID, ...")
                  .assignmentOptional()
                  .defaultValue(Broker::brokerDomainFromEnv())
                  .init()
                  .commit();
        }


        Broker::Broker(const karabo::data::Hash& config)
            : m_availableBrokerUrls(config.get<std::vector<std::string> >("brokers")),
              m_topic(config.get<std::string>("domain")),
              m_instanceId(config.get<std::string>("instanceId")),
              m_consumeBroadcasts(true),
              m_messageHandler(),
              m_errorNotifier() {}


        Broker::Broker(const Broker& o, const std::string& newInstanceId)
            : m_availableBrokerUrls(o.m_availableBrokerUrls),
              m_topic(o.m_topic),
              m_instanceId(newInstanceId),
              m_consumeBroadcasts(true),
              m_messageHandler(),
              m_errorNotifier() {}


        Broker::~Broker() {}


        std::vector<std::string> Broker::brokersFromEnv() {
            const char* env = getenv("KARABO_BROKER");
            return karabo::data::fromString<std::string, std::vector>(
                  env ? env : "amqp://xfel::karabo@exfl-broker-1.desy.de:5672,amqp://guest:guest@localhost:5672");
        }


        std::string Broker::brokerTypeFromEnv() {
            return brokerTypeFrom(brokersFromEnv());
        }


        std::string Broker::brokerTypeFrom(const std::vector<std::string>& urls) {
            std::string type;
            for (const std::string& address : urls) {
                const size_t pos = address.find("://");
                if (pos == std::string::npos || pos == 0u) {
                    throw KARABO_LOGIC_EXCEPTION("Broker address '" + address + "' does not specify protocol.");
                }
                const std::string protocol(address.substr(0, pos));
                if (type.empty()) {
                    type = protocol;
                } else if (type != protocol) {
                    throw KARABO_LOGIC_EXCEPTION("Inconsistent broker types in " +
                                                 karabo::data::toString(brokersFromEnv()));
                }
            }
            return type;
        }

        std::string Broker::brokerDomainFromEnv() {
            // look for environment variables KARABO_BROKER_TOPIC
            // as a fall back the environment variables
            // LOGNAME, USER, LNAME and USERNAME, in order.
            // This implementation is inspired by python's getpass.getuser

            const std::vector<std::string> varNames = {"KARABO_BROKER_TOPIC", "LOGNAME", "USER", "LNAME", "USERNAME"};
            for (const std::string& varName : varNames) {
                const char* env = getenv(varName.c_str());
                if (env && strlen(env) > 0) {
                    return std::string(env);
                }
            }
            return "karabo";
        }

    } // namespace net
} // namespace karabo
