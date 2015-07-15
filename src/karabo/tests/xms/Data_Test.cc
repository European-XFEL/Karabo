/* 
 * File:   Data_Test.cc
 * Author: flucke
 * 
 * Created on July 14, 2015, 9:42 AM
 */

#include "Data_Test.hh"

//#include <algorithm>

#include "karabo/xms.hpp"
#include "karabo/util.hpp"

using namespace karabo;

CPPUNIT_TEST_SUITE_REGISTRATION(Data_Test);

Data_Test::Data_Test() : m_vec(5) { }

Data_Test::~Data_Test() { }

void Data_Test::setUp()
{
    // C++11: std::iota(m_vec.begin(), m_vec.end(), 0);
    for (int i = 0; i < m_vec.size(); ++i) {
        m_vec[i] = i;
    }
}

void Data_Test::tearDown()
{
//    delete this->example;
}

void Data_Test::testSetHash()
{
    std::cout << "testSetHashPtr" << std::endl; 
    // put vector into a Hash (i.e. not a Hash::Pointer)
    util::Hash hash("array", m_vec);
    // add hash via set to a Data object
    xms::Data data;
    data.set("node", hash);

    // get the hash back in different ways
    // 1. fails as reference since Hash in Data are converted to Hash::Pointer
// FIXME: put back
    bool exceptionIfByRef = false;
    try {
//        (&data).get<util::Hash>("node");
        const_cast<const xms::Data*>(&data)->get<util::Hash>("node");
    } catch (int &e) {
        std::cout << "in catch: " << e << std::endl; 
        exceptionIfByRef = true;
    } catch (...) {
        exceptionIfByRef = true;
    }
    CPPUNIT_ASSERT(exceptionIfByRef == true);

    // 2. fine as Hash::Pointer
    const util::Hash::Pointer &hashPtr = data.get<util::Hash::Pointer>("node");
    const std::vector<int> &vecViaPtr = hashPtr->get<std::vector<int> >("array");
    CPPUNIT_ASSERT(vecViaPtr == m_vec);
    CPPUNIT_ASSERT(vecViaPtr[1] == 1);

    // 3. maybe better via getNode
    const xms::Data &nodeData = data.getNode<xms::Data>("node");
    const std::vector<int> &vecViaNode = nodeData.get<std::vector<int> >("array");
    CPPUNIT_ASSERT(vecViaNode == m_vec);
    CPPUNIT_ASSERT(vecViaNode[1] == 1);
}

void Data_Test::testSetHashPtr()
{
    // put vector into a Hash::Pointer
    util::Hash::Pointer hashPtr(new util::Hash("array", m_vec));
    // add hashPtr via set to a Data object
    xms::Data data;
    data.set("node", hashPtr);

    // get the hash back in different ways
    // 1. fails as reference since we put in a pointer
    bool exceptionIfByRef = false;
    try {
        data.get<util::Hash>("node");
    } catch (...) {
        exceptionIfByRef = true;
    }
    CPPUNIT_ASSERT(exceptionIfByRef == true);

    // 2. fine as Hash::Pointer    
    const util::Hash::Pointer &hashPtrGot = data.get<util::Hash::Pointer>("node");
    const std::vector<int> &vecViaPtr = hashPtrGot->get<std::vector<int> >("array");
    CPPUNIT_ASSERT(vecViaPtr == m_vec);
    CPPUNIT_ASSERT(vecViaPtr[1] == 1);
    // no - data is copied...: CPPUNIT_ASSERT(&(vecViaPtr[0]) == &(m_vec[0]));

    // 3. maybe better via getNode
    const xms::Data &nodeData = data.getNode<xms::Data>("node");
    const std::vector<int> &vecViaNode = nodeData.get<std::vector<int> >("array");
    CPPUNIT_ASSERT(vecViaNode == m_vec);
    CPPUNIT_ASSERT(vecViaNode[1] == 1);
}

void Data_Test::testHashCtr()
{
    // create hierarchy of hashes (not of hash pointers!)
    util::Hash hash3("array", m_vec);
    util::Hash hash2("node2", hash3);
    util::Hash hash1("node1", hash2);
    xms::Data data1(hash1);
    
    // The recommended way to get stuff back is via getNode:
    xms::Data data2(data1.getNode<xms::Data>("node1"));  // construct or ...
    const xms::Data &data3 = data2.getNode<xms::Data>("node2"); // ... take const ref

    const std::vector<int> &vec = data3.get<std::vector<int> >("array");
    CPPUNIT_ASSERT(vec == m_vec);
    CPPUNIT_ASSERT(vec[1] == 1);
    
    // Now see that all Hash have been converted to Hash::Pointer:
    // first level:
    bool exceptionIfByRef = false;
    try {
        data1.get<util::Hash>("node1");
    } catch (...) {
        exceptionIfByRef = true;
    }
    CPPUNIT_ASSERT(exceptionIfByRef == true);

    const util::Hash::Pointer &hash2PtrGot = data1.get<util::Hash::Pointer>("node1");

    // second level
    bool exceptionIfByRef2 = false;
    try {
        hash2PtrGot->get<util::Hash>("node2");
    } catch (...) {
        exceptionIfByRef2 = true;
    }
    CPPUNIT_ASSERT(exceptionIfByRef2 == true);

    const util::Hash::Pointer &hash3PtrGot = hash2PtrGot->get<util::Hash::Pointer>("node2");

    // finally see that data is there:
    const std::vector<int> &vec2 = hash3PtrGot->get<std::vector<int> >("array");
    CPPUNIT_ASSERT(vec2 == m_vec);
    CPPUNIT_ASSERT(vec2[1] == 1);
}
