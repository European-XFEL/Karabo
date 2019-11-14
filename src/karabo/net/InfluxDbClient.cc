/*
 * File:   InfluxDbClient.cc
 * Author: <serguei.essenov@xfel.eu>, <raul.costa@xfel.eu>
 *
 * Created on November 14, 2019, 9:57 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "InfluxDbClient.hh"

#include "karabo/util/MetaTools.hh"

namespace karabo {

    namespace net {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::xms;


        static std::string _urlencode(const std::string& value) {
            std::ostringstream escaped;
            escaped.fill('0');
            escaped << std::hex;

            for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
                std::string::value_type c = (*i);

                // Keep alphanumeric and other accepted characters intact
                if (c == ' ') {
                    escaped << '+';
                    continue;
                } else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                    escaped << c;
                    continue;
                }

                // Any other characters are percent-encoded
                escaped << std::uppercase;
                escaped << '%' << std::setw(2) << int((unsigned char) c);
                escaped << std::nouppercase;
            }

            return escaped.str();
        }


        std::string HttpResponse::toString() const {
            std::ostringstream oss;
            oss << "code: " << code << '\n'
                    << "message: " << message << '\n'
                    << "contentType: " << contentType << '\n'
                    << "requestId: " << requestId << '\n'
                    << "build: " << build << '\n'
                    << "version: " << version << '\n'
                    << "xRequestId: " << xRequestId << '\n'
                    << "date: " << date << '\n'
                    << "connection: " << connection << '\n'
                    << "transferEncoding: " << transferEncoding << '\n'
                    << "payloadArrived: " << payloadArrived << '\n'
                    << "payload: " << payload;
            return oss.str();
        }


        std::ostream& operator<<(std::ostream& os, const HttpResponse& o) {
            os << "HTTP/1.1 " << o.code << " " << o.message << '\n'
                    << "Content-Type: " << o.contentType << '\n'
                    << "Request-Id: " << o.requestId << '\n'
                    << "X-Influxdb-Build: " << o.build << '\n'
                    << "X-Influxdb-Version: " << o.version << '\n'
                    << "X-Request-Id: " << o.xRequestId << '\n'
                    << "Date: " << o.date << '\n'
                    << "Connection: " << o.connection << '\n'
                    << "Transfer-Encoding: " << o.transferEncoding << '\n'
                    << "Payload arrived: " << o.payloadArrived << '\n'
                    << "Payload: " << o.payload << '\n';
            return os;
        }


        void InfluxDbClient::writeDb(const std::string& message) {
            auto datap = boost::make_shared<std::vector<char> >(std::vector<char>());
            datap->assign(message.begin(), message.end());
            m_dbChannel->writeAsyncVectorPointer(datap, bind_weak(&InfluxDbClient::onDbWrite, this, _1, datap));
        }


        void InfluxDbClient::showDatabases(const Action& action) {
            std::string uuid(boost::uuids::to_string(m_uuidGenerator()));
            m_registeredActions.emplace(std::make_pair(uuid, action));
            std::ostringstream oss;
            oss << "POST /query?chunked=true&db=&epoch=" << DUR << "&q=show+databases HTTP/1.1\r\n"
                    << "Host: " << m_hostname << "\r\n"
                    << "Request-Id: " << uuid << "\r\n\r\n";
            const std::string message(oss.str());
            writeDb(message);
        }











    } // namespace net

} // namespace karabo

