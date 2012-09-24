/* 
 * File:   TcpChannel.hh
 * Author: esenov
 *
 * Created on June 3, 2011, 6:01 PM
 */

#ifndef EXFEL_NET_TCPCHANNEL_HH
#define	EXFEL_NET_TCPCHANNEL_HH

#include <boost/enable_shared_from_this.hpp>
#include "Channel.hh"
#include "TcpConnection.hh"



namespace exfel {
    namespace net {

        class TcpChannel : public Channel, public boost::enable_shared_from_this<TcpChannel> {
        public:

            EXFEL_CLASSINFO(TcpChannel, "TcpChannel", "1.0")

            typedef boost::shared_ptr<TcpChannel> Pointer;

            TcpChannel(TcpConnection& connection);
            virtual ~TcpChannel();

            TcpChannel::Pointer channel() {
                return shared_from_this();
            }

            virtual void read(char*& data, size_t& size);
            virtual void read(char*& data, size_t& size, exfel::util::Hash& header);

            virtual void readAsyncRaw(char*& data, size_t& size, const ReadRawHandler& handler);
            virtual void readAsyncVector(const Channel::ReadVectorHandler& handler);
            virtual void readAsyncString(const Channel::ReadStringHandler& handler);
            virtual void readAsyncHash(const Channel::ReadHashHandler& handler);
            virtual void readAsyncVectorHash(const ReadVectorHashHandler& handler);
            virtual void readAsyncStringHash(const ReadStringHashHandler& handler);

            virtual void write(const char* data, const size_t& size);
            virtual void write(const char* data, const size_t& size, const exfel::util::Hash& header);

            virtual void writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler);
            virtual void writeAsyncVector(const std::vector<char>& data, const WriteCompleteHandler& handler);
            virtual void writeAsyncString(const std::string& data, const WriteCompleteHandler& handler);
            virtual void writeAsyncHash(const exfel::util::Hash& data, const WriteCompleteHandler& handler);
            virtual void writeAsyncRawHash(const char* data, const size_t& size, const exfel::util::Hash& header, const WriteCompleteHandler& handler);
            virtual void writeAsyncVectorHash(const std::vector<char>& data, const exfel::util::Hash& header, const WriteCompleteHandler& handler);
            virtual void writeAsyncStringHash(const std::string& data, const exfel::util::Hash& header, const WriteCompleteHandler& handler);

            virtual void waitAsync(int milliseconds, const WaitHandler& handler);

            virtual void setErrorHandler(const ErrorHandler& handler) {
                m_errorHandler = handler;
            }
            virtual void close();
            virtual bool isOpen();

            boost::asio::ip::tcp::socket& socket() {
                return m_socket;
            }

        private:

            void read(char*& data, size_t& size, char*& hdr, size_t& hsize);
            void write(const char* data, const size_t& size, const char* hdr, const std::string::size_type& hsize);

            void readRawPrefixHandler(char*& data, size_t& size, const ReadRawHandler& handler, const ErrorCode& e, size_t bytes_transferred);
            void asyncReadRawHandler(char*& data, size_t& size, const ReadRawHandler& handler, const ErrorCode& e, size_t bytes_transferred);
            void readVectorPrefixHandler(const Channel::ReadVectorHandler& handler, const ErrorCode& e, size_t bytes_transferred);
            void readStringPrefixHandler(const Channel::ReadStringHandler& handler, const ErrorCode& e, size_t bytes_transferred);
            void readHashPrefixHandler(const Channel::ReadHashHandler& handler, const ErrorCode& e, size_t bytes_transferred);
            void readStringHashPrefixHandler(const Channel::ReadStringHashHandler& handler, const ErrorCode& e, size_t bytes_transferred);
            void asyncReadVectorHandler(const Channel::ReadVectorHandler& handler, const boost::system::error_code& e, size_t bytes_transferred);
            void asyncReadStringHandler(const Channel::ReadStringHandler& handler, const boost::system::error_code& e, size_t bytes_transferred);
            void asyncReadHashHandler(const Channel::ReadHashHandler& handler, const boost::system::error_code& e, size_t bytes_transferred);
            void asyncReadStringHashHandler(const Channel::ReadStringHashHandler& handler, const boost::system::error_code& e, size_t bytes_transferred);
            void readPrefixHandler(const Channel::ReadVectorHashHandler& handler, const ErrorCode& ec, std::size_t bytes_transferred);
            void asyncReadHandler(const Channel::ReadVectorHashHandler& handler, const ErrorCode& e, std::size_t bytes_transferred);
            void writeAsyncVectorVector(const std::vector<char>&, const std::vector<char>&, const Channel::WriteCompleteHandler& handler);
            void asyncWriteHandler(const Channel::WriteCompleteHandler& handler, const ErrorCode& e);
            void asyncWaitHandler(const Channel::WriteCompleteHandler& handler, const ErrorCode& e);

        private:


            boost::asio::ip::tcp::socket m_socket;
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

#endif	/* EXFEL_NET_TCPCHANNEL_HH */

