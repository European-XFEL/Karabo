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
#include "TcpChannel.hh"

#include <assert.h>
#include <sys/socket.h> // Linux...

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/pointer_cast.hpp>
#include <iostream>
#include <karabo/util/MetaTools.hh>
#include <string>
#include <utility>
#include <vector>

#include "Channel.hh"
#include "EventLoop.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/io/HashBinarySerializer.hh"

using std::placeholders::_1;
using std::placeholders::_2;

namespace karabo {
    namespace net {

        using namespace std;
        using namespace boost::asio;
        using namespace karabo::data;
        using namespace karabo::util;

        const size_t kDefaultQueueCapacity = 5000; // JW: Moved from Queue.h


        karabo::data::Hash TcpChannel::getChannelInfo(const std::shared_ptr<karabo::net::TcpChannel>& tcpChannel) {
            if (!tcpChannel)
                return Hash("remoteAddress", "?", "remotePort", uint16_t(0), "localAddress", "?", "localPort",
                            uint16_t(0));
            return tcpChannel->_getChannelInfo();
        }


        TcpChannel::TcpChannel(const TcpConnection::Pointer& connection)
            : m_connectionPointer(connection),
              m_sizeofLength(connection->getSizeofLength()),
              m_lengthIsText(connection->lengthIsText()),
              m_manageAsyncData(connection->m_manageAsyncData),
              m_keepAliveSettings(connection->m_keepAliveSettings),
              m_socket(EventLoop::getIOService()),
              m_activeHandler(TcpChannel::NONE),
              m_readHeaderFirst(false),
              m_inboundData(new std::vector<char>()),
              m_inboundHeader(new std::vector<char>()),
              m_outboundData(new std::vector<char>()),
              m_outboundHeader(new std::vector<char>()),
              m_writeCompleteHandlers(),
              m_queue(10),
              m_queueWrittenBytes(m_queue.size(), 0),
              m_readBytes(0),
              m_writtenBytes(0),
              m_writeInProgress(false),
              m_syncCounter(0),
              m_asyncCounter(0) {
            m_queue[4] = Queue::Pointer(new LosslessQueue);
            m_queue[2] = Queue::Pointer(new RemoveOldestQueue(kDefaultQueueCapacity));
            m_queue[0] = Queue::Pointer(new RejectNewestQueue(kDefaultQueueCapacity));

            if (connection->m_serializationType == "binary") {
                m_binarySerializer = karabo::io::BinarySerializer<Hash>::create("Bin");
            } else {
                m_textSerializer = karabo::io::TextSerializer<Hash>::create("Xml", Hash("indentation", -1));
            }
        }


        TcpChannel::~TcpChannel() {
            close();
            std::lock_guard<std::mutex> lock(m_writeCompleteHandlersMutex); // not really needed in destructor...
            for (auto& indexHandlerPair : m_writeCompleteHandlers) {
                // Ensure that write complete handlers are called - no need to bother about the order
                EventLoop::post(std::bind(indexHandlerPair.second, boost::asio::error::operation_aborted));
            }
        }

#define _KARABO_VECTOR_TO_SIZE(x, v)                                                              \
    {                                                                                             \
        assert((x).size() == m_sizeofLength);                                                     \
        if (m_lengthIsText) {                                                                     \
            try {                                                                                 \
                v = boost::lexical_cast<size_t>(std::string((x).begin(), (x).end()));             \
            } catch (const boost::bad_lexical_cast& e) {                                          \
                throw KARABO_CAST_EXCEPTION(std::string(e.what()) + " ( the source is '" +        \
                                            std::string((x).begin(), (x).end()) +                 \
                                            "', source_type=" + e.source_type().name() +          \
                                            " and target_type=" + e.target_type().name() + " )"); \
            }                                                                                     \
        } else if (m_sizeofLength == sizeof(uint8_t)) {                                           \
            v = *reinterpret_cast<uint8_t*>(&(x)[0]);                                             \
        } else if (m_sizeofLength == sizeof(uint16_t)) {                                          \
            v = *reinterpret_cast<uint16_t*>(&(x)[0]);                                            \
        } else if (m_sizeofLength == sizeof(uint64_t)) {                                          \
            v = *reinterpret_cast<uint64_t*>(&(x)[0]);                                            \
        } else {                                                                                  \
            v = *reinterpret_cast<uint32_t*>(&(x)[0]);                                            \
        }                                                                                         \
    }

#define _KARABO_SIZE_TO_VECTOR(x, v)                           \
    {                                                          \
        if (m_lengthIsText) {                                  \
            ostringstream oss;                                 \
            oss.fill('0');                                     \
            oss.width(m_sizeofLength);                         \
            oss << v;                                          \
            string slen = oss.str();                           \
            (x).assign(slen.begin(), slen.end());              \
        } else {                                               \
            const char* p = reinterpret_cast<const char*>(&v); \
            (x).assign(p, p + m_sizeofLength);                 \
        }                                                      \
    }


