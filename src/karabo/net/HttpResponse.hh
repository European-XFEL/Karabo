/*
 * File:   HttpResponse.hh
 * Author: <serguei.essenov@xfel.eu>
 *
 * Created on November, 15, 2019, 15:07.
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

#ifndef HTTPRESPONSE_HH
#define HTTPRESPONSE_HH

#include <sstream>
#include <string>
#include <vector>

namespace karabo {

    namespace net {

        struct HttpResponse {
            int code;
            std::string message;
            std::string contentType;
            std::string requestId;
            std::string xRequestId;
            std::string build;
            std::string version;
            std::string xError;
            std::string date;
            std::string connection;
            std::string transferEncoding;
            int contentLength;

            std::string payload;
            bool payloadArrived;

            std::vector<std::string> fields{
                  "Content-Type: ", "Request-Id: ", "X-Influxdb-Build: ", "X-Influxdb-Version: ",
                  "X-Request-Id: ", "Date: ",       "Transfer-Encoding: "};

            HttpResponse()
                : code(-1),
                  message(""),
                  contentType(""),
                  requestId(""),
                  xRequestId(""),
                  build(""),
                  version(""),
                  xError(""),
                  date(""),
                  connection(""),
                  transferEncoding(""),
                  contentLength(-1),
                  payload(""),
                  payloadArrived(true) {}

            void clear() {
                *this = HttpResponse();
            }

            void parseHttpHeader(const std::string& line);

            void parseHttpChunks(const std::string& line);

            std::string toString() const;

            friend std::ostream& operator<<(std::ostream& os, const HttpResponse& o);
        };

    } // namespace net

} // namespace karabo

#endif /* HTTPRESPONSE_HH */
