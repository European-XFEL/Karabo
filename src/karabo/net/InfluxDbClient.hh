/*
 * File:   InfluxDbClient.hh
 *
 * Created on November 14, 2019, 9:57 AM
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

#ifndef KARABO_NET_INFLUXDBCLIENT_HH
#define KARABO_NET_INFLUXDBCLIENT_HH

#include <atomic>
#include <boost/uuid/random_generator.hpp>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>

#include "karabo/net/Channel.hh"
#include "karabo/net/Connection.hh"
#include "karabo/net/HttpResponse.hh"
#include "karabo/util/ClassInfo.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/TimeDuration.hh"

namespace karabo {
    namespace util {
        class Schema;
    }
    namespace net {


        using InfluxResponseHandler = std::function<void(const HttpResponse&)>;
        using InfluxConnectedHandler = std::function<void(bool)>;


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
        class InfluxDbClient : public std::enable_shared_from_this<InfluxDbClient> {
           public:
            KARABO_CLASSINFO(InfluxDbClient, "InfluxDbClient", "2.6")

            InfluxDbClient(const karabo::util::Hash& input);

            virtual ~InfluxDbClient();

            static void expectedParameters(karabo::util::Schema& expected);

            /**
             * Check if connection is lost and try to re-establish connection to InfluxDB server
             * @param  hook function that will be called when connection is established
             */
            void startDbConnectIfDisconnected(const InfluxConnectedHandler& hook = InfluxConnectedHandler());

            /**
             * Returns true if connection is established to InfluxDB server
             */
            bool isConnected() {
                std::lock_guard<std::mutex> lock(m_connectionRequestedMutex);
                return m_dbChannel && m_dbChannel->isOpen();
            }

            /**
             * @brief The version of the InfluxDb server the client is connected to.
             *
             * @return std::string the connected InfluxDb server version (empty if no server is currently connected).
             */
            std::string influxVersion();

            /**
             * @brief The url of the InfluxDb server the client is connected to (or supposed to connect to).
             *
             * @return std::string the InfluxDb server url.
             */
            std::string serverUrl() noexcept {
                return m_url;
            }

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
            void flushBatch(const InfluxResponseHandler& respHandler = InfluxResponseHandler());

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
             * @return true if connection established, or false in case of timeout
             */
            bool connectWait(std::size_t millis);

            void disconnect() noexcept;

           private:
            /**
             * Writing HTTP request
             * @param message formed in compliance of HTTP protocol
             *        Malformed requests resulting in response code 4xx
             * @param requestId unique id of the HTTP request to be sent to Influx.
             */
            void writeDb(const std::string& message, const std::string& requestId);

            /**
             * Low-level callback called when connection to InfluxDB is established
             */
            void onDbConnect(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel,
                             const InfluxConnectedHandler& hook);

            /**
             * Low-level callback called when reading is done
             */
            void onDbRead(const karabo::net::ErrorCode& ec, const std::string& data);

            /**
             * Low-level callback called when writing into networks interface is done
             */
            void onDbWrite(const karabo::net::ErrorCode& ec, std::shared_ptr<std::vector<char> > p);

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
             *
             * @param requestQueueLock must be locked scoped_lock of m_requestQueueMutex, will be unlocked
             * afterwards
             */
            void tryNextRequest(std::unique_lock<std::mutex>& requestQueueLock);

            /**
             * Send HTTP request to InfluxDb.  Helper function.
             *
             * Wraps the given InfluxResponseHandler within a callback to
             * onResponse. It will be up to onResponse to call the action
             * InfluxResponseHandler and keep the consumption of requests
             * submitted to the InfluxDbClient going.
             */
            void sendToInfluxDb(const std::string& msg, const InfluxResponseHandler& action,
                                const std::string& requestId);

            void flushBatchImpl(const InfluxResponseHandler& respHandler = InfluxResponseHandler());

            /**
             * Gets the raw form of the http Authorization header with values of dbUser and dbPassword
             * separated by a colon and base64 encoded.
             *
             * @return The raw form of the Authorization header.
             */
            std::string getRawBasicAuthHeader();

            /**
             * Handle unrecoverable read and parsing errors while processing HTTP
             * responses from Influx.
             *
             * The recovery involves recycling the network connection, as there
             * is no way to recover synchronism in the read operation within the
             * current connection after those kind of errors happen.
             * Also generates an HTTP response with status code 700 and an error
             * message to communicate to users of the InfluxDbClient instance.
             *
             * @param errMsg the error message to be put in the generated 700
             *               coded http response.
             *
             * @param requestId the unique identifier of the HTTP request whose
             *                  response could not be processed (needed to update
             *                  the internal bookeeping of the InfluxDb client).
             *
             * @param logEsError if true (default), log as info, else as error
             */
            void handleHttpReadError(const std::string& errMsg, const std::string& requestId, bool logAsError = true);

           private:
            std::string m_url;
            karabo::net::Connection::Pointer m_dbConnection;
            karabo::net::Channel::Pointer m_dbChannel;
            std::queue<std::function<void()> > m_requestQueue;
            std::mutex m_requestQueueMutex;
            std::atomic<bool> m_active;
            std::mutex m_connectionRequestedMutex;
            std::atomic<bool> m_connectionRequested;
            // maps Request-Id to pair of HTTP request string and action callback
            std::mutex m_responseHandlersMutex;
            std::unordered_map<std::string, std::pair<std::string, InfluxResponseHandler> >
                  m_registeredInfluxResponseHandlers;

            // Unique id of the individual HTTP request that can be "flying" between the DbClient and Influx at
            // a given moment for the TCP channel. If the TCP connection between the DbClient and Influx gets
            // compromissed before the HTTP response is received, this temporarily stored Id is used to clean
            // up the map that stores the association between requests and response handlers.
            std::string m_flyingId;

            HttpResponse m_response;
            std::string m_hostname;
            std::string m_dbname;
            std::string m_durationUnit;
            std::mutex m_influxVersionMutex;
            std::string m_influxVersion;
            static std::mutex m_uuidGeneratorMutex;
            static boost::uuids::random_generator m_uuidGenerator;
            static const unsigned int k_connTimeoutMs;
            const std::uint32_t m_maxPointsInBuffer;
            std::mutex m_bufferMutex;
            std::stringstream m_buffer;
            std::uint32_t m_nPoints;
            std::string m_dbUser;
            std::string m_dbPassword;
            bool m_disconnectOnIdle;
        };

    } // namespace net

} // namespace karabo


#endif /* KARABO_NET_INFLUXDBCLIENT_HH */
