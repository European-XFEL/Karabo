/*
 * $Id: BrokerConnection.hh 6930 2012-08-03 10:45:21Z heisenb $
 *
 *   * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 2, 2010, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_NET_BROKERCONNECTION_HH
#define	KARABO_NET_BROKERCONNECTION_HH

#include <string>
#include <boost/signals2.hpp>
#include <boost/shared_ptr.hpp>
#include <karabo/util/Configurator.hh>
#include <karabo/io/BinarySerializer.hh>
#include <karabo/io/TextSerializer.hh>

#include "BrokerIOService.hh"


/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {

        class BrokerChannel;
        
        typedef boost::function<void (boost::shared_ptr<BrokerChannel>, const std::string&) > BrokerErrorHandler;

        /**
         * The BrokerConnection class.
         * This class serves as the interface for all connections.
         * A connection is only established upon call of the start() function.
         * The usage of stop() function is optional.  The user may create an object of
         * BrokerConnection class (BrokerConnection::Pointer) via usual factory 
         * interface using some configuration.
         * 
         * The BrokerConnection class is used by BrokerChannel class that implements
         * IO operations.  BrokerChannel objects are created by createChannel()
         * function and each contains BrokerConnection::Pointer inside.
         * So bookkeeping is automatic i.e. BrokerConnection object is alive until
         * at least one BrokerConnection::Pointer is still alive.  This preserves the
         * proper calling order of the destructors.
         */
        class BrokerConnection : public boost::enable_shared_from_this<BrokerConnection> {

            typedef boost::shared_ptr<BrokerChannel> BrokerChannelPointer;

            friend class BrokerChannel;

            boost::mutex m_channelMutex;

        public:

            KARABO_CLASSINFO(BrokerConnection, "Connection", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS;

            typedef boost::function<void (BrokerChannelPointer) > ConnectionHandler;

            virtual ~BrokerConnection() {
            }

            static void expectedParameters(karabo::util::Schema& expected);

            BrokerConnection(const karabo::util::Hash& input);

            /**
             * Starts the connection
             */
            virtual void start() = 0;

            /**
             * Stops the connection
             */
            virtual void stop() = 0;

            /**
             * Returns the broker hostname
             * @return hostname
             */
            virtual const std::string& getBrokerHostname() const = 0;
            
            /**
             * Returns the broker port
             * @return port
             */
            virtual unsigned int getBrokerPort() const = 0;
            
            /**
             * Returns the broker topic
             * @return topic
             */
            virtual const std::string& getBrokerTopic() const = 0;
                            
            /**
             * Returns vector of the brokers in cluster, or just empty vector
             * @return a vector of string elements of <hostname1>:<port1>, <hostname2>:<port2>, ...
             */
            virtual const std::vector<std::string>& getBrokerHosts() const = 0;
            
            /**
             * This function creates a "channel" for the given connection.
             * @return Pointer to BrokerChannel
             */
            virtual BrokerChannelPointer createChannel(const std::string& subDestination = "") = 0;

            /**
             * This function returns a pointer to a IO service that had to be
             * injected via configuration parameter
             * @return a pointer to an IO service instance
             */
            BrokerIOService::Pointer getIOService() const;

            void setIOService(const BrokerIOService::Pointer& ioService);

            /**
             * This function sets the error handler that will be called if connection process failed
             * @param Handler as a function object of BrokerErrorHandler type
             * @return void
             */
            void setErrorHandler(const BrokerErrorHandler& handler) {
                m_errorHandler = handler;
            }

        protected: // functions

            void setIOServiceType(const std::string& serviceType);

            boost::shared_ptr<BrokerConnection> getConnectionPointer() {
                return shared_from_this();
            }


        protected: // members

            BrokerIOService::Pointer m_service;
            BrokerErrorHandler m_errorHandler;
            std::string m_serializationType;

        };


    }
}

// TODO Windows
//KARABO_REGISTER_FACTORY_BASE_HH(karabo::net::BrokerConnection, TEMPLATE_NET, DECLSPEC_NET)

#endif
