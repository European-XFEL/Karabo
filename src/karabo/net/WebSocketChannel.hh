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
#pragma once

#include <atomic>
#include <boost/asio/dispatch.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <map>
#include <memory>
#include <string>

#include "Channel.hh"
#include "Connection.hh"
#include "Queues.hh"
#include "WebSocketConnection.hh"
#include "karabo/data/io/BinarySerializer.hh"
#include "karabo/data/types/Hash.hh"


namespace karabo::net {

    class WebSocketChannel : public Channel {
        enum HandlerType { NONE = 0, HASH = 4 };

        std::shared_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>> m_ws; // websocket ptr
        Connection::WeakPointer m_connectionPointer;
        karabo::data::BinarySerializer<karabo::data::Hash>::Pointer m_binarySerializer;
        std::any m_readHandler;
        HandlerType m_activeHandlerType;
        boost::beast::flat_buffer m_buffer;
        std::vector<boost::asio::const_buffer> m_vbuf;
        std::mutex m_writeCompleteHandlersMutex;
        std::map<unsigned int, WriteCompleteHandler> m_writeCompleteHandlers;
        std::mutex m_queueMutex;
        std::vector<karabo::net::Queue::Pointer> m_queue;
        std::vector<size_t> m_queueWrittenBytes;
        std::size_t m_readBytes;
        std::size_t m_writtenBytes;
        std::atomic<bool> m_writeInProgress;
        unsigned int m_messageSize;

       public:
        KARABO_CLASSINFO(WebSocketChannel, "WebSocketChannel", "1.0")

        explicit WebSocketChannel(std::shared_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>>&& ws,
                                  Connection::WeakPointer connection);

        virtual ~WebSocketChannel();

        /**
         * Pointer to connection of this channel - empty if that is not alive anymore
         */
        Connection::Pointer getConnection() const override {
            return m_connectionPointer.lock();
        }

        boost::asio::any_io_executor get_executor() {
            return m_ws->get_executor();
        }

        void readAsyncHash(const ReadHashHandler& handler) override;

        void writeAsyncHash(const karabo::data::Hash& data, const WriteCompleteHandler& handler) override;

        /**
         *  When copyAllData is false, elements of type NDArray in the hash won't be copied before being sent.
         */
        void writeAsync(const karabo::data::Hash& data, int prio = 4, bool copyAllData = false);

        virtual size_t dataQuantityRead();

        virtual size_t dataQuantityWritten();

        void close() override;

        virtual bool isOpen();

        /**
         * Records the sizes of the write queues in a Hash.
         * Useful for debugging devices with multiple channels open (like the GuiServerDevice...)
         */
        karabo::data::Hash queueInfo() override;

        /**
         * Address of the remote endpoint
         */
        std::string remoteAddress() const override;


        virtual void setAsyncChannelPolicy(int priority, const std::string& policy, const size_t capacity = 0);

        /**
         * This function returns low level info about connection like ...
         * "localIp", "localPort", "remoteIp", "remotePort" that constitute active connection.
         * in form of Hash container
         * @param ptr input WebSocketChannel boost shared pointer
         * @return Hash with 4 key/value pairs
         */
        static karabo::data::Hash getChannelInfo(const std::shared_ptr<karabo::net::WebSocketChannel>& ptr);

       private:
        karabo::data::Hash _getChannelInfo();

        void doRead();

        void onRead(boost::beast::error_code ec, std::size_t bytes_transferred);

        void dispatchWriteAsync(const Message::Pointer& mp, int prio);

        void doWrite();

        void onWrite(const Message::Pointer& mp, boost::beast::error_code ec, std::size_t bytes_transferred,
                     int queueIndex);

        unsigned int storeCompleteHandler(const WriteCompleteHandler& handler);

        void asyncWriteHandler(const ErrorCode& e, const size_t length, unsigned int handlerIndex,
                               const std::shared_ptr<std::vector<char>>& data);
    };

} // namespace karabo::net
