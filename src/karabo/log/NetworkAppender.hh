/* 
 * File:   NetworkAppender.hh
 * Author: irinak
 *
 * Created on September 21, 2011, 4:39 PM
 */

#ifndef KARABO_LOGCONFIG_NETWORKAPPENDER_HH
#define	KARABO_LOGCONFIG_NETWORKAPPENDER_HH

#include <karabo/net/BrokerChannel.hh>
#include <krb_log4cpp/LayoutAppender.hh>
#include <krb_log4cpp/Portability.hh>

namespace karabo {
    namespace log {

        class NetworkAppender : public krb_log4cpp::LayoutAppender {

        public:

            NetworkAppender(const std::string& name, const karabo::net::BrokerChannel::Pointer& channel);
            virtual ~NetworkAppender();

            virtual bool reopen();
            virtual void close();

        protected: // functions
            virtual void _append(const krb_log4cpp::LoggingEvent& event);

        protected: // members
            
            karabo::net::BrokerChannel::Pointer m_channel;

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

#endif	/* KARABO_LOGCONFIG_NETWORKAPPENDER_HH */

