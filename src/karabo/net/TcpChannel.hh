/* 
 * File:   TcpChannel.hh
 * Author: esenov
 *
 * Created on June 3, 2011, 6:01 PM
 */

#ifndef KARABO_NET_TCPCHANNEL_HH
#define	KARABO_NET_TCPCHANNEL_HH

//#include <boost/enable_shared_from_this.hpp>

#include <atomic>
#include <karabo/io/TextSerializer.hh>
#include <karabo/io/BinarySerializer.hh>

#include "Channel.hh"
#include "TcpConnection.hh"
#include "Queues.hh"

namespace karabo {
    namespace net {

        class TcpChannel : public Channel {

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
                HASH_POINTER,
                HASH_POINTER_HASH_POINTER,
                HASH_VECTOR_BUFFERSET_POINTER
            };

            TcpConnection::Pointer m_connectionPointer;
            mutable boost::mutex m_socketMutex;
            boost::asio::ip::tcp::socket m_socket;
            HandlerType m_activeHandler;
            bool m_readHeaderFirst;
            boost::any m_readHandler;
            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_textSerializer;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;

            std::vector<char> m_inboundMessagePrefix;
            std::vector<char> m_inboundHeaderPrefix;
            boost::shared_ptr<std::vector<char> > m_inboundData;
            boost::shared_ptr<std::vector<char> > m_inboundHeader;
            karabo::util::Hash::Pointer m_inHashHeader;
            std::vector<char> m_outboundMessagePrefix;
            std::vector<char> m_outboundHeaderPrefix;
            boost::shared_ptr<std::vector<char> > m_outboundData;
            boost::shared_ptr<std::vector<char> > m_outboundHeader;

            // MQ channel supported parameters
            unsigned int m_bodySize;
            unsigned int m_headerSize;
            boost::mutex m_queueMutex;
            std::vector<karabo::net::Queue::Pointer> m_queue;
            std::vector<size_t> m_queueWrittenBytes;
            size_t m_readBytes;
            size_t m_writtenBytes;
            std::atomic<bool> m_writeInProgress;
            bool m_quit;
            unsigned long long m_syncCounter;
            unsigned long long m_asyncCounter;

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

            void readAsyncHashPointer(const ReadHashPointerHandler& handler);

            void readAsyncVectorPointer(const ReadVectorPointerHandler& handler);
                        
            //**************************************************************/
            //*              Asynchronous Read - With Header               */
            //**************************************************************/

            void readAsyncHashVector(const ReadHashVectorHandler& handler);

            void readAsyncHashString(const ReadHashStringHandler& handler);

            void readAsyncHashHash(const ReadHashHashHandler& handler);

            void readAsyncHashPointerHashPointer(const ReadHashPointerHashPointerHandler& handler);

            void readAsyncHashVectorPointer(const ReadHashVectorPointerHandler& handler);                        

            void readAsyncHashVectorBufferSetPointer(const ReadHashVectorBufferSetPointerHandler& handler);

            void readAsyncRaw(char* data, size_t& size, const ReadRawHandler& handler);

            void write(const char* data, const size_t& size);

            void write(const karabo::util::Hash& header, const char* data, const size_t& size);

            void write(const karabo::util::Hash& data);

            void write(const karabo::util::Hash& header, const boost::shared_ptr<std::vector<char> >& body);
            
            void write(const karabo::util::Hash& header, const karabo::io::BufferSet& body);

            void write(const karabo::util::Hash& header, const std::vector<karabo::io::BufferSet::Pointer>& body);

            void write(const char* header, const size_t& headerSize, const karabo::io::BufferSet& body);

            void write(const char* header, const size_t& headerSize, const std::vector<karabo::io::BufferSet::Pointer>& body);

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

            virtual size_t dataQuantityRead();

            virtual size_t dataQuantityWritten();

            virtual void close();

            virtual bool isOpen();

            /**
             * Records the sizes of the write queues in a Hash.
             * Useful for debugging devices with multiple channels open (like the GuiServerDevice...)
             */
            karabo::util::Hash queueInfo();

            /**
             * Address of the remote endpoint
             */
            std::string remoteAddress() const;

            void writeAsync(const char* data, const size_t& size, int prio);

            void writeAsync(const std::vector<char>& data, int prio);

            void writeAsync(const boost::shared_ptr<std::vector<char> >& data, int prio);

            void writeAsync(const std::string& data, int prio);

