/*
 * $Id: AlarmTester.cc 7755 2016-06-24 14:10:56Z haufs $
 *
 * Author: <steffen.hauf@xfel.eu>
 *
 * Created on June, 2016, 03:03 PM
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

#include "TcpAdapter.hh"

#include <cassert>
#include <chrono>

using namespace std::chrono;
using namespace std;
using namespace karabo::util;
using namespace karabo::core;
using namespace karabo::net;
using namespace karabo::io;
using namespace karabo::xms;

using std::placeholders::_1;
using std::placeholders::_2;

namespace karabo {


    const Hash TcpAdapter::k_defaultLoginData("type", "login", "username", "mrusp", "password", "12345", "version",
                                              karabo::util::Version::getKaraboVersion().getVersion());

    TcpAdapter::TcpAdapter(const karabo::util::Hash& config)
        : m_deadline(karabo::net::EventLoop::getIOService()), m_MessageId(0), m_callback() {
        Hash h;
        h.set("port", config.get<unsigned int>("port"));
        h.set("serializationType", "binary");
        m_dataConnection = Connection::create("Tcp", h);

        // Cannot yet use bind_weak in constructor - not worth to recfactor this test class
        m_dataConnection->startAsync(std::bind(&karabo::TcpAdapter::onConnect, this, _1, 500, 10, _2));
        m_debug = config.has("debug") ? config.get<bool>("debug") : false;
    }


    TcpAdapter::~TcpAdapter() {
        if (m_dataConnection) m_dataConnection->stop();
    }


    void TcpAdapter::onConnect(const karabo::net::ErrorCode& ec, int timeout, int repetition,
                               const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            onError(ec, channel);
            if (ec != boost::asio::error::eof && repetition >= 0) {
                m_deadline.expires_after(milliseconds(timeout));
                m_deadline.async_wait(bind_weak(&karabo::TcpAdapter::waitHandler, this,
                                                boost::asio::placeholders::error, timeout, repetition));
            }
            return;
        }
        m_channel = std::dynamic_pointer_cast<TcpChannel>(channel);
        if (channel->isOpen()) channel->readAsyncHash(bind_weak(&karabo::TcpAdapter::onRead, this, _1, channel, _2));
    }

    void TcpAdapter::waitHandler(const karabo::net::ErrorCode& ec, int timeout, int repetition) {
        if (ec == boost::asio::error::operation_aborted) return;
        --repetition;
        timeout *= 2;

        if (repetition == 0) {
            if (m_debug) std::clog << "Connecting failed. Timed out!" << std::endl;

            return;
        };

        m_dataConnection->startAsync(bind_weak(&karabo::TcpAdapter::onConnect, this, _1, timeout, repetition, _2));
    }


    void TcpAdapter::onRead(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel,
                            karabo::util::Hash& info) {
        if (e) {
            onError(e, channel);
            if (channel) channel->close();
            return;
        }

        try {
            // GUI communication scenarios
            if (m_debug) {
                std::clog << "Received message: " << info << std::endl;
            }

            {
                const std::string& type = info.has("type") ? info.get<std::string>("type") : "unspecified";
                {
                    std::unique_lock lock(m_messageAccessMutex);
                    std::vector<Hash>& messages = m_messages[type];
                    messages.push_back(info);
                }
                {
                    std::shared_lock lock(m_queueAccessMutex);
                    if (m_nextMessageQueues.find(type) != m_nextMessageQueues.end()) {
                        if (m_debug) std::clog << "Pushing to queue " << type << std::endl;
                        m_nextMessageQueues[type]->push(info);
                    }
                }
                {
                    std::shared_lock lock(m_callbackMutex);
                    if (m_callback) {
                        m_callback(info);
                    }
                }
            }

            if (channel->isOpen())
                channel->readAsyncHash(bind_weak(&karabo::TcpAdapter::onRead, this, _1, channel, _2));
        } catch (const Exception& e) {
            std::cerr << "Problem in onRead(): " << e.userFriendlyMsg(true) << std::endl;
            if (channel->isOpen())
                channel->readAsyncHash(bind_weak(&karabo::TcpAdapter::onRead, this, _1, channel, _2));
        }
    }

    void TcpAdapter::onError(const karabo::net::ErrorCode& errorCode, karabo::net::Channel::Pointer channel) {
        try {
            if (m_debug)
                std::clog << "onError : TCP socket got error : " << errorCode.value() << " -- \"" << errorCode.message()
                          << "\",  Close connection to GuiServerDevice" << std::endl;

        } catch (const std::exception& se) {
            std::cerr << "Standard exception in onError(): " << se.what() << std::endl;
        }
    }


    void TcpAdapter::login(const karabo::util::Hash& extraLoginData) {
        Hash loginData(k_defaultLoginData);
        loginData.merge(extraLoginData);

        QueuePtr messageQ = getNextMessages("systemTopology", 1, [this, loginData] { sendMessage(loginData); });
        // Clean received message from internal cache.
        Hash lastMessage;
        messageQ->pop(lastMessage);
    }


    std::vector<Hash> TcpAdapter::getAllMessages(const std::string& type) {
        std::shared_lock lock(m_messageAccessMutex);
        return m_messages[type];
    }


    void TcpAdapter::clearAllMessages(const std::string& type) {
        std::shared_lock lock(m_messageAccessMutex);
        if (type.empty()) {
            m_messages.clear();
        } else {
            m_messages[type].clear();
        }
    }


    bool TcpAdapter::connected() const {
        return m_channel && m_channel->isOpen();
    };

    void TcpAdapter::sendMessage(const karabo::util::Hash& message, bool block) {
        if (!connected()) return;
        std::unique_lock lock(m_writeConditionMutex);
        m_writeWaitForId = ++m_MessageId;
        m_channel->writeAsyncHash(message,
                                  bind_weak(&karabo::TcpAdapter::onWriteComplete, this, _1, m_channel, m_MessageId));
        if (block) {
            m_writeCondition.wait(lock);
        }
    }

    void TcpAdapter::onWriteComplete(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel,
                                     size_t id) {
        if (ec) {
            onError(ec, channel);
            if (channel) channel->close();
            std::lock_guard<std::mutex> lock(m_writeConditionMutex);
            m_writeCondition.notify_all();
            return;
        }

        if (m_writeWaitForId == id) {
            std::lock_guard<std::mutex> lock(m_writeConditionMutex);
            m_writeCondition.notify_all();
        }
    }

    void TcpAdapter::disconnect() {
        m_channel->close();
        m_dataConnection->stop();
    }

    std::future_status TcpAdapter::waitFor(const std::string& type,
                                           const std::function<bool(const karabo::util::Hash&)>& callback,
                                           unsigned int timeoutInMs) {
        auto cbPromise = std::make_shared<std::promise<void>>();
        auto cbFuture = cbPromise->get_future();
        {
            std::shared_lock lock(m_callbackMutex);
            // create a lambda function to be attached to the m_callback
            m_callback = [cbPromise, type, callback](const karabo::util::Hash& newData) {
                if (newData.has("type") && newData.get<std::string>("type") == type) {
                    if (callback(newData)) {
                        cbPromise->set_value();
                    }
                }
            };
        }
        // wait for the future
        const std::future_status status = cbFuture.wait_for(std::chrono::milliseconds(timeoutInMs));
        // empty the callback
        {
            std::shared_lock lock(m_callbackMutex);
            m_callback = std::function<void(const karabo::util::Hash&)>();
        }
        return status;
    }
} // namespace karabo
