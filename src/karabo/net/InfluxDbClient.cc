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

namespace karabo {

    namespace net {

        using namespace karabo::io;
        using namespace karabo::net;
        using namespace karabo::util;

        boost::mutex InfluxDbClient::m_uuidGeneratorMutex;
        boost::uuids::random_generator InfluxDbClient::m_uuidGenerator;
        const unsigned int InfluxDbClient::k_connTimeoutMs = 1500u;


        InfluxDbClient::InfluxDbClient(const karabo::util::Hash& input)
            : m_url(input.get<std::string>("url"))
            , m_dbConnection()
            , m_dbChannel()
            , m_connectionRequestedMutex()
            , m_connectionRequested(false)
            , m_registeredInfluxResponseHandlers()
            , m_dbname(input.get<std::string>("dbname"))
            , m_durationUnit(input.get<std::string>("durationUnit")) {
            const boost::tuple<std::string, std::string,
                    std::string, std::string, std::string> parts = karabo::net::parseUrl(m_url);
            m_hostname = parts.get<1>();
        }


        InfluxDbClient::~InfluxDbClient() {}


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


        void InfluxDbClient::postQueryDb(const std::string& statement,
                                           const InfluxResponseHandler& action) {
            std::string uuid(generateUUID());
            std::ostringstream oss;

            oss << "POST /query?chunked=true&db=&epoch=" << m_durationUnit << "&q=" << urlencode(statement) << " HTTP/1.1\r\n"
                << "Host: " << m_hostname << "\r\n"
                << "Request-Id: " << uuid << "\r\n\r\n";

            std::string message(oss.str());
            m_registeredInfluxResponseHandlers.emplace(std::make_pair(uuid, std::make_pair(message, action)));
            writeDb(message);
        }


        void InfluxDbClient::getPingDb(const InfluxResponseHandler& action) {
            std::string uuid(generateUUID());
            std::ostringstream oss;
            oss << "GET /ping HTTP/1.1\r\n"
                    << "Host: " << m_hostname << "\r\n"
                    << "Request-Id: " << uuid << "\r\n\r\n";
            const std::string message(oss.str());
            m_registeredInfluxResponseHandlers.emplace(std::make_pair(uuid, std::make_pair(message,action)));
            writeDb(message);
        }


        void InfluxDbClient::writeDb(const std::string& message) {
            auto datap = boost::make_shared<std::vector<char> >(std::vector<char>(message.begin(), message.end()));
            m_dbChannel->writeAsyncVectorPointer(datap, bind_weak(&InfluxDbClient::onDbWrite, this, _1, datap));
            KARABO_LOG_FRAMEWORK_DEBUG << "writeDb: \n" << message;
        }


        void InfluxDbClient::writeDbComplete(const std::string& message, const boost::shared_ptr<void>& r) {
            auto datap = boost::make_shared<std::vector<char> >(std::vector<char>(message.begin(), message.end()));
            m_dbChannel->writeAsyncVectorPointer(datap, bind_weak(&InfluxDbClient::onDbComplete, this,
                                                                  _1, datap, shared_from_this(), r));
            KARABO_LOG_FRAMEWORK_DEBUG << "writeDbComplete: \n" << message;
        }


        void InfluxDbClient::onDbConnect(const karabo::net::ErrorCode& ec,
                                         const karabo::net::Channel::Pointer& channel,
                                         const AsyncHandler& hook) {
            if (ec) {
                KARABO_LOG_FRAMEWORK_WARN << "InfluxDbClient: connection failed: code #" << ec.value() << " -- " << ec.message();
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_dbChannel.reset();
                m_connectionRequested = false;
                return;
            } else {
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_connectionRequested = false;
            }

            m_dbChannel = channel;

            KARABO_LOG_FRAMEWORK_INFO << "InfluxDbClient: connection established";

            if (hook) hook();

            m_dbChannel->readAsyncStringUntil("\r\n\r\n", bind_weak(&InfluxDbClient::onDbRead, this, _1, _2));
        }


