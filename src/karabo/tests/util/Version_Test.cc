/* 
 * File:   Version_Test.cc
 * Author: cas
 * 
 * Created on February 11, 2016, 2:23 PM
 */

#include "Version_Test.hh"
#include <karabo/util/Version.hh>

CPPUNIT_TEST_SUITE_REGISTRATION(Version_Test);


Version_Test::Version_Test() {
}


Version_Test::~Version_Test() {
}


void Version_Test::testVersion() {
    std::clog << "### KARABO VERSION: " << karabo::util::Version::getVersion() << " ###" << std::endl;
}

void Version_Test::testVersionFromString(){
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

void Version_Test::testVersionComparison(){
    std::vector<std::tuple<std::string /* v1 */, std::string /* v2 */, bool /* v1>=v2 */>> tests;



    tests.push_back(std::tuple<std::string, std::string, bool>(
        "99.1.1", "100.1.0", false ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.0.1", "100.1.0", false ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.1", "100.1.0", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0", "100.1.0", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0.dev1", "100.1.0", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0", "100.1.0.dev1", false ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0.dev2", "100.1.0.dev1", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.1", "100.1.0", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0.post1", "100.1.0", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.1.post1", "100.1.0", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.1rc0", "100.1.0", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.1rc1", "100.1.1rc1", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.1rc1.dev1", "100.1.1rc1", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.1rc1", "100.1.1rc1.dev1", false ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0rc0", "100.1.0", false ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0.dev0", "100.1.0", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0a1", "100.1.0a2", false ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0a2", "100.1.0a1", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0a21", "100.1.0b1", false ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0rc1", "100.1.0b1", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0.post1", "100.1.0a10", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0.post1", "100.1.0b12", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0.post1", "100.1.0rc42", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0.post1", "100.1.0", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0.post1", "100.1.0.post0", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "100.1.0.post1", "100.1.0.post2", false ));
    // Tests if the instance info correctly reports scene availability
    for (const auto& test : tests){
        const std::string &version1 = std::get<0>(test);
        const std::string &version2 = std::get<1>(test);
        const bool lte = std::get<2>(test);
        std::string message = "Failed calculating '" + version1 + "' >= '" + version2 + "'";
        karabo::util::Version v1(version1);
        karabo::util::Version v2(version2);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(message, lte, (v1 >= v2));
    }
}