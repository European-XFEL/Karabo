/* 
 * File:   NetworkAppenderConfigurator.hh
 * Author: irinak
 *
 * Created on September 21, 2011, 3:14 PM
 */

#ifndef KARABO_LOGCONFIG_NETWORKAPPENDERCONFIGURATOR_HH
#define	KARABO_LOGCONFIG_NETWORKAPPENDERCONFIGURATOR_HH

#include <karabo/util/Factory.hh>
#include <karabo/net/BrokerConnection.hh>
#include <krb_log4cpp/Appender.hh>
#include "AppenderConfigurator.hh"

namespace karabo {
    namespace log {

        class NetworkAppenderConfigurator : public AppenderConfigurator {

        public:
            KARABO_CLASSINFO(NetworkAppenderConfigurator, "Network", "1.0")

            NetworkAppenderConfigurator();

            virtual ~NetworkAppenderConfigurator();

            krb_log4cpp::Appender* create();

            static void expectedParameters(karabo::util::Schema& expected);
            NetworkAppenderConfigurator(const karabo::util::Hash& input);

        private:
            karabo::net::BrokerConnection::Pointer m_connection;

        };
    }
}

#endif	/* KARABO_LOGCONFIG_NETWORKAPPENDERCONFIGURATOR_HH */