        void InfluxDbClient::onDbRead(const karabo::net::ErrorCode& ec, const std::string& line) {
            if (ec) {
                m_dbChannel.reset();
                KARABO_LOG_FRAMEWORK_WARN << "Influxdb read failed: code #" << ec.value() << " -- " << ec.message();
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_dbChannel.reset();
                m_connectionRequested = false;
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
                        auto it = m_registeredInfluxResponseHandlers.find(m_response.requestId);
                        if (it != m_registeredInfluxResponseHandlers.end()) {
                            KARABO_LOG_FRAMEWORK_ERROR << "... on request: " << it->second.first.substr(0,1024) << "...";
                        }
                    }
                }
                if (m_response.connection == "close") {
                    m_dbChannel.reset();
                    KARABO_LOG_FRAMEWORK_ERROR << "InfluxDB server closed connection...\n" << line;
                    boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                    m_dbChannel.reset();
                    m_connectionRequested = false;
                }
                // Call callback
                if (!m_response.requestId.empty() && m_response.payloadArrived) {
                    auto it = m_registeredInfluxResponseHandlers.find(m_response.requestId);
                    if (it != m_registeredInfluxResponseHandlers.end()) {
                        it->second.second(m_response);
                        m_registeredInfluxResponseHandlers.erase(it);
                    }
                }
            }
            if (m_dbChannel && m_dbChannel->isOpen()) {
                m_dbChannel->readAsyncStringUntil("\r\n\r\n", bind_weak(&InfluxDbClient::onDbRead, this, _1, _2));
            }
        }


        void InfluxDbClient::onDbWrite(const karabo::net::ErrorCode& ec, boost::shared_ptr<std::vector<char> > p) {
            p.reset();
            if (ec) {
                m_dbChannel.reset();
                KARABO_LOG_FRAMEWORK_WARN << "Influxdb write failed: code #" << ec.value() << " -- " << ec.message();
                boost::mutex::scoped_lock lock(m_connectionRequestedMutex);
                m_dbChannel.reset();
                m_connectionRequested = false;
                return;
            }
        }


        void InfluxDbClient::onDbComplete(const karabo::net::ErrorCode& ec,
                                          boost::shared_ptr<std::vector<char> > p,
                                          InfluxDbClient::Pointer q, boost::shared_ptr<void> r) {
        }


        boost::shared_ptr<std::string> InfluxDbClient::postWriteDbStr(const std::string& batch,
                                                                      const InfluxResponseHandler& action) {
            const std::string uuid(generateUUID());
            std::ostringstream oss;
            oss << "POST /write?db=" << m_dbname << "&precision=" << m_durationUnit << " HTTP/1.1\r\n"
                    << "Host: " << m_hostname << "\r\n"
                    << "Request-Id: " << uuid << "\r\n"
                    << "Content-Length: " << batch.size() << "\r\n\r\n" << batch;
            auto msg = boost::make_shared<std::string>(oss.str());
            m_registeredInfluxResponseHandlers.emplace(std::make_pair(uuid, std::make_pair(*msg,action)));
            return msg;
        }


        void InfluxDbClient::getQueryDb(const std::string& sel, const InfluxResponseHandler& action) {
            std::string uuid(generateUUID());
            std::ostringstream oss;
            oss << "GET /query?chunked=true&db=" << m_dbname << "&epoch=" << m_durationUnit
                    << "&q=" << urlencode(sel) << " HTTP/1.1\r\n"
                    << "Host: " << m_hostname << "\r\n"
                    << "Request-Id: " << uuid << "\r\n\r\n";
            const std::string message(oss.str());
            m_registeredInfluxResponseHandlers.emplace(std::make_pair(uuid, std::make_pair(message,action)));
            writeDb(message);
        }


        void InfluxDbClient::queryDb(const std::string& sel, const InfluxResponseHandler& action) {
            if (!this->connectWait(k_connTimeoutMs)) {
                std::ostringstream oss;
                oss << "No connection to InfluxDb server available. "
                    << "Timed out after waiting for " << k_connTimeoutMs << " ms.";
                throw KARABO_TIMEOUT_EXCEPTION(oss.str());
            }
            std::string uuid(generateUUID());
            std::ostringstream oss;
            oss << "GET /query?chunked=true&db=" << m_dbname << "&epoch=" << m_durationUnit
                    << "&q=" << urlencode(sel) << " HTTP/1.1\r\n"
                    << "Host: " << m_hostname << "\r\n"
                    << "Request-Id: " << uuid << "\r\n\r\n";
            const std::string message(oss.str());
            m_registeredInfluxResponseHandlers.emplace(std::make_pair(uuid, std::make_pair(message,action)));
            writeDb(message);
        }


        bool InfluxDbClient::connectWait(std::size_t millis) {
            if (m_dbChannel && m_dbChannel->isOpen()) return true;
            std::promise<void> prom;
            std::future<void> fut = prom.get_future();
            connectDbIfDisconnected([&prom]() {
                prom.set_value();
            });
            auto status = fut.wait_for(std::chrono::milliseconds(millis));
            if (status != std::future_status::ready) return false;
            fut.get();
            return true;
        }
    } // namespace net

} // namespace karabo