            // When copyAllData is false, elements of type NDArray in the hash won't be copied before being sent.
            void writeAsync(const karabo::util::Hash& data, int prio, bool copyAllData);

            void writeAsync(const karabo::util::Hash& header, const char* data, const size_t& size, int prio);

            void writeAsync(const karabo::util::Hash& header, const std::vector<char>& data, int prio);

            void writeAsync(const karabo::util::Hash& header, const boost::shared_ptr<std::vector<char> >& data, int prio);

            void writeAsync(const karabo::util::Hash& header, const std::string& data, int prio);

            // When copyAllData is false, elements of type NDArray in the body hash won't be copied before being sent.
            // copyAllData doesn't influence the handling of the header hash.
            void writeAsync(const karabo::util::Hash& header, const karabo::util::Hash& data, int prio, bool copyAllData);

            virtual void setAsyncChannelPolicy(int priority, const std::string& policy, const size_t capacity = 0);
            
            int getSyncPercent() {
                double nomin = double(m_syncCounter);
                double denom = nomin + double(m_asyncCounter);
                int result = int(nomin/denom * 100);
                m_syncCounter = m_asyncCounter = 0;
                return result;
            }
            
            /**
             * This function returns low level info about connection like ...
             * "localIp", "localPort", "remoteIp", "remotePort" that constitute active connection.
             * in form of Hash container
             * @param ptr input TcpChannel boost shared pointer
             * @return Hash with 4 key/value pairs
             */
            static karabo::util::Hash getChannelInfo(const boost::shared_ptr<karabo::net::TcpChannel>& ptr);
            
        private:

            karabo::util::Hash _getChannelInfo();

            void onBytesAvailable(const ErrorCode& error, const size_t length, const ReadRawHandler& handler);

            /**
             * Internal default handler
             * @param channel
             */
            void bytesAvailableHandler(const boost::system::error_code& e);

            /**
             * This function calls the corresponding handler
             * @param handler
             * @param error
             */
            void onSizeInBytesAvailable(const ErrorCode& error, const ReadSizeInBytesHandler& handler);

            /**
             * Internal handler called after filling the buffer set
             * @param error  error code
             * @param length number of bytes read == total size of buffer set
             * @param buffers vector of buffer set pointers with the data
             * @param handler to be called
             */
            void onVectorBufferSetPointerAvailable(const ErrorCode& error, size_t length,
                                                   const std::vector<karabo::io::BufferSet::Pointer>& buffers,
                                                   const ReadVectorBufferSetPointerHandler& handler);

            void onHashVectorBufferSetPointerRead(const boost::system::error_code& e,
                                                  const std::vector<karabo::io::BufferSet::Pointer>& buffers);

            void onHashVectorBufferSetPointerVectorPointerRead(const boost::system::error_code& e,
                                                               const boost::shared_ptr<std::vector<char>>& vecptr,
                                                               const ReadHashVectorBufferSetPointerHandler& handler);

            void readAsyncVectorBufferSetPointerImpl(const std::vector<karabo::io::BufferSet::Pointer>& buffers,
                                                     const ReadVectorBufferSetPointerHandler& handler);

            void readAsyncVectorPointerImpl(const ReadVectorPointerHandler& handler);
                        
            /**
             * Internal default handler
             * @param byteSize
             */
            void byteSizeAvailableHandler(const size_t byteSize);

            void readAsyncSizeInBytesImpl(const ReadSizeInBytesHandler& handler, bool allowNonAsync);

            void readAsyncRawImpl(char* data, const size_t& size, const ReadRawHandler& handler, bool allowNonAsync);

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

            /**
             * Creates a buffer set with the given string stored in its sole buffer.
             *
             * @param str the string to be stored in the buffer set.
             * @return shared_ptr to the buffer set with the string stored.
             *
             * @note actually places a copy of the string into the buffer set.
             */
            karabo::io::BufferSet::Pointer bufferSetFromString(const std::string& str);

            /**
             * Creates a buffer set with contents of a given buffer of chars stored
             * in its sole buffer.
             *
             * @param data a pointer to the first char in the input sequence.
             * @return shared_ptr to the buffer set with the input buffer contents stored.
             *
             * @note actually places a copy of the contents of the input buffer into the buffer set.
             */
            karabo::io::BufferSet::Pointer bufferSetFromPointerToChar(const char* data, size_t size);

