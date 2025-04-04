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

#include "BrokerUtils.hh"

#include <boost/algorithm/string.hpp>
#include <iostream>

std::vector<std::string> getBrokersFromString(const std::string& brokers, const std::string& expectedProtocol) {
    std::vector<std::string> brokerUrls;
    boost::split(brokerUrls, brokers, boost::is_any_of(";"));
    for (const std::string& brokerUrl : brokerUrls) {
        const std::string::size_type n = brokerUrl.find(":");
        if (n == std::string::npos) {
            std::clog << "Unexpected Broker syntax for broker '" << brokerUrl << "'. Ignoring..." << std::endl;
            continue;
        }
        const std::string protocol = brokerUrl.substr(0, n);
        if (expectedProtocol.empty() || protocol == expectedProtocol) {
            std::vector<std::string> vBrokerUrls;
            boost::split(vBrokerUrls, brokerUrl, boost::is_any_of(","));
            return vBrokerUrls;
        }
    }
    return {};
}


std::vector<std::string> getBrokersFromEnvName(const std::string& varName, const std::string& expectedProtocol) {
    const char* brokers_env = getenv(varName.c_str());
    if (brokers_env) {
        std::string brokers(brokers_env);
        return getBrokersFromString(brokers, expectedProtocol);
    }
    return {};
}


std::vector<std::string> getBrokersFromEnv(const std::string& protocol) {
    std::vector<std::string> brokers = getBrokersFromEnvName("KARABO_CI_BROKERS", protocol);
    // try KARABO_BROKER
    if (brokers.empty()) {
        brokers = getBrokersFromEnvName("KARABO_BROKER", protocol);
    }
    return brokers;
}


karabo::data::Hash getBrokersFromEnv() {
    karabo::data::Hash ret;
    char* brokers_env = getenv("KARABO_CI_BROKERS");
    if (!brokers_env) {
        brokers_env = getenv("KARABO_BROKER");
    }
    if (brokers_env) {
        std::string brokers(brokers_env);
        std::vector<std::string> brokerUrls;
        boost::split(brokerUrls, brokers, boost::is_any_of(";"));
        for (const std::string& brokerUrl : brokerUrls) {
            std::vector<std::string> vBrokerUrl = getBrokersFromString(brokerUrl, "");
            if (vBrokerUrl.empty()) {
                continue;
            }
            const std::string::size_type n = brokerUrl.find(":");
            if (n == std::string::npos) {
                std::clog << "Unexpected Broker syntax for broker '" << brokerUrl << "'. Ignoring..." << std::endl;
                continue;
            }
            const std::string protocol = vBrokerUrl[0].substr(0, n);
            if (protocol == "amqp") {
                ret.set(protocol, vBrokerUrl);
            }
        }
    }
    return ret;
}

std::vector<std::string> getBrokerFromEnv(const std::string& protocol) {
    std::vector<std::string> brokers = getBrokersFromEnvName("KARABO_CI_BROKERS", protocol);
    // try KARABO_BROKER
    if (brokers.empty()) {
        brokers = getBrokersFromEnvName("KARABO_BROKER", protocol);
    }
    return brokers;
}
