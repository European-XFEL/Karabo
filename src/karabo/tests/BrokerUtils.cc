/*
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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


karabo::util::Hash getBrokersFromEnv() {
    karabo::util::Hash ret;
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
            if (protocol == "tcp") {
                ret.set("jms", vBrokerUrl);
            } else if (protocol == "redis" || protocol == "mqtt" || protocol == "amqp") {
                ret.set(protocol, vBrokerUrl);
            }
        }
    }
    return ret;
}


std::vector<std::string> getJmsBrokerFromEnv() {
    std::vector<std::string> brokers = getBrokersFromEnvName("KARABO_CI_BROKERS", "tcp");
    // try KARABO_BROKER
    if (brokers.empty()) {
        brokers = getBrokersFromEnvName("KARABO_BROKER", "tcp");
    }
    return brokers;
}


std::vector<std::string> getMqttBrokerFromEnv() {
    std::vector<std::string> brokers = getBrokersFromEnvName("KARABO_CI_BROKERS", "mqtt");
    // try KARABO_BROKER
    if (brokers.empty()) {
        brokers = getBrokersFromEnvName("KARABO_BROKER", "mqtt");
    }
    // try KARABO_BROKER_MQTT
    if (brokers.empty()) {
        brokers = getBrokersFromEnvName("KARABO_BROKER_MQTT", "mqtt");
    }
    return brokers;
}
