/* 
 * File:   UdpChannel.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 17, 2011, 3:40 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_NET_UDPCHANNEL_HH
#define	KARABO_NET_UDPCHANNEL_HH

#include <boost/enable_shared_from_this.hpp>
#include "Channel.hh"
#include "UdpConnection.hh"



namespace karabo {
    namespace net {

        class UdpChannel : public Channel, public boost::enable_shared_from_this<UdpChannel> {
        public:

            KARABO_CLASSINFO(UdpChannel, "UdpChannel", "1.0")

            typedef boost::shared_ptr<UdpChannel> Pointer;

            UdpChannel(UdpConnection& connection);
            virtual ~UdpChannel();

            UdpChannel::Pointer channel() {
                return shared_from_this();
            }

            virtual void read(char*& data, size_t& size);
            virtual void read(std::vector<char>& data);
            virtual void readAsyncRaw(char*& data, size_t& size, const ReadRawHandler& handler);
            virtual void readAsyncVector(const ReadVectorHandler& handler);

            virtual void write(const char* data, const size_t& size);
            virtual void write(const std::vector<char>& data);
            virtual void writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler);
            virtual void writeAsyncVector(const std::vector<char>& data, const WriteCompleteHandler& handler);

            virtual void waitAsync(int milliseconds, const WaitHandler& handler);

            virtual void setErrorHandler(const ErrorHandler& handler) {
                m_errorHandler = handler;
            }
            virtual void close();
            virtual bool isOpen();

            boost::asio::ip::udp::socket& socket() {
                return m_socket;
            }

            boost::asio::ip::udp::endpoint& channelEndpoint() {
                return m_remote_endpoint;
            }

            void setChannelEndpoint(const boost::asio::ip::udp::endpoint& remote) {
                m_remote_endpoint = remote;
            }

        private:

            void readRawPrefixHandler(char*& data, size_t& size, const ReadRawHandler& handler, const ErrorCode& e, size_t bytes_transferred);
            void asyncReadRawHandler(char*& data, size_t& size, const ReadRawHandler& handler, const ErrorCode& e, size_t bytes_transferred);
            void readVectorPrefixHandler(const Channel::ReadVectorHandler& handler, const ErrorCode& e, size_t bytes_transferred);
            void asyncReadVectorHandler(const Channel::ReadVectorHandler& handler, const boost::system::error_code& e, size_t bytes_transferred);
            void asyncWriteHandler(const Channel::WriteCompleteHandler& handler, const ErrorCode& e);
            void asyncWaitHandler(const Channel::WriteCompleteHandler& handler, const ErrorCode& e);

        private:

            boost::asio::ip::udp::socket& m_socket;
            boost::asio::ip::udp::endpoint& m_remote_endpoint;
            std::size_t m_MaxLength;
            boost::asio::deadline_timer m_timer;
            std::size_t m_inboundMessagePrefix;
            std::size_t m_inboundHeaderPrefix;
            std::vector<char> m_inboundData;
            std::vector<char> m_inboundHeader;
            std::size_t m_outboundMessagePrefix;
            std::size_t m_outboundHeaderPrefix;
            std::vector<char> m_outboundData;
            std::vector<char> m_outboundHeader;
            ErrorHandler m_errorHandler;
        };
    }
}

#endif	/* KARABO_NET_UDPCHANNEL_HH */

