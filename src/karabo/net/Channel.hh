/* 
 * File:   Channel.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on June 1, 2011, 5:02 PM
 */

#ifndef KARABO_NET_CHANNEL_HH
#define	KARABO_NET_CHANNEL_HH

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>

#include <karabo/util/Factory.hh>

#include "Connection.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {

        class Channel {

        public:

            KARABO_CLASSINFO(Channel, "Channel", "1.0")
                    
            typedef boost::function<void (Channel::Pointer, const size_t&) > ReadSizeInBytesCompleted;
            typedef boost::function<void (Channel::Pointer)> ReadRawCompleted;
            typedef boost::function<void (Channel::Pointer, const karabo::util::Hash&)> ReadRawHashCompleteHandler;
            
            typedef boost::function<void (Channel::Pointer, const char*, const size_t&) > ReadRawHandler;
            typedef boost::function<void (Channel::Pointer, const std::vector<char>&) > ReadVectorHandler;
            typedef boost::function<void (Channel::Pointer, const std::string&) > ReadStringHandler;
            typedef boost::function<void (Channel::Pointer, const karabo::util::Hash&) > ReadHashHandler;

            typedef boost::function<void (Channel::Pointer, const char*, const size_t&, const karabo::util::Hash&) > ReadRawHashHandler;
            typedef boost::function<void (Channel::Pointer, const std::vector<char>&, const karabo::util::Hash&) > ReadVectorHashHandler;
            typedef boost::function<void (Channel::Pointer, const std::string&, const karabo::util::Hash&) > ReadStringHashHandler;
            typedef boost::function<void (Channel::Pointer, const karabo::util::Hash&, const karabo::util::Hash&) > ReadHashHashHandler;

            typedef boost::function<void (Channel::Pointer) > WriteCompleteHandler;

            typedef boost::function<void (Channel::Pointer) > WaitHandler;

           
            virtual ~Channel() {
            }

            // TODO Check, whether is needed
            virtual Connection::Pointer getConnection() const = 0;
               
            
            /*
             * Synchronously reads the message's size.
             * Will block until a message arrives on the socket.
             * @return Size in bytes of incoming TCP message
             */
            virtual size_t readSizeInBytes() {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }
            
            /**
             * In case a message arrived, handler will be called back
             * The handler will inform about the number of bytes going to come in
             * The handler must have the following signature:
             * void handler(Channel::Pointer, const size_t&)
             * @param handler A function pointer or boost::function object
             */
            virtual void readAsyncSizeInBytes(const ReadSizeInBytesCompleted& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**************************************************************/
            /*              Synchronous Read - No Header                  */
            /**************************************************************/

            /**
             * Synchronously reads size bytes into data.
             * @param data Pre-allocated contiguous block of memory
             * @param size This number of bytes will be copied into data
             */
            virtual void read(char* data, const size_t& size) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the data record is read.
             * The vector will be updated accordingly.
             * @return void 
             */
            virtual void read(std::vector<char>& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the data record is read.
             * The size of data record is the first 4 bytes in a channel stream.
             * The hash will be updated accordingly.
             * @return void 
             */
            virtual void read(karabo::util::Hash& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**************************************************************/
            /*              Synchronous Read - With Header                */
            /**************************************************************/
            
            /**
             * Synchronously reads size bytes from the TCP socket into data.
             * @param data Pre-allocated contiguous block of memory
             * @param size This number of bytes will be copied into data
             */
            virtual void read(char* data, const size_t& size, karabo::util::Hash& header) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }
            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the header and data records are read.
             * The size of data record is the first 4 bytes in a channel stream
             * followed by 4 byte's length of header, then header and data.
             * The vector data and hash header will be updated accordingly.
             * @return void 
             */
            virtual void read(std::vector<char>& data, karabo::util::Hash& header) {
               throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            
            virtual void read(karabo::util::Hash& body, karabo::util::Hash& header) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Asynchronous Read - No Header                 */
            //**************************************************************/

            /**
             * Asynchronously reads size number of bytes into pre-allocated data buffer
             * A handler can be registered to inform about completion of writing
             * @param data
             * @param size
             * @param handler
             */
            virtual void readAsyncRaw(char* data, const size_t& size, const ReadRawCompleted& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }
            
            virtual void readAsyncVector(const ReadVectorHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void readAsyncHash(const ReadHashHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }
               

            //**************************************************************/
            //*              Asynchronous Read - With Header               */
            //**************************************************************/

            virtual void readAsyncRawHash(char* data, const size_t& size, const ReadRawHashCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void readAsyncVectorHash(const ReadVectorHashHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void readAsyncHashHash(const ReadHashHashHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Synchronous Write - No Header                 */
            //**************************************************************/

            virtual void write(const char* data, const size_t& size) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void write(const std::vector<char>& data) {
                 throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void write(const karabo::util::Hash& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Synchronous Write - With Header               */
            //**************************************************************/

            virtual void write(const char* data, const size_t& size, const karabo::util::Hash& header) {
                 throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void write(const std::vector<char>& data, const karabo::util::Hash& header) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }
            
            virtual void write(const karabo::util::Hash& data, const karabo::util::Hash& header) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Asynchronous Write - No Header                */
            //**************************************************************/

            virtual void writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsyncVector(const std::vector<char>& data, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsyncHash(const karabo::util::Hash& data, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Asynchronous Write - With Header              */
            //**************************************************************/

            virtual void writeAsyncRawHash(const char* data, const size_t& size, const karabo::util::Hash& header, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsyncVectorHash(const std::vector<char>& data, const karabo::util::Hash& header, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsyncHashHash(const karabo::util::Hash& data, const karabo::util::Hash& header, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*                     Misc                                   */ 
            //**************************************************************/
            
            virtual void setErrorHandler(const ErrorHandler& handler) = 0;

            virtual void waitAsync(int milliseconds, const WaitHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void setTimeoutSyncRead(int milliseconds) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void close() = 0;


        private: // functions

//            void raw2Vector(Channel::Pointer channel, const char* data, const size_t& size) {
//                std::vector<char> v;
//                v.resize(size);
//                memcpy(&v[0], data, size);
//                m_readVectorHandler(channel, v);
//            }
//
//            void raw2String(Channel::Pointer channel, const char* data, const size_t& size) {
//                std::string s(data, size);
//                m_readStringHandler(channel, s);
//            }          
//
//            void rawHash2VectorHash(Channel::Pointer channel, const char* data, const size_t& size, const karabo::util::Hash& header) {
//                std::vector<char> v;
//                v.resize(size);
//                memcpy(&v[0], data, size);
//                m_readVectorHashHandler(channel, v, header);
//            }
//
//            void rawHash2StringHash(Channel::Pointer channel, const char* data, const size_t& size, const karabo::util::Hash& header) {
//                std::string s(data, size);
//                m_readStringHashHandler(channel, s, header);
//            }
        };
    }
}

#endif	/* KARABO_NET_CHANNEL_HH */

