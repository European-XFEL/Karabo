/* 
 * File:   NetworkAppenderConfigurator.cc
 * Author: irinak
 * 
 * Created on September 21, 2011, 3:14 PM
 */

#include "NetworkAppenderConfigurator.hh"
#include "NetworkAppender.hh"
#include <karabo/util/ChoiceElement.hh>

using namespace karabo::util;
using namespace karabo::log;

namespace karabo {
    namespace net {


        KARABO_REGISTER_FOR_CONFIGURATION(AppenderConfigurator, NetworkAppenderConfigurator)

        NetworkAppenderConfigurator::~NetworkAppenderConfigurator() {
        }


        krb_log4cpp::Appender* NetworkAppenderConfigurator::create() {
            return new NetworkAppender(getName(), m_connection->createChannel());
        }


        void NetworkAppenderConfigurator::expectedParameters(Schema& expected) {

            CHOICE_ELEMENT(expected)
                    .key("connection")
                    .displayedName("Connection")
                    .description("Connection")
                    .appendNodesOfConfigurationBase<BrokerConnection>()
                    .assignmentOptional().defaultValue("Jms")
                    .commit();
        }


        NetworkAppenderConfigurator::NetworkAppenderConfigurator(const Hash& input) : AppenderConfigurator(input) {
            m_connection = BrokerConnection::createChoice("connection", input);
            m_connection->start();
        }

    }
}


