/*
 * File:   HttpResponse.hh
 * Author: <serguei.essenov@xfel.eu>
 *
 * Created on November, 15, 2019, 15:07.
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <cassert>
#include <cstring>

#include "HttpResponse.hh"

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
            connection = "";
            transferEncoding = "";

            posBegin = posEnd + 2;
            posEnd = line.find("\r\n", posBegin);
 
            while (posEnd > posBegin) {
                size_t posSeparator = line.find(": ", posBegin);
                const std::string& key = line.substr(posBegin, posSeparator - posBegin);
                size_t posValue = posSeparator + 2;
                assert(posBegin < posEnd && posBegin < posValue && posValue < posEnd);
                std::string val = line.substr(posValue, posEnd - posValue);

                if (key == "Content-Type") {
                    contentType = val;
                } else if (key == "Request-Id") {
                    requestId = val;
                } else if (key == "X-Request-Id") {
                    xRequestId = val;
                } else if (key == "Date") {
                    date = val;
                } else if (key == "X-Influxdb-Build") {
                    build = val;
                } else if (key == "X-Influxdb-Version") {
                    version = val;
                } else if (key == "Connection") {
                    connection = val;
                } else if (key == "Transfer-Encoding") {
                    transferEncoding = val;
                }
                posBegin = posEnd + 2;
                posEnd = line.find("\r\n", posBegin);
            }
        }


        void HttpResponse::parseHttpChunks(const std::string& chunks) {
            payload = "";
            size_t lengthPosition = 0;
            int dataLength = 0;

            do {
                dataLength = std::stoi(chunks.c_str() + lengthPosition, 0, 16);
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
                << "X-Request-Id: " << xRequestId << '\n'
                << "Date: " << date << '\n'
                << "Connection: " << connection << '\n'
                << "Transfer-Encoding: " << transferEncoding << '\n'
                << "Payload arrived: " << payloadArrived << '\n'
                << "Payload: " << payload;
            return oss.str();
        }


        std::ostream& operator<<(std::ostream& os, const HttpResponse& o) {
            os << o.toString();
            return os;
        }

    } // namespace karabo

} // namespace net



