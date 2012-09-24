/* 
 * File:   BrokerChannel.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on June 1, 2011, 5:02 PM
 */

#ifndef EXFEL_NET_BROKERCHANNEL_HH
#define	EXFEL_NET_BROKERCHANNEL_HH

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>

#include <karabo/util/Factory.hh>
#include <karabo/io/Reader.hh>
#include <karabo/io/Writer.hh>

#include "BrokerConnection.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package net
     */
    namespace net {

        class BrokerChannel {
        public:

            EXFEL_CLASSINFO(BrokerChannel, "BrokerChannel", "1.0")

            typedef boost::shared_ptr<BrokerChannel> Pointer;
            typedef boost::function<void (BrokerChannel::Pointer, const char*, const size_t&) > ReadRawHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const std::vector<char>&) > ReadVectorHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const std::string&) > ReadStringHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const exfel::util::Hash&) > ReadHashHandler;

            typedef boost::function<void (BrokerChannel::Pointer, const char*, const size_t&, const exfel::util::Hash&) > ReadRawHashHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const std::vector<char>&, const exfel::util::Hash&) > ReadVectorHashHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const std::string&, const exfel::util::Hash&) > ReadStringHashHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const exfel::util::Hash&, const exfel::util::Hash&) > ReadHashHashHandler;

            typedef boost::function<void (BrokerChannel::Pointer) > WriteCompleteHandler;

