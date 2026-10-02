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

        // To get remote endpoint address it is enough to have TCP connection open:
        if (m_ws) {
            if (m_ws->is_open() || m_ws->next_layer().socket().is_open()) {
                try {
                    address = boost::lexical_cast<std::string>(m_ws->next_layer().socket().remote_endpoint());
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Fail to get remote address: " << e.what();
                }
            } else {
                KARABO_LOG_FRAMEWORK_DEBUG << "Remote address was requested, but websocket not opened";
            }
        } else {
            KARABO_LOG_FRAMEWORK_DEBUG << "Remote address was requested, but websocket not created";
        }
        return address;
    }


    WebSocketChannel::WebSocketChannel(std::shared_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>>&& ws,
                                       Connection::WeakPointer connection)
        : m_ws(std::move(ws)),
          m_connectionPointer(connection),
          m_binarySerializer(BinarySerializer<Hash>::create("Bin")),
          m_activeHandlerType(WebSocketChannel::NONE),
          m_writeCompleteHandlers(),
          m_queue(10),
          m_queueWrittenBytes(m_queue.size(), 0),
          m_readBytes(0),
          m_writtenBytes(0),
          m_writeInProgress(false),
          m_messageSize(0) {
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
        const char* archive = nullptr;
        std::size_t len = 0;
        if (!ec) {
            m_readBytes += bytes_transferred;
            len = m_buffer.size();
            assert(len == bytes_transferred);

            archive = static_cast<const char*>(m_buffer.data().data());
            // serialized archive prefixed with 4 bytes size field
            len -= sizeof(unsigned int);
            archive += sizeof(unsigned int);
        }

        HandlerType type = m_activeHandlerType;

        switch (type) {
            case HASH: {
                Hash h;
                if (!ec) m_binarySerializer->load(h, archive, len);
                KARABO_LOG_FRAMEWORK_DEBUG << "onRead : HASH len=" << len << ", ec=" << ec;
                if (bytes_transferred > 0) m_buffer.consume(bytes_transferred);
                m_activeHandlerType = WebSocketChannel::NONE;
                std::any_cast<ReadHashHandler>(m_readHandler)(ec, h);
                return;
            }
            default:
                m_activeHandlerType = WebSocketChannel::NONE;
                throw KARABO_LOGIC_EXCEPTION("UNKNOWN HANDLER TYPE: " + str(boost::format("%1%") % type) +
                                             " (this should never happen)");
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
    // gathering to be used in understanding aggregate network usage. If a message here and there gets overlooked,
    // it's not a dealbreaker. The complexity needed to protect access here is not worth the risk.


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
        // This function call means the exchange of 'close' with remote peer to close
        // connection gracefully. It is important to follow this protocol for remote
        // application like python or nodejs based. If simple close low-level socket
        // the remote peers will not be noticed.
        if (m_ws) {
            // Capture the 'guard' to prevent premature ~WebSocketChannel() call
            m_ws->async_close(websocket::close_code::normal, [guard{shared_from_this()}](const ErrorCode& ec) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Websocket closed: #" << ec.value() << " -- " << ec.message();
            });
        }
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
                    // this queue is not empty
                    if (!m_ws->is_open()) {
                        auto timer = std::make_shared<boost::asio::steady_timer>(get_executor());
                        timer->expires_after(std::chrono::milliseconds(100));
                        // Bind timer shared_ptr to lambda to keep it alive as long as needed
                        auto func = bind_weak(&WebSocketChannel::doWrite, this);
                        auto exec = get_executor();
                        timer->async_wait([exec{std::move(get_executor())}, func{std::move(func)},
                                           timer](const boost::system::error_code& e) {
                            if (e) return;
                            KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketChannel")
                                  << "doWrite: Websocket is not yet fully open. Sleep 100ms and try again";
                            asio::post(exec, func);
                        });
                        return;
                    }
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

            const BufferSet::Pointer& bptr = mp->body();
            m_vbuf.clear();

            // Serialized archive is prefixed with 4 bytes 'size' field (for KIWI compatibility)
            m_messageSize = bptr->totalSize();
            m_vbuf.push_back(asio::buffer(reinterpret_cast<char*>(&m_messageSize), sizeof(m_messageSize)));

            bptr->appendTo(m_vbuf);
            m_ws->binary(true); // the payload is binary. The default is utf8
            // Keep mp alive until the async write completes
            m_ws->async_write(m_vbuf, bind_weak(&WebSocketChannel::onWrite, this, mp, _1, _2, queueIndex));
        } catch (const std::exception& e) {
            KARABO_LOG_FRAMEWORK_ERROR << "WebSocketChannel::doWrite exception : " << e.what();
            m_writeInProgress = false;
        }
    }


    void WebSocketChannel::onWrite(const Message::Pointer& /*mp*/, beast::error_code ec, std::size_t bytes_transferred,
                                   int queueIndex) {
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


    void WebSocketChannel::writeAsyncHash(const karabo::data::Hash& data, const WriteCompleteHandler& handler) {
        try {
            auto archive = std::make_shared<std::vector<char>>();
            // serialized archive should be prefixed with 4 bytes 'size' field (for KIWI compatibility)
            archive->resize(sizeof(unsigned int));     // resize vector to allocate space for 'size' field
            m_binarySerializer->save2(data, *archive); // 'save2' store binary data after 'size' field
            // Fill the 'size' field ...
            unsigned int* sizePtr = reinterpret_cast<unsigned int*>(archive->data());
            *sizePtr = archive->size() - sizeof(unsigned int);

            asio::const_buffer buf(archive->data(), archive->size());
            const unsigned int handlerIdx = storeCompleteHandler(handler);
            m_ws->binary(true); // the payload is binary. The default is utf8
            m_ws->async_write(buf, bind_weak(&WebSocketChannel::asyncWriteHandler, this, _1, _2, handlerIdx, archive));
        } catch (...) {
            KARABO_RETHROW
        }
    }


    unsigned int WebSocketChannel::storeCompleteHandler(const Channel::WriteCompleteHandler& handler) {
        // 32-bit index means 13.6 years at 10 Hz is OK without duplication/overflow since
        // 13.6 * 365.25 * 24 * 3600 * 10 < (2^32 - 1)
        // And even then, overflow harms only if the 13.6 years old completion handler was not called due to delays
        unsigned int index = 0;

        std::lock_guard<std::mutex> lock(m_writeCompleteHandlersMutex);
        const auto it = m_writeCompleteHandlers.rbegin();
        if (it != m_writeCompleteHandlers.rend()) {
            index = it->first + 1u;
        }
        m_writeCompleteHandlers[index] = handler;

        return index;
    }


    void WebSocketChannel::asyncWriteHandler(const ErrorCode& e, const size_t length, unsigned int handlerIndex,
                                             const std::shared_ptr<std::vector<char>>& data) {
        try {
            m_writtenBytes += length;

            WriteCompleteHandler handler;
            {
                std::lock_guard<std::mutex> lock(m_writeCompleteHandlersMutex);
                auto it = m_writeCompleteHandlers.find(handlerIndex);
                if (it != m_writeCompleteHandlers.end()) {
                    handler.swap(it->second);
                    m_writeCompleteHandlers.erase(it);
                }
            }
            if (handler) {
                handler(e);
            }
        } catch (...) {
            KARABO_RETHROW
        }
    }

} // namespace karabo::net
