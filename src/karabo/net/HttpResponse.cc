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

#include "HttpResponse.hh"

#include <boost/algorithm/string.hpp>
#include <cassert>
#include <cstring>

namespace karabo {

    namespace net {


        void HttpResponse::parseHttpHeader(const std::string& line) {
            // Typical message ... every line ended by '\r\n' and the last line by '\r\n\r\n'
            // HTTP/1.1 204 No Content
            // Content-Type: application/json
            // Request-Id: 7e54e64b-022c-11ea-820b-901b0e4ddbe5
            // X-Influxdb-Build: OSS
            // X-Influxdb-Version: 1.7.8
            // X-Request-Id: 7e54e64b-022c-11ea-820b-901b0e4ddbe5
            // Date: Fri, 08 Nov 2019 13:34:35 GMT
            // ---------------------------------------------------
            size_t posBegin = line.find("HTTP/1.1");
            if (posBegin == std::string::npos) return;
            size_t posCode = posBegin + std::strlen("HTTP/1.1 ");
            size_t posMessage = posCode + 4;
            size_t posEnd = line.find("\r\n", posBegin);

            code = std::stoi(line.substr(posCode, 3));
            message = line.substr(posMessage, posEnd - posMessage);
            contentType = "";
            requestId = "";
            xRequestId = "";
            date = "";
            build = "";
            version = "";
            xError = "";
            connection = "";
            transferEncoding = "";
            contentLength = -1;

            posBegin = posEnd + 2;
            posEnd = line.find("\r\n", posBegin);

            while (posEnd > posBegin) {
                size_t posSeparator = line.find(": ", posBegin);
                std::string key = line.substr(posBegin, posSeparator - posBegin);
                boost::to_lower(key);
                size_t posValue = posSeparator + 2;
                assert(posBegin < posEnd && posBegin < posValue && posValue < posEnd);
                std::string val = line.substr(posValue, posEnd - posValue);

                if (key == "content-type") {
                    contentType = val;
                } else if (key == "request-id") {
                    requestId = val;
                } else if (key == "x-request-id") {
                    xRequestId = val;
                } else if (key == "date") {
                    date = val;
                } else if (key == "x-influxdb-build") {
                    build = val;
                } else if (key == "x-influxdb-version") {
                    version = val;
                } else if (key == "x-influxdb-error") {
                    xError = val;
                } else if (key == "connection") {
                    connection = val;
                } else if (key == "transfer-encoding") {
                    transferEncoding = val;
                } else if (key == "content-length") {
                    contentLength = std::atoi(val.c_str());
                }
                posBegin = posEnd + 2;
                posEnd = line.find("\r\n", posBegin);
            }
        }


        void HttpResponse::parseHttpChunks(const std::string& chunks) {
            payload = "";
            size_t lengthPosition = 0;
            unsigned long long dataLength = 0;

            do {
                dataLength = std::stoull(chunks.c_str() + lengthPosition, 0, 16);
                size_t dataPosition = chunks.find("\r\n", lengthPosition) + 2; // 2 - CRLF
                payload.append(chunks.c_str() + dataPosition, dataLength);
                lengthPosition = dataPosition + dataLength + 2;
            } while (dataLength > 0);
        }


        std::string HttpResponse::toString() const {
            std::ostringstream oss;
            oss << "HTTP/1.1 " << code << " " << message << '\n'
                << "Content-Type: " << contentType << '\n'
                << "Request-Id: " << requestId << '\n'
                << "X-Influxdb-Build: " << build << '\n'
                << "X-Influxdb-Version: " << version << '\n'
                << "X-Request-Id: " << xRequestId << '\n';

            if (!xError.empty()) {
                oss << "X-Influxdb-Error" << xError << '\n';
            }

            if (!connection.empty()) {
                oss << "Connection: " << connection << '\n';
            }

            oss << "Date: " << date << '\n'
                << "Transfer-Encoding: " << transferEncoding << '\n'
                << "Payload arrived: " << payloadArrived << '\n'
                << "Payload: " << payload << '\n';

            if (contentLength >= 0) {
                // The response contained a 'content-length' header - this is an optional header.
                oss << "Content-Length: " << contentLength << '\n';
            }

            return oss.str();
        }


        std::ostream& operator<<(std::ostream& os, const HttpResponse& o) {
            os << o.toString();
            return os;
        }

    } // namespace net

} // namespace karabo
