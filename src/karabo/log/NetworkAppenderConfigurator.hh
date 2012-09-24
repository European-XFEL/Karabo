/* 
 * File:   NetworkAppenderConfigurator.hh
 * Author: irinak
 *
 * Created on September 21, 2011, 3:14 PM
 */

#ifndef EXFEL_LOGCONFIG_NETWORKAPPENDERCONFIGURATOR_HH
#define	EXFEL_LOGCONFIG_NETWORKAPPENDERCONFIGURATOR_HH

#include <karabo/util/Factory.hh>
#include <karabo/net/BrokerConnection.hh>
#include <log4cpp/Appender.hh>
#include "AppenderConfigurator.hh"

namespace exfel {
    namespace log {

        class NetworkAppenderConfigurator : public AppenderConfigurator {
        public:
            EXFEL_CLASSINFO(NetworkAppenderConfigurator, "Network", "1.0")

            NetworkAppenderConfigurator();

            virtual ~NetworkAppenderConfigurator();

            log4cpp::Appender* create();

            static void expectedParameters(exfel::util::Schema& expected);
            void configure(const exfel::util::Hash& input);

        private:
            exfel::net::BrokerConnection::Pointer m_connection;

        };

    }
}


#endif	/* EXFEL_LOGCONFIG_NETWORKAPPENDERCONFIGURATOR_HH */