        size_t TcpChannel::readSizeInBytes() {
            if (m_sizeofLength > 0) {
                ErrorCode error; // in case of error
                m_inboundMessagePrefix.resize(m_sizeofLength);
                m_readBytes += boost::asio::read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(), error);
                if (error)
                    throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message());
                // prefix[0] - message length (body)
                size_t byteSize = 0;
                _KARABO_VECTOR_TO_SIZE(m_inboundMessagePrefix, byteSize); // reads size of message and assigns
                return byteSize;
            } else {
                return 0;
            }
        }


        std::string TcpChannel::consumeBytesAfterReadUntil(const size_t nBytes) {
            // boost::asio::async_read_until can put bytes that go beyond its specified terminator
            // sequence argument, so we must consume from the same streamBuffer instance used by
            // TcpChannel::readAsyncStringUntil and deal with the possibility of part of the bytes
            // to read being already in the streamBuffer. Further details on this can be found at
            // https://github.com/chriskohlhoff/asio/issues/113seem and at
            // http://boost.2283326.n4.nabble.com/Cannot-get-boost-asio-read-until-to-properly-read-a-line-new-to-boost-asio-td4681406.html
            const size_t availOnStream = m_streamBufferInbound.in_avail();
            if (nBytes > availOnStream) {
                // There are some bytes to be read into the StreamBuffer
                std::lock_guard<std::mutex> lock(m_socketMutex);
                const size_t toRead = nBytes - availOnStream;
                ErrorCode error;
                m_readBytes += boost::asio::read(m_socket, m_streamBufferInbound, transfer_exactly(toRead), error);
                if (error) {
                    throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message());
                }
            }
            std::ostringstream ss;
            ss << &m_streamBufferInbound;
            std::string str = ss.str();
            // The redirection to the ostringstream already takes care of consuming the bytes in m_streamBufferInbound.

            return str;
        }


        void TcpChannel::read(char* data, const size_t& size) {
            ErrorCode error;
            m_readBytes += boost::asio::read(m_socket, buffer(data, size), transfer_all(), error);
            if (error) {
                throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message());
            }
        }


        void TcpChannel::read(std::vector<char>& data) {
            const size_t nBytes = this->readSizeInBytes();
            data.resize(nBytes);
            if (!data.empty()) this->read(&data[0], nBytes);
        }


        void TcpChannel::read(std::shared_ptr<std::vector<char>>& data) {
            const size_t nBytes = this->readSizeInBytes();
            data->resize(nBytes);
            if (!data->empty()) this->read(&(*data)[0], nBytes);
        }


        void TcpChannel::read(karabo::data::Hash& data) {
            std::vector<char> tmp;
            this->read(tmp);
            if (m_textSerializer) {
                m_textSerializer->load(data,
                                       reinterpret_cast<const char*>(&tmp[0])); // Fasted method (no copy via string)
            } else {
                m_binarySerializer->load(data, tmp);
            }
        }


        void TcpChannel::read(karabo::data::Hash& header, char* data, const size_t& size) {
            this->read(header);
            this->read(data, size);
        }


        void TcpChannel::read(karabo::data::Hash& header, std::vector<char>& data) {
            this->read(header);
            this->read(data);
        }


        void TcpChannel::read(karabo::data::Hash& header, std::shared_ptr<std::vector<char>>& data) {
            this->read(header);
            this->read(*data);
        }


        void TcpChannel::read(karabo::data::Hash& header, karabo::data::Hash& data) {
            this->read(header);
            this->read(data);
        }


        void TcpChannel::readAsyncSizeInBytes(const ReadSizeInBytesHandler& handler) {
            // This public interface has to ensure that we go at least once via EventLoop, i.e. we are asynchronous.
            readAsyncSizeInBytesImpl(handler, false);
        }


        void TcpChannel::readAsyncSizeInBytesImpl(const ReadSizeInBytesHandler& handler, bool allowNonAsync) {
            if (m_sizeofLength == 0)
                throw KARABO_LOGIC_EXCEPTION(
                      "Message's sizeTag size was configured to be 0. Thus, registration of this function does not "
                      "make sense!");
            m_inboundMessagePrefix.resize(m_sizeofLength);
            std::unique_lock<std::mutex> lock(m_socketMutex);
            if (allowNonAsync && m_socket.available() >= m_sizeofLength) {
                m_syncCounter++;
                boost::system::error_code ec;
                boost::asio::read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(), ec);
                lock.unlock();
                onSizeInBytesAvailable(ec, handler);
            } else {
                m_asyncCounter++;
                boost::asio::async_read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(),
                                        util::bind_weak(&karabo::net::TcpChannel::onSizeInBytesAvailable, this,
                                                        boost::asio::placeholders::error, handler));
            }
        }


        void TcpChannel::onSizeInBytesAvailable(const ErrorCode& e, const ReadSizeInBytesHandler& handler) {
            if (e) {
                bytesAvailableHandler(e);
                return;
            }

            size_t messageSize;
            _KARABO_VECTOR_TO_SIZE(m_inboundMessagePrefix, messageSize);

            m_readBytes += messageSize;

            handler(messageSize);
        }


        void TcpChannel::byteSizeAvailableHandler(const size_t byteSize) {
            m_inboundData->resize(byteSize);
            std::unique_lock<std::mutex> lock(m_socketMutex);
            if (m_socket.available() >= byteSize) {
                m_syncCounter++;
                boost::system::error_code ec;
                boost::asio::read(m_socket, buffer(m_inboundData->data(), byteSize), transfer_all(), ec);
                lock.unlock();
                bytesAvailableHandler(ec);
            } else {
                lock.unlock();
                m_asyncCounter++;
                this->readAsyncRawImpl(m_inboundData->data(), byteSize,
                                       util::bind_weak(&karabo::net::TcpChannel::bytesAvailableHandler, this, _1),
                                       true);
            }
        }


        void TcpChannel::readAsyncStringUntil(const std::string& terminator, const ReadStringHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            m_activeHandler = TcpChannel::STRING_UNTIL;
            m_readHandler = handler;
            m_asyncCounter++;
            // no header is expected so I directly register payload handler, i.e. stringAvailableHandler
            std::lock_guard<std::mutex> lock(m_socketMutex);
            boost::asio::async_read_until(
                  m_socket, m_streamBufferInbound, terminator,
                  util::bind_weak(&karabo::net::TcpChannel::stringAvailableHandler, this, _1, _2));
        }


        void TcpChannel::readAsyncRaw(char* data, const size_t& size, const ReadRawHandler& handler) {
            // This public interface has to ensure that we go at least once via EventLoop, i.e. we are asynchronous.
            readAsyncRawImpl(data, size, handler, false);
        }


        void TcpChannel::readAsyncRawImpl(char* data, const size_t& size, const ReadRawHandler& handler,
                                          bool allowNonAsync) {
            std::unique_lock<std::mutex> lock(m_socketMutex);
            if (allowNonAsync && m_socket.available() >= size) {
                m_syncCounter++;
                boost::system::error_code ec;
                const size_t rsize = boost::asio::read(m_socket, buffer(data, size), transfer_all(), ec);
                lock.unlock();
                onBytesAvailable(ec, rsize, handler);
            } else {
                m_asyncCounter++;
                boost::asio::async_read(m_socket, buffer(data, size), transfer_all(),
                                        util::bind_weak(&karabo::net::TcpChannel::onBytesAvailable, this,
                                                        boost::asio::placeholders::error,
                                                        boost::asio::placeholders::bytes_transferred, handler));
            }
        }

        void TcpChannel::onBytesAvailable(const ErrorCode& error, const size_t length, const ReadRawHandler& handler) {
            // Update the total read bytes count
            m_readBytes += length;
            handler(error);
        }


        void TcpChannel::readAsyncVector(const ReadVectorHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            m_activeHandler = TcpChannel::VECTOR;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(
                  util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncVectorPointer(const ReadVectorPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            this->readAsyncVectorPointerImpl(handler);
        }


        void TcpChannel::readAsyncVectorPointerImpl(const ReadVectorPointerHandler& handler) {
            m_activeHandler = TcpChannel::VECTOR_POINTER;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(
                  util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncString(const ReadStringHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            m_activeHandler = TcpChannel::STRING;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(
                  util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHash(const ReadHashHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            m_activeHandler = TcpChannel::HASH;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(
                  util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashPointer(const ReadHashPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            m_activeHandler = TcpChannel::HASH_POINTER;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(
                  util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        /////////////////////////////////   BufferSet handling  //////////////////////////////////////////////////////


        void TcpChannel::onVectorBufferSetPointerAvailable(const ErrorCode& e, size_t length,
                                                           const std::vector<karabo::io::BufferSet::Pointer>& buffers,
                                                           const ReadVectorBufferSetPointerHandler& handler) {
            m_readBytes += length;

            if (!e) {
                handler(e, buffers);
            } else {
                handler(e, std::vector<karabo::io::BufferSet::Pointer>());
            }
        }


        void TcpChannel::readAsyncVectorBufferSetPointerImpl(const std::vector<karabo::io::BufferSet::Pointer>& buffers,
                                                             const ReadVectorBufferSetPointerHandler& handler) {
            m_inboundMessagePrefix.clear();
            m_inboundMessagePrefix.resize(m_sizeofLength);
            std::vector<boost::asio::mutable_buffer> boostBuffers;
            boostBuffers.push_back(buffer(m_inboundMessagePrefix));
            karabo::io::BufferSet::appendTo(boostBuffers, buffers);

            size_t totalSize = m_inboundMessagePrefix.size();
            for (const auto& p : buffers) totalSize += p->totalSize();
            std::unique_lock<std::mutex> lock(m_socketMutex);
            if (m_socket.available() >= totalSize) {
                ++m_syncCounter;
                boost::system::error_code ec;
                const size_t rsize = boost::asio::read(m_socket, boostBuffers, transfer_all(), ec);
                lock.unlock();
                onVectorBufferSetPointerAvailable(ec, rsize, buffers, handler);
            } else {
                ++m_asyncCounter;
                boost::asio::async_read(
                      m_socket, boostBuffers, transfer_all(),
                      util::bind_weak(&TcpChannel::onVectorBufferSetPointerAvailable, this,
                                      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred,
                                      buffers, handler));
            }
        }


        void TcpChannel::onHashVectorBufferSetPointerRead(const boost::system::error_code& e,
                                                          const std::vector<karabo::io::BufferSet::Pointer>& buffers) {
            Hash::Pointer header = m_inHashHeader;
            m_inHashHeader.reset();
            // Reset handler variables - as in TcpChannel::bytesAvailableHandler
            m_activeHandler = TcpChannel::NONE;
            std::any readHandler;
            readHandler.swap(m_readHandler);

            std::any_cast<ReadHashVectorBufferSetPointerHandler>(readHandler)(e, *header, buffers);
        }


        void TcpChannel::onHashVectorBufferSetPointerVectorPointerRead(
              const boost::system::error_code& e, const std::shared_ptr<std::vector<char>>& data,
              const ReadHashVectorBufferSetPointerHandler& handler) {
            Hash::Pointer header = m_inHashHeader;
            m_inHashHeader.reset();
            m_activeHandler = TcpChannel::NONE;
            std::vector<karabo::io::BufferSet::Pointer> buffers;
            buffers.push_back(karabo::io::BufferSet::Pointer(new karabo::io::BufferSet(false)));
            if (!e) buffers[0]->emplaceBack(data);
            handler(e, *header, buffers);
        }


        void TcpChannel::readAsyncHashVectorBufferSetPointer(const ReadHashVectorBufferSetPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            m_activeHandler = TcpChannel::HASH_VECTOR_BUFFERSET_POINTER;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(
                  util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashVector(const ReadHashVectorHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            m_activeHandler = TcpChannel::HASH_VECTOR;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(
                  util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashVectorPointer(const ReadHashVectorPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            m_activeHandler = TcpChannel::HASH_VECTOR_POINTER;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(
                  util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashString(const ReadHashStringHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            m_activeHandler = TcpChannel::HASH_STRING;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(
                  util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashHash(const ReadHashHashHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            m_activeHandler = TcpChannel::HASH_HASH;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(
                  util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashPointerHashPointer(const ReadHashPointerHashPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE)
                throw KARABO_NETWORK_EXCEPTION(
                      "Multiple async read: You are allowed to register only exactly one asynchronous read or write "
                      "per channel.");
            m_activeHandler = TcpChannel::HASH_POINTER_HASH_POINTER;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(
                  util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }

        void TcpChannel::stringAvailableHandler(const boost::system::error_code& e, const size_t bytes_to_read) {
            HandlerType type = m_activeHandler;
            m_activeHandler = TcpChannel::NONE;
            auto readHandler = std::move(m_readHandler);

            if (type != TcpChannel::STRING_UNTIL) {
                throw KARABO_LOGIC_EXCEPTION("stringAvailableHandler called but handler type is " +
                                             str(boost::format("%1%") % type));
                return;
            }

            // allocate tmp string and copy m_streamBufferInbound content to it
            string tmp(boost::asio::buffers_begin(m_streamBufferInbound.data()),
                       boost::asio::buffers_begin(m_streamBufferInbound.data()) + bytes_to_read);

            m_streamBufferInbound.consume(bytes_to_read);

            m_readBytes += bytes_to_read;

            std::any_cast<ReadStringHandler>(readHandler)(e, tmp);
        }


        void TcpChannel::bytesAvailableHandler(const boost::system::error_code& e) {
            if (!e) {
                // Only parse header information
                if (m_readHeaderFirst) {
                    m_readHeaderFirst = false;
                    m_inboundData.swap(m_inboundHeader);
                    if (m_activeHandler == HASH_VECTOR_BUFFERSET_POINTER) {
                        m_inHashHeader = std::make_shared<Hash>();
                        this->prepareHashFromHeader(*m_inHashHeader);
                        if (m_inHashHeader->has("_bufferSetLayout_")) {
                            // This protocol for karabo 2.2.4 and later : c++ and bound python
                            std::vector<karabo::io::BufferSet::Pointer> buffers;

                            for (const karabo::data::Hash& layout :
                                 m_inHashHeader->get<std::vector<karabo::data::Hash>>("_bufferSetLayout_")) {
                                if (!layout.has("sizes") || !layout.has("types")) {
                                    throw KARABO_LOGIC_EXCEPTION("Pipeline Protocol violation!");
                                }
                                const auto& sizes = layout.get<vector<unsigned int>>("sizes");
                                const auto& types = layout.get<vector<int>>("types");
                                if (sizes.size() != types.size()) {
                                    throw KARABO_LOGIC_EXCEPTION("Pipeline Protocol violation!");
                                }
                                karabo::io::BufferSet::Pointer buffer(new karabo::io::BufferSet(false));
                                for (size_t ii = 0; ii < sizes.size(); ii++) buffer->add(sizes[ii], types[ii]);
                                buffers.push_back(buffer);
                            }
                            this->readAsyncVectorBufferSetPointerImpl(
                                  buffers,
                                  util::bind_weak(&TcpChannel::onHashVectorBufferSetPointerRead, this, _1, _2));
                        } else if (m_inHashHeader->has("byteSizes")) {
                            // This is protocol for middle-layer devices and, in general, for karabo pre-2.2.4
                            const auto& sizes = m_inHashHeader->get<std::vector<unsigned int>>("byteSizes");
                            std::vector<karabo::io::BufferSet::Pointer> buffers;

                            for (size_t ii = 0; ii < sizes.size(); ii++) {
                                karabo::io::BufferSet::Pointer buffer(new karabo::io::BufferSet(false));
                                buffer->add(sizes[ii], karabo::io::BufferSet::COPY);
                                buffers.push_back(buffer);
                            }
                            this->readAsyncVectorBufferSetPointerImpl(
                                  buffers,
                                  util::bind_weak(&TcpChannel::onHashVectorBufferSetPointerRead, this, _1, _2));
                        } else {
                            // No information from remote peer how the data should be structured .... so read it as one
                            // blob
                            KARABO_LOG_FRAMEWORK_WARN
                                  << "Received header for ReadHashVectorBufferSetPointerHandler lacks"
                                  << " both keys '_bufferSetLayout_' and 'byteSizes' - treat data as single BufferSet.";
                            ReadHashVectorBufferSetPointerHandler handler =
                                  std::any_cast<ReadHashVectorBufferSetPointerHandler>(m_readHandler);
                            m_readHandler = nullptr; // see also below about clearing/swapping m_readHandler
                            this->readAsyncVectorPointerImpl(util::bind_weak(
                                  &TcpChannel::onHashVectorBufferSetPointerVectorPointerRead, this, _1, _2, handler));
                        }
                        return;
                    }
                    // 'true' allows non-async shortcut to avoid event loop posts if data has already arrived
                    this->readAsyncSizeInBytesImpl(
                          util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), true);
                    return;
                }
            }
            // Reset handler variables since they usually are re-assigned when m_readHandler is called
            // by registering again via e.g. TcpChannel::readAsync[HashHash|HashString|String|...](handler).
            //
            // Clearing data member m_readHandler before calling the handler it holds protects against the case that a
            // shared pointer to this TcpChannel is bound to this handler and calling it does not re-assign
            // m_readHandler, e.g. due to an error condition like disconnection on remote side:
            // If m_readHandler would not be cleared, this TcpChannel would continue to keep a shared pointer to itself
            // and thus could never be destructed.
            HandlerType type = m_activeHandler;
            m_activeHandler = TcpChannel::NONE;
            std::any readHandler;
            readHandler.swap(m_readHandler);
            switch (type) {
                case VECTOR: {
                    if (!e) std::any_cast<ReadVectorHandler>(readHandler)(e, *m_inboundData);
                    else {
                        std::vector<char> vec;
                        std::any_cast<ReadVectorHandler>(readHandler)(e, vec);
                    }
                    break;
                }
                case VECTOR_POINTER: {
                    auto vec(std::make_shared<std::vector<char>>());
                    if (!e) vec.swap(m_inboundData);
                    std::any_cast<ReadVectorPointerHandler>(readHandler)(e, vec);
                    break;
                }
                case STRING: {
                    string tmp;
                    if (!e) tmp.assign(m_inboundData->begin(), m_inboundData->end());
                    std::any_cast<ReadStringHandler>(readHandler)(e, tmp);
                    break;
                }
                case STRING_UNTIL:
                    throw KARABO_LOGIC_EXCEPTION(
                          "bytesAvailableHandler called with STRING_UNTIL handler type registered");
                    break;
                case HASH: {
                    Hash h;
                    if (!e) this->prepareHashFromData(h);
                    std::any_cast<ReadHashHandler>(readHandler)(e, h);
                    break;
                }
                case HASH_POINTER: {
                    auto h(std::make_shared<Hash>());
                    if (!e) this->prepareHashFromData(*h);
                    std::any_cast<ReadHashPointerHandler>(readHandler)(e, h);
                    break;
                }

                case HASH_VECTOR: {
                    Hash header;
                    std::vector<char> inData;
                    if (!e) {
                        this->prepareHashFromHeader(header);
                        std::any_cast<ReadHashVectorHandler>(readHandler)(e, header, *m_inboundData);
                        break;
                    }
                    std::any_cast<ReadHashVectorHandler>(readHandler)(e, header, inData);
                    break;
                }
                case HASH_VECTOR_POINTER: {
                    Hash header;
                    auto inData(std::make_shared<std::vector<char>>());
                    if (!e) {
                        this->prepareHashFromHeader(header);
                        inData.swap(m_inboundData);
                    }
                    std::any_cast<ReadHashVectorPointerHandler>(readHandler)(e, header, inData);
                    break;
                }

                case HASH_STRING: {
                    Hash header;
                    string tmp;
                    if (!e) {
                        this->prepareHashFromHeader(header);
                        tmp.assign(m_inboundData->begin(), m_inboundData->end());
                    }
                    std::any_cast<ReadHashStringHandler>(readHandler)(e, header, tmp);
                    break;
                }

                case HASH_HASH: {
                    Hash header;
                    Hash body;
                    if (!e) {
                        this->prepareHashFromHeader(header);
                        this->prepareHashFromData(body);
                    }
                    std::any_cast<ReadHashHashHandler>(readHandler)(e, header, body);
                    break;
                }

                case HASH_POINTER_HASH_POINTER: {
                    auto header(std::make_shared<Hash>());
                    auto body(std::make_shared<Hash>());
                    if (!e) {
                        this->prepareHashFromHeader(*header);
                        this->prepareHashFromData(*body);
                    }
                    std::any_cast<ReadHashPointerHashPointerHandler>(readHandler)(e, header, body);
                    break;
                }

                case HASH_VECTOR_BUFFERSET_POINTER: {
                    // we will be here only if error code is not "success": "Operation canceled", "End of file"
                    std::any_cast<ReadHashVectorBufferSetPointerHandler>(readHandler)(
                          e, Hash(), std::vector<karabo::io::BufferSet::Pointer>());
                    break;
                }

                default:
                    throw KARABO_LOGIC_EXCEPTION("UNKNOWN HANDLER TYPE: " + str(boost::format("%1%") % type) +
                                                 " (this should never happen)");
            }
        }


        void TcpChannel::write(const char* data, const size_t& size) {
            try {
                boost::system::error_code error; // in case of error
                vector<const_buffer> buf;
                if (m_sizeofLength > 0) {
                    _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, size); // m_outboundMessagePrefix = size;
                    buf.push_back(buffer(m_outboundMessagePrefix));        // size of the prefix
                }
                buf.push_back(buffer(data, size)); // body

                std::lock_guard<std::mutex> lock(m_socketMutex);
                m_writtenBytes += boost::asio::write(m_socket, buf, transfer_all(), error);

                if (!error) {
                    return;
                } else {
                    try {
                        m_socket.close();
                    } catch (...) {
                    } // close the socket because of error on the network
                    throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message() +
                                                   ". Channel is closed!");
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }

        /// @brief  Helper to extend header for writing header and BufferSet pointers
        ///
        /// @return extended header
        static karabo::data::Hash extendHeaderForBufferSets(const karabo::data::Hash& hdr,
                                                            const std::vector<karabo::io::BufferSet::Pointer>& body) {
            // "byteSizes" and "nData" are kept for pre-karabo 2.2.4 backward compatibility and for middle layer
            if (hdr.has("nData") || hdr.has("byteSizes") || hdr.has("_bufferSetLayout_")) {
                KARABO_LOG_FRAMEWORK_WARN_C("TcpChannel")
                      << "Header contains 'nData', 'byteSizes' or '_bufferSetLayout_', but these "
                      << "will be overwritten by protocol!";
            }
            Hash header(hdr);

            std::vector<Hash>& buffersVector = header.bindReference<std::vector<Hash>>("_bufferSetLayout_");
            for (std::vector<karabo::io::BufferSet::Pointer>::const_iterator it = body.begin(); it != body.end();
                 ++it) {
                buffersVector.push_back(Hash("sizes", (*it)->sizes(), "types", (*it)->types()));
            }

            header.set<unsigned int>("nData", body.size());
            std::vector<unsigned int>& byteSizes = header.bindReference<std::vector<unsigned int>>("byteSizes");
            byteSizes.reserve(body.size());
            for (const auto& bufferSet : body) {
                byteSizes.push_back(bufferSet->totalSize());
            }

            return header;
        }


        void TcpChannel::write(const karabo::data::Hash& data) {
            try {
                if (m_textSerializer) {
                    string archive;
                    m_textSerializer->save(data, archive);
                    this->write(archive.c_str(), archive.size());
                } else {
                    std::vector<char> archive;
                    m_binarySerializer->save(data, archive);
                    this->write(&archive[0], archive.size());
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const karabo::data::Hash& header, const karabo::data::Hash& body) {
            try {
                if (m_textSerializer) {
                    string headerBuf;
                    string bodyBuf;
                    m_textSerializer->save(body, bodyBuf);
                    m_textSerializer->save(header, headerBuf);
                    this->write(headerBuf.c_str(), headerBuf.size(), bodyBuf.c_str(), bodyBuf.size());
                } else {
                    std::vector<char> headerBuf;
                    std::vector<char> bodyBuf;
                    m_binarySerializer->save(body, bodyBuf);
                    m_binarySerializer->save(header, headerBuf);
                    this->write(&headerBuf[0], headerBuf.size(), &bodyBuf[0], bodyBuf.size());
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const karabo::data::Hash& header, const char* data, const size_t& size) {
            try {
                if (m_sizeofLength == 0) {
                    throw KARABO_PARAMETER_EXCEPTION(
                          "With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const "
                          "size_t& size) instead.");
                }
                if (m_textSerializer) {
                    string headerBuf;
                    m_textSerializer->save(header, headerBuf);
                    write(headerBuf.c_str(), headerBuf.size(), data, size);
                } else {
                    std::vector<char> headerBuf;
                    m_binarySerializer->save(header, headerBuf);
                    write(&headerBuf[0], headerBuf.size(), data, size);
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const char* header, const size_t& headerSize, const char* body, const size_t& bodySize) {
            try {
                boost::system::error_code error; // in case of error

                if (m_sizeofLength == 0) {
                    throw KARABO_PARAMETER_EXCEPTION(
                          "With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const "
                          "size_t& size) instead.");
                }
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, headerSize);
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, bodySize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(header, headerSize));
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(body, bodySize));
                std::lock_guard<std::mutex> lock(m_socketMutex);
                m_writtenBytes += boost::asio::write(m_socket, buf, transfer_all(), error);
                if (error) {
                    try {
                        m_socket.close();
                    } catch (...) {
                    }
                    throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message() +
                                                   ". Channel is closed!");
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const karabo::data::Hash& hdr, const std::vector<karabo::io::BufferSet::Pointer>& body) {
            const Hash header(extendHeaderForBufferSets(hdr, body));

            try {
                boost::system::error_code error; // in case of error

                if (m_sizeofLength == 0) {
                    throw KARABO_PARAMETER_EXCEPTION(
                          "With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const "
                          "size_t& size) instead.");
                }
                if (m_textSerializer) {
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION(
                          "Text serialization is not implemented for vectors of BufferSets");
                }
                std::vector<char> headerBuf;
                m_binarySerializer->save(header, headerBuf);
                const size_t headerSize = headerBuf.size();
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, headerSize);
                size_t total_size = 0;
                for (const auto& b : body) total_size += b->totalSize();
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, total_size);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(headerBuf));
                buf.push_back(buffer(m_outboundMessagePrefix));
                karabo::io::BufferSet::appendTo(buf, body);
                std::lock_guard<std::mutex> lock(m_socketMutex);
                m_writtenBytes += boost::asio::write(m_socket, buf, transfer_all(), error);
                if (error) {
                    std::string local_ep_ip;
                    std::string remote_ep_ip;
                    try { // m_socket might not be able to provide that info anymore...
                        local_ep_ip = m_socket.local_endpoint().address().to_string();
                        remote_ep_ip = m_socket.remote_endpoint().address().to_string();
                    } catch (...) {
                    }
                    try {
                        m_socket.close();
                    } catch (...) {
                    }
                    throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message() +
                                                   ". Channel '" + local_ep_ip + "'->'" + remote_ep_ip +
                                                   "' is closed!");
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        // asynchronous write


        void TcpChannel::managedWriteAsync(const WriteCompleteHandler& handler) {
            try {
                vector<const_buffer> buf;
                if (m_sizeofLength > 0) {
                    size_t s = m_outboundData->size();
                    _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, s);
                    buf.push_back(buffer(m_outboundMessagePrefix));
                }
                buf.push_back(buffer(*m_outboundData));
                const unsigned int handlerIndex = storeCompleteHandler(handler);
                boost::asio::async_write(
                      m_socket, buf,
                      util::bind_weak(&TcpChannel::asyncWriteHandler, this, handlerIndex,
                                      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::managedWriteAsyncWithHeader(const WriteCompleteHandler& handler) {
            try {
                size_t hsize = m_outboundHeader->size(); // Header size
                size_t dsize = m_outboundData->size();   // Data size
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, hsize);
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, dsize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(*m_outboundHeader));
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(*m_outboundData));
                const unsigned int handlerIndex = storeCompleteHandler(handler);
                boost::asio::async_write(
                      m_socket, buf,
                      util::bind_weak(&TcpChannel::asyncWriteHandler, this, handlerIndex,
                                      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::unmanagedWriteAsync(const char* data, const size_t& size,
                                             const WriteCompleteHandler& handler) {
            try {
                vector<const_buffer> buf;
                if (m_sizeofLength > 0) {
                    _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, size);
                    buf.push_back(buffer(m_outboundMessagePrefix));
                }
                buf.push_back(buffer(data, size));
                const unsigned int handlerIndex = storeCompleteHandler(handler);
                boost::asio::async_write(
                      m_socket, buf,
                      util::bind_weak(&TcpChannel::asyncWriteHandler, this, handlerIndex,
                                      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::unmanagedWriteAsyncWithHeader(const char* data, const size_t& size,
                                                       const WriteCompleteHandler& handler) {
            try {
                size_t hsize = m_outboundHeader->size(); // Header size
                size_t dsize = m_outboundData->size();   // Data size
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, hsize);
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, dsize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(*m_outboundHeader));
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(data, size));
                const unsigned int handlerIndex = storeCompleteHandler(handler);
                boost::asio::async_write(
                      m_socket, buf,
                      util::bind_weak(&TcpChannel::asyncWriteHandler, this, handlerIndex,
                                      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::prepareHeaderFromHash(const karabo::data::Hash& hash) {
            if (m_textSerializer) {
                string archive;
                m_textSerializer->save(hash, archive);
                m_outboundHeader->assign(archive.begin(), archive.end());
            } else {
                m_binarySerializer->save(hash, *m_outboundHeader);
            }
        }


        void TcpChannel::prepareHashFromHeader(karabo::data::Hash& hash) const {
            if (m_textSerializer) {
                if (!m_inboundHeader->empty()) {
                    m_textSerializer->load(hash, &(*m_inboundHeader)[0], m_inboundHeader->size());
                }
            } else {
                m_binarySerializer->load(hash, *m_inboundHeader);
            }
        }


        void TcpChannel::prepareDataFromHash(const karabo::data::Hash& hash) {
            if (m_textSerializer) {
                string archive;
                m_textSerializer->save(hash, archive);
                m_outboundData->assign(archive.begin(), archive.end());
            } else {
                m_binarySerializer->save(hash, *m_outboundData);
            }
        }


        void TcpChannel::prepareDataFromHash(const karabo::data::Hash& hash,
                                             std::shared_ptr<std::vector<char>>& dataPtr) {
            if (m_textSerializer) {
                string archive;
                m_textSerializer->save(hash, archive);
                dataPtr->assign(archive.begin(), archive.end());
            } else {
                m_binarySerializer->save(hash, *dataPtr);
            }
        }


        void TcpChannel::prepareHashFromData(karabo::data::Hash& hash) const {
            if (m_textSerializer) {
                if (!m_inboundData->empty()) {
                    m_textSerializer->load(hash, &(*m_inboundData)[0], m_inboundData->size());
                }
            } else {
                m_binarySerializer->load(hash, *m_inboundData);
            }
        }


        void TcpChannel::writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
            try {
                if (m_manageAsyncData) {
                    m_outboundData->resize(size);
                    std::memcpy(&(*m_outboundData)[0], data, size);
                    this->managedWriteAsync(handler);
                } else {
                    this->unmanagedWriteAsync(data, size, handler);
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncVector(const vector<char>& data, const Channel::WriteCompleteHandler& handler) {
            try {
                this->writeAsyncRaw(&data[0], data.size(), handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncVectorPointer(const std::shared_ptr<vector<char>>& dataPtr,
                                                 const Channel::WriteCompleteHandler& handler) {
            try {
                if (!dataPtr)
                    throw KARABO_PARAMETER_EXCEPTION("Input parameter dataPtr is uninitialized shared pointer.");
                vector<const_buffer> buf;
                if (m_sizeofLength > 0) {
                    const size_t size = dataPtr->size();
                    _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, size);
                    buf.push_back(buffer(m_outboundMessagePrefix));
                }
                buf.push_back(buffer(*dataPtr));
                const unsigned int handlerIndex = storeCompleteHandler(handler);
                boost::asio::async_write(m_socket, buf,
                                         util::bind_weak(&TcpChannel::asyncWriteHandlerBody, this, handlerIndex,
                                                         boost::asio::placeholders::error,
                                                         boost::asio::placeholders::bytes_transferred, dataPtr));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHash(const Hash& hash, const Channel::WriteCompleteHandler& handler) {
            try {
                auto dataPtr(std::make_shared<std::vector<char>>());
                this->prepareDataFromHash(hash, dataPtr);
                this->writeAsyncVectorPointer(dataPtr, handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashRaw(const karabo::data::Hash& header, const char* data, const size_t& size,
                                           const WriteCompleteHandler& handler) {
            try {
                this->prepareHeaderFromHash(header);
                if (m_manageAsyncData) {
                    m_outboundData->resize(size);
                    std::memcpy(&(*m_outboundData)[0], data, size);
                    this->managedWriteAsyncWithHeader(handler);
                } else {
                    this->unmanagedWriteAsyncWithHeader(data, size, handler);
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashVector(const Hash& header, const std::vector<char>& data,
                                              const Channel::WriteCompleteHandler& handler) {
            try {
                this->writeAsyncHashRaw(header, &data[0], data.size(), handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHeaderBodyImpl(const std::shared_ptr<std::vector<char>>& header,
                                                  const std::shared_ptr<std::vector<char>>& body,
                                                  const Channel::WriteCompleteHandler& handler) {
            try {
                size_t hsize = header->size(); // Header size
                size_t dsize = body->size();   // Data size
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, hsize);
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, dsize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(*header));
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(*body));
                const unsigned int handlerIndex = storeCompleteHandler(handler);
                boost::asio::async_write(m_socket, buf,
                                         util::bind_weak(&TcpChannel::asyncWriteHandlerHeaderBody, this, handlerIndex,
                                                         boost::asio::placeholders::error,
                                                         boost::asio::placeholders::bytes_transferred, header, body));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashVectorPointer(const karabo::data::Hash& header,
                                                     const std::shared_ptr<std::vector<char>>& data,
                                                     const Channel::WriteCompleteHandler& handler) {
            try {
                if (!data) throw KARABO_PARAMETER_EXCEPTION("Input parameter dataPtr is uninitialized shared pointer.");
                auto headerPtr(std::make_shared<std::vector<char>>());
                this->prepareDataFromHash(header, headerPtr);
                writeAsyncHeaderBodyImpl(headerPtr, data, handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashHash(const karabo::data::Hash& header, const karabo::data::Hash& hash,
                                            const WriteCompleteHandler& handler) {
            try {
                auto dataPtr(std::make_shared<std::vector<char>>());
                this->prepareDataFromHash(hash, dataPtr);
                this->writeAsyncHashVectorPointer(header, dataPtr, handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashVectorBufferSetPointer(const karabo::data::Hash& hdr,
                                                              const std::vector<karabo::io::BufferSet::Pointer>& body,
                                                              const WriteCompleteHandler& handler) {
            if (m_sizeofLength == 0) {
                throw KARABO_PARAMETER_EXCEPTION(
                      "With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const "
                      "size_t& size) instead.");
            }
            if (m_textSerializer) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION(
                      "Text serialization is not implemented for vectors of BufferSets");
            }
            const Hash header(extendHeaderForBufferSets(hdr, body));
            try {
                auto headerBuf(std::make_shared<std::vector<char>>());
                m_binarySerializer->save(header, *headerBuf);
                const size_t headerSize = headerBuf->size();
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, headerSize);

                size_t total_size = 0;
                for (const auto& b : body) total_size += b->totalSize();
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, total_size);

                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(headerBuf->data(), headerSize));
                buf.push_back(buffer(m_outboundMessagePrefix));
                karabo::io::BufferSet::appendTo(buf, body);

                const unsigned int handlerIndex = storeCompleteHandler(handler);
                boost::asio::async_write(
                      m_socket, buf,
                      // It is documented that the buffers in 'body' must stay alive until 'handler' is called,
                      // so no need to keep them alive by binding something here as is done for 'headerBuf'.
                      util::bind_weak(&TcpChannel::asyncWriteHandlerBody, this, handlerIndex,
                                      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred,
                                      headerBuf));
            } catch (...) {
                KARABO_RETHROW
            }
        }

        unsigned int TcpChannel::storeCompleteHandler(const Channel::WriteCompleteHandler& handler) {
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

        void TcpChannel::asyncWriteHandler(unsigned int handlerIndex, const ErrorCode& e, const size_t length) {
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


        void TcpChannel::asyncWriteHandlerBody(unsigned int handlerIndex, const ErrorCode& e, const size_t length,
                                               const std::shared_ptr<std::vector<char>>& body) {
            asyncWriteHandler(handlerIndex, e, length);
        }


        void TcpChannel::asyncWriteHandlerHeaderBody(unsigned int handlerIndex, const ErrorCode& e, const size_t length,
                                                     const std::shared_ptr<std::vector<char>>& header,
                                                     const std::shared_ptr<std::vector<char>>& body) {
            asyncWriteHandler(handlerIndex, e, length);
        }


        // NOTE: There is the potential for a race condition here. However,
        // we are not worried about it. This method and `dataQuantityWritten`
        // should only be used for rough statistics gathering to be used
        // in understanding aggregate network usage. If a message here and there
        // gets overlooked, it's not a dealbreaker. The complexity needed to
        // protect access here is not worth the risk.


        size_t TcpChannel::dataQuantityRead() {
            const size_t ret = m_readBytes;
            m_readBytes = 0;
            return ret;
        }


        size_t TcpChannel::dataQuantityWritten() {
            const size_t ret = m_writtenBytes;
            m_writtenBytes = 0;
            return ret;
        }


        void TcpChannel::close() {
            std::lock_guard<std::mutex> lock(m_socketMutex);
            if (m_socket.is_open()) m_socket.cancel();
            m_socket.close();
        }


        bool TcpChannel::isOpen() {
            std::lock_guard<std::mutex> lock(m_socketMutex);
            return m_socket.is_open();
        }


        karabo::data::Hash TcpChannel::queueInfo() {
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


#undef _KARABO_VECTOR_TO_SIZE
#undef _KARABO_SIZE_TO_VECTOR


        void TcpChannel::setAsyncChannelPolicy(int priority, const std::string& new_policy, const size_t capacity) {
            std::string candidate = boost::to_upper_copy<std::string>(new_policy);
            std::lock_guard<std::mutex> lock(m_queueMutex);

            if (candidate == "LOSSLESS") {
                m_queue[priority] = Queue::Pointer(new LosslessQueue);

                if (capacity > 0) {
                    KARABO_LOG_FRAMEWORK_WARN << "Setting the max capacity of a LosslessQueue is not allowed!";
                }
            } else if (candidate == "REJECT_NEWEST") {
                m_queue[priority] =
                      Queue::Pointer(new RejectNewestQueue(capacity > 0 ? capacity : kDefaultQueueCapacity));
            } else if (candidate == "REMOVE_OLDEST") {
                m_queue[priority] =
                      Queue::Pointer(new RemoveOldestQueue(capacity > 0 ? capacity : kDefaultQueueCapacity));
            } else {
                throw KARABO_NOT_SUPPORTED_EXCEPTION(
                      "Trying to assign not supported channel policy : \"" + new_policy +
                      "\".  Supported policies are \"LOSSLESS\", \"REJECT_NEWEST\", \"REMOVE_OLDEST\"");
            }
        }


        void TcpChannel::prepareVectorFromHash(const karabo::data::Hash& hash, std::vector<char>& vec) {
            if (m_textSerializer) {
                string archive;
                m_textSerializer->save(hash, archive);
                vec.assign(archive.begin(), archive.end());
            } else {
                m_binarySerializer->save(hash, vec);
            }
        }


        void TcpChannel::prepareHashFromVector(const std::vector<char>& vec, karabo::data::Hash& hash) const {
            if (m_textSerializer) {
                m_textSerializer->load(hash, &vec[0], vec.size());
            } else {
                m_binarySerializer->load(hash, vec);
            }
        }


        void TcpChannel::dispatchWriteAsync(const Message::Pointer& mp, int prio) {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_queue[prio]->push_back(mp);

            if (!m_writeInProgress) {
                m_writeInProgress = true;
                EventLoop::getIOService().post(bind_weak(&TcpChannel::doWrite, this));
            }
        }


        void TcpChannel::doWrite() {
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

                std::lock_guard<std::mutex> lock(m_socketMutex);

                if (m_socket.is_open()) {
                    vector<const_buffer> buf;

                    if (mp->header()) {
                        VectorCharPointer hdr = mp->header();
                        m_headerSize = hdr->size();
                        buf.push_back(buffer(&m_headerSize, sizeof(unsigned int)));
                        buf.push_back(buffer(*hdr));
                    }

                    const karabo::io::BufferSet::Pointer& data = mp->body();
                    m_bodySize = data->totalSize();
                    buf.push_back(buffer(&m_bodySize, sizeof(unsigned int)));
                    data->appendTo(buf);

                    boost::asio::async_write(
                          m_socket, buf,
                          util::bind_weak(&TcpChannel::doWriteHandler, this, mp, boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred, queueIndex));
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "TcpChannel::doWrite exception : " << e.what();
                m_writeInProgress = false;
            }
        }


        void TcpChannel::doWriteHandler(Message::Pointer& mp, boost::system::error_code ec, const size_t length,
                                        const int queueIndex) {
            // Update the total written bytes counts
            m_queueWrittenBytes[queueIndex] += length;
            m_writtenBytes += length;

            if (!ec) {
                doWrite();
            } else {
                m_writeInProgress = false;
                try {
                    std::lock_guard<std::mutex> lock(m_socketMutex);
                    m_socket.close();
                } catch (...) {
                }
                KARABO_LOG_FRAMEWORK_ERROR << "TcpChannel::doWriteHandler error : " << ec.value() << " -- "
                                           << ec.message() << "  --  Channel is closed now!";
            }
        }


        karabo::io::BufferSet::Pointer TcpChannel::bufferSetFromPointerToChar(const char* data, size_t size) {
            std::shared_ptr<char> spCharBuff(new char[size], std::default_delete<char[]>());
            std::memcpy(spCharBuff.get(), data, size);
            karabo::data::ByteArray bArray = std::make_pair(spCharBuff, size);
            // BufferSet's copyAllData ctor parameter set to false to prevent yet another copy (besides the one
            // above).
            karabo::io::BufferSet::Pointer pBuffSet = std::make_shared<karabo::io::BufferSet>(false);
            pBuffSet->emplaceBack(bArray, false);
            return pBuffSet;
        }


        karabo::io::BufferSet::Pointer TcpChannel::bufferSetFromString(const std::string& str) {
            std::shared_ptr<char> spCharBuff(new char[str.size()], std::default_delete<char[]>());
            std::copy(str.begin(), str.end(), spCharBuff.get());
            karabo::data::ByteArray bArray = std::make_pair(spCharBuff, static_cast<size_t>(str.size()));
            // BufferSet's copyAllData ctor parameter set to false to prevent yet another copy (besides the one
            // above).
            karabo::io::BufferSet::Pointer pBuffSet = std::make_shared<karabo::io::BufferSet>(false);
            pBuffSet->emplaceBack(bArray, false);
            return pBuffSet;
        }


        karabo::io::BufferSet::Pointer TcpChannel::bufferSetFromVectorCharPointer(const VectorCharPointer& dataVect) {
            // BufferSet's copyAllData ctor parameter set to false to prevent copy.
            karabo::io::BufferSet::Pointer datapBs = std::make_shared<karabo::io::BufferSet>(false);
            datapBs->emplaceBack(dataVect);
            return datapBs;
        }


        karabo::io::BufferSet::Pointer TcpChannel::bufferSetFromHash(const karabo::data::Hash& data, bool copyAllData) {
            karabo::io::BufferSet::Pointer pBS = std::make_shared<karabo::io::BufferSet>(copyAllData);

            if (m_binarySerializer) {
                m_binarySerializer->save(data, *pBS);
            } else {
                std::string serializedData = m_textSerializer->save(data);
                pBS = bufferSetFromString(serializedData);
            }
            return pBS;
        }


        void TcpChannel::writeAsync(const char* data, const size_t& dsize, int prio) {
            auto datap = bufferSetFromPointerToChar(data, dsize);
            Message::Pointer mp = std::make_shared<Message>(datap);
            dispatchWriteAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const vector<char>& data, int prio) {
            if (data.empty()) {
                KARABO_LOG_FRAMEWORK_INFO << "Skip writeAsync of empty data.";
            } else {
                writeAsync(&(data[0]), data.size(), prio);
            }
        }


        void TcpChannel::writeAsync(const VectorCharPointer& datap, int prio) {
            karabo::io::BufferSet::Pointer datapBs = bufferSetFromVectorCharPointer(datap);
            Message::Pointer mp = std::make_shared<Message>(datapBs);
            dispatchWriteAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const std::string& data, int prio) {
            auto datap = bufferSetFromString(data);
            Message::Pointer mp = std::make_shared<Message>(datap);
            dispatchWriteAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::data::Hash& data, int prio, bool copyAllData) {
            auto datap = bufferSetFromHash(data, copyAllData);
            Message::Pointer mp = std::make_shared<Message>(datap);
            dispatchWriteAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::data::Hash& header, const char* data, const size_t& dsize, int prio) {
            auto datap = bufferSetFromPointerToChar(data, dsize);
            VectorCharPointer headerp(new std::vector<char>());
            prepareVectorFromHash(header, *headerp);
            Message::Pointer mp = std::make_shared<Message>(datap, headerp);
            dispatchWriteAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::data::Hash& header, const vector<char>& data, int prio) {
            VectorCharPointer vecPtr(new std::vector<char>(data));
            auto datap = bufferSetFromVectorCharPointer(vecPtr);
            VectorCharPointer headerp(new std::vector<char>());
            prepareVectorFromHash(header, *headerp);
            Message::Pointer mp = std::make_shared<Message>(datap, headerp);
            dispatchWriteAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::data::Hash& header, const VectorCharPointer& datap, int prio) {
            karabo::io::BufferSet::Pointer datapBs = bufferSetFromVectorCharPointer(datap);
            VectorCharPointer headerp(new std::vector<char>());
            prepareVectorFromHash(header, *headerp);
            Message::Pointer mp = std::make_shared<Message>(datapBs, headerp);
            dispatchWriteAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::data::Hash& header, const std::string& data, int prio) {
            auto datap = bufferSetFromPointerToChar(data.c_str(), data.length());
            VectorCharPointer headerp(new std::vector<char>());
            prepareVectorFromHash(header, *headerp);
            Message::Pointer mp = std::make_shared<Message>(datap, headerp);
            dispatchWriteAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::data::Hash& header, const karabo::data::Hash& data, int prio,
                                    bool copyAllData) {
            VectorCharPointer headerp(new std::vector<char>());
            prepareVectorFromHash(header, *headerp);
            auto datap = bufferSetFromHash(data, copyAllData);
            Message::Pointer mp = std::make_shared<Message>(datap, headerp);
            dispatchWriteAsync(mp, prio);
        }

        void TcpChannel::applySocketKeepAlive() {
            // See TcpConnection::expectedParameters(..) for the keys in m_keepAliveSettings
            if (m_keepAliveSettings.get<bool>("enabled")) {
                // Taken from
                // https://stackoverflow.com/questions/16568672/boost-asio-how-can-a-server-know-if-a-client-is-still-connected
                const boost::asio::socket_base::keep_alive option(true);
                m_socket.set_option(option);
                // The following is linux (or rather POSIX) only...
                const int socketId = m_socket.native_handle();
                int flag = m_keepAliveSettings.get<int>("toleratedSilence");
                if (setsockopt(socketId, SOL_TCP, TCP_KEEPIDLE, &flag, sizeof(flag))) {
                    KARABO_LOG_FRAMEWORK_WARN << "Failed to apply Tcp keep-alive 'toleratedSilence'";
                }
                flag = m_keepAliveSettings.get<int>("interval");
                if (setsockopt(socketId, SOL_TCP, TCP_KEEPINTVL, &flag, sizeof(flag))) {
                    KARABO_LOG_FRAMEWORK_WARN << "Failed to apply Tcp keep-alive 'interval'";
                }
                flag = m_keepAliveSettings.get<int>("numProbes");
                if (setsockopt(socketId, SOL_TCP, TCP_KEEPCNT, &flag, sizeof(flag))) {
                    KARABO_LOG_FRAMEWORK_WARN << "Failed to apply Tcp keep-alive 'numProbes'";
                }
            }
        }

        void TcpChannel::socketConnect(const boost::asio::ip::tcp::endpoint& endpoint) {
            // part of sync start of connection for client
            std::lock_guard<std::mutex> lock(m_socketMutex);
            m_socket.connect(endpoint);
            applySocketKeepAlive();
        }


        void TcpChannel::acceptSocket(boost::asio::ip::tcp::acceptor& acceptor) {
            // part of sync start of connection for server
            std::lock_guard<std::mutex> lock(m_socketMutex);
            acceptor.accept(m_socket);
            applySocketKeepAlive();
        }


        std::string TcpChannel::remoteAddress() const {
            std::lock_guard<std::mutex> lock(m_socketMutex);
            std::string address("unknown");
            if (m_socket.is_open()) {
                try {
                    address = boost::lexical_cast<std::string>(m_socket.remote_endpoint());
                } catch (...) {
                }
            }
            return address;
        }


        karabo::data::Hash TcpChannel::_getChannelInfo() {
            karabo::data::Hash info("localAddress", "?", "localPort", uint16_t(0), "remoteAddress", "?", "remotePort",
                                    uint16_t(0));
            try {
                std::lock_guard<std::mutex> lock(m_socketMutex);
                info.set<std::string>("localAddress", m_socket.local_endpoint().address().to_string());
                info.set<unsigned short>("localPort", m_socket.local_endpoint().port());
                if (m_socket.is_open()) {
                    info.set<std::string>("remoteAddress", m_socket.remote_endpoint().address().to_string());
                    info.set<unsigned short>("remotePort", m_socket.remote_endpoint().port());
                }
            } catch (...) {
            }
            return info;
        }
    } // namespace net
} // namespace karabo
