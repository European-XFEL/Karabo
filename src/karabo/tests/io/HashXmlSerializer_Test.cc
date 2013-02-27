/* 
 * File:   HashXmlSerializer_Test.cc
 * Author: heisenb
 * 
 * Created on February 25, 2013, 6:03 PM
 */

#include <karabo/io/HashXmlSerializer.hh>
#include "HashXmlSerializer_Test.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(HashXmlSerializer_Test);

using namespace karabo::io;
using namespace karabo::util;

HashXmlSerializer_Test::HashXmlSerializer_Test() {
}

HashXmlSerializer_Test::~HashXmlSerializer_Test() {
}

void HashXmlSerializer_Test::setUp() {
}

void HashXmlSerializer_Test::tearDown() {
}

void HashXmlSerializer_Test::testSave() {
    Hash h("a.b.c", 1, "a.b.d", vector<int>(5,1), "a.b.e", vector<Hash>(2, Hash("a", 1)));
    h.setAttribute("a", "a1", true);
    h.setAttribute("a", "a2", 3.4);
    h.setAttribute("a.b", "b1", "3");
    h.setAttribute("a.b.c", "c1", 2);
    
    TextSerializer<Hash>::Pointer p = TextSerializer<Hash>::create("Xml");
    std::string archive;
    p->save(h, archive);
    
    cout << endl << archive << endl;
    
    Hash h2;
    p->load(h2, archive);
    cout << endl << h2 << endl;
    
    
}

void HashXmlSerializer_Test::testLoad() {
    
}
