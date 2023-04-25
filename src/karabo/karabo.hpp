/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   karabo.hpp
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 3, 2013, 3:27 PM
 */

#ifndef KARABO_HPP
#define KARABO_HPP

#include "karabo/core.hpp"
#include "karabo/io.hpp"
#include "karabo/log.hpp"
#include "karabo/net.hpp"
#include "karabo/util.hpp"
#include "karabo/xms.hpp"

#define USING_KARABO_NAMESPACES   \
    using namespace karabo::util; \
    using namespace karabo::io;   \
    using namespace karabo::net;  \
    using namespace karabo::log;  \
    using namespace karabo::xms;  \
    using namespace karabo::core;

#endif