//            typedef boost::function<void (BrokerChannel::Pointer, const std::string&) > ErrorHandler;

            typedef boost::function<void (BrokerChannel::Pointer) > WaitHandler;

            BrokerChannel(BrokerConnection& connection) : m_connection(connection) {
            }

            virtual ~BrokerChannel() {
            }
            
            BrokerConnection::Pointer getConnection() const {
                return m_connection.getConnectionPointer();
            }


            /**************************************************************/
            /*              Synchronous Read - No Header                  */
            /**************************************************************/
            
            /**
             * This function hands over an allocated stretch of raw memory
             * @param data Pointer to allocated memory block
             * @param size Size (in bytes) of the memory block
             */
            virtual void read(char*& data, size_t& size) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the data record is read.
             * The size of data record is the first 4 bytes in a channel stream.
             * The vector will be updated accordingly.
             * @return void 
             */
            virtual void read(std::vector<char>& data) {
                char* raw = 0;
                size_t size = 0;
                read(raw, size);
                data.assign(raw, raw + size);
            }

            /**
             * This function reads from a channel into string 
             * The reading will block until the data record is read.
             * The size of data record is the first 4 bytes in a channel stream.
             * The string will be updated accordingly.
             * @return void 
             */
            virtual void read(std::string& data) {
                char* raw = 0;
                size_t size = 0;
                read(raw, size);
                data.assign(raw, size);
            }

            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the data record is read.
             * The size of data record is the first 4 bytes in a channel stream.
             * The hash will be updated accordingly.
             * @return void 
             */
            virtual void read(exfel::util::Hash& data) {
                char* raw = 0;
                size_t size = 0;
                read(raw, size);
                std::string s(raw, size);
                if (s.size() > 0) {
                    stringToHash(s, data);
                }
            }

            /**************************************************************/
            /*              Synchronous Read - With Header                */
            /**************************************************************/

            /**
             * This function hands over an allocated stretch of raw memory and header information
             * @param data Pointer to allocated memory block
             * @param size Size (in bytes) of the memory block
             * @param header Hash that will be automatically updated
             */
            virtual void read(char*& data, size_t& size, exfel::util::Hash& header) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented");
            }

            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the header and data records are read.
             * The size of data record is the first 4 bytes in a channel stream
             * followed by 4 byte's length of header, then header and data.
             * The vector data and hash header will be updated accordingly.
             * @return void 
             */
            virtual void read(std::vector<char>& data, exfel::util::Hash& header) {
                char* raw = 0;
                size_t size;
                read(raw, size, header);
                data.resize(size);
                memcpy(&data[0], raw, size);
            }

            /**
             * This function reads from a channel into std::string 
             * The reading will block until the header and data records are read.
             * The size of data record is the first 4 bytes in a channel stream
             * followed by 4 byte's length of a header, then header and data.
             * The string data and hash header will be updated accordingly.
             * @return void 
             */
            virtual void read(std::string& data, exfel::util::Hash& header) {
                char* raw = 0;
                size_t size;
                read(raw, size, header);
                data.assign(raw, size);
            }

            virtual void read(exfel::util::Hash& body, exfel::util::Hash& header) {
                std::string s;
                exfel::util::Hash h;
                read(s, header);
                if (s.size() > 0) {
                    stringToHash(s, body);
                }
            }

            //**************************************************************/
            //*              Asynchronous Read - No Header                 */
            //**************************************************************/

            virtual void readAsyncRaw(const ReadRawHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void readAsyncRaw(char*& data, size_t& size, const ReadRawHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void readAsyncVector(const ReadVectorHandler& handler) {
                m_readVectorHandler = handler;
                readAsyncRaw(boost::bind(&exfel::net::BrokerChannel::raw2Vector, this, _1, _2, _3));
            }

            virtual void readAsyncString(const ReadStringHandler& handler) {
                m_readStringHandler = handler;
                readAsyncRaw(boost::bind(&exfel::net::BrokerChannel::raw2String, this, _1, _2, _3));
            }

            virtual void readAsyncHash(const ReadHashHandler& handler) {
                m_readHashHandler = handler;
                readAsyncRaw(boost::bind(&exfel::net::BrokerChannel::raw2Hash, this, _1, _2, _3));
            }

            //**************************************************************/
            //*              Asynchronous Read - With Header               */
            //**************************************************************/

            virtual void readAsyncRawHash(const ReadRawHashHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void readAsyncVectorHash(const ReadVectorHashHandler& handler) {
                m_readVectorHashHandler = handler;
                readAsyncRawHash(boost::bind(&exfel::net::BrokerChannel::rawHash2VectorHash, this, _1, _2, _3, _4));
            }

            virtual void readAsyncStringHash(const ReadStringHashHandler& handler) {
                m_readStringHashHandler = handler;
                readAsyncRawHash(boost::bind(&exfel::net::BrokerChannel::rawHash2StringHash, this, _1, _2, _3, _4));
            }

            virtual void readAsyncHashHash(const ReadHashHashHandler& handler) {
                m_readHashHashHandler = handler;
                readAsyncRawHash(boost::bind(&exfel::net::BrokerChannel::rawHash2HashHash, this, _1, _2, _3, _4));
            }

            //**************************************************************/
            //*              Synchronous Write - No Header                 */
            //**************************************************************/

            virtual void write(const char* data, const size_t& size) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void write(const std::vector<char>& data) {
                write(&data[0], data.size());
            }

            virtual void write(const std::string& data) {
                write(data.c_str(), data.size());
            }

            virtual void write(const exfel::util::Hash& data) {
                std::string s;
                hashToString(data, s);
                write(s);
            }

            //**************************************************************/
            //*              Synchronous Write - With Header               */
            //**************************************************************/

            virtual void write(const char* data, const size_t& size, const exfel::util::Hash& header) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void write(const std::vector<char>& data, const exfel::util::Hash& header) {
                write(static_cast<const char*>(&data[0]), data.size(), header);
            }

            virtual void write(const std::string& data, const exfel::util::Hash& header) {
                write(data.c_str(), data.size(), header);
            }

            virtual void write(const exfel::util::Hash& data, const exfel::util::Hash& header) {
                std::string s;
                hashToString(data, s);
                write(s, header);
            }

            //**************************************************************/
            //*              Asynchronous Write - No Header                */
            //**************************************************************/

            virtual void writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void writeAsyncVector(const std::vector<char>& data, const WriteCompleteHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void writeAsyncString(const std::string& data, const WriteCompleteHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void writeAsyncHash(const exfel::util::Hash& data, const WriteCompleteHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            //**************************************************************/
            //*              Asynchronous Write - With Header              */
            //**************************************************************/

            virtual void writeAsyncRawHash(const char* data, const size_t& size, const exfel::util::Hash& header, const WriteCompleteHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void writeAsyncVectorHash(const std::vector<char>& data, const exfel::util::Hash& header, const WriteCompleteHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void writeAsyncStringHash(const std::string& data, const exfel::util::Hash& header, const WriteCompleteHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void writeAsyncHashHash(const exfel::util::Hash& data, const exfel::util::Hash& header, const WriteCompleteHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }
            
            //**************************************************************/
            //*                Errors, Timing, Selections                  */
            //**************************************************************/

            virtual void setErrorHandler(const BrokerErrorHandler& handler) = 0;

            virtual void waitAsync(int milliseconds, const WaitHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }
            
            virtual void setTimeoutSyncRead(int milliseconds) {
                throw NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void setFilter(const std::string& filterCondition) {
                throw NOT_SUPPORTED_EXCEPTION("Filtering is not supported for this network protocol");
            }
            
            virtual const std::string& getFilter() const {
                throw NOT_SUPPORTED_EXCEPTION("Filtering is not supported for this network protocol");
            }
            
            virtual void preRegisterSynchronousRead() {
                throw NOT_SUPPORTED_EXCEPTION("Pre-registration of synchronous reads is not support by this broker protocol");
            }
            
            virtual void close() = 0;


        protected: // functions

            void hashToString(const exfel::util::Hash& hash, std::string& serializedHash) {
                m_connection.hashToString(hash, serializedHash);
            }

            void stringToHash(const std::string& serializedHash, exfel::util::Hash& hash) {
                m_connection.stringToHash(serializedHash, hash);
            }

            void unregisterChannel(BrokerChannel::Pointer channel) {
                m_connection.unregisterChannel(channel);
            }
            
            std::string getHashFormat() {
                return m_connection.getHashFormat();
            }

        protected: // members

            BrokerConnection& m_connection;

        private: // functions

            void raw2Vector(BrokerChannel::Pointer channel, const char* data, const size_t& size) {
                std::vector<char> v;
                v.resize(size);
                memcpy(&v[0], data, size);
                m_readVectorHandler(channel, v);
            }

            void raw2String(BrokerChannel::Pointer channel, const char* data, const size_t& size) {
                std::string s(data, size);
                m_readStringHandler(channel, s);
            }

            void raw2Hash(BrokerChannel::Pointer channel, const char* data, const size_t& size) {
                exfel::util::Hash h;
                std::string s(data, size);
                stringToHash(s, h);
                m_readHashHandler(channel, h);
            }

            void rawHash2VectorHash(BrokerChannel::Pointer channel, const char* data, const size_t& size, const exfel::util::Hash& header) {
                std::vector<char> v;
                v.resize(size);
                memcpy(&v[0], data, size);
                m_readVectorHashHandler(channel, v, header);
            }

            void rawHash2StringHash(BrokerChannel::Pointer channel, const char* data, const size_t& size, const exfel::util::Hash& header) {
                std::string s(data, size);
                m_readStringHashHandler(channel, s, header);
            }

            void rawHash2HashHash(BrokerChannel::Pointer channel, const char* data, const size_t& size, const exfel::util::Hash& header) {
                exfel::util::Hash h;
                std::string s(data, size);
                stringToHash(s, h);
                m_readHashHashHandler(channel, h, header);
            }
            

        private: // members

            ReadVectorHandler m_readVectorHandler;

            ReadStringHandler m_readStringHandler;

            ReadHashHandler m_readHashHandler;

            ReadVectorHashHandler m_readVectorHashHandler;

            ReadStringHashHandler m_readStringHashHandler;

            ReadHashHashHandler m_readHashHashHandler;

        };
    }
}

#endif	/* EXFEL_NET_CHANNEL_HH */

