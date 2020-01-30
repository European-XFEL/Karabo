/*
 * File:   InfluxDbClient.cc
 * Author: <serguei.essenov@xfel.eu>, <raul.costa@xfel.eu>
 *
 * Created on November 14, 2019, 9:57 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <cmath>
#include <sstream>

#include <boost/chrono/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "karabo/io/BinarySerializer.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/utils.hh"
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

            STRING_ELEMENT(expected).key("url")
                    .displayedName("Influxdb URL")
                    .description("URL should be given in form: tcp://host:port")
                    .assignmentMandatory()
                    .commit();

            STRING_ELEMENT(expected).key("dbname")
                    .displayedName("Database name")
                    .description("The name of the database inside the InfluxDB installation")
                    .assignmentMandatory()
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
            : m_url(input.get<std::string>("url"))
            , m_dbConnection()
            , m_dbChannel()
            , m_requestQueue()
            , m_requestQueueMutex()
            , m_active(false)
            , m_connectionRequestedMutex()
            , m_connectionRequested(false)
            , m_responseHandlersMutex()
            , m_registeredInfluxResponseHandlers()
            , m_dbname(input.get<std::string>("dbname"))
            , m_durationUnit(input.get<std::string>("durationUnit"))
            , m_currentUuid("")
            , m_maxPointsInBuffer(input.get<std::uint32_t>("maxPointsInBuffer"))
            , m_bufferMutex()
            , m_buffer()
            , m_nPoints(0) {
            const boost::tuple<std::string, std::string,
                    std::string, std::string, std::string> parts = karabo::net::parseUrl(m_url);
            m_hostname = parts.get<1>();
        }


        InfluxDbClient::~InfluxDbClient() {
        }

        std::string InfluxDbClient::generateUUID() {
            // The generator is not thread safe, but we rely on real uniqueness!
            boost::mutex::scoped_lock lock(m_uuidGeneratorMutex);
            return boost::uuids::to_string(m_uuidGenerator());
        }


        void InfluxDbClient::connectDbIfDisconnected(const AsyncHandler& hook) {
            boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
            if (!m_dbChannel || !m_dbChannel->isOpen()) {
                if (m_connectionRequested) return;
                Hash config("url", m_url, "sizeofLength", 0, "type", "client");
                m_dbConnection = karabo::net::Connection::create("Tcp", config);
                m_dbConnection->startAsync(bind_weak(&InfluxDbClient::onDbConnect, this, _1, _2, hook));
                m_connectionRequested = true;
            }
        }


        void InfluxDbClient::tryNextRequest() {
            if (!m_active) { // activate processing
                m_active = true;
                m_requestQueue.front()();
                m_requestQueue.pop();
            }
        }


        void InfluxDbClient::onResponse(const HttpResponse& o, const InfluxResponseHandler& action) {
            {
                boost::mutex::scoped_lock lock(m_requestQueueMutex);
                if (!m_requestQueue.empty()) {
                    m_requestQueue.front()();
                    m_requestQueue.pop();
                    m_active = true;
                } else {
                    m_active = false;   // Inform 'requestor' that it needs to "activate" processing
                }
            }
            action(o);
        }


        void InfluxDbClient::sendToInfluxDb(const std::string& message, const InfluxResponseHandler& action) {
            auto handler = bind_weak(&InfluxDbClient::onResponse, this, _1, action);
            {
                boost::mutex::scoped_lock(m_responseHandlersMutex);
                m_registeredInfluxResponseHandlers.emplace(std::make_pair(m_currentUuid,
                                                                          std::make_pair(message, handler)));
            }
            writeDb(message);
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

            oss << "POST /query?chunked=true&db=&epoch=" << m_durationUnit << "&q=" << urlencode(statement) << " HTTP/1.1\r\n"
                << "Host: " << m_hostname << "\r\n"
                << "Request-Id: " << m_currentUuid << "\r\n\r\n";

            sendToInfluxDb(oss.str(), action);
        }


        void InfluxDbClient::getPingDb(const InfluxResponseHandler& action) {
            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            m_requestQueue.push(bind_weak(&InfluxDbClient::getPingDbTask, this, action));
            tryNextRequest();
        }


        void InfluxDbClient::getPingDbTask(const InfluxResponseHandler& action) {
            m_currentUuid.assign(generateUUID());
            std::ostringstream oss;
            oss << "GET /ping HTTP/1.1\r\n"
                    << "Host: " << m_hostname << "\r\n"
                    << "Request-Id: " << m_currentUuid << "\r\n\r\n";

            sendToInfluxDb(oss.str(), action);
        }


        void InfluxDbClient::writeDb(const std::string& message) {
            if (m_dbChannel) {
                auto datap = boost::make_shared<std::vector<char> >(std::vector<char>(message.begin(), message.end()));
                m_dbChannel->writeAsyncVectorPointer(datap, bind_weak(&InfluxDbClient::onDbWrite, this, _1, datap));
                KARABO_LOG_FRAMEWORK_DEBUG << "writeDb: \n" << message;
            } else {
                // TODO: Add a fallback - save message to file in case there's no channel to the database anymore.
                KARABO_LOG_FRAMEWORK_ERROR << "No channel available for communicating will InfluxDb.\n"
                        << "Message that couldn't be saved:\n" << message;
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
                        KARABO_LOG_FRAMEWORK_ERROR << "Code " << response.code << " " << response.message;
                    }
                    if (respHandler) {
                        respHandler(response);
                    }
                });
            }
            m_buffer.str(""); // clear buffer stream
            m_nPoints = 0;
        }


        void InfluxDbClient::onDbConnect(const karabo::net::ErrorCode& ec,
                                         const karabo::net::Channel::Pointer& channel,
                                         const AsyncHandler& hook) {
            if (ec) {
                std::ostringstream oss;
                oss << "Connection to InfluxDB failed: code #" << ec.value() << ", message: '" << ec.message() << "'";
                KARABO_LOG_FRAMEWORK_ERROR << oss.str();
                {
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannel.reset();
                    m_connectionRequested = false;
                }
                {
                    boost::mutex::scoped_lock(m_responseHandlersMutex);
                    m_currentUuid = "";
                    m_registeredInfluxResponseHandlers.clear();
                }
                return;
            } else {
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_connectionRequested = false;
                m_dbChannel = channel;
            }

            KARABO_LOG_FRAMEWORK_INFO << "InfluxDbClient: connection established";

            if (hook) hook();

            m_dbChannel->readAsyncStringUntil("\r\n\r\n", bind_weak(&InfluxDbClient::onDbRead, this, _1, _2));
        }


        void InfluxDbClient::onDbRead(const karabo::net::ErrorCode& ec, const std::string& line) {
            if (ec) {
                std::ostringstream oss;
                oss << "Reading from InfluxDB failed: code #" << ec.value() << " -- " << ec.message();
                KARABO_LOG_FRAMEWORK_WARN << oss.str();
                {
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannel.reset();
                    m_connectionRequested = false;
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

            KARABO_LOG_FRAMEWORK_DEBUG << "DBREAD Ack:\n" << line;

            if (line.substr(0, 9) == "HTTP/1.1 ") {
                m_response.clear(); // Clear for new header
                m_response.parseHttpHeader(line); // Fill out m_response object
                if (m_response.transferEncoding == "chunked") m_response.payloadArrived = false;
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
                if (m_response.code >= 300) {
                    KARABO_LOG_FRAMEWORK_ERROR << "InfluxDB ERROR RESPONSE:\n" << m_response;
                    if (!m_response.requestId.empty()) {
                        boost::mutex::scoped_lock(m_responseHandlersMutex);
                        auto it = m_registeredInfluxResponseHandlers.find(m_response.requestId);
                        if (it != m_registeredInfluxResponseHandlers.end()) {
                            KARABO_LOG_FRAMEWORK_ERROR << "... on request: " << it->second.first.substr(0,1024) << "...";
                        }
                    }
                }
                if (m_response.connection == "close") {
                    KARABO_LOG_FRAMEWORK_ERROR << "InfluxDB server closed connection...\n" << line;
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannel.reset();
                    m_connectionRequested = false;
                }
                // Call callback
                if (!m_response.requestId.empty()) {
                    boost::mutex::scoped_lock(m_responseHandlersMutex);
                    auto it = m_registeredInfluxResponseHandlers.find(m_response.requestId);
                    if (it != m_registeredInfluxResponseHandlers.end()) {
                        it->second.second(m_response);
                        m_registeredInfluxResponseHandlers.erase(it);
                    }
                }
            }
            if (isConnected()) {
                m_dbChannel->readAsyncStringUntil("\r\n\r\n", bind_weak(&InfluxDbClient::onDbRead, this, _1, _2));
            }
        }


        void InfluxDbClient::onDbWrite(const karabo::net::ErrorCode& ec, boost::shared_ptr<std::vector<char> > p) {
            p.reset();
            if (ec) {
                std::ostringstream oss;
                oss << "Writing into InfluxDB failed: code #" << ec.value() << " -- " << ec.message();
                KARABO_LOG_FRAMEWORK_WARN << oss.str();
                {
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannel.reset();
                    m_connectionRequested = false;
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
            m_currentUuid.assign(generateUUID());
            std::ostringstream oss;
            oss << "POST /write?db=" << m_dbname << "&precision=" << m_durationUnit << " HTTP/1.1\r\n"
                    << "Host: " << m_hostname << "\r\n"
                    << "Request-Id: " << m_currentUuid << "\r\n"
                    << "Content-Length: " << batch.size() << "\r\n\r\n" << batch;

            sendToInfluxDb(oss.str(), action);
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
                    << "&q=" << urlencode(sel) << " HTTP/1.1\r\n"
                    << "Host: " << m_hostname << "\r\n"
                    << "Request-Id: " << m_currentUuid << "\r\n\r\n";

            sendToInfluxDb(oss.str(), action);
        }


        void InfluxDbClient::queryDb(const std::string& sel, const InfluxResponseHandler& action) {
            boost::mutex::scoped_lock lock(m_requestQueueMutex);
            m_requestQueue.push(bind_weak(&InfluxDbClient::queryDbTask, this, sel, action));
            tryNextRequest();
        }


        void InfluxDbClient::queryDbTask(const std::string& sel, const InfluxResponseHandler& action) {
            if (!this->connectWait(k_connTimeoutMs)) {
                std::ostringstream oss;
                oss << "No connection to InfluxDb server available. "
                        << "Timed out after waiting for " << k_connTimeoutMs << " ms.";
                const std::string errMsg = oss.str();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                // Synthetizes a 503 (Service Unavailable) response and sends it back to the client.
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
                    << "&q=" << urlencode(sel) << " HTTP/1.1\r\n"
                    << "Host: " << m_hostname << "\r\n"
                    << "Request-Id: " << m_currentUuid << "\r\n\r\n";

            sendToInfluxDb(oss.str(), action);
        }


        bool InfluxDbClient::connectWait(std::size_t millis) {
            if (isConnected()) return true;
            std::promise<void> prom;
            std::future<void> fut = prom.get_future();
            connectDbIfDisconnected([&prom]() {
                prom.set_value();
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

