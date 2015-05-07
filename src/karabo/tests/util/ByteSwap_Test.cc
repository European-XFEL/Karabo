/* 
 * File:   ByteSwap_Test.cc
 * Author: parenti
 * 
 * Created on October 4, 2013, 3:51 PM
 */

#include "ByteSwap_Test.hh"
#include <karabo/util/ByteSwap.hh>

using namespace karabo::util;
using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION(ByteSwap_Test);


ByteSwap_Test::ByteSwap_Test() {
}


ByteSwap_Test::~ByteSwap_Test() {
}


void ByteSwap_Test::setUp() {
    
}


void ByteSwap_Test::tearDown() {
    
}


void ByteSwap_Test::test16() {

    CPPUNIT_ASSERT(byteSwap16(0x1234) == (uint16_t)0x3412);

}

void ByteSwap_Test::test32() {
    
    CPPUNIT_ASSERT(byteSwap32(0x12345678) == (uint32_t)0x78563412);

}

void ByteSwap_Test::test64() {
    
    CPPUNIT_ASSERT(byteSwap64(0x1234567890ABCDEF) == (uint64_t)0xEFCDAB9078563412);

}
