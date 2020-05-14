/* 
 * File:   InfluxDbClient.hh
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
#include <queue>

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
    namespace util {
        class Schema;
    }
    namespace net {


        using InfluxResponseHandler = boost::function<void(const HttpResponse &) >;
        typedef boost::function<void()> AsyncHandler;


        /**
         * This class uses HTTP protocol for communications with InfluxDB server.
         * The protocol follows request/response pattern and before sending next
         * request the response from the current one should be received.  Only one
         * request/response session per connection is allowed.  To follow this
         * rule the internal queue is used.  Any request (functor) first is
         * pushed into internal queue and then checked the internal state if
         * some current request/response session is ongoing.  If not, the next
         * request is popped from front side of internal queue and executed.
         * The internal flag (state) is raised.  When response callback is called
         * it checks if the internal queue has next entry and if so this entry is
         * popped and executed.  If not, the internal flag is lowered.
         * For the time being the internal queue has no limits defined so it is
         * possible that if the client cannot cope with input load rate some overflow
         * condition can be encountered.  The practice should show how we can handle
         * these problems.
         */    
        class InfluxDbClient : public boost::enable_shared_from_this<InfluxDbClient> {
        public:

            KARABO_CLASSINFO(InfluxDbClient, "InfluxDbClient", "2.6")

            InfluxDbClient(const karabo::util::Hash& input);

            virtual ~InfluxDbClient();

            static void expectedParameters(karabo::util::Schema& expected);

            /**
             * Check if connection is lost and try to re-establish connection to InfluxDB server
             * @param  hook function that will be called when connection is established
             */
            void connectDbIfDisconnectedWrite(const AsyncHandler& hook = AsyncHandler());

            void connectDbIfDisconnectedQuery(const AsyncHandler& hook = AsyncHandler());

            /**
             * Returns true if connection is established to InfluxDB server
             */
            bool isConnectedWrite() {
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                return m_dbChannelWrite && m_dbChannelWrite->isOpen();
            }

            /**
             * Returns true if connection is established to InfluxDB server
             */
            bool isConnectedQuery() {
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                return m_dbChannelQuery && m_dbChannelQuery->isOpen();
            }

            /**
             * HTTP request "GET /query ..." to InfluxDB server is registered in internal queue.
             * Non-blocking.  Expect connection to InfluxDB.
             * @param digest is SELECT expression
             * @param action callback: void(const HttpResponse&) is called when response comes
             *               from InfluxDB server
             */ 
            void getQueryDb(const std::string& digest, const InfluxResponseHandler& action);

            /**
             * HTTP request "GET /query ..." to InfluxDB server is registered in internal queue.
             * Can be called with connection to InfluxDB or without. Blocking if no connection exists.
             * Otherwise non-blocking.
             * @param statement is SELECT expression.
             * @param action callback: void(const HttpResponse&) is called when response comes
             *               from InfluxDB server
             */
            void queryDb(const std::string& statement, const InfluxResponseHandler& action);

            /**
             * HTTP request "POST /query ..." to InfluxDB server is registered in internal queue.
             * @param statement SELECT, SHOW, DROP and others QL commands
             * @param action callback: void(const HttpResponse&) is called when response comes
             *               from InfluxDB server
             */
            void postQueryDb(const std::string& statement, const InfluxResponseHandler& action);

            void enqueueQuery(const std::string& line);

            /**
             * Flushes the contents of the write buffer to the InfluxDb.
             *
             * @param respHandler If defined, the handler function will be called with the response
             * sent by Influx after it accepted and processed the current batch of updates. If not
             * defined, the flushBatch will work in a call-and-forget mode.
             */
            void flushBatch(const InfluxResponseHandler &respHandler = InfluxResponseHandler());

            /**
             * HTTP request "GET /ping ..." to InfluxDB server is registered in internal queue.
             * @param action callback: void(const HttpResponse&) is called when response comes
             *               from InfluxDB server
             */
            void getPingDb(const InfluxResponseHandler& action);

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
            bool connectWaitWrite(std::size_t millis);

            bool connectWaitQuery(std::size_t millis);

            void disconnectWrite();

            void disconnectQuery();

        private:

            /**
             * Writing HTTP request
             * @param message formed in compliance of HTTP protocol
             *        Malformed requests resulting in response code 4xx
             */
            void writeDb(const karabo::net::Channel::Pointer& channel, const std::string& message);

            /**
             * Low-level callback called when connection to InfluxDB is established
             */
            void onDbConnectWrite(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel, const AsyncHandler& hook);

            void onDbConnectQuery(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel, const AsyncHandler& hook);

            /**
             * Low-level callback called when reading is done
             */
            void onDbReadWrite(const karabo::net::ErrorCode& ec, const std::string& data);

            void onDbReadQuery(const karabo::net::ErrorCode& ec, const std::string& data);

            /**
             * Low-level callback called when writing into networks interface is done
             */
            void onDbWriteWrite(const karabo::net::ErrorCode& ec, boost::shared_ptr<std::vector<char> > p);

            void onDbWriteQuery(const karabo::net::ErrorCode& ec, boost::shared_ptr<std::vector<char> > p);

            /**
             * Actual "GET /query ..." is accomplished. Non-blocking call. The connection to InfluxDB
             * has to be established before this call
             */
            void getQueryDbTask(const std::string& digest, const InfluxResponseHandler& action);

            /**
             * HTTP request "POST /write ..." to InfluxDB server is registered in internal queue.
             * @param batch is a bunch of lines following InfluxDB "line protocol" separated by newline ('\n')
             *        the "line protocol" is detailed at
             *        https://influxdbcom.readthedocs.io/en/latest/content/docs/v0.9/write_protocols/write_syntax/
             * @param action callback is called when acknowledgment (response) comes from InfluxDB
             *               server.  The callback signature is void(const HttpResponse&).  The success
             *               error code in HttpResponse structure is 204.
             */
            void postWriteDb(const std::string& batch, const InfluxResponseHandler& action);

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
             * Actual "POST /write ..." is accomplished. Non-blocking call,  Connection should be
             * established before this call
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
             * Try to take the next request from internal queue and execute it.
             * Set internal state to be "active" if it was not.  Helper function.
             */
            void tryNextRequest();

            /**
             * Send HTTP request to InfluxDb.  Helper function.
             */
            void sendToInfluxDb(const karabo::net::Channel::Pointer& channel, const std::string& msg, const InfluxResponseHandler& action);

            void flushBatchImpl(const InfluxResponseHandler &respHandler = InfluxResponseHandler());

            /**
             * Gets the raw form of the http Authorization header with values of dbUser and dbPassword
             * separated by a colon and base64 encoded.
             *
             * @return The raw form of the Authorization header.
             */
            std::string getRawBasicAuthHeader();

       private:

            std::string m_urlWrite;
            std::string m_urlQuery;
            karabo::net::Connection::Pointer m_dbConnectionWrite;
            karabo::net::Channel::Pointer m_dbChannelWrite;
            karabo::net::Connection::Pointer m_dbConnectionQuery;
            karabo::net::Channel::Pointer m_dbChannelQuery;
            std::queue<boost::function<void()> > m_requestQueue;
            boost::mutex m_requestQueueMutex;
            bool m_active;
            boost::mutex m_connectionRequestedMutex;
            bool m_connectionRequestedWrite;
            bool m_connectionRequestedQuery;
            // maps Request-Id to pair of HTTP request string and action callback
            boost::mutex m_responseHandlersMutex;
            std::unordered_map<std::string, std::pair<std::string, InfluxResponseHandler> > m_registeredInfluxResponseHandlers;
            HttpResponse m_response;
            std::string m_hostnameWrite;
            std::string m_hostnameQuery;
            std::string m_dbname;
            std::string m_durationUnit;
            static boost::mutex m_uuidGeneratorMutex;
            static boost::uuids::random_generator m_uuidGenerator;
            static const unsigned int k_connTimeoutMs;
            std::string m_currentUuid;
            const std::uint32_t m_maxPointsInBuffer;
            boost::mutex m_bufferMutex;
            std::stringstream m_buffer;
            std::uint32_t m_nPoints;
            std::string m_dbUserWrite;
            std::string m_dbPasswordWrite;
            std::string m_dbUserQuery;
            std::string m_dbPasswordQuery;
        };

    } // namespace net
    
} // namespace karabo



#endif	/* KARABO_NET_INFLUXDBCLIENT_HH */

