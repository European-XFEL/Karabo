/*
 * $Id: Connection.hh 3602 2011-05-31 21:09:02Z esenov@DESY.DE $
 *
 * File:   AConnection.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 3, 2011, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_NET_ACONNECTION_HH
#define	KARABO_NET_ACONNECTION_HH

#include <string>
#include <boost/signals2.hpp>
#include <boost/shared_ptr.hpp>
#include <karabo/util/Factory.hh>
#include <karabo/io/Format.hh>

#include "IOService.hh"
#include "netdll.hh"

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
        typedef boost::shared_ptr<Channel> ChannelPointer;
        typedef boost::function<void (ChannelPointer, const std::string&) > ErrorHandler;

        /**
         * The Connection class.
         * This class serves as the interface for all connections.
         * A connection is only established upon call of the start() function.
         */
        class Connection : public boost::enable_shared_from_this<Connection> {
            friend class Channel;

        public:

            KARABO_CLASSINFO(Connection, "Connection", "1.0")
            KARABO_FACTORY_BASE_CLASS

            typedef boost::function<void (ChannelPointer) > ConnectionHandler;


            typedef karabo::io::Format<karabo::util::Hash> HashFormat;

            virtual ~Connection() {
            }

            static void expectedParameters(karabo::util::Schema& expected);

            void configure(const karabo::util::Hash& input);

            /**
             * Starts the connection
             */
            virtual ChannelPointer start() = 0;

            /**
             * Starts the connection asynchronously
             */
            virtual void startAsync(const ConnectionHandler& handler) {
                throw NOT_SUPPORTED_EXCEPTION("Asynchronous connect is not available for " + this->classInfo().getClassId() + "connections");
            }

            /**
             * Stops the connection
             */
            virtual void stop() = 0;

            /**
             * Closes the connection
             */
            virtual void close() = 0;

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
             * @param Handler as a function object of ErrorHandler type
             * @return void
             */
            void setErrorHandler(const ErrorHandler& handler) {
                m_errorHandler = handler;
            }

            /**
             * @return size of message length field in protocol
             * 
             */
            std::size_t getSizeofLength() {
                return m_sizeofLength;
            }

            /**
             * @return true if content of message_length field should be considered as text string
             */
            bool lengthIsText() {
                return m_lengthIsTextFlag;
            }
            
        protected: // functions

            void registerChannel(ChannelPointer channel) {
                boost::mutex::scoped_lock lock(m_channelMutex);
                m_channels.insert(channel);
            }

            void setIOServiceType(const std::string& serviceType);


        protected: // members

            IOService::Pointer m_service;
            ErrorHandler m_errorHandler;

        private: // functions

            void hashToString(const karabo::util::Hash& hash, std::string& serializedHash);

            void stringToHash(const std::string& serializedHash, karabo::util::Hash& hash);

            void unregisterChannel(ChannelPointer channel) {
                boost::mutex::scoped_lock lock(m_channelMutex);
                std::set<ChannelPointer>::iterator it = m_channels.find(channel);
                if (it != m_channels.end()) {
                    m_channels.erase(it);
                }
            }

            boost::shared_ptr<Connection> getConnectionPointer() {
                return shared_from_this();
            }

            std::string getHashFormat() {
                return m_hashFormat->getClassInfo().getClassId();
            }

        private: // members

            HashFormat::Pointer m_hashFormat;

            std::set<ChannelPointer> m_channels;

            boost::mutex m_channelMutex;

            unsigned int m_sizeofLength;
            bool m_lengthIsTextFlag;
        };


    }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::net::Connection, TEMPLATE_NET, DECLSPEC_NET)

#endif	/* KARABO_NET_ACONNECTION_HH */
