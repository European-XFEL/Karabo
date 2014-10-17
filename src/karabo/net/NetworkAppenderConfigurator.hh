/* 
 * File:   NetworkAppenderConfigurator.hh
 * Author: irinak
 *
 * Created on September 21, 2011, 3:14 PM
 */

#ifndef KARABO_LOGCONFIG_NETWORKAPPENDERCONFIGURATOR_HH
#define	KARABO_LOGCONFIG_NETWORKAPPENDERCONFIGURATOR_HH

#include <krb_log4cpp/Appender.hh>
#include <karabo/log/AppenderConfigurator.hh>
#include <karabo/util/Configurator.hh>

#include "BrokerConnection.hh"

namespace karabo {
    namespace net {

        class NetworkAppenderConfigurator : public karabo::log::AppenderConfigurator {

        public:
            KARABO_CLASSINFO(NetworkAppenderConfigurator, "Network", "1.0")

            virtual ~NetworkAppenderConfigurator();

            krb_log4cpp::Appender* create();

            static void expectedParameters(karabo::util::Schema& expected);

            NetworkAppenderConfigurator(const karabo::util::Hash& input);

        private:
            BrokerConnection::Pointer m_connection;

        };
    }
}

#endif	/* KARABO_LOGCONFIG_NETWORKAPPENDERCONFIGURATOR_HH */

