/*
 *
 */

#ifndef KARABO_LOGCONFIG_NETWORKAPPENDER_HH
#define	KARABO_LOGCONFIG_NETWORKAPPENDER_HH

#include "karabo/util/Hash.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/net/Broker.hh"
#include <krb_log4cpp/LayoutAppender.hh>
#include <krb_log4cpp/PatternLayout.hh>
#include <boost/asio/deadline_timer.hpp>
#include <vector>


namespace karabo {

    namespace net {
        // Forward
        class Broker;
    }

    namespace log {

        // Forward declare
        class Log4CppNetApp;
        class NetworkFilter;

        /**
         * Helper class to configure an underlying log4cpp appender.
         * NOTE: Do NOT use this class directly. It is indirectly involved in the static functions
         * of the Logger!!
         */
        class NetworkAppender {

        public:

            KARABO_CLASSINFO(NetworkAppender, "NetworkAppender", "");

            static void expectedParameters(karabo::util::Schema& s);

            NetworkAppender(const karabo::util::Hash& config);

            virtual ~NetworkAppender() {
            }

            krb_log4cpp::Appender* getAppender();

        private:

            Log4CppNetApp* m_appender;
        };

        /** This class is used to propagate log messages through the broker.
         *
         * It sends via a header(Hash("target", "log")) and a body.
         * The latter is a hash with a single key("messages") The corresponding value
         * is a std::vector<Hash> containing one Hash for each message. Its keys are
         * "timestamp", "type", "category" and "message".
         */
        class Log4CppNetApp : public krb_log4cpp::LayoutAppender {

        public:

            KARABO_CLASSINFO(Log4CppNetApp, "Log4CppNetApp", "");

            Log4CppNetApp(const karabo::util::Hash& config);

            virtual ~Log4CppNetApp();

            virtual bool reopen();

            virtual void close();

            krb_log4cpp::Appender* getAppender();

        protected: // functions
            virtual void _append(const krb_log4cpp::LoggingEvent& event);

        private: // functions

            void startLogWriting();

            void checkLogCache(const boost::system::error_code& e);

            void writeNow();

        private: // members

            boost::shared_ptr<karabo::net::Broker> m_producer;
            std::string m_topic;
            unsigned int m_interval;
            unsigned int m_maxMessages;

            /// layouts for each component
            krb_log4cpp::PatternLayout m_timeLayout;
            krb_log4cpp::PatternLayout m_priorityLayout;
            krb_log4cpp::PatternLayout m_categoryLayout;
            krb_log4cpp::PatternLayout m_messageLayout;
            /// cash for messages
            boost::mutex m_mutex;
            std::vector<karabo::util::Hash> m_logCache;
            boost::asio::deadline_timer m_timer;
        };


        class NetworkFilter : public krb_log4cpp::Filter {
        public:

            NetworkFilter();

            virtual ~NetworkFilter();

        protected:

            krb_log4cpp::Filter::Decision _decide(const krb_log4cpp::LoggingEvent& event);

        };
    }
}

#endif

