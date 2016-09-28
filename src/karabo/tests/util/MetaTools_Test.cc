/* 
 * File:   MetaTools_Test.cc
 * Author: heisenb
 * 
 * Created on August 11, 2016, 11:18 AM
 */

#include "MetaTools_Test.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(MetaTools_Test);

using namespace karabo::util;

MetaTools_Test::MetaTools_Test() {
}


MetaTools_Test::~MetaTools_Test() {
}


void MetaTools_Test::testMethod() {

    CPPUNIT_ASSERT(PointerTest::isSharedPointer<boost::shared_ptr<int> >());
    CPPUNIT_ASSERT(!PointerTest::isSharedPointer<int>());

    CPPUNIT_ASSERT((karabo::util::is_base_of<Hash, MyPublicHash>::value));
    CPPUNIT_ASSERT((karabo::util::is_base_of<Hash, MyProtectedHash>::value));
    CPPUNIT_ASSERT((karabo::util::is_base_of<Hash, MyPrivateHash>::value));
    CPPUNIT_ASSERT((!karabo::util::is_base_of<Hash, int>::value));
}

void MetaTools_Test::testWeakBind(){
    std::vector<std::string> messages;
    _DeviceServer* d = new _DeviceServer(&messages);
    karabo::net::EventLoop::addThread(4);
    karabo::net::EventLoop::run();

    CPPUNIT_ASSERT(messages.size() >= 4);
    CPPUNIT_ASSERT(messages[0] == "_Device created");
    CPPUNIT_ASSERT(messages[1] == "Tick 5");
    CPPUNIT_ASSERT(messages[2] == "Tick 6");
    CPPUNIT_ASSERT(messages[messages.size()-1] == "_Device deleted");
    karabo::net::EventLoop::stop();
    delete d;
}
