/*
 * File:   InfluxDbClient.cc
 *
 * Created on November 14, 2019, 9:57 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "InfluxDbClient.hh"

#include <boost/chrono/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cmath>
#include <cstdlib>
#include <sstream>

#include "karabo/io/BinarySerializer.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/Base64.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/SimpleElement.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::InfluxDbClient);

using boost::placeholders::_1;
using boost::placeholders::_2;

namespace karabo {

    namespace net {

        using namespace karabo::io;
        using namespace karabo::net;
        using namespace karabo::util;

        boost::mutex InfluxDbClient::m_uuidGeneratorMutex;
        boost::uuids::random_generator InfluxDbClient::m_uuidGenerator;
        const unsigned int InfluxDbClient::k_connTimeoutMs = 3500u;

        void InfluxDbClient::expectedParameters(karabo::util::Schema& expected) {
            STRING_ELEMENT(expected)
                  .key("dbname")
                  .displayedName("Database name")
                  .description("The name of the database inside the InfluxDB installation")
                  .assignmentMandatory()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("url")
                  .displayedName("Influxdb URL")
                  .description("URL should be given in form: tcp://host:port")
                  .assignmentMandatory()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("dbUser")
                  .displayedName("DB username")
                  .description("The name of the database user for the InfluxDB session")
                  .assignmentOptional()
                  .defaultValue("")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("dbPassword")
                  .displayedName("DB password")
                  .description("The password of the database user for the InfluxDB session")
                  .assignmentOptional()
                  .defaultValue("")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("durationUnit")
                  .displayedName("Duration unit")
                  .description(
                        "Time unit used: 'd' => day, 'h' => hour, 'm' => minute, 's' => second, "
                        "'ms' => millisec., 'u' => microsec., 'ns' => nanosec.")
                  .assignmentOptional()
                  .defaultValue("u")
                  .options({"d", "h", "m", "s", "ms", "u", "ns"})
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("maxPointsInBuffer")
                  .displayedName("Max. points in buffer")
                  .description("Maximum number of enqueued points in buffer")
                  .assignmentOptional()
                  .defaultValue(200u)
                  .commit();
        }


        InfluxDbClient::InfluxDbClient(const karabo::util::Hash& input)
            : m_url(input.get<std::string>("url")),
              m_dbConnection(),
              m_dbChannel(),
              m_requestQueue(),
              m_requestQueueMutex(),
              m_active(false),
              m_connectionRequestedMutex(),
              m_connectionRequested(false),
              m_responseHandlersMutex(),
              m_registeredInfluxResponseHandlers(),
              m_dbname(input.get<std::string>("dbname")),
              m_durationUnit(input.get<std::string>("durationUnit")),
              m_maxPointsInBuffer(input.get<std::uint32_t>("maxPointsInBuffer")),
              m_bufferMutex(),
              m_buffer(),
              m_nPoints(0),
              m_dbUser(input.get<std::string>("dbUser")),
              m_dbPassword(input.get<std::string>("dbPassword")) {
            if (!m_url.empty()) {
                const boost::tuple<std::string, std::string, std::string, std::string, std::string> partsWrite =
                      karabo::net::parseUrl(m_url);
                m_hostname = partsWrite.get<1>();
            } else {
                m_hostname = "";
            }

            std::ostringstream oss;
            oss << "InfluxDbClient: URL -> \"" << m_url << "\", user : \"" << m_dbUser << "\", host : \"" << m_hostname
                << "\"\n";
            KARABO_LOG_FRAMEWORK_DEBUG << oss.str();
        }


        InfluxDbClient::~InfluxDbClient() {
            disconnect();
        }


        std::string InfluxDbClient::generateUUID() {
            // The generator is not thread safe, but we rely on real uniqueness!
            boost::mutex::scoped_lock lock(m_uuidGeneratorMutex);
            return boost::uuids::to_string(m_uuidGenerator());
        }


        std::string InfluxDbClient::getRawBasicAuthHeader() {
            std::string authHeader;
            if (!m_dbUser.empty() && !m_dbPassword.empty()) {
                std::string credential = m_dbUser + ":" + m_dbPassword;
                std::string credentials(credential);
                const unsigned char* pCredentials = reinterpret_cast<const unsigned char*>(credentials.c_str());
                std::string b64Credent = karabo::util::base64Encode(pCredentials, credentials.length());
                authHeader = std::string("Authorization: Basic ").append(b64Credent);
            }
            return authHeader;
        }


        void InfluxDbClient::connectDbIfDisconnected(const InfluxConnectedHandler& hook) {
            boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
            if (!m_dbChannel || !m_dbChannel->isOpen()) {
                if (m_connectionRequested) return;
                Hash config("url", m_url, "sizeofLength", 0, "type", "client");
                m_dbConnection = karabo::net::Connection::create("Tcp", config);
                m_dbConnection->startAsync(bind_weak(&InfluxDbClient::onDbConnect, this, _1, _2, hook));
                m_connectionRequested = true;
            }
        }


        void InfluxDbClient::disconnect() {
            m_dbChannel.reset();
            m_dbConnection.reset();
        }


        std::string InfluxDbClient::influxVersion() {
            boost::mutex::scoped_lock lock(m_influxVersionMutex);
            return m_influxVersion;
        }


        void InfluxDbClient::tryNextRequest(boost::mutex::scoped_lock& requestQueueLock) {
            if (!m_active) { // activate processing
                m_active = true;
                boost::function<void()> nextRequest;
                nextRequest.swap(m_requestQueue.front());
                m_requestQueue.pop();
                requestQueueLock.unlock();
                nextRequest();
            } else {
                requestQueueLock.unlock();
            }
        }


        void InfluxDbClient::onResponse(const HttpResponse& o, const InfluxResponseHandler& action) {
            try {
                if (action) action(o);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "onResponse: call InfluxResponseHandler resulting in exception : "
                                           << e.what();
            }

            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            if (m_requestQueue.empty()) {
                m_active = false;
            } else {
                boost::function<void()> nextRequest;
                nextRequest.swap(m_requestQueue.front());
                m_requestQueue.pop();
                lock.unlock();
                try {
                    nextRequest();
                } catch (const std::exception& e2) {
                    KARABO_LOG_FRAMEWORK_ERROR << "onResponse: next request resulting in exception: " << e2.what();
                }
            }
        }


        void InfluxDbClient::sendToInfluxDb(const std::string& message, const InfluxResponseHandler& action,
                                            const std::string& requestId) {
            auto handler = bind_weak(&InfluxDbClient::onResponse, this, _1, action);
            {
                boost::mutex::scoped_lock lock(m_responseHandlersMutex);
                m_registeredInfluxResponseHandlers.emplace(std::make_pair(requestId, std::make_pair(message, handler)));
            }
            writeDb(message, requestId);
        }


        void InfluxDbClient::postQueryDb(const std::string& sel, const InfluxResponseHandler& action) {
            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            m_requestQueue.push(bind_weak(&InfluxDbClient::postQueryDbTask, this, sel, action));
            tryNextRequest(lock);
        }


        void InfluxDbClient::postQueryDbTask(const std::string& statement, const InfluxResponseHandler& action) {
            if (!this->connectWait(k_connTimeoutMs)) {
                std::ostringstream oss;
                oss << "Could not connect to InfluxDb at \"" << m_url << "\".";
                const std::string errMsg = oss.str();
                {
                    boost::mutex::scoped_lock lock(m_influxVersionMutex);
                    m_influxVersion.clear();
                }
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                // Synthesizes a 503 (Service Unavailable) response and sends it back to the client.
                if (action != nullptr) {
                    HttpResponse resp;
                    resp.code = 503;
                    resp.payload = errMsg;
                    resp.contentType = "text/plain";
                    KARABO_LOG_FRAMEWORK_DEBUG << "Will call action with response:\n" << resp;
                    action(resp);
                }
                // Resets m_active to allow the m_requestQueue consumption to start going again
                // with next call to tryNextRequest - but this task is lost!
                m_active = false;
                return;
            }
            const std::string requestId(generateUUID());
            std::ostringstream oss;

            oss << "POST /query?chunked=true&db=&epoch=" << m_durationUnit << "&q=" << urlencode(statement);
            if (!m_dbUser.empty() && !m_dbPassword.empty()) {
                oss << "&u=" << urlencode(m_dbUser) << "&p=" << urlencode(m_dbPassword);
            }
            oss << " HTTP/1.1\r\n"
                << "Host: " << m_hostname << "\r\n"
                << "Request-Id: " << requestId << "\r\n";

            const std::string rawAuth(getRawBasicAuthHeader());
            if (!rawAuth.empty()) {
                oss << rawAuth << "\r\n";
            }

            oss << "\r\n";
            sendToInfluxDb(oss.str(), action, requestId);
        }


        void InfluxDbClient::getPingDb(const InfluxResponseHandler& action) {
            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            m_requestQueue.push(bind_weak(&InfluxDbClient::getPingDbTask, this, action));
            tryNextRequest(lock);
        }


        void InfluxDbClient::getPingDbTask(const InfluxResponseHandler& action) {
            const std::string requestId(generateUUID());
            std::ostringstream oss;
            oss << "GET /ping";
            if (!m_dbUser.empty() && !m_dbPassword.empty()) {
                oss << "?u=" << urlencode(m_dbUser) << "&p=" << urlencode(m_dbPassword);
            }
            oss << " HTTP/1.1\r\n"
                << "Host: " << m_hostname << "\r\n"
                << "Request-Id: " << requestId << "\r\n";

            const std::string rawAuth(getRawBasicAuthHeader());
            if (!rawAuth.empty()) {
                oss << rawAuth << "\r\n";
            }

            oss << "\r\n";
            sendToInfluxDb(oss.str(), action, requestId);
        }


        void InfluxDbClient::writeDb(const std::string& message, const std::string& requestId) {
            if (m_dbChannel) {
                auto datap = boost::make_shared<std::vector<char>>(std::vector<char>(message.begin(), message.end()));
                m_flyingId = requestId;
                m_dbChannel->writeAsyncVectorPointer(datap, bind_weak(&InfluxDbClient::onDbWrite, this, _1, datap));
                KARABO_LOG_FRAMEWORK_DEBUG << "writeDb: \n" << message;
            } else {
                // TODO: Add a fallback - save message to file in case there's no channel to the database anymore.
                std::ostringstream oss;
                oss << "writeDb: ";
                oss << "No channel available for communicating with InfluxDb.\n"
                    << "Message that couldn't be sent:\n"
                    << message;
                KARABO_LOG_FRAMEWORK_ERROR << oss.str();
            }
        }


        void InfluxDbClient::enqueueQuery(const std::string& line) {
            boost::mutex::scoped_lock lock(m_bufferMutex);
            m_buffer << line;
            if (++m_nPoints > m_maxPointsInBuffer) {
                flushBatchImpl();
            }
        }


        void InfluxDbClient::flushBatch(const InfluxResponseHandler& respHandler) {
            boost::mutex::scoped_lock lock(m_bufferMutex);
            flushBatchImpl(respHandler);
        }

        void InfluxDbClient::flushBatchImpl(const InfluxResponseHandler& respHandler) {
            if (m_nPoints > 0) {
                // Post accumulated batch ...
                postWriteDb(m_buffer.str(), [respHandler](const HttpResponse& response) {
                    if (response.code != 204) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Flushing failed (" << response.code << "): " << response.payload;
                    }
                    if (respHandler) {
                        respHandler(response);
                    }
                });
            } else if (respHandler) {
                // Buffer empty and nothing to do. But a response is requested, so create one with success code:
                HttpResponse resp;
                resp.code = 204;
                // Go via event loop to avoid dead lock in case the handler calls a function that locks m_bufferMutex
                EventLoop::getIOService().post(boost::bind(respHandler, resp));
            }
            m_buffer.str(""); // clear buffer stream
            m_nPoints = 0;
        }

        void InfluxDbClient::onDbConnect(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel,
                                         const InfluxConnectedHandler& hook) {
            if (ec) {
                std::ostringstream oss;
                oss << "No connection to InfluxDb server at '" << m_hostname << "'. Code #" << ec.value()
                    << ", message: '" << ec.message() << "'";
                KARABO_LOG_FRAMEWORK_ERROR << oss.str();
                {
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannel.reset();
                    m_connectionRequested = false;
                }
                {
                    boost::mutex::scoped_lock lock(m_responseHandlersMutex);
                    m_registeredInfluxResponseHandlers.clear();
                }
                {
                    boost::mutex::scoped_lock lock(m_influxVersionMutex);
                    m_influxVersion.clear();
                }
                if (hook) hook(false); // false means connection failed
                return;
            } else {
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_connectionRequested = false;
                m_dbChannel = channel;
            }

            std::ostringstream oss;
            // NOTE: At this point the connection has been established at the TCP level; no
            //       HTTP response with a header indicating the Influx server version has
            //       been received.
            oss << "InfluxDbClient : connection to Influx Server at\"" << m_url << "\" established";
            KARABO_LOG_FRAMEWORK_INFO << oss.str();

            if (hook) hook(true); // true means connected successfuly.

            m_dbChannel->readAsyncStringUntil("\r\n\r\n", bind_weak(&InfluxDbClient::onDbRead, this, _1, _2));
        }


        void InfluxDbClient::onDbRead(const karabo::net::ErrorCode& ec, const std::string& line) {
            std::string flyingId;
            {
                boost::mutex::scoped_lock lock(m_requestQueueMutex);
                flyingId = m_flyingId;
            }

            if (ec) {
                std::ostringstream oss;
                oss << "Reading response from InfluxDB failed: code #" << ec.value() << " -- " << ec.message();
                handleHttpReadError(oss.str(), flyingId);
                return;
            }

            KARABO_LOG_FRAMEWORK_DEBUG << "DBREAD Ack:\n" << line;

            {
                if (line.substr(0, 9) == "HTTP/1.1 ") {
                    m_response.clear(); // Clear for new header
                    try {
                        m_response.parseHttpHeader(line); // Fill out m_response object
                    } catch (std::exception& e) {
                        // An error parsing the http header is not recoverable within the
                        // same connection - the client would lose sync with the server in
                        // a permanent way.
                        std::ostringstream oss;
                        oss << "Error parsing HttpHeader: " << e.what() << std::endl
                            << "Content being parsed: " << line << std::endl;
                        handleHttpReadError(oss.str(), flyingId);
                        return;
                    }
                    if (m_response.requestId.empty()) {
                        m_response.requestId = flyingId;
                        m_response.contentType = "application/json";
                    }
                    if (!m_response.version.empty() && m_influxVersion != m_response.version) {
                        // Note: the test above for the m_influxVersion can be outside of the mutex protection
                        // because of the one-request-at-a-time policy enforced by the InfluxDbClient.
                        // As the external access to m_influxVersion is read-only and all the internal writes
                        // happen on different phases of the processing of a single request, the possible
                        // race conditions are all between external reads and internal writes. Internal reads
                        // and writes won't concur with each other.
                        boost::mutex::scoped_lock lock(m_influxVersionMutex);
                        m_influxVersion = m_response.version;
                        KARABO_LOG_FRAMEWORK_INFO << "Connected Influx instance has version '" << m_influxVersion
                                                  << "'.\n";
                    }
                    m_response.payloadArrived = true;
                    if (m_response.transferEncoding == "chunked") {
                        m_response.payloadArrived = false;
                    } else if (m_response.transferEncoding.empty() && m_response.contentLength > 0) {
                        // According to the HTTP message specification
                        // (https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html), messages with 'Content-Length'
                        // specified but without any transfer-encoding should carry 'Content-Length' bytes of message
                        // body. For Influx the client only cares about 'chunked' message bodies but it must consume
                        // non chunked bodies so it doesn't lose data alignment.
                        m_response.payload = (*m_dbChannel).consumeBytesAfterReadUntil(m_response.contentLength);
                    }
                } else if (m_response.transferEncoding == "chunked") {
                    try {
                        m_response.parseHttpChunks(line);
                    } catch (std::exception& e) {
                        // An error parsing an http chunk is not recoverable within the
                        // same connection - the client would lose sync with the server in
                        // a permanent way.
                        std::ostringstream oss;
                        oss << "Error parsing HttpChunk: " << e.what() << std::endl
                            << "Content being parsed: " << line << std::endl;
                        handleHttpReadError(oss.str(), flyingId);
                        return;
                    }
                    if (m_response.contentType != "application/json") {
                        std::ostringstream oss;
                        oss << "Currently only 'application/json' Content-Type is supported";
                        throw KARABO_NOT_SUPPORTED_EXCEPTION(oss.str());
                    }
                    // Now payload should contain json string
                    m_response.payloadArrived = true; // If payload arrived we can call action
                } else if (m_response.contentLength > 0 && !m_response.payloadArrived) {
                    m_response.payloadArrived = true;
                    m_response.payload = line;
                }

                if (m_response.payloadArrived) {
                    // 20x  -- no errors
                    // 40x  -- client request errors
                    // 50x  -- server problems
                    // Report if the problem arises ...
                    // Call callback
                    if (!m_response.requestId.empty()) {
                        if (m_response.code >= 300) {
                            KARABO_LOG_FRAMEWORK_ERROR << "InfluxDB ERROR RESPONSE:\n" << m_response;
                        }
                        boost::mutex::scoped_lock lock(m_responseHandlersMutex);
                        auto it = m_registeredInfluxResponseHandlers.find(m_response.requestId);
                        if (it != m_registeredInfluxResponseHandlers.end()) {
                            if (m_response.code >= 300) {
                                KARABO_LOG_FRAMEWORK_ERROR << "... on request: " << it->second.first.substr(0, 1024)
                                                           << "...";
                            }
                            InfluxResponseHandler
                                  handler; // NOTE: This will store the handler so it can be called after
                            // releasing m_responseHandlersMutex. This is required because
                            // the handler (always InfluxDbClient::OnResponse) will call
                            // InfluxDbClient::sendToInfluxDb which will try to lock
                            // m_responseHandlersMutex. If the lock is not released before
                            // the call, the thread will block trying to acquire the same
                            // lock it acquired previously.
                            handler = std::move(it->second.second);
                            m_registeredInfluxResponseHandlers.erase(it);
                            lock.unlock();
                            handler(m_response);
                        } else {
                            // A handler hasn't been found for the request - this should not happen!
                            KARABO_LOG_FRAMEWORK_ERROR << "No handler found for request '" << m_response.requestId
                                                       << "'. Response being ignored:\n"
                                                       << m_response;
                        }
                    }
                }
            }

            if (m_response.connection == "close") {
                KARABO_LOG_FRAMEWORK_ERROR << "InfluxDB server at '" << m_hostname << "' closed connection...\n"
                                           << line;
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_dbChannel.reset();
                m_connectionRequested = false;
            }

            if (isConnected()) {
                m_dbChannel->readAsyncStringUntil("\r\n\r\n", bind_weak(&InfluxDbClient::onDbRead, this, _1, _2));
            }
        }


        void InfluxDbClient::onDbWrite(const karabo::net::ErrorCode& ec, boost::shared_ptr<std::vector<char>> p) {
            p.reset();
            if (ec) {
                std::string flyingId;
                {
                    boost::mutex::scoped_lock lock(m_requestQueueMutex);
                    flyingId = m_flyingId;
                }
                std::ostringstream oss;
                oss << "Sending request to InfluxDB server at '" << m_hostname << "' failed: code #" << ec.value()
                    << " -- " << ec.message();
                handleHttpReadError(oss.str(), flyingId);
                return;
            }
            // For the ec == 0 condition - no error at tcp level -
            // Relies on readAsyncStringUntil call made at onDbConnect to consume
            // the http response.
        }


        void InfluxDbClient::handleHttpReadError(const std::string& errMsg, const std::string& requestId) {
            KARABO_LOG_FRAMEWORK_ERROR << errMsg;
            {
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_dbChannel.reset();
                m_connectionRequested = false;
            }
            InfluxResponseHandler handler;
            {
                boost::mutex::scoped_lock lock(m_responseHandlersMutex);
                auto it = m_registeredInfluxResponseHandlers.find(requestId);
                if (it != m_registeredInfluxResponseHandlers.end()) {
                    handler = it->second.second;
                    m_registeredInfluxResponseHandlers.erase(it);
                }
            }
            if (handler) {
                HttpResponse o;
                o.code = 700;
                o.message = errMsg;
                o.requestId = requestId;
                o.connection = "close";
                handler(o);
            }
        }


        void InfluxDbClient::postWriteDb(const std::string& batch, const InfluxResponseHandler& action) {
            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            m_requestQueue.push(bind_weak(&InfluxDbClient::postWriteDbTask, this, batch, action));
            tryNextRequest(lock);
        }


        void InfluxDbClient::postWriteDbTask(const std::string& batch, const InfluxResponseHandler& action) {
            if (!this->connectWait(k_connTimeoutMs)) {
                std::ostringstream oss;
                oss << "Could not connect to InfluxDb at \"" << m_url << "\".";
                const std::string errMsg = oss.str();
                {
                    boost::mutex::scoped_lock lock(m_influxVersionMutex);
                    m_influxVersion.clear();
                }
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                // Synthesizes a 503 (Service Unavailable) response and sends it back to the client.
                if (action != nullptr) {
                    HttpResponse resp;
                    resp.code = 503;
                    resp.payload = errMsg;
                    resp.contentType = "text/plain";
                    KARABO_LOG_FRAMEWORK_DEBUG << "Will call action with response:\n" << resp;
                    action(resp);
                }
                // Resets m_active to allow the m_requestQueue consumption to start going again
                // with next call to tryNextRequest - but this task is lost!
                m_active = false;
                return;
            }
            const std::string requestId(generateUUID());
            std::ostringstream oss;
            oss << "POST /write?db=" << m_dbname << "&precision=" << m_durationUnit;
            if (!m_dbUser.empty() && !m_dbPassword.empty()) {
                oss << "&u=" << urlencode(m_dbUser) << "&p=" << urlencode(m_dbPassword);
            }
            oss << " HTTP/1.1\r\n"
                << "Host: " << m_hostname << "\r\n"
                << "Request-Id: " << requestId << "\r\n";

            const std::string rawAuth(getRawBasicAuthHeader());
            if (!rawAuth.empty()) {
                oss << rawAuth << "\r\n";
            }

            oss << "Content-Length: " << batch.size() << "\r\n\r\n" << batch;
            sendToInfluxDb(oss.str(), action, requestId);
        }


        void InfluxDbClient::queryDb(const std::string& sel, const InfluxResponseHandler& action) {
            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            m_requestQueue.push(bind_weak(&InfluxDbClient::queryDbTask, this, sel, action));
            tryNextRequest(lock);
        }


        void InfluxDbClient::queryDbTask(const std::string& sel, const InfluxResponseHandler& action) {
            if (!this->connectWait(k_connTimeoutMs)) {
                std::ostringstream oss;
                oss << "Could not connect to InfluxDb at \"" << m_url << "\".";
                const std::string errMsg = oss.str();
                {
                    boost::mutex::scoped_lock lock(m_influxVersionMutex);
                    m_influxVersion.clear();
                }
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                // Synthesizes a 503 (Service Unavailable) response and sends it back to the client.
                if (action != nullptr) {
                    HttpResponse resp;
                    resp.code = 503;
                    resp.payload = errMsg;
                    resp.contentType = "text/plain";
                    KARABO_LOG_FRAMEWORK_DEBUG << "Will call action with response:\n" << resp;
                    action(resp);
                }
                // Resets m_active to allow the m_requestQueue consumption to start going again
                // with next call to tryNextRequest - but this task is lost!
                m_active = false;
                return;
            }
            const std::string requestId(generateUUID());
            std::ostringstream oss;
            oss << "GET /query?db=" << m_dbname << "&epoch=" << m_durationUnit << "&q=" << urlencode(sel);
            if (!m_dbUser.empty() && !m_dbPassword.empty()) {
                oss << "&u=" << urlencode(m_dbUser) << "&p=" << urlencode(m_dbPassword);
            }
            oss << " HTTP/1.1\r\n"
                << "Host: " << m_hostname << "\r\n"
                << "Request-Id: " << requestId << "\r\n";

            const std::string rawAuth(getRawBasicAuthHeader());
            if (!rawAuth.empty()) {
                oss << rawAuth << "\r\n";
            }
            oss << "\r\n";

            sendToInfluxDb(oss.str(), action, requestId);
        }


        bool InfluxDbClient::connectWait(std::size_t millis) {
            if (isConnected()) return true;
            auto prom = boost::make_shared<std::promise<bool>>();
            std::future<bool> fut = prom->get_future();
            connectDbIfDisconnected([prom](bool connected) { prom->set_value(connected); });
            auto status = fut.wait_for(std::chrono::milliseconds(millis));
            if (status != std::future_status::ready) {
                return false;
            }
            return fut.get();
        }

    } // namespace net

} // namespace karabo
