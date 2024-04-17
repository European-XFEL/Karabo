/*
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
/*
 * File:   AmqpUtils.hh
 * Author: gero.flucke@xfel.eu, based on code from serguei.essenov@xfel.eu
 *
 * Created on Apr 16, 2024
 */

#ifndef KARABO_NET_AMQPUTILS_HH
#define KARABO_NET_AMQPUTILS_HH

#include <boost/asio.hpp>
// // neither this
// #include <boost/asio/error.hpp>
// // nor these
// #include <boost/system/errc.hpp>
// #include <boost/system/error_code.hpp>
// #include <boost/system/is_error_code_enum.hpp> // error_code.hpp>
// #include <boost/system/is_error_condition_enum.hpp>
// // nor both are enough without boost/asio.hpp...

#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>

// TODO: Once AmqpClient.hh with Transceiver etc. is gone, chek which codes are needed
#define KARABO_ERROR_CODE_SUCCESS boost::system::errc::make_error_code(boost::system::errc::success)
#define KARABO_ERROR_CODE_WRONG_PROTOCOL boost::system::errc::make_error_code(boost::system::errc::wrong_protocol_type)
#define KARABO_ERROR_CODE_IO_ERROR boost::system::errc::make_error_code(boost::system::errc::io_error)
#define KARABO_ERROR_CODE_CONNECT_REFUSED boost::system::errc::make_error_code(boost::system::errc::connection_refused)
#define KARABO_ERROR_CODE_OP_CANCELLED boost::system::errc::make_error_code(boost::system::errc::operation_canceled)
#define KARABO_ERROR_CODE_NOT_CONNECTED boost::system::errc::make_error_code(boost::system::errc::not_connected)
#define KARABO_ERROR_CODE_ALREADY_CONNECTED boost::system::errc::make_error_code(boost::system::errc::already_connected)
#define KARABO_ERROR_CODE_TIMED_OUT boost::system::errc::make_error_code(boost::system::errc::timed_out)
#define KARABO_ERROR_CODE_STREAM_TIMEOUT boost::system::errc::make_error_code(boost::system::errc::stream_timeout)
#define KARABO_ERROR_CODE_RESOURCE_BUSY \
    boost::system::errc::make_error_code(boost::system::errc::device_or_resource_busy)

namespace karabo::net {

    //----- https://akrzemi1.wordpress.com/2017/07/12/your-own-error-code/ -----v

    // TODO: Once AmqpClient.hh with Transceiver etc. is gone, check which enum values are needed
    enum class AmqpCppErrc {
        eCreateChannelError = 1000,
        eCreateExchangeError,
        eCreateQueueError,
        eBindQueueError,
        eCreateConsumerError,
        eConsumerCancelError,
        eUnbindQueueError,
        eDrop
    };

} // namespace karabo::net


namespace boost::system {
    template <>
    struct is_error_code_enum<karabo::net::AmqpCppErrc> : true_type {};
} // namespace boost::system


namespace karabo::net {


    boost::system::error_code make_error_code(AmqpCppErrc e);

    //----- https://akrzemi1.wordpress.com/2017/07/12/your-own-error-code/ -----^

    //------------------------------------------------------------------------------------ ConnectionHandler

    // TODO: Once AmqpClient.hh with Transceiver etc. is gone, this might move to AmqpConnection.hh
    /**
     * @brief Declare our custom ConnectionHandler to implement their callbacks.
     * Every callback delegates processing to externally assigned callback if it was set.
     * ConnectionHandler object calls its callbacks as follows...
     * while connecting...
     *     onAttached, onConnected, onReady
     * in case of error ...
     *     onError, (onLost), onDetached
     * in case of normal closure ...
     *     onClosed, onDetached
     *
     * onAttached is called if new connection is associated with the handler
     * onConnected is called if physical (TCP) connection is successfully established.
     * onReady is called if login is successful
     * onClosed is called in case of normal connection closure
     * onError is called if errors encountered on connection
     * onLost is called if onError was called and earlier onConnected was called.
     * onDetached id called as a last step of connection loss
     */
    class ConnectionHandler : public virtual AMQP::LibBoostAsioHandler {
       public:
        typedef std::function<void(AMQP::TcpConnection*)> ConnectionHandlerCallback;
        typedef std::function<void(AMQP::TcpConnection*, const char*)> ConnectionErrorCallback;

       private:
        ConnectionHandlerCallback m_onAttached;
        ConnectionHandlerCallback m_onConnected;
        ConnectionHandlerCallback m_onReady;
        ConnectionErrorCallback m_onError;
        ConnectionHandlerCallback m_onClosed;
        ConnectionHandlerCallback m_onLost;
        ConnectionHandlerCallback m_onDetached;

       private:
        ConnectionHandler() = delete;

        ConnectionHandler(ConnectionHandler&&) = delete;
        ConnectionHandler(const ConnectionHandler&) = delete;

       public:
        explicit ConnectionHandler(boost::asio::io_context& ctx);

        virtual ~ConnectionHandler() = default;

        void onAttached(AMQP::TcpConnection* connection) override {
            if (m_onAttached) m_onAttached(connection);
        }

        void setOnAttachedHandler(ConnectionHandlerCallback&& cb) {
            m_onAttached = std::move(cb);
        }

        void onConnected(AMQP::TcpConnection* connection) override {
            if (m_onConnected) m_onConnected(connection);
        }

        void setOnConnectedHandler(ConnectionHandlerCallback&& cb) {
            m_onConnected = std::move(cb);
        }

        void onReady(AMQP::TcpConnection* connection) override {
            if (m_onReady) m_onReady(connection);
        }

        void setOnReadyHandler(ConnectionHandlerCallback&& cb) {
            m_onReady = std::move(cb);
        }

        void onError(AMQP::TcpConnection* connection, const char* message) override {
            if (m_onError) m_onError(connection, message);
        }

        void setOnErrorHandler(ConnectionErrorCallback&& cb) {
            m_onError = std::move(cb);
        }

        void onClosed(AMQP::TcpConnection* connection) override {
            if (m_onClosed) m_onClosed(connection);
        }

        void setOnClosedHandler(ConnectionHandlerCallback&& cb) {
            m_onClosed = std::move(cb);
        }

        void onLost(AMQP::TcpConnection* connection) override {
            if (m_onLost) m_onLost(connection);
        }

        void setOnLostHandler(ConnectionHandlerCallback&& cb) {
            m_onLost = std::move(cb);
        }

        void onDetached(AMQP::TcpConnection* connection) override {
            if (m_onDetached) m_onDetached(connection);
        }

        void setOnDetachedHandler(ConnectionHandlerCallback&& cb) {
            m_onDetached = std::move(cb);
        }
    };

} /* namespace karabo::net */


#endif /* KARABO_NET_AMQPUTILS_HH */
