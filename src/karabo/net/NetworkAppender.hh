/* 
 * File:   NetworkAppender.hh
 * Author: irinak
 *
 * Created on September 21, 2011, 4:39 PM
 *
 * This class is used to propagate log messages through the network to the GUI.
 *
 * It sends via the broker with a header (Hash("target", "log")) and a body.
 * The latter is a hash with a single key ("messages") The corresponding value
 * is a std::vector<Hash> containing one Hash for each message. Its keys are
 * "timestamp", "type", "category" and "message".
 */

#ifndef KARABO_LOGCONFIG_NETWORKAPPENDER_HH
#define	KARABO_LOGCONFIG_NETWORKAPPENDER_HH

#include <vector>

#include <karabo/net/BrokerChannel.hh>

#include <krb_log4cpp/LayoutAppender.hh>
#include <krb_log4cpp/PatternLayout.hh>
#include <krb_log4cpp/Portability.hh>

namespace karabo {
    namespace net {

        class NetworkAppender : public krb_log4cpp::LayoutAppender {

        public:
            KARABO_CLASSINFO(NetworkAppender, "NetworkAppender", "1.1")

            NetworkAppender(const std::string& name, const BrokerChannel::Pointer& channel);
            virtual ~NetworkAppender();

            virtual bool reopen();
            virtual void close();

        protected: // functions
            virtual void _append(const krb_log4cpp::LoggingEvent& event);

        protected: // members
            
            BrokerChannel::Pointer m_channel;

        private: // functions

            void checkLogCache(); // runs in thread

            void writeNow(); // called within thread

        private: // members            

            boost::thread m_thread;
            boost::mutex m_mutex;

            /// layouts for each component
            krb_log4cpp::PatternLayout m_timeLayout;
            krb_log4cpp::PatternLayout m_priorityLayout;
            krb_log4cpp::PatternLayout m_categoryLayout;
            krb_log4cpp::PatternLayout m_messageLayout;
            /// cash for messages
            std::vector<util::Hash>  m_logCache;
            bool m_ok;
        };

    }
}

#endif

