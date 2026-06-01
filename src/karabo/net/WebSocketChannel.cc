/*
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
#include "WebSocketChannel.hh"

#include <assert.h>
#include <sys/socket.h> // Linux...

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/pointer_cast.hpp>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "Channel.hh"
#include "EventLoop.hh"
#include "karabo/data/io/HashBinarySerializer.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/util/MetaTools.hh"

// #define NO_SIZE_PREFIX

using std::placeholders::_1;
using std::placeholders::_2;
using tcp = boost::asio::ip::tcp;

namespace karabo::net {

    namespace asio = boost::asio;
    namespace beast = boost::beast;
    namespace http = boost::beast::http;
    namespace websocket = beast::websocket;

    using namespace std;
    using namespace boost::asio;
    using namespace karabo::data;
    using namespace karabo::util;

    const size_t kDefaultQueueCapacity = 5000; // JW: Moved from Queue.h


    static void fail(beast::error_code ec, char const* what) {
        std::ostringstream oss;
        oss << what << ": " << ec.message();
        throw KARABO_NETWORK_EXCEPTION(oss.str());
    }

    karabo::data::Hash WebSocketChannel::getChannelInfo(
          const std::shared_ptr<karabo::net::WebSocketChannel>& wsChannel) {
        if (!wsChannel)
            return Hash("remoteAddress", "?", "remotePort", uint16_t(0), "localAddress", "?", "localPort", uint16_t(0));
        return wsChannel->_getChannelInfo();
    }


    karabo::data::Hash WebSocketChannel::_getChannelInfo() {
        Hash info("remoteAddress", "?", "remotePort", uint16_t(0), "localAddress", "?", "localPort", uint16_t(0));
        try {
            info.set<std::string>("localAddress", m_ws->next_layer().socket().local_endpoint().address().to_string());
            info.set<unsigned short>("localPort", m_ws->next_layer().socket().local_endpoint().port());
            if (m_ws->is_open()) {
                info.set<std::string>("remoteAddress",
                                      m_ws->next_layer().socket().remote_endpoint().address().to_string());
                info.set<unsigned short>("remotePort", m_ws->next_layer().socket().remote_endpoint().port());
            }
        } catch (...) {
        }
        return info;
    }


    std::string WebSocketChannel::remoteAddress() const {
        std::string address("unknown");

        if (m_ws->is_open()) {
            try {
                address = boost::lexical_cast<std::string>(m_ws->next_layer().socket().remote_endpoint());
            } catch (...) {
            }
        }
        return address;
    }


    WebSocketChannel::WebSocketChannel(std::shared_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>>&& ws,
                                       Connection::WeakPointer connection)
        : m_ws(std::move(ws)),
          m_connectionPointer(connection),
          m_binarySerializer(BinarySerializer<Hash>::create("Bin")),
          m_activeHandlerType(WebSocketChannel::NONE),
          m_queue(10),
          m_queueWrittenBytes(m_queue.size(), 0),
          m_readBytes(0),
          m_writtenBytes(0),
          m_writeInProgress(false) {
        m_queue[4] = Queue::Pointer(new LosslessQueue);
        m_queue[2] = Queue::Pointer(new RemoveOldestQueue(kDefaultQueueCapacity));
        m_queue[0] = Queue::Pointer(new RejectNewestQueue(kDefaultQueueCapacity));
    }


    WebSocketChannel::~WebSocketChannel() {
        if (m_ws) beast::close_socket(beast::get_lowest_layer(*m_ws));
    }


    void WebSocketChannel::doRead() {
        // Read a message into our buffer
        m_ws->async_read(m_buffer, bind_weak(&WebSocketChannel::onRead, this, _1, _2));
    }


    void WebSocketChannel::onRead(beast::error_code ec, std::size_t bytes_transferred) {
        KARABO_LOG_FRAMEWORK_DEBUG << "onRead: ec=" << ec.value() << " -- " << ec.message()
                                   << ", bytes_transferred=" << bytes_transferred;

        // This indicates that the session was closed
        if (ec == websocket::error::closed) return;
        if (ec) fail(ec, "read");
        m_readBytes += bytes_transferred;
        std::size_t len = m_buffer.size();
        assert(len == bytes_transferred);
        const char* archive = static_cast<const char*>(m_buffer.data().data());
#ifndef NO_SIZE_PREFIX
        // serialized archive prefixed with 4 bytes size field
        len -= sizeof(unsigned int);
        archive += sizeof(unsigned int);
#endif
        // Deserialize into hash
        Hash hash;
        m_binarySerializer->load(hash, archive, len);
        // Clear the buffer
        m_buffer.consume(bytes_transferred);
        // Allow next read request
        m_activeHandlerType = WebSocketChannel::NONE;
        if (m_readHandler) {
            auto success = boost::system::errc::make_error_code(boost::system::errc::success);
            asio::dispatch(beast::bind_handler(std::move(m_readHandler), success, hash));
        }
    }


    void WebSocketChannel::readAsyncHash(const ReadHashHandler& handler) {
        if (!handler) throw KARABO_PARAMETER_EXCEPTION("Attempt to call readAsyncHash with empty handler");
        if (m_activeHandlerType != WebSocketChannel::NONE)
            throw KARABO_NETWORK_EXCEPTION(
                  "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                  "per channel.");
        m_activeHandlerType = WebSocketChannel::HASH;
        m_readHandler = handler;
        this->doRead();
    }

    // NOTE: There is the potential for a race condition here. However,
    // we are not worried about it. This method and `dataQuantityWritten` should only be used for rough statistics
    // gathering to be used in understanding aggregate network usage. If a message here and there gets overlooked, it's
    // not a dealbreaker. The complexity needed to protect access here is not worth the risk.


    size_t WebSocketChannel::dataQuantityRead() {
        const size_t ret = m_readBytes;
        m_readBytes = 0;
        return ret;
    }


    size_t WebSocketChannel::dataQuantityWritten() {
        const size_t ret = m_writtenBytes;
        m_writtenBytes = 0;
        return ret;
    }


    void WebSocketChannel::close() {
        beast::close_socket(beast::get_lowest_layer(*m_ws));
    }


    bool WebSocketChannel::isOpen() {
        return m_ws->is_open();
    }


    karabo::data::Hash WebSocketChannel::queueInfo() {
        Hash info;
        std::lock_guard<std::mutex> lock(m_queueMutex);
        for (size_t i = 0; i < m_queue.size(); ++i) {
            if (!m_queue[i]) continue;

            Hash queueStats;
            queueStats.set<unsigned long long>("pendingCount", m_queue[i]->size());
            queueStats.set<unsigned long long>("writtenBytes", m_queueWrittenBytes[i]);
            info.set<Hash>(karabo::data::toString(i), queueStats);
        }
        return info;
    }


    void WebSocketChannel::setAsyncChannelPolicy(int priority, const std::string& new_policy, const size_t capacity) {
        std::string candidate = boost::to_upper_copy<std::string>(new_policy);
        std::lock_guard<std::mutex> lock(m_queueMutex);

        if (candidate == "LOSSLESS") {
            m_queue[priority] = Queue::Pointer(new LosslessQueue);

            if (capacity > 0) {
                KARABO_LOG_FRAMEWORK_WARN << "Setting the max capacity of a LosslessQueue is not allowed!";
            }
        } else if (candidate == "REJECT_NEWEST") {
            m_queue[priority] = Queue::Pointer(new RejectNewestQueue(capacity > 0 ? capacity : kDefaultQueueCapacity));
        } else if (candidate == "REMOVE_OLDEST") {
            m_queue[priority] = Queue::Pointer(new RemoveOldestQueue(capacity > 0 ? capacity : kDefaultQueueCapacity));
        } else {
            throw KARABO_NOT_SUPPORTED_EXCEPTION(
                  "Trying to assign not supported channel policy : \"" + new_policy +
                  "\".  Supported policies are \"LOSSLESS\", \"REJECT_NEWEST\", \"REMOVE_OLDEST\"");
        }
    }


    void WebSocketChannel::dispatchWriteAsync(const Message::Pointer& mp, int prio) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queue[prio]->push_back(mp);

        if (!m_writeInProgress) {
            m_writeInProgress = true;
            asio::post(get_executor(), bind_weak(&WebSocketChannel::doWrite, this));
        }
    }


    void WebSocketChannel::doWrite() {
        try {
            Message::Pointer mp;
            int queueIndex = 0;
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                for (int i = 9; i >= 0; --i) {
                    if (!m_queue[i] || m_queue[i]->empty()) continue;
                    mp = m_queue[i]->front();
                    m_queue[i]->pop_front();
                    queueIndex = i;
                    break;
                }
                // if all queues are empty
                if (!mp) {
                    m_writeInProgress = false;
                    return;
                }
            }

            if (!m_ws->is_open()) {
                KARABO_LOG_FRAMEWORK_ERROR << "WebSocketChannel::doWrite : unexpectedly the websocket is not open!";
                m_writeInProgress = false;
                return;
            }

            const BufferSet::Pointer& bptr = mp->body();
            unsigned int bsize = bptr->totalSize();
            m_vbuf.clear();
#ifndef NO_SIZE_PREFIX
            // serialized archive should be prefixed with 4 bytes 'size' field (for KIWI compatibility)
            m_vbuf.push_back(asio::buffer(reinterpret_cast<char*>(&bsize), sizeof(bsize)));
#endif
            m_vbuf.push_back(asio::buffer(bptr->current()));
            m_ws->binary(true); // the payload is binary. The default is utf8
            m_ws->async_write(m_vbuf, bind_weak(&WebSocketChannel::onWrite, this, _1, _2, queueIndex));
        } catch (const std::exception& e) {
            KARABO_LOG_FRAMEWORK_ERROR << "WebSocketChannel::doWrite exception : " << e.what();
            m_writeInProgress = false;
        }
    }


    void WebSocketChannel::onWrite(beast::error_code ec, std::size_t bytes_transferred, int queueIndex) {
        KARABO_LOG_FRAMEWORK_DEBUG << "onWrite : ec=" << ec << ", bytes_transferred=" << bytes_transferred;

        m_queueWrittenBytes[queueIndex] += bytes_transferred;
        m_writtenBytes += bytes_transferred;

        if (!ec) {
            doWrite();
        } else {
            m_writeInProgress = false;
            this->close();
            KARABO_LOG_FRAMEWORK_ERROR << "TcpChannel::doWriteHandler error : " << ec.value() << " -- " << ec.message()
                                       << "  --  Channel is closed now!";
        }
    }


    void WebSocketChannel::writeAsync(const karabo::data::Hash& data, int prio, bool copyAllData) {
        BufferSet::Pointer archive = std::make_shared<BufferSet>(copyAllData);
        m_binarySerializer->save(data, *archive);
        Message::Pointer mp = std::make_shared<Message>(archive);
        dispatchWriteAsync(mp, prio);
    }
} // namespace karabo::net
