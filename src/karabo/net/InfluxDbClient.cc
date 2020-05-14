/*
 * File:   InfluxDbClient.cc
 *
 * Created on November 14, 2019, 9:57 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <cmath>
#include <cstdlib>
#include <sstream>

#include <boost/chrono/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "karabo/io/BinarySerializer.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/utils.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/util/Base64.hh"
#include "karabo/util/Hash.hh"

#include "InfluxDbClient.hh"
#include "karabo/util/SimpleElement.hh"

namespace karabo {

    namespace net {

        using namespace karabo::io;
        using namespace karabo::net;
        using namespace karabo::util;

        boost::mutex InfluxDbClient::m_uuidGeneratorMutex;
        boost::uuids::random_generator InfluxDbClient::m_uuidGenerator;
        const unsigned int InfluxDbClient::k_connTimeoutMs = 1500u;

        KARABO_REGISTER_FOR_CONFIGURATION(InfluxDbClient);


        void InfluxDbClient::expectedParameters(karabo::util::Schema& expected) {

            STRING_ELEMENT(expected).key("dbname")
                    .displayedName("Database name")
                    .description("The name of the database inside the InfluxDB installation")
                    .assignmentMandatory()
                    .commit();

            STRING_ELEMENT(expected).key("urlWrite")
                    .displayedName("Influxdb URL (write)")
                    .description("URL (write interface) should be given in form: tcp://host:port")
                    .assignmentMandatory()
                    .commit();

            STRING_ELEMENT(expected).key("dbUserWrite")
                    .displayedName("DB username (write)")
                    .description("The name of the database user for the InfluxDB session")
                    .assignmentOptional().defaultValue("")
                    .commit();

            STRING_ELEMENT(expected).key("dbPasswordWrite")
                    .displayedName("DB password (write)")
                    .description("The password of the database user for the InfluxDB session (write interface)")
                    .assignmentOptional().defaultValue("")
                    .commit();

            STRING_ELEMENT(expected).key("urlQuery")
                    .displayedName("Influxdb URL (query)")
                    .description("URL (query interface) should be given in form: tcp://host:port")
                    .assignmentMandatory()
                    .commit();

            STRING_ELEMENT(expected).key("dbUserQuery")
                    .displayedName("DB username (query)")
                    .description("The name of the database user for the InfluxDB session (query interface)")
                    .assignmentOptional().defaultValue("")
                    .commit();

            STRING_ELEMENT(expected).key("dbPasswordQuery")
                    .displayedName("DB password (query)")
                    .description("The password (query interface) of the database user for the InfluxDB session")
                    .assignmentOptional().defaultValue("")
                    .commit();

            STRING_ELEMENT(expected).key("durationUnit")
                    .displayedName("Duration unit")
                    .description("Time unit used: 'd' => day, 'h' => hour, 'm' => minute, 's' => second, "
                                 "'ms' => millisec., 'u' => microsec., 'ns' => nanosec.")
                    .assignmentOptional().defaultValue("u")
                    .options({"d", "h", "m", "s", "ms", "u", "ns"})
                    .commit();

            UINT32_ELEMENT(expected).key("maxPointsInBuffer")
                    .displayedName("Max. points in buffer")
                    .description("Maximum number of enqueued points in buffer")
                    .assignmentOptional()
                    .defaultValue(200u)
                    .commit();
        }


        // TODO: pass the shared pointer to the DbClient as an argument and remove InfluxDbClient::setDbClient(...)
        InfluxDbClient::InfluxDbClient(const karabo::util::Hash& input)
            : m_urlWrite(input.get<std::string>("urlWrite"))
            , m_urlQuery(input.get<std::string>("urlQuery"))
            , m_dbConnectionWrite()
            , m_dbChannelWrite()
            , m_dbConnectionQuery()
            , m_dbChannelQuery()
            , m_requestQueue()
            , m_requestQueueMutex()
            , m_active(false)
            , m_connectionRequestedMutex()
            , m_connectionRequestedWrite(false)
            , m_connectionRequestedQuery(false)
            , m_responseHandlersMutex()
            , m_registeredInfluxResponseHandlers()
            , m_dbname(input.get<std::string>("dbname"))
            , m_durationUnit(input.get<std::string>("durationUnit"))
            , m_currentUuid("")
            , m_maxPointsInBuffer(input.get<std::uint32_t>("maxPointsInBuffer"))
            , m_bufferMutex()
            , m_buffer()
            , m_nPoints(0)
            , m_dbUserWrite(input.get<std::string>("dbUserWrite"))
            , m_dbPasswordWrite(input.get<std::string>("dbPasswordWrite"))
            , m_dbUserQuery(input.get<std::string>("dbUserQuery"))
            , m_dbPasswordQuery(input.get<std::string>("dbPasswordQuery")) {

            if (!m_urlWrite.empty()) {
                const boost::tuple<std::string, std::string,
                    std::string, std::string, std::string> partsWrite = karabo::net::parseUrl(m_urlWrite);
                m_hostnameWrite = partsWrite.get<1>();
            } else {
                m_hostnameWrite = "";
            }
            if (!m_urlQuery.empty()) {
                const boost::tuple<std::string, std::string,
                    std::string, std::string, std::string> partsQuery = karabo::net::parseUrl(m_urlQuery);
                m_hostnameQuery = partsQuery.get<1>();
            } else {
                m_hostnameQuery = "";
            }

            std::ostringstream oss;
            oss << "InfluxDbClient: URL (Write) -> \"" << m_urlWrite
                    << "\", user : \"" << m_dbUserWrite << "\", pass : \"" << m_dbPasswordWrite
                    << "\", host : \"" << m_hostnameWrite
                    << "\"\n\t\tURL (query) -> \"" << m_urlQuery
                    << ", user : \"" << m_dbUserQuery << "\", pass : \"" << m_dbPasswordQuery
                    << "\", host : \"" << m_hostnameQuery << "\"\n";
            KARABO_LOG_FRAMEWORK_DEBUG << oss.str();
        }


        InfluxDbClient::~InfluxDbClient() {
            disconnectWrite();
            disconnectQuery();
        }


        std::string InfluxDbClient::generateUUID() {
            // The generator is not thread safe, but we rely on real uniqueness!
            boost::mutex::scoped_lock lock(m_uuidGeneratorMutex);
            return boost::uuids::to_string(m_uuidGenerator());
        }


        std::string InfluxDbClient::getRawBasicAuthHeader() {
            std::string authHeader;
            if (!m_dbUserWrite.empty() && !m_dbPasswordWrite.empty()) {
                std::string credential = m_dbUserWrite + ":" + m_dbPasswordWrite;
                std::string credentials(credential);
                const unsigned char* pCredentials = reinterpret_cast<const unsigned char*> (credentials.c_str());
                std::string b64Credent = karabo::util::base64Encode(pCredentials, credentials.length());
                authHeader = std::string("Authorization: Basic ").append(b64Credent);
            }
            return authHeader;
        }


        void InfluxDbClient::connectDbIfDisconnectedWrite(const AsyncHandler& hook) {
            boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
            if (!m_dbChannelWrite || !m_dbChannelWrite->isOpen()) {
                if (m_connectionRequestedWrite) return;
                Hash config("url", m_urlWrite, "sizeofLength", 0, "type", "client");
                m_dbConnectionWrite = karabo::net::Connection::create("Tcp", config);
                m_dbConnectionWrite->startAsync(bind_weak(&InfluxDbClient::onDbConnectWrite, this, _1, _2, hook));
                m_connectionRequestedWrite = true;
            }
        }


        void InfluxDbClient::connectDbIfDisconnectedQuery(const AsyncHandler& hook) {
            boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
            if (!m_dbChannelQuery || !m_dbChannelQuery->isOpen()) {
                if (m_connectionRequestedQuery) return;
                Hash config("url", m_urlQuery, "sizeofLength", 0, "type", "client");
                m_dbConnectionQuery = karabo::net::Connection::create("Tcp", config);
                m_dbConnectionQuery->startAsync(bind_weak(&InfluxDbClient::onDbConnectQuery, this, _1, _2, hook));
                m_connectionRequestedQuery = true;
            }
        }


        void InfluxDbClient::disconnectWrite() {
            m_dbChannelWrite.reset();
            m_dbConnectionWrite.reset();
        }


        void InfluxDbClient::disconnectQuery() {
            m_dbChannelQuery.reset();
            m_dbConnectionQuery.reset();
        }


        void InfluxDbClient::tryNextRequest() {
            if (!m_active) { // activate processing
                m_active = true;
                m_requestQueue.front()();
                m_requestQueue.pop();
            }
        }


        void InfluxDbClient::onResponse(const HttpResponse& o, const InfluxResponseHandler& action) {
            try {
                if (action) action(o);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "onResponse: call InfluxResponseHandler resulting in exception : " << e.what();
            }

            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            if (m_requestQueue.empty()) {
                m_active = false;
            } else {
                try {
                    m_requestQueue.front()();
                } catch (const std::exception& e2) {
                    KARABO_LOG_FRAMEWORK_ERROR << "onResponse: next request resulting in exception: " << e2.what();
                }
                m_requestQueue.pop();
            }
        }


        void InfluxDbClient::sendToInfluxDb(const karabo::net::Channel::Pointer& channel,
                                            const std::string& message,
                                            const InfluxResponseHandler& action) {
            auto handler = bind_weak(&InfluxDbClient::onResponse, this, _1, action);
            {
                boost::mutex::scoped_lock(m_responseHandlersMutex);
                m_registeredInfluxResponseHandlers.emplace(std::make_pair(m_currentUuid,
                                                                          std::make_pair(message, handler)));
            }
            writeDb(channel, message);
        }


        void InfluxDbClient::postQueryDb(const std::string& sel, const InfluxResponseHandler& action) {
            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            m_requestQueue.push(bind_weak(&InfluxDbClient::postQueryDbTask, this, sel, action));
            tryNextRequest();
        }


        void InfluxDbClient::postQueryDbTask(const std::string& statement,
                                             const InfluxResponseHandler& action) {
            m_currentUuid.assign(generateUUID());
            std::ostringstream oss;

            oss << "POST /query?chunked=true&db=&epoch=" << m_durationUnit << "&q=" << urlencode(statement);
            if (!m_dbUserQuery.empty() && !m_dbPasswordQuery.empty()) {
                oss << "&u=" << urlencode(m_dbUserQuery) << "&p=" << urlencode(m_dbPasswordQuery);
            }
            oss << " HTTP/1.1\r\n"
                << "Host: " << m_hostnameQuery << "\r\n"
                    << "Request-Id: " << m_currentUuid << "\r\n";

            const std::string rawAuth(getRawBasicAuthHeader());
            if (!rawAuth.empty()) {
                oss << rawAuth << "\r\n";
            }

            oss << "\r\n";
            sendToInfluxDb(m_dbChannelQuery, oss.str(), action);
        }


        void InfluxDbClient::getPingDb(const InfluxResponseHandler& action) {
            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            m_requestQueue.push(bind_weak(&InfluxDbClient::getPingDbTask, this, action));
            tryNextRequest();
        }


        void InfluxDbClient::getPingDbTask(const InfluxResponseHandler& action) {
            m_currentUuid.assign(generateUUID());
            std::ostringstream oss;
            oss << "GET /ping";
            if (!m_dbUserQuery.empty() && !m_dbPasswordQuery.empty()) {
                oss << "?u=" << urlencode(m_dbUserQuery) << "&p=" << urlencode(m_dbPasswordQuery);
            }
            oss << " HTTP/1.1\r\n"
                    << "Host: " << m_hostnameQuery << "\r\n"
                    << "Request-Id: " << m_currentUuid << "\r\n";

            const std::string rawAuth(getRawBasicAuthHeader());
            if (!rawAuth.empty()) {
                oss << rawAuth << "\r\n";
            }

            oss << "\r\n";
            sendToInfluxDb(m_dbChannelQuery, oss.str(), action);
        }


        void InfluxDbClient::writeDb(const karabo::net::Channel::Pointer& channel, const std::string& message) {
            if (channel) {
                auto datap = boost::make_shared<std::vector<char> >(std::vector<char>(message.begin(), message.end()));
                if (channel == m_dbChannelWrite) {
                    channel->writeAsyncVectorPointer(datap, bind_weak(&InfluxDbClient::onDbWriteWrite, this, _1, datap));
                    KARABO_LOG_FRAMEWORK_DEBUG << "writeDb (write) : \n" << message;
                } else {
                    channel->writeAsyncVectorPointer(datap, bind_weak(&InfluxDbClient::onDbWriteQuery, this, _1, datap));
                    KARABO_LOG_FRAMEWORK_DEBUG << "writeDb (query) : \n" << message;
                }
            } else {
                // TODO: Add a fallback - save message to file in case there's no channel to the database anymore.
                std::ostringstream oss;
                if (channel == m_dbChannelWrite) {
                    oss << "writeDb (write) : ";
                } else {
                    oss << "writeDb (query) : ";
                }
                oss << "No channel available for communicating with InfluxDb.\n"
                        << "Message that couldn't be saved:\n" << message;
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


        void InfluxDbClient::flushBatch(const InfluxResponseHandler &respHandler) {
            boost::mutex::scoped_lock lock(m_bufferMutex);
            flushBatchImpl(respHandler);
        }

        void InfluxDbClient::flushBatchImpl(const InfluxResponseHandler &respHandler) {
            if (m_nPoints > 0) {
                // Post accumulated batch ...
                postWriteDb(m_buffer.str(), [respHandler](const HttpResponse & response) {
                    if (response.code != 204) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Flushing failed: " << response.code << " " << response.message;
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

        void InfluxDbClient::onDbConnectQuery(const karabo::net::ErrorCode& ec,
                                              const karabo::net::Channel::Pointer& channel,
                                              const AsyncHandler& hook) {
            if (ec) {
                std::ostringstream oss;
                oss << "No connection to InfluxDb (query) server available. Code #" << ec.value() << ", message: '" << ec.message() << "'";
                KARABO_LOG_FRAMEWORK_ERROR << oss.str();
                {
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannelQuery.reset();
                    m_connectionRequestedQuery = false;
                }
                {
                    boost::mutex::scoped_lock(m_responseHandlersMutex);
                    m_currentUuid = "";
                    m_registeredInfluxResponseHandlers.clear();
                }
                return;
            } else {
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_connectionRequestedQuery = false;
                m_dbChannelQuery = channel;
            }

            std::ostringstream oss;
            oss << "InfluxDbClient (query) : connection to \"" << m_urlQuery << "\" established";
            KARABO_LOG_FRAMEWORK_DEBUG << oss.str();

            if (hook) hook();

            m_dbChannelQuery->readAsyncStringUntil("\r\n\r\n", bind_weak(&InfluxDbClient::onDbReadQuery, this, _1, _2));
        }


        void InfluxDbClient::onDbConnectWrite(const karabo::net::ErrorCode& ec,
                                              const karabo::net::Channel::Pointer& channel,
                                              const AsyncHandler& hook) {
            if (ec) {
                std::ostringstream oss;
                oss << "No connection to InfluxDb (write) server available. Code #" << ec.value() << ", message: '" << ec.message() << "'";
                KARABO_LOG_FRAMEWORK_ERROR << oss.str();
                {
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannelWrite.reset();
                    m_connectionRequestedWrite = false;
                }
                {
                    boost::mutex::scoped_lock(m_responseHandlersMutex);
                    m_currentUuid = "";
                    m_registeredInfluxResponseHandlers.clear();
                }
                return;
            } else {
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_connectionRequestedWrite = false;
                m_dbChannelWrite = channel;
            }

            std::ostringstream oss;
            oss << "InfluxDbClient (write) : connection to \"" << m_urlWrite << "\" established";
            KARABO_LOG_FRAMEWORK_DEBUG << oss.str();

            if (hook) hook();

            m_dbChannelWrite->readAsyncStringUntil("\r\n\r\n", bind_weak(&InfluxDbClient::onDbReadWrite, this, _1, _2));
        }


        void InfluxDbClient::onDbReadWrite(const karabo::net::ErrorCode& ec, const std::string& line) {
            if (ec) {
                std::ostringstream oss;
                oss << "Reading from InfluxDB (write)  failed: code #" << ec.value() << " -- " << ec.message();
                KARABO_LOG_FRAMEWORK_WARN << oss.str();
                {
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannelWrite.reset();
                    m_connectionRequestedWrite = false;
                }
                InfluxResponseHandler handler;
                HttpResponse o;
                {
                    boost::mutex::scoped_lock(m_responseHandlersMutex);
                    assert(m_registeredInfluxResponseHandlers.size() == 1);
                    auto it = m_registeredInfluxResponseHandlers.find(m_currentUuid);
                    if (it != m_registeredInfluxResponseHandlers.end()) {
                        handler = it->second.second;
                        m_registeredInfluxResponseHandlers.erase(it);
                    }
                    o.code = 700;
                    o.message = oss.str();
                    o.requestId = m_currentUuid;
                    o.connection = "close";
                }
                if (handler) handler(o);
                return;
            }

            KARABO_LOG_FRAMEWORK_DEBUG << "DBREAD (write)  Ack:\n" << line;

            {
                boost::mutex::scoped_lock(m_responseHandlersMutex);

                if (line.substr(0, 9) == "HTTP/1.1 ") {
                    m_response.clear(); // Clear for new header
                    m_response.parseHttpHeader(line); // Fill out m_response object
                    if (m_response.requestId.empty()) {
                        m_response.requestId = m_currentUuid;
                        m_response.contentType = "application/json";
                    }
                    m_response.payloadArrived = true;
                    if (m_response.transferEncoding == "chunked" || m_response.contentLength > 0) {
                        m_response.payloadArrived = false;
                    }
                } else if (m_response.transferEncoding == "chunked") {
                    m_response.parseHttpChunks(line);
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
                            KARABO_LOG_FRAMEWORK_ERROR << "InfluxDB (write)  ERROR RESPONSE:\n" << m_response;
                        }
                        auto it = m_registeredInfluxResponseHandlers.find(m_response.requestId);
                        if (it != m_registeredInfluxResponseHandlers.end()) {
                            if (m_response.code >= 300) {
                                KARABO_LOG_FRAMEWORK_ERROR << "... on request: " << it->second.first.substr(0,1024) << "...";
                            }
                            it->second.second(m_response);
                            m_registeredInfluxResponseHandlers.erase(it);
                        }
                    }
                }
            }

            if (m_response.connection == "close") {
                KARABO_LOG_FRAMEWORK_ERROR << "InfluxDB server (write) closed connection...\n" << line;
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_dbChannelWrite.reset();
                m_connectionRequestedWrite = false;
            }

            if (isConnectedWrite()) {
                if (m_response.contentLength > 0 && !m_response.payloadArrived) {
                    m_dbChannelWrite->readAsyncStringUntil("}", bind_weak(&InfluxDbClient::onDbReadWrite, this, _1, _2));
                } else {
                    m_dbChannelWrite->readAsyncStringUntil("\r\n\r\n", bind_weak(&InfluxDbClient::onDbReadWrite, this, _1, _2));
                }
            }
        }


        void InfluxDbClient::onDbReadQuery(const karabo::net::ErrorCode& ec, const std::string& line) {
            if (ec) {
                std::ostringstream oss;
                oss << "Reading from InfluxDB (query)  failed: code #" << ec.value() << " -- " << ec.message();
                KARABO_LOG_FRAMEWORK_WARN << oss.str();
                {
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannelQuery.reset();
                    m_connectionRequestedQuery = false;
                }
                InfluxResponseHandler handler;
                {
                    boost::mutex::scoped_lock(m_responseHandlersMutex);
                    auto it = m_registeredInfluxResponseHandlers.find(m_currentUuid);
                    if (it == m_registeredInfluxResponseHandlers.end()) return;
                    handler = it->second.second;
                    m_registeredInfluxResponseHandlers.erase(it);
                }
                HttpResponse o;
                o.code = 700;
                o.message = oss.str();
                o.requestId = m_currentUuid;
                o.connection = "close";
                handler(o);
                return;
            }

            KARABO_LOG_FRAMEWORK_DEBUG << "DBREAD (query) Ack:\n" << line.substr(0,500) << " ...";

            {
                boost::mutex::scoped_lock(m_responseHandlersMutex);
                if (line.substr(0, 9) == "HTTP/1.1 ") {
                    m_response.clear(); // Clear for new header
                    m_response.parseHttpHeader(line); // Fill out m_response object
                    m_response.payloadArrived = true;
                    if (m_response.transferEncoding == "chunked") {
                        m_response.payloadArrived = false;
                    }
                } else if (m_response.transferEncoding == "chunked") {
                    m_response.parseHttpChunks(line);
                    if (m_response.contentType != "application/json") {
                        std::ostringstream oss;
                        oss << "Currently only 'application/json' Content-Type is supported";
                        throw KARABO_NOT_SUPPORTED_EXCEPTION(oss.str());
                    }
                    // Now payload should contain json string
                    m_response.payloadArrived = true; // If payload arrived we can call action
                }

                if (m_response.payloadArrived) {
                    // 20x  -- no errors
                    // 40x  -- client request errors
                    // 50x  -- server problems
                    // Report if the problem arises ...
                    // Call callback
                    if (!m_response.requestId.empty()) {
                        if (m_response.code >= 300) {
                            KARABO_LOG_FRAMEWORK_ERROR << "InfluxDB (query)  ERROR RESPONSE:\n" << m_response;
                        }
                        auto it = m_registeredInfluxResponseHandlers.find(m_response.requestId);
                        if (it != m_registeredInfluxResponseHandlers.end()) {
                            if (m_response.code >= 300) {
                                KARABO_LOG_FRAMEWORK_ERROR << "... on request: " << it->second.first.substr(0,1024) << "...";
                            }
                            it->second.second(m_response);
                            m_registeredInfluxResponseHandlers.erase(it);
                        }
                    }
                }
            }
            if (m_response.connection == "close") {
                KARABO_LOG_FRAMEWORK_ERROR << "InfluxDB server (query)  closed connection...\n" << line;
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_dbChannelQuery.reset();
                m_connectionRequestedQuery = false;
            }
            if (isConnectedQuery()) {
                m_dbChannelQuery->readAsyncStringUntil("\r\n\r\n", bind_weak(&InfluxDbClient::onDbReadQuery, this, _1, _2));
            }
        }


        void InfluxDbClient::onDbWriteWrite(const karabo::net::ErrorCode& ec, boost::shared_ptr<std::vector<char> > p) {
            p.reset();
            if (ec) {
                std::ostringstream oss;
                oss << "Writing into InfluxDB (write) failed: code #" << ec.value() << " -- " << ec.message();
                KARABO_LOG_FRAMEWORK_WARN << oss.str();
                {
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannelWrite.reset();
                    m_connectionRequestedWrite = false;
                }
                InfluxResponseHandler handler;
                {
                    boost::mutex::scoped_lock(m_responseHandlersMutex);
                    auto it = m_registeredInfluxResponseHandlers.find(m_currentUuid);
                    if (it == m_registeredInfluxResponseHandlers.end()) return;
                    handler = it->second.second;
                    m_registeredInfluxResponseHandlers.erase(it);
                }
                HttpResponse o;
                o.code = 700;
                o.message = oss.str();
                o.requestId = m_currentUuid;
                o.connection = "close";
                if (handler) handler(o);
                return;
            }
        }


        void InfluxDbClient::onDbWriteQuery(const karabo::net::ErrorCode& ec, boost::shared_ptr<std::vector<char> > p) {
            p.reset();
            if (ec) {
                std::ostringstream oss;
                oss << "Writing into InfluxDB (query) failed: code #" << ec.value() << " -- " << ec.message();
                KARABO_LOG_FRAMEWORK_WARN << oss.str();
                {
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannelQuery.reset();
                    m_connectionRequestedQuery = false;
                }
                InfluxResponseHandler handler;
                {
                    boost::mutex::scoped_lock(m_responseHandlersMutex);
                    auto it = m_registeredInfluxResponseHandlers.find(m_currentUuid);
                    if (it == m_registeredInfluxResponseHandlers.end()) return;
                    handler = it->second.second;
                    m_registeredInfluxResponseHandlers.erase(it);
                }
                HttpResponse o;
                o.code = 700;
                o.message = oss.str();
                o.requestId = m_currentUuid;
                o.connection = "close";
                if (handler) handler(o);
                return;
            }
        }


        void InfluxDbClient::postWriteDb(const std::string& batch, const InfluxResponseHandler& action) {
            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            m_requestQueue.push(bind_weak(&InfluxDbClient::postWriteDbTask, this, batch, action));
            tryNextRequest();
        }


        void InfluxDbClient::postWriteDbTask(const std::string& batch, const InfluxResponseHandler& action) {
            if (!this->connectWaitWrite(k_connTimeoutMs)) {
                std::ostringstream oss;
                oss << "No connection to InfluxDb (write) server available. "
                        << "Timed out after waiting for " << k_connTimeoutMs << " ms.";
                const std::string errMsg = oss.str();
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
                // Resets m_active to allow the m_requestQueue consumption to keep going.
                m_active = false;
                return;
            }
            m_currentUuid.assign(generateUUID());
            std::ostringstream oss;
            oss << "POST /write?db=" << m_dbname << "&precision=" << m_durationUnit;
            if (!m_dbUserWrite.empty() && !m_dbPasswordWrite.empty()) {
                oss << "&u=" << urlencode(m_dbUserWrite) << "&p=" << urlencode(m_dbPasswordWrite);
            }
            oss << " HTTP/1.1\r\n"
                    << "Host: " << m_hostnameWrite << "\r\n"
                    << "Request-Id: " << m_currentUuid << "\r\n";

            const std::string rawAuth(getRawBasicAuthHeader());
            if (!rawAuth.empty()) {
                oss << rawAuth << "\r\n";
            }

            oss << "Content-Length: " << batch.size() << "\r\n\r\n" << batch;
            std::string req = oss.str().substr(0,1024);
            sendToInfluxDb(m_dbChannelWrite, oss.str(), action);
        }


        void InfluxDbClient::getQueryDb(const std::string& sel, const InfluxResponseHandler& action) {
            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            m_requestQueue.push(bind_weak(&InfluxDbClient::getQueryDbTask, this, sel, action));
            tryNextRequest();
        }


        void InfluxDbClient::getQueryDbTask(const std::string& sel, const InfluxResponseHandler& action) {
            m_currentUuid.assign(generateUUID());
            std::ostringstream oss;
            oss << "GET /query?chunked=true&db=" << m_dbname << "&epoch=" << m_durationUnit
                    << "&q=" << urlencode(sel);
            if (!m_dbUserQuery.empty() && !m_dbPasswordQuery.empty()) {
                oss << "&u=" << urlencode(m_dbUserQuery) << "&p=" << urlencode(m_dbPasswordQuery);
            }
            oss << " HTTP/1.1\r\n"
                    << "Host: " << m_hostnameQuery << "\r\n"
                    << "Request-Id: " << m_currentUuid << "\r\n";

            const std::string rawAuth(getRawBasicAuthHeader());
            if (!rawAuth.empty()) {
                oss << rawAuth << "\r\n";
            }

            oss << "\r\n";

            sendToInfluxDb(m_dbChannelQuery, oss.str(), action);
        }


        void InfluxDbClient::queryDb(const std::string& sel, const InfluxResponseHandler& action) {
            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            m_requestQueue.push(bind_weak(&InfluxDbClient::queryDbTask, this, sel, action));
            tryNextRequest();
        }


        void InfluxDbClient::queryDbTask(const std::string& sel, const InfluxResponseHandler& action) {
            if (!this->connectWaitQuery(k_connTimeoutMs)) {
                std::ostringstream oss;
                oss << "No connection to InfluxDb (query) server available. "
                        << "Timed out after waiting for " << k_connTimeoutMs << " ms.";
                const std::string errMsg = oss.str();
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
                // Resets m_active to allow the m_requestQueue consumption to keep going.
                m_active = false;
                return;
            }
            m_currentUuid.assign(generateUUID());
            std::ostringstream oss;
            oss << "GET /query?chunked=true&db=" << m_dbname << "&epoch=" << m_durationUnit
                    << "&q=" << urlencode(sel);
            if (!m_dbUserQuery.empty() && !m_dbPasswordQuery.empty()) {
                oss << "&u=" << urlencode(m_dbUserQuery) << "&p=" << urlencode(m_dbPasswordQuery);
            }
            oss << " HTTP/1.1\r\n"
                    << "Host: " << m_hostnameQuery << "\r\n"
                    << "Request-Id: " << m_currentUuid << "\r\n";

            const std::string rawAuth(getRawBasicAuthHeader());
            if (!rawAuth.empty()) {
                oss << rawAuth << "\r\n";
            }

            oss << "\r\n";

            sendToInfluxDb(m_dbChannelQuery, oss.str(), action);
        }


        bool InfluxDbClient::connectWaitWrite(std::size_t millis) {
            if (isConnectedWrite()) return true;
            auto prom = boost::make_shared<std::promise<void>>();
            std::future<void> fut = prom->get_future();
            connectDbIfDisconnectedWrite([prom]() {
                prom->set_value();
            });
            auto status = fut.wait_for(std::chrono::milliseconds(millis));
            if (status != std::future_status::ready) {
                return false;
            }
            fut.get();
            return true;
        }


        bool InfluxDbClient::connectWaitQuery(std::size_t millis) {
            if (isConnectedQuery()) return true;
            auto prom = boost::make_shared<std::promise<void>>();
            std::future<void> fut = prom->get_future();
            connectDbIfDisconnectedQuery([prom]() {
                prom->set_value();
            });
            auto status = fut.wait_for(std::chrono::milliseconds(millis));
            if (status != std::future_status::ready) {
                return false;
            }
            fut.get();
            return true;
        }
    } // namespace net

} // namespace karabo

