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
                NONE,
                VECTOR,
                STRING,
                HASH,
                HASH_VECTOR,
                HASH_STRING,
                HASH_HASH,
                VECTOR_POINTER,
                HASH_VECTOR_POINTER,
            };
            
            TcpConnection::Pointer m_connectionPointer;
            boost::asio::ip::tcp::socket m_socket;
            boost::asio::deadline_timer m_timer;
            HandlerType m_activeHandler;
            ErrorHandler m_errorHandler;
            bool m_readHeaderFirst;
            boost::any m_readHandler;            
            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_textSerializer;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;
            
            std::vector<char> m_inboundMessagePrefix;
            std::vector<char> m_inboundHeaderPrefix;
            boost::shared_ptr<std::vector<char> > m_inboundData;
            boost::shared_ptr<std::vector<char> > m_inboundHeader;
            std::vector<char> m_outboundMessagePrefix;
            std::vector<char> m_outboundHeaderPrefix;
            boost::shared_ptr<std::vector<char> > m_outboundData;
            boost::shared_ptr<std::vector<char> > m_outboundHeader;
            
        public:

            KARABO_CLASSINFO(TcpChannel, "TcpChannel", "1.0")

            TcpChannel(Connection::Pointer connection);

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
            void readAsyncSizeInBytes(const ReadSizeInBytesHandler& handler);

            /**************************************************************/
            /*              Synchronous Read - No Header                  */
            /**************************************************************/

            /**
             * Synchronously reads size bytes from TCP socket into data.
             * @param data Pre-allocated contiguous block of memory
             * @param size This number of bytes will be copied into data
             */
            void read(char* data, const size_t& size);

            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the data record is read.
             * The vector will be updated accordingly (must not be pre-allocated before)
             * @return void 
             */
            void read(std::vector<char>& data);

            /**
             * This function reads from a channel into shared pointer of vector of chars 
             * The reading will block until the data record is read.
             * The shared pointer of vector will be updated accordingly (must not be pre-allocated before)
             * @return void 
             */
            void read(boost::shared_ptr<std::vector<char> >& data);
            
            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the data record is read.
             * The size of data record is the first 4 bytes in a channel stream.
             * The hash will be updated accordingly.
             * @return void 
             */
            void read(karabo::util::Hash& data);

            /**
             * Synchronously reads size bytes from socket into data and provides a header.
             * @param header Hash object which will be updated to contain header information
             * @param data Pre-allocated contiguous block of memory
             * @param size This number of bytes will be copied into data
             */
            void read(karabo::util::Hash& header, char* data, const size_t& size);

            /**
             * This function reads into a header and a vector of chars. 
             * The reading will block until the data record is read.
             * @param header Hash object which will be updated to contain header information
             * @param data A vector which will be updated accordingly
             */
            void read(karabo::util::Hash& header, std::vector<char>& data);

            /**
             * This function reads into a header and shared pointer of vector of chars. 
             * The reading will block until the data record is read.
             * @param header Hash object which will be updated to contain header information
             * @param data A shared pointer of a vector which will be updated accordingly
             */
            void read(karabo::util::Hash& header, boost::shared_ptr<std::vector<char> >& data);
            
            /**
             * This function reads into a header hash and a data hash.
             * The reading will block until the data record is read.
             * The reading will block until the data record is read.
             * @param header Hash object which will be updated to contain header information
             * @param data Hash object which will be updated to contain data information
             */
            void read(karabo::util::Hash& header, karabo::util::Hash& data);


            //**************************************************************/
            //*              Asynchronous Read - No Header                 */
            //**************************************************************/

            /**
             * Asynchronously reads size number of bytes into pre-allocated data buffer
             * A handler can be registered to inform about completion of writing
             * @param data Pre-allocated contiguous block of memory
             * @param size This number of bytes will be copied into data
             * @param handler Function of signature: <void (Channel::Pointer)> which will be call-backed upon read completion
             */
            void readAsyncRaw(char* data, const size_t& size, const ReadRawHandler& handler);

            void readAsyncVector(const ReadVectorHandler& handler);
            
            void readAsyncString(const ReadStringHandler& handler);
            
            void readAsyncHash(const ReadHashHandler& handler);
            
            void readAsyncVectorPointer(const ReadVectorPointerHandler& handler);
            
            //**************************************************************/
            //*              Asynchronous Read - With Header               */
            //**************************************************************/
            
            void readAsyncHashVector(const ReadHashVectorHandler& handler);
            
            void readAsyncHashString(const ReadHashStringHandler& handler);
            
            void readAsyncHashHash(const ReadHashHashHandler& handler);
            
            void readAsyncHashVectorPointer(const ReadHashVectorPointerHandler& handler);

            /**
             * This function calls the corresponding handler
             * @param handler
             * @param byteSize
             * @param error
             */
            void onSizeInBytesAvailable(const ReadSizeInBytesHandler& handler, const size_t byteSize, const ErrorCode& error);

            /**
             * Internal default handler
             * @param channel
             * @param byteSize
             */
            void byteSizeAvailableHandler(const Channel::Pointer& channel, const size_t byteSize);




            void onBytesAvailable(const ReadRawHandler& handler, const ErrorCode& error);

            /**
             * Internal default handler
             * @param channel
             */
            void bytesAvailableHandler(const Channel::Pointer& channel);

            void readAsyncRaw(char* data, size_t& size, const ReadRawHandler& handler);

            void write(const char* data, const size_t& size);

            void write(const karabo::util::Hash& header, const char* data, const size_t& size);

            void write(const karabo::util::Hash& data);

            void write(const karabo::util::Hash& header, const boost::shared_ptr<std::vector<char> >& body);

            void write(const karabo::util::Hash& header, const karabo::util::Hash& body);

            void writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler);

            void writeAsyncVector(const std::vector<char>& data, const WriteCompleteHandler& handler);

            void writeAsyncVectorPointer(const boost::shared_ptr<std::vector<char> >& data, const WriteCompleteHandler& handler);

            void writeAsyncHash(const karabo::util::Hash& data, const WriteCompleteHandler& handler);

            void writeAsyncHashRaw(const karabo::util::Hash& header, const char* data, const size_t& size, const WriteCompleteHandler& handler);

            void writeAsyncHashVector(const karabo::util::Hash& header, const std::vector<char>& data, const WriteCompleteHandler& handler);

            void writeAsyncHashVectorPointer(const karabo::util::Hash& header, const boost::shared_ptr<std::vector<char> >& data, const WriteCompleteHandler& handler);

            //void writeAsyncStringHash(const std::string& data, const karabo::util::Hash& header, const WriteCompleteHandler& handler);

            void writeAsyncHashHash(const karabo::util::Hash& header, const karabo::util::Hash& data, const WriteCompleteHandler& handler);

            virtual void waitAsync(int milliseconds, const WaitHandler& handler, const std::string& id);

            virtual void setErrorHandler(const ErrorHandler& handler) {
                m_errorHandler = handler;
            }
            
            virtual void close();
            
            virtual bool isOpen();

            boost::asio::ip::tcp::socket& socket() {
                return m_socket;
            }

        private:

            void managedWriteAsync(const WriteCompleteHandler& handler);

            void unmanagedWriteAsync(const char* data, const size_t& size, const WriteCompleteHandler& handler);

            void managedWriteAsyncWithHeader(const WriteCompleteHandler& handler);

            void unmanagedWriteAsyncWithHeader(const char* data, const size_t& size, const WriteCompleteHandler& handler);

            void writeAsyncHeaderBodyImpl(const boost::shared_ptr<std::vector<char> >& header, const boost::shared_ptr<std::vector<char> >& body, const WriteCompleteHandler& handler);
            
            void prepareHeaderFromHash(const karabo::util::Hash& hash);
            
            void prepareHashFromHeader(karabo::util::Hash& hash) const;

            void prepareDataFromHash(const karabo::util::Hash& hash);
            
            void prepareDataFromHash(const karabo::util::Hash& hash, boost::shared_ptr<std::vector<char> >& dataPtr);
            
            void prepareHashFromData(karabo::util::Hash& hash) const;

            void decompress(karabo::util::Hash& header, const std::vector<char>&source, char* data, const size_t& size);
            void decompress(karabo::util::Hash& header, const std::vector<char>&source, std::vector<char>& target);
            void decompress(karabo::util::Hash& header, const std::vector<char>&source, std::string& target);
            
            void decompressSnappy(const char* compressed, size_t compressed_length, char* data, const size_t& size);
            void decompressSnappy(const char* compressed, size_t compressed_length, std::vector<char>& data);
            
            void compress(karabo::util::Hash& header, const std::string& cmprs, const char* src,  const size_t& srclen, std::vector<char>& target);
            void compress(karabo::util::Hash& header, const std::string& cmprs, const std::string& source, std::string& target);
            void compress(karabo::util::Hash& header, const std::string& cmprs, const std::vector<char>& source, std::vector<char>& target);
            
            void compressSnappy(const char* source, const size_t& source_length, std::vector<char>& target);
            
            void read(char*& data, size_t& size, char*& hdr, size_t& hsize);
            void write(const char* header, const size_t& headerSize, const char* body, const size_t& bodySize);

            void asyncWriteHandler(const Channel::WriteCompleteHandler& handler, const ErrorCode& e);
            void asyncWriteHandler(const Channel::WriteCompleteHandler& handler, const boost::shared_ptr<std::vector<char> >& body, const ErrorCode& e);
            void asyncWriteHandler(const Channel::WriteCompleteHandler& handler, const boost::shared_ptr<std::vector<char> >& header, const boost::shared_ptr<std::vector<char> >& body, const ErrorCode& e);

            void asyncWaitHandler(const Channel::WaitHandler& handler, const std::string& id, const ErrorCode& e);

        private:


           
        };
    }
}

#ifndef __SO__
extern 
#endif

#endif	/* KARABO_NET_TCPCHANNEL_HH */

