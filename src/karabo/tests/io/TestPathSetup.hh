/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   TestPathSetup.hh
 * Author: heisenb
 *
 * Created on March 6, 2013, 6:32 PM
 */

#ifndef KARABO_TESTPATHSETUP_HH
#define KARABO_TESTPATHSETUP_HH

#include <boost/filesystem.hpp>

inline std::string resourcePath(const std::string& filename) {
    boost::filesystem::path testPath(KARABO_TESTPATH);
    testPath /= "io/resources";
    testPath /= filename;
    return testPath.normalize().string();
}

#endif
