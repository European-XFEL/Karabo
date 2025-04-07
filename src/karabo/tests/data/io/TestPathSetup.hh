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
 * File:   TestPathSetup.hh
 * Author: heisenb
 *
 * Created on March 6, 2013, 6:32 PM
 */

#ifndef KARABO_TESTPATHSETUP_HH
#define KARABO_TESTPATHSETUP_HH

#include <filesystem>

inline std::string resourcePath(const std::string& filename) {
    std::filesystem::path testPath(KARABO_TESTPATH);
    testPath /= "data";
    testPath /= "io";
    testPath /= "resources";
    testPath /= filename;
    return testPath.lexically_normal().string();
}

#endif
