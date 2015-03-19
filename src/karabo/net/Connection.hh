/*
 * $Id: Connection.hh 3602 2011-05-31 21:09:02Z esenov@DESY.DE $
 *
 * File:   Connection.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 3, 2011, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_NET_CONNECTION_HH
#define	KARABO_NET_CONNECTION_HH

#include <string>
#include <boost/shared_ptr.hpp>
#include <karabo/util/Factory.hh>
#include "IOService.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {

        typedef boost::system::error_code ErrorCode;

        class Channel;
        
        typedef boost::function<void (const ErrorCode&) > ErrorHandler;

        /**
         * The Connection class.
         * This class serves as the interface for all connections.
         * A connection is only established upon call of the start() function.
         */
        class Connection : public boost::enable_shared_from_this<Connection> {

            friend class Channel;
            
        protected:
            
            typedef boost::shared_ptr<Channel> ChannelPointer;

        public:

            KARABO_CLASSINFO(Connection, "Connection", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            typedef boost::function<void (const ChannelPointer&) > ConnectionHandler;

            virtual ~Connection() {
            }

            static void expectedParameters(karabo::util::Schema& expected);

            Connection(const karabo::util::Hash& input);

            /**
             * Starts the connection
             */
            virtual ChannelPointer start() = 0;

            /**
             * Starts the connection asynchronously
             * @param handler A callback with the following signature: void myHandler(ChannelPointer)
             */
            virtual void startAsync(const ConnectionHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Asynchronous connect is not available for " + this->classInfo().getClassId() + "connections");
            }

            /**
             * Stops the connection
             */
            virtual void stop() = 0;


            /**
             * This function creates a "channel" for the given connection.
             * @return Pointer to Channel
             */
            virtual ChannelPointer createChannel() = 0;

            /**
             * This function returns a pointer to a IO service that had to be
             * injected via configuration parameter
             * @return a pointer to an IO service instance
             */
            IOService::Pointer getIOService() const;

            void setIOService(const IOService::Pointer& ioService);

            /**
             * This function sets the error handler that will be called if connection process failed
             * @param Call-back function of signature: void (boost::shared_ptr<Channel>, const ErrorCode&)
             * @return void
             */
            void setErrorHandler(const ErrorHandler& handler) {
                m_errorHandler = handler;
            }

        protected: // functions

            void setIOServiceType(const std::string& serviceType);

            boost::shared_ptr<Connection> getConnectionPointer() {
                return shared_from_this();
            }

        protected: // members

            IOService::Pointer m_service;
            ErrorHandler m_errorHandler;
            std::string m_serializationType;

        };


    }
}

// TODO Windows
//KARABO_REGISTER_FACTORY_BASE_HH(karabo::net::Connection, TEMPLATE_NET, DECLSPEC_NET)

#endif	/* KARABO_NET_ACONNECTION_HH */
