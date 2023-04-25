/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Version_Test.cc
 * Author: cas
 *
 * Created on February 11, 2016, 2:23 PM
 */

#include "Version_Test.hh"

#include <karabo/util/Version.hh>
#include <karabo/util/repositoryVersion>

CPPUNIT_TEST_SUITE_REGISTRATION(Version_Test);


Version_Test::Version_Test() {}


Version_Test::~Version_Test() {}


void Version_Test::testVersion() {
    std::clog << "### KARABO VERSION: " << karabo::util::Version::getVersion() << " ###" << std::endl;
    const karabo::util::Version& v = karabo::util::Version::getKaraboVersion();
    CPPUNIT_ASSERT_EQUAL(karabo::util::Version::getVersion(), v.getString());
    CPPUNIT_ASSERT_EQUAL(karabo::util::Version::getVersion(), std::string(KARABO_VERSION)); // from repositoryVersion
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