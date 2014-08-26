/* 
 * File:   BrokerChannel.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on June 1, 2011, 5:02 PM
 */

#ifndef KARABO_NET_BROKERCHANNEL_HH
#define	KARABO_NET_BROKERCHANNEL_HH

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>

#include <karabo/util/Factory.hh>
#include "BrokerConnection.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {

        class BrokerChannel {

        public:

            KARABO_CLASSINFO(BrokerChannel, "BrokerChannel", "1.0")                    
                    
            typedef boost::function<void (BrokerChannel::Pointer, const char*, const size_t&) > ReadRawHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const std::vector<char>&) > ReadVectorHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const std::string&) > ReadStringHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const karabo::util::Hash&) > ReadHashHandler;

            typedef boost::function<void (BrokerChannel::Pointer, const char* /*body*/, const size_t& /*body size*/, const karabo::util::Hash::Pointer& /*header*/) > ReadRawHashHandler;            
            typedef boost::function<void (BrokerChannel::Pointer, const std::vector<char>& /*body*/, const karabo::util::Hash::Pointer& /*header*/) > ReadVectorHashHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const std::string& /*body*/, const karabo::util::Hash::Pointer& /*header*/) > ReadStringHashHandler;                        
            typedef boost::function<void (BrokerChannel::Pointer, const karabo::util::Hash::Pointer& /*body*/, const karabo::util::Hash::Pointer& /*header*/) > ReadHashHashHandler;

            typedef boost::function<void (BrokerChannel::Pointer) > WriteCompleteHandler;
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
             * This function reads from a channel into vector of chars 
             * The reading will block until the data record is read.
             * The vector will be updated accordingly.
             * @return void 
             */
            virtual void read(std::vector<char>& data) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Function not implemented by this broker implementation");
            }

            /**
             * This function reads from a channel into string 
             * The reading will block until the data record is read.
             * The string will be updated accordingly.
             * @return void 
             */
            virtual void read(std::string& data) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Function not implemented by this broker implementation");
            }

            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the data record is read.
             * The hash will be updated accordingly.
             * @return void 
             */
            virtual void read(karabo::util::Hash& data) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Function not implemented by this broker implementation");
            }

            /**************************************************************/
            /*              Synchronous Read - With Header                */
            /**************************************************************/

            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the header and data records are read.
             * @return void 
             */
            virtual void read(std::vector<char>& data, karabo::util::Hash& header) = 0;
            /**
             * This function reads from a channel into std::string 
             * The reading will block until the header and data records are read.
             * @return void 
             */
            virtual void read(std::string& data, karabo::util::Hash& header) = 0;

            virtual void read(karabo::util::Hash& body, karabo::util::Hash& header) = 0;

            //**************************************************************/
            //*              Asynchronous Read - No Header                 */
            //**************************************************************/

            virtual void readAsyncRaw(const ReadRawHandler& handler) {
                 throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Function not implemented by this broker implementation");
            }

            virtual void readAsyncVector(const ReadVectorHandler& handler) {
                m_readVectorHandler = handler;
                readAsyncRaw(boost::bind(&karabo::net::BrokerChannel::raw2Vector, this, _1, _2, _3));
            }

            virtual void readAsyncString(const ReadStringHandler& handler) {
                m_readStringHandler = handler;
                readAsyncRaw(boost::bind(&karabo::net::BrokerChannel::raw2String, this, _1, _2, _3));
            }

            virtual void readAsyncHash(const ReadHashHandler& handler) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Function not implemented by this broker implementation");
            }

            //**************************************************************/
            //*              Asynchronous Read - With Header               */
            //**************************************************************/

            virtual void readAsyncRawHash(const ReadRawHashHandler& handler) = 0;
            
            virtual void readAsyncVectorHash(const ReadVectorHashHandler& handler) {
                m_readVectorHashHandler = handler;
                readAsyncRawHash(boost::bind(&karabo::net::BrokerChannel::rawHash2VectorHash, this, _1, _2, _3, _4));
            }

            virtual void readAsyncStringHash(const ReadStringHashHandler& handler) {
                m_readStringHashHandler = handler;
                readAsyncRawHash(boost::bind(&karabo::net::BrokerChannel::rawHash2StringHash, this, _1, _2, _3, _4));
            }

            virtual void readAsyncHashHash(const ReadHashHashHandler& handler) = 0;
            
            
            //**************************************************************/
            //*              Synchronous Write - No Header                 */
            //**************************************************************/

            virtual void write(const char* data, const size_t& size) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Function not implemented by this broker implementation");
            }

            virtual void write(const std::vector<char>& data) {
                write(&data[0], data.size());
            }

            virtual void write(const std::string& data) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Function not implemented by this broker implementation");
            }

            virtual void write(const karabo::util::Hash& data) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Function not implemented by this broker implementation");
            }

            //**************************************************************/
            //*              Synchronous Write - With Header               */
            //**************************************************************/

            virtual void write(const char* data, const size_t& size, const karabo::util::Hash& header) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void write(const std::vector<char>& data, const karabo::util::Hash& header) {
                write(static_cast<const char*> (&data[0]), data.size(), header);
            }

            virtual void write(const std::string& data, const karabo::util::Hash& header) = 0;

            virtual void write(const karabo::util::Hash& data, const karabo::util::Hash& header) = 0;

            //**************************************************************/
            //*                Errors, Timing, Selections                  */
            //**************************************************************/

            virtual void setErrorHandler(const BrokerErrorHandler& handler) = 0;

            virtual void waitAsync(int milliseconds, const WaitHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void setTimeoutSyncRead(int milliseconds) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void setFilter(const std::string& filterCondition) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Filtering is not supported for this network protocol");
            }

            virtual const std::string& getFilter() const {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Filtering is not supported for this network protocol");
            }

            virtual void preRegisterSynchronousRead() {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Pre-registration of synchronous reads is not supported by this broker protocol");
            }

            virtual void close() = 0;


        protected: // functions

            void unregisterChannel(BrokerChannel::Pointer channel) {
                m_connection.unregisterChannel(channel);
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
         
            void rawHash2VectorHash(BrokerChannel::Pointer channel, const char* data, const size_t& size, const karabo::util::Hash::Pointer& header) {
                std::vector<char> v;
                v.resize(size);
                memcpy(&v[0], data, size);
                m_readVectorHashHandler(channel, v, header);
            }

            void rawHash2StringHash(BrokerChannel::Pointer channel, const char* data, const size_t& size, const karabo::util::Hash::Pointer& header) {
                std::string s(data, size);
                m_readStringHashHandler(channel, s, header);
            }


        private: // members

            ReadVectorHandler m_readVectorHandler;

            ReadStringHandler m_readStringHandler;

            ReadVectorHashHandler m_readVectorHashHandler;

            ReadStringHashHandler m_readStringHashHandler;

        };
    }
}

#endif	/* KARABO_NET_CHANNEL_HH */

