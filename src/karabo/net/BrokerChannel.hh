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
            typedef boost::function<void (BrokerChannel::Pointer, const karabo::util::Hash::Pointer&) > ReadHashHandler;

            typedef boost::function<void (BrokerChannel::Pointer, const karabo::util::Hash::Pointer& /*header*/, const char* /*body*/, const size_t& /*body size*/) > ReadHashRawHandler;            
            typedef boost::function<void (BrokerChannel::Pointer, const karabo::util::Hash::Pointer& /*header*/, const std::vector<char>& /*body*/) > ReadVectorHashHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const karabo::util::Hash::Pointer& /*header*/, const std::string& /*body*/) > ReadHashStringHandler;                        
            typedef boost::function<void (BrokerChannel::Pointer, const karabo::util::Hash::Pointer& /*header*/, const karabo::util::Hash::Pointer& /*body*/) > ReadHashHashHandler;

            typedef boost::function<void (BrokerChannel::Pointer) > WriteCompleteHandler;
            typedef boost::function<void (BrokerChannel::Pointer, const std::string&) > WaitHandler;
            
        protected:
            
            ReadRawHandler m_readRawHandler;
            ReadStringHandler m_readStringHandler;
            ReadHashHandler m_readHashHandler;
            
            ReadHashRawHandler m_readHashRawHandler;
            ReadHashStringHandler m_readHashStringHandler;
            ReadHashHashHandler m_readHashHashHandler;
            
        private:
            
            ReadVectorHandler m_readVectorHandler;
            ReadVectorHashHandler m_readVectorHashHandler;
            
            
        public:

            BrokerChannel() {
            }

            virtual ~BrokerChannel() {
            }

            virtual BrokerConnection::Pointer getConnection() = 0;


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
            virtual void read(karabo::util::Hash& header, std::vector<char>& data) = 0;
            /**
             * This function reads from a channel into std::string 
             * The reading will block until the header and data records are read.
             * @return void 
             */
            virtual void read(karabo::util::Hash& header, std::string& data) = 0;

            virtual void read(karabo::util::Hash& header, karabo::util::Hash& body) = 0;

            //**************************************************************/
            //*              Asynchronous Read - No Header                 */
            //**************************************************************/

            virtual void readAsyncRaw(const ReadRawHandler& handler) = 0;                 

            void readAsyncVector(const ReadVectorHandler& handler) {
                m_readVectorHandler = handler;
                readAsyncRaw(boost::bind(&karabo::net::BrokerChannel::raw2Vector, this, _1, _2, _3));
            }

            virtual void readAsyncString(const ReadStringHandler& handler) = 0;
            
            virtual void readAsyncHash(const ReadHashHandler& handler) = 0;
                

            //**************************************************************/
            //*              Asynchronous Read - With Header               */
            //**************************************************************/

            virtual void readAsyncHashRaw(const ReadHashRawHandler& handler) = 0;
            
            void readAsyncHashVector(const ReadVectorHashHandler& handler) {
                m_readVectorHashHandler = handler;
                readAsyncHashRaw(boost::bind(&karabo::net::BrokerChannel::hashRaw2hashVector, this, _1, _2, _3, _4));
            }

            virtual void readAsyncHashString(const ReadHashStringHandler& handler) = 0;

            virtual void readAsyncHashHash(const ReadHashHashHandler& handler) = 0;
            
            
            //**************************************************************/
            //*              Synchronous Write - No Header                 */
            //**************************************************************/

            virtual void write(const char* data, const size_t& size, const int priority = 4, const int messageTimeToLive = 600000) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Function not implemented by this broker implementation");
            }

            virtual void write(const std::vector<char>& data, const int priority = 4, const int messageTimeToLive = 600000) {
                write(&data[0], data.size(), messageTimeToLive);
            }

            virtual void write(const std::string& data, const int priority = 4, const int messageTimeToLive = 600000) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Function not implemented by this broker implementation");
            }

            virtual void write(const karabo::util::Hash& data, const int priority = 4, const int messageTimeToLive = 600000) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Function not implemented by this broker implementation");
            }

            //**************************************************************/
            //*              Synchronous Write - With Header               */
            //**************************************************************/

            virtual void write(const karabo::util::Hash& header, const char* data, const size_t& size, const int priority = 4, const int messageTimeToLive = 600000) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void write(const karabo::util::Hash& header, const std::vector<char>& data, const int priority = 4, const int messageTimeToLive = 600000) {
                write(header, static_cast<const char*> (&data[0]), data.size(), messageTimeToLive);
            }

            virtual void write(const karabo::util::Hash& header, const std::string& data, const int priority = 4, const int messageTimeToLive = 600000) = 0;

            virtual void write(const karabo::util::Hash& header, const karabo::util::Hash& data, const int priority = 4, const int messageTimeToLive = 600000) = 0;

            //**************************************************************/
            //*                Errors, Timing, Selections                  */
            //**************************************************************/

            virtual void setErrorHandler(const BrokerErrorHandler& handler) = 0;

            virtual void waitAsync(int milliseconds, const WaitHandler& handler, const std::string& id) {
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


        private: // functions

            void raw2Vector(BrokerChannel::Pointer channel, const char* data, const size_t& size) {
                std::vector<char> v;
                v.resize(size);
                memcpy(&v[0], data, size);
                m_readVectorHandler(channel, v);
            }            
         
            void hashRaw2hashVector(BrokerChannel::Pointer channel, const karabo::util::Hash::Pointer& header, const char* data, const size_t& size) {
                std::vector<char> v;
                v.resize(size);
                memcpy(&v[0], data, size);
                m_readVectorHashHandler(channel, header, v);
            }         

        };
    }
}

#endif	/* KARABO_NET_CHANNEL_HH */

