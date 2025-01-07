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
 * File:   Version_Test.cc
 * Author: cas
 *
 * Created on February 11, 2016, 2:23 PM
 */

#include "Version_Test.hh"

#include <karabo/util/Version.hh>
#include <karabo/util/VersionMacros.hh>

CPPUNIT_TEST_SUITE_REGISTRATION(Version_Test);


Version_Test::Version_Test() {}


Version_Test::~Version_Test() {}


void Version_Test::testVersion() {
    std::clog << "### KARABO VERSION: " << karabo::util::Version::getVersion() << " ###" << std::endl;
    const karabo::util::Version& v = karabo::util::Version::getKaraboVersion();
    CPPUNIT_ASSERT_EQUAL(karabo::util::Version::getVersion(), v.getString());
    CPPUNIT_ASSERT_EQUAL(karabo::util::Version::getVersion(), std::string(KARABO_VERSION)); // from VersionMacros.hh
}

void Version_Test::testVersionFromString() {
    karabo::util::Version v("12.2.3");
    CPPUNIT_ASSERT_EQUAL(12, v.getMajor());
    CPPUNIT_ASSERT_EQUAL(2, v.getMinor());
    CPPUNIT_ASSERT_EQUAL(3, v.getPatch());
    CPPUNIT_ASSERT_EQUAL(false, v.isPreRelease());
    CPPUNIT_ASSERT_EQUAL(false, v.isPostRelease());
    CPPUNIT_ASSERT_EQUAL(false, v.isDevRelease());
    v = karabo::util::Version("12.2.3rc32");
    CPPUNIT_ASSERT_EQUAL(12, v.getMajor());
    CPPUNIT_ASSERT_EQUAL(2, v.getMinor());
    CPPUNIT_ASSERT_EQUAL(3, v.getPatch());
    CPPUNIT_ASSERT_EQUAL(true, v.isPreRelease());
    CPPUNIT_ASSERT_EQUAL(false, v.isPostRelease());
    CPPUNIT_ASSERT_EQUAL(false, v.isDevRelease());
    v = karabo::util::Version("12.2.3.post32");
    CPPUNIT_ASSERT_EQUAL(12, v.getMajor());
    CPPUNIT_ASSERT_EQUAL(2, v.getMinor());
    CPPUNIT_ASSERT_EQUAL(3, v.getPatch());
    CPPUNIT_ASSERT_EQUAL(false, v.isPreRelease());
    CPPUNIT_ASSERT_EQUAL(true, v.isPostRelease());
    CPPUNIT_ASSERT_EQUAL(false, v.isDevRelease());
    v = karabo::util::Version("12.2.3rc32.dev21");
    CPPUNIT_ASSERT_EQUAL(12, v.getMajor());
    CPPUNIT_ASSERT_EQUAL(2, v.getMinor());
    CPPUNIT_ASSERT_EQUAL(3, v.getPatch());
    CPPUNIT_ASSERT_EQUAL(true, v.isPreRelease());
    CPPUNIT_ASSERT_EQUAL(false, v.isPostRelease());
    CPPUNIT_ASSERT_EQUAL(true, v.isDevRelease());
}

void Version_Test::testVersionComparison() {
    // release version comparisons
    std::vector<std::string> versionsInStrictOrder{"b00b1e5",
                                                   "0.0.0",
                                                   "0.0.0.dev2",
                                                   "99.0.0",
                                                   "99.0.0.dev0",
                                                   "100.0.0",
                                                   "100.0.0.dev2",
                                                   "100.0.1",
                                                   "100.0.1.dev3",
                                                   "100.1.0",
                                                   "100.1.0.dev1",
                                                   "100.1.1a1",
                                                   "100.1.1a1.dev1",
                                                   "100.1.1a2",
                                                   "100.1.1a2.dev1",
                                                   "100.1.1a2.dev1000",
                                                   "100.1.1b1",
                                                   "100.1.1b1.dev1",
                                                   "100.1.1b1.dev13",
                                                   "100.1.1b2",
                                                   "100.1.1b2.dev1",
                                                   "100.1.1b2.dev12",
                                                   "100.1.1rc1",
                                                   "100.1.1rc1.dev1",
                                                   "100.1.1rc1.dev15",
                                                   "100.1.1rc2",
                                                   "100.1.1",
                                                   "100.1.1.post1",
                                                   "100.1.1.post1.dev1",
                                                   "100.1.1.post1.dev3"};

    for (size_t i1 = 0; i1 < versionsInStrictOrder.size(); i1++) {
        for (size_t i2 = 0; i2 < versionsInStrictOrder.size(); i2++) {
            const std::string& version1 = versionsInStrictOrder[i1];
            const std::string& version2 = versionsInStrictOrder[i2];
            const karabo::util::Version v1(version1);
            const karabo::util::Version v2(version2);
            if (i1 < i2) {
                std::string message = "Failed calculating '" + version1 + "' < '" + version2 + "'";
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, true, (v1 < v2));
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, false, (v1 >= v2));
            }
            if (i1 == i2) {
                std::string message = "Failed calculating '" + version1 + "' == '" + version2 + "'";
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, true, (v1 == v2));
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, false, (v1 != v2));
            }
            if (i1 >= i2) {
                std::string message = "Failed calculating '" + version1 + "' >= '" + version2 + "'";
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, true, (v1 >= v2));
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, false, (v1 < v2));
            }
            if (i1 <= i2) {
                std::string message = "Failed calculating '" + version1 + "' <= '" + version2 + "'";
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, true, (v1 <= v2));
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, false, (v1 > v2));
            }
            if (i1 > i2) {
                std::string message = "Failed calculating '" + version1 + "' > '" + version2 + "'";
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, true, (v1 > v2));
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, false, (v1 <= v2));
            }
            if (i1 != i2) {
                std::string message = "Failed calculating '" + version1 + "' != '" + version2 + "'";
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, true, (v1 != v2));
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, false, (v1 == v2));
            }
        }
    }
}

void Version_Test::testVersionMacro() {
    CPPUNIT_ASSERT_LESS(KARABO_VERSION_NUM(1, 2, 3), KARABO_VERSION_NUM(0, 3, 4));
    CPPUNIT_ASSERT_LESS(KARABO_VERSION_NUM(1, 2, 3), KARABO_VERSION_NUM(1, 1, 4));
    CPPUNIT_ASSERT_LESS(KARABO_VERSION_NUM(1, 2, 3), KARABO_VERSION_NUM(1, 2, 2));

    // Minor and patch are supported up to 999
    CPPUNIT_ASSERT_LESS(KARABO_VERSION_NUM(2, 0, 0), KARABO_VERSION_NUM(1, 999, 999));

    // Exact represantation (test needed?)
    CPPUNIT_ASSERT_EQUAL(1'004'014, KARABO_VERSION_NUM(1, 4, 14));

    // Version macros are introduced far after Karabo 1.4.14
    CPPUNIT_ASSERT_GREATER(KARABO_VERSION_NUM(1, 4, 14), KARABO_VERSION_NUM_CURRENT);
}
