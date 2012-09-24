/* 
 * File:   SnmpConnection.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on July 15, 2011, 2:30 PM
 */

#ifndef EXFEL_NET_SNMPCONNECTION_HH
#define	EXFEL_NET_SNMPCONNECTION_HH

#include <boost/shared_ptr.hpp>
#include <string>
#include "Connection.hh"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>


/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package net
     */
    namespace net {

        //class Channel;
        //class SnmpChannel;
        typedef boost::system::error_code ErrorCode;
        //typedef boost::shared_ptr<Channel> ChannelPointer;
        //typedef boost::shared_ptr<SnmpChannel> SnmpChannelPointer;

        // @DISCUSS Forward to later declared class?
        //class SnmpConnection;
        //typedef boost::shared_ptr<SnmpConnection > SnmpConnectionPointer;
            
        /**
         * The Connection class.
         * This class serves as the interface for all connections.
         * A connection is only established upon call of the start() function.
         */
        class SnmpConnection : public Connection {
            friend class SnmpChannel;

        public:

            EXFEL_CLASSINFO(SnmpConnection, "Snmp", "1.0")


            SnmpConnection();

            virtual ~SnmpConnection() {
            }

            static void expectedParameters(exfel::util::Schema& expected);

            void configure(const exfel::util::Hash& input);

            /**
             * Starts the connection
             */
            virtual ChannelPointer start();

            /**
             * Starts the connection asynchronously providing a slot
             */
            virtual void startAsync(const ConnectionHandler& slot);

            /**
             * Stops the connection
             */
            virtual void stop();

            /**
             * Closes the connection
             */
            virtual void close();

            /**
             * This function creates a "channel" for the given connection.
             * @return Pointer to Channel
             */
            virtual ChannelPointer createChannel();

            void setService(IOService::Pointer service) {
                m_service = service;
            }

        private:
            
            std::string m_hostname;
            unsigned int m_port;
            long m_version;
            std::string m_community;
            std::string m_aliasMode;
            exfel::util::Schema m_schema;
        };
    }
}

#endif	/* EXFEL_NET_SNMPCONNECTION_HH */

