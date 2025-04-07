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
 * File:   karabo.hpp
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 3, 2013, 3:27 PM
 */

#ifndef KARABO_HPP
#define KARABO_HPP

#include "karabo/core.hpp"
#include "karabo/data.hpp"
#include "karabo/log.hpp"
#include "karabo/net.hpp"
#include "karabo/util.hpp"
#include "karabo/xms.hpp"

#define USING_KARABO_NAMESPACES   \
    using namespace karabo::data; \
    using namespace karabo::util; \
    using namespace karabo::net;  \
    using namespace karabo::log;  \
    using namespace karabo::xms;  \
    using namespace karabo::core;

#endif
