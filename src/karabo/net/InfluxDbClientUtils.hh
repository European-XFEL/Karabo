/*
 * File:   InfluxDbClientUtils.hh
 *
 * Created on September 13, 2023, 18:30 PM
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

#ifndef KARABO_NET_INFLUXDBCLIENTUTILS_HH
#define KARABO_NET_INFLUXDBCLIENTUTILS_HH

#include "InfluxDbClient.hh"

namespace karabo {
    namespace net {
        /**
         * @brief Instantiates an InfluxDbClient that connects to an InfluxDb
         * reading node.
         *
         * The connection parameters for the InfluxDb reading node are
         * obtained via the following environment variables:
         *
         * KARABO_INFLUXDB_QUERY_URL
         * KARABO_INFLUXDB_DBNAME (with fallback to the Broker domain)
         * KARABO_INFLUXDB_QUERY_USER
         * KARABO_INFLUXDB_QUERY_PASSWORD
         *
         * @returns A std::shared_ptr to the built InfluxDbClient instance.
         */
        InfluxDbClient::Pointer buildInfluxReadClient();

    } // namespace net
} // namespace karabo

#endif
