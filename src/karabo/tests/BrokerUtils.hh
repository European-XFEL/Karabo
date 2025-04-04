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
#include <string>
#include <vector>

#include "karabo/data/types/Hash.hh"

// please note that these network address might change in the future
// or might be not reacheable to outside the European XFEL network
// use the environment variables
// KARABO_CI_BROKERS
// e.g. export KARABO_CI_BROKERS=amqp://an-amqp-broker:5672

#define AMQP_BROKER_DEFAULT "amqp://xfel:karabo@exfl-broker-1:5672"
#define INVALID_AMQP "amqp://invalid.example.org:5672"


std::vector<std::string> getBrokersFromEnvVar(const std::string& envVar, const std::string& expectedProtocol);
std::vector<std::string> getBrokersFromEnv(const std::string& protocol);
karabo::data::Hash getBrokersFromEnv();
std::vector<std::string> getBrokerFromEnv(const std::string& protocol);
