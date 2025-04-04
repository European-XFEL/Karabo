/*
 * File:   InfluxDbClientUtils.cc
 *
 * Created on September 13, 2023, 18:40 PM
 *
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


#include "InfluxDbClientUtils.hh"

#include <karabo/net/Broker.hh>
#include <string>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/Hash.hh"

using karabo::data::Configurator;
using karabo::data::Hash;

namespace karabo {
    namespace net {

        InfluxDbClient::Pointer buildInfluxReadClient() {
            std::string influxUrlRead;
            if (getenv("KARABO_INFLUXDB_QUERY_URL")) {
                influxUrlRead = getenv("KARABO_INFLUXDB_QUERY_URL");
            } else {
                influxUrlRead = "tcp://localhost:8086";
            }

            const char* envDbName = getenv("KARABO_INFLUXDB_DBNAME");
            // If a DbName is not explicitly specified, uses the Karabo Broker Topic
            std::string dbName(envDbName ? envDbName : Broker::brokerDomainFromEnv());

            std::string dbUser;
            if (getenv("KARABO_INFLUXDB_QUERY_USER")) {
                dbUser = getenv("KARABO_INFLUXDB_QUERY_USER");
            } else {
                dbUser = "infadm";
            }
            std::string dbPassword;
            if (getenv("KARABO_INFLUXDB_QUERY_PASSWORD")) {
                dbPassword = getenv("KARABO_INFLUXDB_QUERY_PASSWORD");
            } else {
                dbPassword = "admpasswd";
            }

            karabo::data::Hash dbClientCfg;
            dbClientCfg.set("dbname", dbName);
            dbClientCfg.set("url", influxUrlRead);
            dbClientCfg.set("durationUnit", "u");
            dbClientCfg.set("dbUser", dbUser);
            dbClientCfg.set("dbPassword", dbPassword);

            return Configurator<InfluxDbClient>::create("InfluxDbClient", dbClientCfg);
        };


    } // namespace net
} // namespace karabo
