/* 
 * File:   InfluxDbClient.hh
 * Author: <serguei.essenov@xfel.eu>, <raul.costa@xfel.eu>
 *
 * Created on November 14, 2019, 9:57 AM
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_NET_INFLUXDBCLIENT_HH
#define	KARABO_NET_INFLUXDBCLIENT_HH

#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <future>

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/uuid/random_generator.hpp>

#include "karabo/net/Connection.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/HttpResponse.hh"
#include "karabo/util/ClassInfo.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/TimeDuration.hh"

namespace karabo {
    namespace net {


        using InfluxResponseHandler = boost::function<void(const HttpResponse &) >;
        typedef boost::function<void()> AsyncHandler;


        /**
         * This class uses HTTP protocol for communications with InfluxDB server.
         * The protocol follows request/response pattern and before sending next
         * request the response from the current one should be received.  Only one
         * request/response session per connection is allowed.  To follow this
         * rule the circular buffer is used.  Any request (functor) first is
         * pushed back into circular buffer and then checked the internal state if
         * some current request/response session is ongoing.  If not, the next
         * request is popped from front side of circular buffer and executed.
         * The internal flag (state) is raised.  When response callback is called
         * it checks if circular buffer has next entry and if so this entry is
         * popped* and executed.  If not, the internal flag is lowered.
         * The circular buffer has some fixed capacity and requests will be dropped
         * if the client cannot cope with the input rate.
         */    
        class InfluxDbClient : public boost::enable_shared_from_this<InfluxDbClient> {
        public:

            KARABO_CLASSINFO(InfluxDbClient, "InfluxDbClient", "2.6")

            InfluxDbClient(const karabo::util::Hash& input);

            virtual ~InfluxDbClient();

            /**
             * Check if connection is lost and try to re-establish connection to InfluxDB server
             * @param  hook function that will be called when connection is established
             */
            void connectDbIfDisconnected(const AsyncHandler& hook = AsyncHandler());

            /**
             * Returns true if connection is established to InfluxDB server
             */
            bool isConnected() {
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                return m_dbChannel && m_dbChannel->isOpen();
            }

            /**
             * HTTP request "GET /query ..." to InfluxDB server is registered in circular buffer.
             * Non-blocking.  Expect connection to InfluxDB.
             * @param digest is SELECT expression
             * @param action callback: void(const HttpResponse&) is called when response comes
             *               from InfluxDB server
             */ 
            void getQueryDb(const std::string& digest, const InfluxResponseHandler& action);

            /**
             * HTTP request "GET /query ..." to InfluxDB server is registered in circular buffer.
             * Can be called with connection to InfluxDB or without. Blocking if no connection exists.
             * Otherwise non-blocking.
             * @param statement is SELECT expression.
             * @param action callback: void(const HttpResponse&) is called when response comes
             *               from InfluxDB server
             */
            void queryDb(const std::string& statement, const InfluxResponseHandler& action);

            /**
             * HTTP request "POST /query ..." to InfluxDB server is registered in circular buffer.
             * @param statement SELECT, SHOW, DROP and others QL commands
             * @param action callback: void(const HttpResponse&) is called when response comes
             *               from InfluxDB server
             */
            void postQueryDb(const std::string& statement, const InfluxResponseHandler& action);

            /**
             * HTTP request "GET /ping ..." to InfluxDB server is registered in circular buffer.
             * @param action callback: void(const HttpResponse&) is called when response comes
             *               from InfluxDB server
             */
            void getPingDb(const InfluxResponseHandler& action);

            /**
             * HTTP request "POST /write ..." to InfluxDB server is registered in circular buffer.
             * @param batch is a bunch of lines following InfluxDB "line protocol" separated by newline
             * @param action callback is called when acknowledgment (response) comes from InfluxDB
             *               server.  The callback signature is void(const HttpResponse&).  The success
             *               error code in HttpResponse structure is 204.
             */
            void postWriteDb(const std::string& batch, const InfluxResponseHandler& action);

            /**
             * Returns UUID used as Request-ID for HTTP requests
             */
            static std::string generateUUID();

            /**
             * Returns true if connection is established in "millis" time range, otherwise timeout
             * condition comes up and returns false.
             * @param millis time in milliseconds to wait for connection to be established
             * @return true if connection establshed, or false in case of timeout
             */
            bool connectWait(std::size_t millis);

        private:

            /**
             * Writing HTTP request
             * @param message formed in compliance of HTTP protocol
             *        Malformed requests resulting in response code 4xx
             */
            void writeDb(const std::string& message);

            /**
             * Low-level callback called when connection to InfluxDB is established
             */
            void onDbConnect(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel, const AsyncHandler& hook);

            /**
             * Low-level callback called when reading is done
             */
            void onDbRead(const karabo::net::ErrorCode& ec, const std::string& data);

            /**
             * Low-level callback called when writing into networks interface is done
             */
            void onDbWrite(const karabo::net::ErrorCode& ec, boost::shared_ptr<std::vector<char> > p);

            /**
             * Actual "GET /query ..." is accomplished. Non-blocking call. The connection to InfluxDB
             * has to be established before this call
             */
            void getQueryDbTask(const std::string& digest, const InfluxResponseHandler& action);

            /**
             * Actual "POST /query ..." is accomplished. Non-blocking call. The connection to InfluxDB
             * has to be established before this call
             */
            void postQueryDbTask(const std::string& statement, const InfluxResponseHandler& action);

            /**
             * Actual "GET /ping ..." is accomplished. Non-blocking call. The connection to InfluxDB
             * has to be established before this call
             */
            void getPingDbTask(const InfluxResponseHandler& action);

            /**
             * Actual "POST /write ..." is accomplished. If no connection to DB, this call is blocked
             * until the connection is established.  Otherwise the call is non-blocking.
             */
            void postWriteDbTask(const std::string& batch, const InfluxResponseHandler& action);

            /**
             * Actual "GET /query ..." is accomplished. If no connection to DB, this call is blocked
             * until the connection is established.  Otherwise the call is non-blocking.
             */
            void queryDbTask(const std::string& statement, const InfluxResponseHandler& action);

            /**
             * Generic wrap callback is called and call in turn the user "action".
             */
            void onResponse(const HttpResponse& o, const InfluxResponseHandler& action);

            /**
             * Try to take the next request from circular buffer and execute it.
             * Set internal state to be "active" if it was not.  Helper function.
             */
            void tryNextRequest();

            /**
             * Send HTTP request to InfluxDb.  Helper function.
             */
            void sendToInfluxDb(const std::string& msg, const InfluxResponseHandler& action);

       private:

            std::string m_url;
            karabo::net::Connection::Pointer m_dbConnection;
            karabo::net::Channel::Pointer m_dbChannel;
            boost::circular_buffer<boost::function<void()> > m_circular;
            boost::mutex m_circularMutex;
            bool m_active;
            boost::mutex m_connectionRequestedMutex;
            bool m_connectionRequested;
            // maps Request-Id to pair of HTTP request string and action callback
            boost::mutex m_responseHandlersMutex;
            std::unordered_map<std::string, std::pair<std::string, InfluxResponseHandler> > m_registeredInfluxResponseHandlers;
            HttpResponse m_response;
            std::string m_hostname;
            std::string m_dbname;
            std::string m_durationUnit;
            static boost::mutex m_uuidGeneratorMutex;
            static boost::uuids::random_generator m_uuidGenerator;
            static const unsigned int k_connTimeoutMs;
            static const unsigned int k_circularBufferCapacity;
            std::string m_currentUuid;
        };

    } // namespace net
    
} // namespace karabo



#endif	/* KARABO_NET_INFLUXDBCLIENT_HH */

