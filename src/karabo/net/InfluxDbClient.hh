/* 
 * File:   InfluxDbClient.hh
 * Author: <serguei.essenov@xfel.eu>, <raul.costa@xfel.eu>
 *
 * Created on November 14, 2019, 9:57 AM
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef INFLUXDBCLIENT_HH
#define	INFLUXDBCLIENT_HH

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/uuid/random_generator.hpp>

#include "karabo/net/Connection.hh"

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
            std::string date;
            std::string connection;
            std::string transferEncoding;

            std::string payload;
            bool payloadArrived;

            std::vector<std::string> fields{"Content-Type: ", "Request-Id: ",
                                            "X-Influxdb-Build: ", "X-Influxdb-Version: ",
                                            "X-Request-Id: ", "Date: ",
                                            "Transfer-Encoding: "};

            HttpResponse()
                : code(-1)
                , message("")
                , contentType("")
                , requestId("")
                , xRequestId("")
                , build("")
                , version("")
                , date("")
                , connection("")
                , transferEncoding("")
                , payload("")
                , payloadArrived(true) {
            }

            void clear() {
                *this = HttpResponse();
            }

            bool parseHttpHeader(const std::string& line);

            void parseHttpChunks(const std::string& line);

            std::string toString() const;

            friend std::ostream& operator<<(std::ostream& os, const HttpResponse& o);
        };


        using Action = boost::function<void(const HttpResponse&) >;


        class InfluxDbClient {

        public:
            
            InfluxDbClient();
            virtual ~InfluxDbClient();

            void checkDb();
            void reconnect();
            void enqueueQuery(const std::stringstream& ss);
            void onDbConnect(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);
            void onDbRead(const karabo::net::ErrorCode& ec, const std::string& data);
            void onDbWrite(const karabo::net::ErrorCode& ec, boost::shared_ptr<std::vector<char> > p);
            void writeDb(const std::string& message);
            void pingInfluxDb(const Action& action);
            void showDatabases(const Action& action);
            void createDatabase(const std::string& dbname, const Action& action);
            void postToDB(const Action& action);
            void schemaToDB(const std::string& entries, const Action action);
            void digestSearch(const std::string& digest, const Action action);
            void onPing(const HttpResponse& o);
            void onShowDatabases(const HttpResponse& o);
            void onCreateDatabase(const HttpResponse& o);
            void onSchemaToDb(const HttpResponse& o);

        private:

            std::string m_url;
            karabo::net::Connection::Pointer m_dbConnection;
            karabo::net::Channel::Pointer m_dbChannel;
            boost::mutex m_bufferMutex;
            std::stringstream m_buffer;
            std::size_t m_bufferLen;
            std::string m_topic;
            bool m_topicIsDatabase;
            std::unordered_map<std::string, Action> m_registeredActions;
            HttpResponse m_response;
            std::string m_hostname;
            static boost::uuids::random_generator m_uuidGenerator;

        };

    } // namespace net
    
} // namespace karabo



#endif	/* INFLUXDBCLIENT_HH */

