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

#define KARABO_ERROR_CODE_SUCCESS boost::system::errc::make_error_code(boost::system::errc::success)
#define KARABO_ERROR_CODE_WRONG_PROTOCOL boost::system::errc::make_error_code(boost::system::errc::wrong_protocol_type)
#define KARABO_ERROR_CODE_IO_ERROR boost::system::errc::make_error_code(boost::system::errc::io_error)
#define KARABO_ERROR_CODE_CONNECT_REFUSED boost::system::errc::make_error_code(boost::system::errc::connection_refused)
#define KARABO_ERROR_CODE_OP_CANCELLED boost::system::errc::make_error_code(boost::system::errc::operation_canceled)
#define KARABO_ERROR_CODE_NOT_CONNECTED boost::system::errc::make_error_code(boost::system::errc::not_connected)
#define KARABO_ERROR_CODE_ALREADY_CONNECTED boost::system::errc::make_error_code(boost::system::errc::already_connected)

namespace karabo::net {

    //----- https://akrzemi1.wordpress.com/2017/07/12/your-own-error-code/ -----v

    enum class AmqpCppErrc {
        eCreateChannelError = 1000,
        eCreateExchangeError, // TODO: Rename to eDeclareExchangeError
        eCreateQueueError,    // TODO: Rename to eDeclareQueueError
        eBindQueueError,
        eCreateConsumerError,
        eUnbindQueueError,
        eDrop,
        eMessageDrop
    };

} // namespace karabo::net


namespace boost::system {
    template <>
    struct is_error_code_enum<karabo::net::AmqpCppErrc> : true_type {};
} // namespace boost::system


namespace karabo::net {


    boost::system::error_code make_error_code(AmqpCppErrc e);

    //----- https://akrzemi1.wordpress.com/2017/07/12/your-own-error-code/ -----^

} /* namespace karabo::net */


#endif /* KARABO_NET_AMQPUTILS_HH */
