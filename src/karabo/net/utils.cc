/*
 * $Id$
 *
 * Author: gero.flucke@xfel.eu>
 *
 * Created on February 11, 2016
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "karabo/net/utils.hh"

#include <boost/asio.hpp>

std::string karabo::net::bareHostName()
{
    std::string hostName = boost::asio::ip::host_name();

    // Find first dot and erase everything after it:
    const std::string::size_type dotPos = hostName.find('.');
    if (dotPos != std::string::npos) hostName.erase(dotPos);

    return hostName;
}