            /**
             * Creates a buffer set with characters in a given vector of chars stored
             * in its sole buffer.
             *
             * @param data a pointer to the vector of chars to be stored in the buffer set.
             * @return shared_ptr to the buffer set with the character in the vector stored.
             *
             * @note actually places a copy of the characters in the vector of chars into the buffer set.
             */
            karabo::io::BufferSet::Pointer bufferSetFromVectorCharPointer(const VectorCharPointer& dataVect);

            /**
             * Creates a buffer set with a given hash stored in its sole buffer.
             *
             * @param data the hash to be stored in the buffer set.
             * @param pBuffSet a shared pointer that will be pointed to the newly created buffer set with the hash
             *        (the shared pointer argument can be either pointing to an existing buffer set or nullptr; it
             *         will always point to the new buffer after a successful execution of the method).
             * @param copyAllData if false no copy of any NDArray internal to the hash will be made upon storing the
             *        hash in the bufferset (the buffer set will actually become one of the "owners" of the NDArray).
             */
            void bufferSetFromHash(const karabo::util::Hash& data, karabo::io::BufferSet::Pointer& pBuffSet,
                                   bool copyAllData);

            void decompress(karabo::util::Hash& header, const std::vector<char>&source, char* data, const size_t& size);
            void decompress(karabo::util::Hash& header, const std::vector<char>&source, std::vector<char>& target);
            void decompress(karabo::util::Hash& header, const std::vector<char>&source, std::string& target);

            void decompressSnappy(const char* compressed, size_t compressed_length, char* data, const size_t& size);
            void decompressSnappy(const char* compressed, size_t compressed_length, std::vector<char>& data);

            void compress(karabo::util::Hash& header, const std::string& cmprs, const char* src, const size_t& srclen, std::vector<char>& target);
            void compress(karabo::util::Hash& header, const std::string& cmprs, const std::string& source, std::string& target);
            void compress(karabo::util::Hash& header, const std::string& cmprs, const std::vector<char>& source, std::vector<char>& target);

            void compressSnappy(const char* source, const size_t& source_length, std::vector<char>& target);

            void read(char*& data, size_t& size, char*& hdr, size_t& hsize);
            void write(const char* header, const size_t& headerSize, const char* body, const size_t& bodySize);

            void asyncWriteHandler(const ErrorCode& e, const size_t length, const Channel::WriteCompleteHandler& handler);
            void asyncWriteHandlerBody(const ErrorCode& e, const size_t length, const Channel::WriteCompleteHandler& handler, const boost::shared_ptr<std::vector<char> >& body);
            void asyncWriteHandlerHeaderBody(const ErrorCode& e, const size_t length, const Channel::WriteCompleteHandler& handler, const boost::shared_ptr<std::vector<char> >& header, const boost::shared_ptr<std::vector<char> >& body);

            // MQ support methods
        private:

            void prepareVectorFromHash(const karabo::util::Hash& hash, std::vector<char>& vec);

            void prepareHashFromVector(const std::vector<char>& vec, karabo::util::Hash& hash) const;

            void dispatchWriteAsync(const Message::Pointer& mp, int prio);

            void doWrite();

            void doWriteHandler(Message::Pointer& msg, boost::system::error_code, const size_t length, const int queueIndex);

            //
            // Finally some methods for exclusive use in TcpConnection,
            // we grant friendship exclusively to the methods that need it:
            //
            friend Channel::Pointer TcpConnection::startServer(); // for acceptSocket(..)
            friend void TcpConnection::startServer(const TcpConnection::ConnectionHandler& handler); // for asyncAcceptSocket(..)
            friend Channel::Pointer TcpConnection::startClient(); // for socketConnect(..)
            friend void TcpConnection::resolveHandler(const ErrorCode& e, boost::asio::ip::tcp::resolver::iterator it,
                                                      const TcpConnection::ConnectionHandler& handler); // for asyncSocketConnect(..))

            void acceptSocket(boost::asio::ip::tcp::acceptor& acceptor);

            template<typename Handler>
            void asyncAcceptSocket(boost::asio::ip::tcp::acceptor& acceptor, Handler&& handler) {
                boost::mutex::scoped_lock lock(m_socketMutex);
                acceptor.async_accept(m_socket, handler);
            }

            void socketConnect(const boost::asio::ip::tcp::endpoint& endpoint);

            template<typename Handler>
            void asyncSocketConnect(const boost::asio::ip::tcp::endpoint& endpoint, Handler&& handler) {
                boost::mutex::scoped_lock lock(m_socketMutex);
                m_socket.async_connect(endpoint, handler);
            }
        };
    }
}

#endif	/* KARABO_NET_TCPCHANNEL_HH */

