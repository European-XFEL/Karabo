/* 
 * File:   TcpChannel.hh
 * Author: esenov
 *
 * Created on June 3, 2011, 6:01 PM
 */

#ifndef KARABO_NET_TCPCHANNEL_HH
#define	KARABO_NET_TCPCHANNEL_HH

#include <boost/enable_shared_from_this.hpp>

#include <karabo/io/TextSerializer.hh>
#include <karabo/io/BinarySerializer.hh>

#include "Channel.hh"
#include "TcpConnection.hh"

namespace karabo {
    namespace net {

        class TcpChannel : public Channel, public boost::enable_shared_from_this<TcpChannel> {

            enum HandlerType {

                RAW,
                VECTOR,
                STRING
            };

            TcpConnection::Pointer m_connectionPointer;

            HandlerType m_activeHandler;

            ReadRawHandler m_rawHandler;
            ReadVectorHandler m_vectorHandler;
            ReadStringHandler m_stringHandler;

            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_textSerializer;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;

        public:

            KARABO_CLASSINFO(TcpChannel, "TcpChannel", "1.0")

            TcpChannel(const TcpConnection::Pointer& connection);

            virtual ~TcpChannel();

            Connection::Pointer getConnection() const {
                return m_connectionPointer;
            }

            /**
             * Synchronously reads the TCP message's size.
             * Will block until a message arrives on the socket.
             * @return Size in bytes of incoming TCP message
             */
            size_t readSizeInBytes();

            /**
             * In case a TCP message arrived, handler will be called back
             * The handler will inform about the number of bytes going to come in
             * The handler must have the following signature:
             * void handler(Channel::Pointer, const size_t&)
             * @param handler Callback with signature: void (Channel::Pointer, const size_t&)
             */
            void readAsyncSizeInBytes(const ReadSizeInBytesCompleted& handler);

            /**************************************************************/
            /*              Synchronous Read - No Header                  */
            /**************************************************************/
            
            /**
             * Synchronously reads size bytes from the TCP socket into data.
             * @param data Pre-allocated contiguous block of memory
             * @param size This number of bytes will be copied into data
             */
            void read(char* data, const size_t& size);

            /**
             * This function calls the corresponding handler
             * @param handler
             * @param byteSize
             * @param error
             */
            void onSizeInBytesAvailable(const ReadSizeInBytesCompleted& handler, const size_t byteSize, const ErrorCode& error);

            /**
             * Internal default handler
             * @param channel
             * @param byteSize
             */
            void byteSizeAvailableHandler(Channel::Pointer channel, const size_t byteSize);


            /**
             * Asynchronously reads size number of bytes into pre-allocated data buffer
             * A handler can be registered to inform about completion of writing
             * @param data
             * @param size
             * @param handler
             */
            void readAsyncRaw(char* data, const size_t& size, const ReadRawCompleted& handler);

            void onBytesAvailable(const ReadRawCompleted& handler, const ErrorCode& error);

            /**
             * Internal default handler
             * @param channel
             */
            void bytesAvailableHandler(Channel::Pointer channel);

            void readAsyncRaw(char* data, size_t& size, const ReadRawHandler& handler);

            void write(const char* data, const size_t& size);

            void write(const char* data, const size_t& size, const karabo::util::Hash& header);

            void writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler);

            void writeAsyncVector(const std::vector<char>& data, const WriteCompleteHandler& handler);

            void writeAsyncHash(const karabo::util::Hash& data, const WriteCompleteHandler& handler);

            void writeAsyncRawHash(const char* data, const size_t& size, const karabo::util::Hash& header, const WriteCompleteHandler& handler);

            void writeAsyncVectorHash(const std::vector<char>& data, const karabo::util::Hash& header, const WriteCompleteHandler& handler);

            void writeAsyncStringHash(const std::string& data, const karabo::util::Hash& header, const WriteCompleteHandler& handler);

            void writeAsyncHashHash(const karabo::util::Hash& data, const karabo::util::Hash& header, const WriteCompleteHandler& handler);

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

            virtual void managedWriteAsync(const WriteCompleteHandler& handler);

            virtual void unmanagedWriteAsync(const char* data, const size_t& size, const WriteCompleteHandler& handler);

            virtual void managedWriteAsyncWithHeader(const WriteCompleteHandler& handler);

            virtual void unmanagedWriteAsyncWithHeader(const char* data, const size_t& size, const WriteCompleteHandler& handler);

            virtual void prepareHeaderFromHash(const karabo::util::Hash& hash);

            virtual void prepareDataFromHash(const karabo::util::Hash& hash);


            void read(char*& data, size_t& size, char*& hdr, size_t& hsize);
            void write(const char* data, const size_t& size, const char* hdr, const std::string::size_type& hsize);

            void asyncWriteHandler(const Channel::WriteCompleteHandler& handler, const ErrorCode& e);
            void asyncWaitHandler(const Channel::WriteCompleteHandler& handler, const ErrorCode& e);

        private:


            boost::asio::ip::tcp::socket m_socket;
            boost::asio::deadline_timer m_timer;
            std::vector<char> m_inboundMessagePrefix;
            std::vector<char> m_inboundHeaderPrefix;
            std::vector<char> m_inboundData;
            std::vector<char> m_inboundHeader;
            std::vector<char> m_outboundMessagePrefix;
            std::vector<char> m_outboundHeaderPrefix;
            std::vector<char> m_outboundData;
            std::vector<char> m_outboundHeader;
            ErrorHandler m_errorHandler;
        };
    }
}

#endif	/* KARABO_NET_TCPCHANNEL_HH */

