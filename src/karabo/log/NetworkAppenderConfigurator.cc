/* 
 * File:   NetworkAppenderConfigurator.cc
 * Author: irinak
 * 
 * Created on September 21, 2011, 3:14 PM
 */

#include "NetworkAppenderConfigurator.hh"
#include "NetworkAppender.hh"

using namespace karabo::util;

namespace karabo {
    namespace log {

        KARABO_REGISTER_FACTORY_CC(AppenderConfigurator, NetworkAppenderConfigurator)

        NetworkAppenderConfigurator::NetworkAppenderConfigurator() {
        }

        NetworkAppenderConfigurator::~NetworkAppenderConfigurator() {
        }

        log4cpp::Appender* NetworkAppenderConfigurator::create() {
            return new NetworkAppender(getName(), m_connection->createChannel());
        }

        void NetworkAppenderConfigurator::expectedParameters(Schema& expected) {

            CHOICE_ELEMENT<karabo::net::BrokerConnection > (expected)
                    .key("connection")
                    .displayedName("Connection")
                    .description("Connection")
                    .assignmentOptional().defaultValue("Jms")
                    .commit();
            
            //OVERWRITE_ELEMENT
        }

        void NetworkAppenderConfigurator::configure(const Hash& input) {
            m_connection = karabo::net::BrokerConnection::createChoice("connection", input);
            m_connection->start();
        }

    }
}


