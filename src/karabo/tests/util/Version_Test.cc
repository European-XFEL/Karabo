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
