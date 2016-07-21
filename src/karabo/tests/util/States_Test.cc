/* 
 * File:   States_Test.cc
 * Author: haufs
 *
 * Created on July 8, 2016, 10:08 AM
 */

#include "States_Test.hh"
#include <karabo/util/StateSignifier.hh>
#include <karabo/util/State.hh>

using namespace karabo::util;
using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION(States_Test);


States_Test::States_Test() {
}


States_Test::~States_Test() {
}


void States_Test::setUp() {
    
}


void States_Test::tearDown() {
    
}



void States_Test::testStringRoundTrip() {
    const State & s = State::CLOSED;
    const std::string & sstr = s.name();
    const State & s2 = State::fromString(sstr);
    CPPUNIT_ASSERT(s == s2);
}

void States_Test::testSignifier() {
    
    std::vector<State> s;
    s.push_back(State::DISABLED);
    s.push_back(State::COOLED);
    s.push_back(State::DECREASING);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::DECREASING);
    s.push_back(State::RAMPING_UP);
    CPPUNIT_ASSERT(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::RAMPING_UP);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::DECREASING);
    s.push_back(State::INTERLOCKED);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::INTERLOCKED);
    s.push_back(State::UNKNOWN);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::UNKNOWN);
}

void States_Test::testSignifierNonDefaultList() {
    
    std::vector<State> trumpList;
    trumpList.push_back(State::INTERLOCKED);
    trumpList.push_back(State::UNKNOWN);
    trumpList.push_back(State::KNOWN);
    
    std::vector<State> s;
    s.push_back(State::DISABLED);
    s.push_back(State::CHANGING);
    s.push_back(State::COOLED);
    s.push_back(State::DECREASING);
    s.push_back(State::UNKNOWN);
    s.push_back(State::INTERLOCKED);
    CPPUNIT_ASSERT(StateSignifier(trumpList).returnMostSignificant(s) == State::CHANGING);
}

void States_Test::testComparisons(){
    CPPUNIT_ASSERT(State::CHANGING.isDerivedFrom(State::NORMAL));// direct parentage
    CPPUNIT_ASSERT(!State::NORMAL.isDerivedFrom(State::CHANGING)); // direct parentage the other way round should not compare
    CPPUNIT_ASSERT(!State::CHANGING.isDerivedFrom(State::ERROR)); // no parentage
    CPPUNIT_ASSERT(!State::ERROR.isDerivedFrom(State::CHANGING));  // the other way round
    CPPUNIT_ASSERT(State::HEATED.isDerivedFrom(State::NORMAL)); // longer list of ancestors
    CPPUNIT_ASSERT(!State::KNOWN.isDerivedFrom(State::INCREASING)); // longer list of ancestors the other way round should not compare
}