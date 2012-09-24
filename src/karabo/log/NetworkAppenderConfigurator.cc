/* 
 * File:   NetworkAppenderConfigurator.cc
 * Author: irinak
 * 
 * Created on September 21, 2011, 3:14 PM
 */

#include "NetworkAppenderConfigurator.hh"
#include "NetworkAppender.hh"

using namespace exfel::util;

namespace exfel {
    namespace log {

        EXFEL_REGISTER_FACTORY_CC(AppenderConfigurator, NetworkAppenderConfigurator)

        NetworkAppenderConfigurator::NetworkAppenderConfigurator() {
        }

        NetworkAppenderConfigurator::~NetworkAppenderConfigurator() {
        }

        log4cpp::Appender* NetworkAppenderConfigurator::create() {
            return new NetworkAppender(getName(), m_connection->createChannel());
        }

        void NetworkAppenderConfigurator::expectedParameters(Schema& expected) {

            CHOICE_ELEMENT<exfel::net::BrokerConnection > (expected)
                    .key("connection")
                    .displayedName("Connection")
                    .description("Connection")
                    .assignmentOptional().defaultValue("Jms")
                    .commit();
            
            //OVERWRITE_ELEMENT
        }

        void NetworkAppenderConfigurator::configure(const Hash& input) {
            m_connection = exfel::net::BrokerConnection::createChoice("connection", input);
            m_connection->start();
        }

    }
}


