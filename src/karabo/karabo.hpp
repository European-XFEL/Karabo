/* 
 * File:   karabo.hpp
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 3, 2013, 3:27 PM
 */

#ifndef KARABO_HPP
#define	KARABO_HPP

#include "karabo/util.hpp"
#include "karabo/log.hpp"
#include "karabo/webAuth.hpp"
#include "karabo/io.hpp"
#include "karabo/net.hpp"
#include "karabo/xms.hpp"
#include "karabo/xip.hpp"
#include "karabo/core.hpp"

// Deprecated, use USING_KARABO_NAMESPACES
#define KARABO_NAMESPACES \
using namespace karabo::util; \
using namespace karabo::io; \
using namespace karabo::net; \
using namespace karabo::log; \
using namespace karabo::xms; \
using namespace karabo::core; \
using namespace karabo::xip;

#define USING_KARABO_NAMESPACES \
using namespace karabo::util; \
using namespace karabo::io; \
using namespace karabo::net; \
using namespace karabo::log; \
using namespace karabo::xms; \
using namespace karabo::core; \
using namespace karabo::xip;

#endif
