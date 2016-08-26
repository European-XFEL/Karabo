/* 
 * File:   Serializable_Test.cc
 * Author: heisenb
 * 
 * Created on August 5, 2016, 12:13 PM
 */

#include "Serializable_Test.h"

#include <karabo/util/Hash.hh>

CPPUNIT_TEST_SUITE_REGISTRATION(Serializable_Test);

using namespace karabo::util;
using namespace std;


void FancyData::setScalar(const int value) {
    set("scalar", value);
}


const int& FancyData::getScalar() const {
    return get<int>("scalar");
}


Serializable_Test::Serializable_Test() {
}


Serializable_Test::~Serializable_Test() {
}


void Serializable_Test::testMethod() {    

    

    FancyData fd2;
    fd2.setScalar(2);

    Hash h;
    
    h.set("fd2", fd2); // Here the original object on the stack is copied

    // Internally all objects are hold as shared_ptrs
    
    CPPUNIT_ASSERT(h.get<FancyData>("fd2").getScalar() == 2);

    
    h.get<FancyData>("fd2").setScalar(-2);
    CPPUNIT_ASSERT(fd2.getScalar() == 2);

    // Here a copy is done
    FancyData fd3 = h.get<FancyData>("fd2");
    fd3.setScalar(1);
    
    
    CPPUNIT_ASSERT(fd3.getScalar() == 1);
    CPPUNIT_ASSERT(h.get<FancyData>("fd2").getScalar() == -2);
}


