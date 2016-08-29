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

    // Create a fresh own object on the stack
    FancyData fd1;
    fd1.setScalar(2);

    Hash h;
    h.set("fd1", fd1); // Here the original object on the stack is copied

    CPPUNIT_ASSERT(h.get<FancyData>("fd1").getScalar() == 2);

    // Manipulated on a copy
    h.get<FancyData>("fd1").setScalar(-2);

    // Does not change the original
    CPPUNIT_ASSERT(fd1.getScalar() == 2);

    // But the one in the hash
    const FancyData& fd2 = h.get<FancyData>("fd1");
    CPPUNIT_ASSERT(fd2.getScalar() == -2);
}


