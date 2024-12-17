/*
 * $Id: Connection.hh 3602 2011-05-31 21:09:02Z esenov@DESY.DE $
 *
 * File:   Connection.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 3, 2011, 9:47 AM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef KARABO_NET_CONNECTION_HH
#define KARABO_NET_CONNECTION_HH

#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <string>

#include "karabo/util/Configurator.hh"
#include "karabo/util/Factory.hh"
#include "karabo/util/karaboDll.hh"


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

        typedef std::function<void(const ErrorCode&)> ErrorHandler;

        /**
         * @class Connection
         * @brief This class serves as the interface for all connections.
         * A connection is only established upon call of the start() function.
         */
        class Connection : public std::enable_shared_from_this<Connection> {
            friend class Channel;

           protected:
            typedef std::shared_ptr<Channel> ChannelPointer;

           public:
            KARABO_CLASSINFO(Connection, "Connection", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            typedef std::function<void(const ErrorCode&, const ChannelPointer&)> ConnectionHandler;

            virtual ~Connection();

            static void expectedParameters(karabo::util::Schema& expected);

            Connection(const karabo::util::Hash& input);

            /**
             * Starts the connection
             */
            virtual ChannelPointer start() = 0;

            /**
             * Starts the connection asynchronously
             * @param handler A callback with the following signature: void myHandler(ErrorCode, ChannelPointer)
             */
            virtual int startAsync(const ConnectionHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Asynchronous connect is not available for " +
                                                     this->classInfo().getClassId() + "connections");
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


           protected: // functions
            std::shared_ptr<Connection> getConnectionPointer() {
                return shared_from_this();
            }

           protected: // members
            std::string m_serializationType;
        };


    } // namespace net
} // namespace karabo

// TODO Windows
// KARABO_REGISTER_FACTORY_BASE_HH(karabo::net::Connection, TEMPLATE_NET, DECLSPEC_NET)

#endif /* KARABO_NET_ACONNECTION_HH */
