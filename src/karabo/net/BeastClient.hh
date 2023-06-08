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
#include <boost/version.hpp>
#if BOOST_VERSION < 107000
#include "BeastClientBase_1_68.hh"
#else
#include "BeastClientBase_1_81.hh"
#endif

namespace karabo {
    namespace net {
        int httpsPost(const std::string& host, const std::string& port, const std::string& target,
                      const std::string& body, int version, const std::function<void(bool, std::string)>& onComplete);
    }
} // namespace karabo
