/* 
 * File:   NetworkAppender.hh
 * Author: irinak
 *
 * Created on September 21, 2011, 4:39 PM
 */

#ifndef EXFEL_LOGCONFIG_NETWORKAPPENDER_HH
#define	EXFEL_LOGCONFIG_NETWORKAPPENDER_HH

#include <karabo/net/BrokerChannel.hh>
#include <log4cpp/LayoutAppender.hh>
#include <log4cpp/Portability.hh>

namespace exfel {
    namespace log {

        class NetworkAppender : public log4cpp::LayoutAppender {
        public:
            NetworkAppender(const std::string& name, const exfel::net::BrokerChannel::Pointer& channel);
            virtual ~NetworkAppender();

            virtual bool reopen();
            virtual void close();

        protected: // functions
            virtual void _append(const log4cpp::LoggingEvent& event);

        protected: // members
            exfel::net::BrokerChannel::Pointer m_channel;
            
            
        private: // functions
            
            void checkLogCache(); // runs in thread
            
            void writeNow(); // called within thread
            
        private: // members            
            
            boost::thread m_thread;
            boost::mutex m_mutex;
            
            std::string m_logCache;
            bool m_ok;
        };

    }
}

#endif	/* EXFEL_LOGCONFIG_NETWORKAPPENDER_HH */

