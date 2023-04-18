/*
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */
#include <karabo/util/Hash.hh>
#include <string>
#include <vector>

// please note that these network address might change in the future
// or might be not reacheable to outside the European XFEL network
// use the environment variables
// KARABO_CI_BROKERS
// e.g. export KARABO_CI_BROKERS=tcp://a-jms-broker:7777;amqp://an-amqp-broker:5672

#define MQTT_BROKER_DEFAULT "mqtt://exfldl02n0:1883"
#define JMS_BROKER_DEFAULT "tcp://exfl-broker:7777"
#define AMQP_BROKER_DEFAULT "amqp://xfel:karabo@exflctrl01:5672"
#define REDIS_BROKER_DEFAULT "redis://exflctrl01:6379"
#define INVALID_MQTT "mqtt://invalid.example.org:1883"
#define INVALID_JMS "tcp://invalid.example.org:7777"
#define INVALID_AMQP "amqp://invalid.example.org:5672"
#define INVALID_REDIS "redis://invalid.example.org:6379"


std::vector<std::string> getBrokersFromEnvVar(const std::string& envVar, const std::string& expectedProtocol);
std::vector<std::string> getBrokersFromEnv(const std::string& protocol);
karabo::util::Hash getBrokersFromEnv();
std::vector<std::string> getJmsBrokerFromEnv();
std::vector<std::string> getMqttBrokerFromEnv();
