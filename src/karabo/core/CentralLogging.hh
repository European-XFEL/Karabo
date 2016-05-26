/* 
 * File:   CentralLogging.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on November 9, 2015, 12:23 PM
 */

#ifndef KARABO_CORE_CENTRALLOGGING_HH
#define	KARABO_CORE_CENTRALLOGGING_HH

#include "Device.hh"
 
/**
 * The main karabo namespace
 */
namespace karabo {
 
    /**
     * Namespace for package core
     */
    namespace core {
 
        class CentralLogging : public karabo::core::Device<> {
        public:
 
            KARABO_CLASSINFO(CentralLogging, "CentralLogging", "1.0")
 
            static void expectedParameters(karabo::util::Schema& expected);
 
            CentralLogging(const karabo::util::Hash& input);
 
            virtual ~CentralLogging();
 
        private:
 
            void initialize();
 
            int determineLastIndex();
 
            int incrementLastIndex();
 
            void flushHandler(const boost::system::error_code& ec);

            void logHandler(karabo::net::BrokerChannel::Pointer channel, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& data);


        private:
            void runIoService();

            karabo::util::Hash m_loggerInput;
            unsigned int m_lastIndex;
            std::fstream m_logstream;
            boost::mutex m_streamMutex;
            karabo::net::BrokerConnection::Pointer m_loggerConnection;
            karabo::net::BrokerIOService::Pointer m_loggerIoService;
            karabo::net::BrokerChannel::Pointer m_loggerChannel;
            boost::thread m_logThread;
            boost::thread m_svcThread;
            boost::asio::io_service m_svc;
            boost::asio::deadline_timer m_timer;
        };
    }
}


#endif	/* KARABO_CORE_CENTRALLOGGING_HH */

