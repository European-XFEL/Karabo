/* 
 * File:   HashXmlSerializer_Test.cc
 * Author: heisenb
 * 
 * Created on February 25, 2013, 6:03 PM
 */

#include <karabo/io/HashXmlSerializer.hh>
#include "HashXmlSerializer_Test.hh"
#include "karabo/io/BinarySerializer.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(HashXmlSerializer_Test);

using namespace karabo::io;
using namespace karabo::util;

HashXmlSerializer_Test::HashXmlSerializer_Test() {
}

HashXmlSerializer_Test::~HashXmlSerializer_Test() {
}

void HashXmlSerializer_Test::setUp() {
    Hash h("a.b.c", 1, "a.b.d", vector<int>(5, 1)/*, "a.b.e", vector<Hash > (2, Hash("a", 1))*/);
    h.setAttribute("a", "a1", true);
    h.setAttribute("a", "a2", 3.4);
    h.setAttribute("a.b", "b1", "3");
    h.setAttribute("a.b.c", "c1", 2);
    m_hash = h;
}

void HashXmlSerializer_Test::tearDown() {
}

void HashXmlSerializer_Test::testSave() {

    {
        TextSerializer<Hash>::Pointer p = TextSerializer<Hash>::create("Xml", Hash("writeDataTypes", false, "readDataTypes", false));
        std::string archive;
        p->save(m_hash, archive);

        cout << "XML" << endl << archive << endl;
        
        Hash h;
        p->load(h, archive);
        cout << endl << h << endl;
    }
    
    {
        BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Default");
        std::vector<char> archive;
        p->save(m_hash, archive);
        
        
        //cout << "BINARY (size " << archive.size() << " bytes)" << endl << karabo::util::toString(archive) << endl;
        
        Hash h;
        p->load(h, archive);
        cout << endl << h << endl;
    }
}

void HashXmlSerializer_Test::testLoad() {

}
