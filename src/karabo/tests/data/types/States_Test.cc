/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   States_Test.cc
 * Author: haufs
 *
 * Created on July 8, 2016, 10:08 AM
 */

#include "States_Test.hh"

#include "karabo/data/types/State.hh"
#include "karabo/data/types/StateSignifier.hh"

using namespace karabo::data;
using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION(States_Test);


States_Test::States_Test() {}


States_Test::~States_Test() {}


void States_Test::setUp() {}


void States_Test::tearDown() {}


void States_Test::testStringRoundTrip() {
    const State& s = State::CLOSED;
    const std::string& sstr = s.name();
    const State& s2 = State::fromString(sstr);
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


void States_Test::testSignifierInitTrump() {
    std::vector<State> s;
    s.push_back(State::INIT);
    s.push_back(State::RUNNING);
    s.push_back(State::CHANGING);
    s.push_back(State::NORMAL);
    s.push_back(State::PASSIVE);
    s.push_back(State::ACTIVE);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::INIT);
}


void States_Test::testInterlockTrump() {
    std::vector<State> s;
    s.push_back(State::RUNNING);
    s.push_back(State::CHANGING);
    s.push_back(State::NORMAL);
    s.push_back(State::PASSIVE);
    s.push_back(State::ACTIVE);
    s.push_back(State::INTERLOCKED);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::INTERLOCKED);
}


void States_Test::testSignifierNonDefaultList() {
    std::vector<State> trumpList;
    trumpList.push_back(State::INTERLOCKED);
    trumpList.push_back(State::UNKNOWN);
    trumpList.push_back(State::KNOWN);

    std::vector<State> s;
    s.push_back(State::DISABLED);
    s.push_back(State::RUNNING);
    s.push_back(State::PAUSED);
    s.push_back(State::CHANGING);
    s.push_back(State::COOLED);
    s.push_back(State::DECREASING);
    s.push_back(State::UNKNOWN);
    s.push_back(State::INTERLOCKED);
    CPPUNIT_ASSERT_EQUAL(State::CHANGING, StateSignifier(trumpList).returnMostSignificant(s));
}


void States_Test::testRunningTrumpActivePassive() {
    std::vector<State> s;
    s.push_back(State::DISABLED);
    s.push_back(State::RUNNING);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::RUNNING);
    s.push_back(State::ACTIVE);
    s.push_back(State::PASSIVE);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::RUNNING);
    s.push_back(State::PAUSED);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::PAUSED);
}


void States_Test::testChainStatesPassive() {
    std::vector<State> s;
    s.push_back(State::ON);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::ON);
    s.push_back(State::STOPPED);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::STOPPED);
    s.push_back(State::ACQUIRING);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::ACQUIRING);
    s.push_back(State::MOVING);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::MOVING);
    s.push_back(State::INTERLOCKED);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::INTERLOCKED);
    s.push_back(State::ERROR);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::ERROR);
    s.push_back(State::INIT);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::INIT);
    s.push_back(State::UNKNOWN);
    CPPUNIT_ASSERT(StateSignifier().returnMostSignificant(s) == State::UNKNOWN);
}


void States_Test::testChainStatesActive() {
    std::vector<State> s;

    s.push_back(State::ON);
    CPPUNIT_ASSERT(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::ON);
    s.push_back(State::STOPPED);
    CPPUNIT_ASSERT(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::ON);
    s.push_back(State::ACQUIRING);
    CPPUNIT_ASSERT(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::ACQUIRING);
    s.push_back(State::MOVING);
    CPPUNIT_ASSERT(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::MOVING);
    s.push_back(State::INTERLOCKED);
    CPPUNIT_ASSERT(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::INTERLOCKED);
    s.push_back(State::ERROR);
    CPPUNIT_ASSERT(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::ERROR);
    s.push_back(State::INIT);
    CPPUNIT_ASSERT(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::INIT);
    s.push_back(State::UNKNOWN);
    CPPUNIT_ASSERT(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::UNKNOWN);
}


void States_Test::testComparisons() {
    CPPUNIT_ASSERT(State::CHANGING.isDerivedFrom(State::NORMAL)); // direct parentage
    CPPUNIT_ASSERT(
          !State::NORMAL.isDerivedFrom(State::CHANGING)); // direct parentage the other way round should not compare
    CPPUNIT_ASSERT(State::RUNNING.isDerivedFrom(State::NORMAL));  // direct parentage
    CPPUNIT_ASSERT(!State::CHANGING.isDerivedFrom(State::ERROR)); // no parentage
    CPPUNIT_ASSERT(!State::ERROR.isDerivedFrom(State::CHANGING)); // the other way round
    CPPUNIT_ASSERT(State::HEATED.isDerivedFrom(State::NORMAL));   // longer list of ancestors
    CPPUNIT_ASSERT(!State::KNOWN.isDerivedFrom(
          State::INCREASING)); // longer list of ancestors the other way round should not compare
    CPPUNIT_ASSERT(State::PAUSED.isDerivedFrom(State::DISABLED));

    const State state(State::fromString("ON"));
    CPPUNIT_ASSERT(state == State::ON);
    CPPUNIT_ASSERT(state != State::INIT);
}


void States_Test::testStatesSignifierDefault() {
    std::vector<State> states{State::DISABLED, State::ON, State::STOPPED};
    auto signifier = StateSignifier(State::PASSIVE,     // staticMoreSignificant
                                    State::DECREASING); // changingMoreSignificant
    CPPUNIT_ASSERT_EQUAL(State::STOPPED, signifier.returnMostSignificant(states));
    states.push_back(State::RUNNING);
    CPPUNIT_ASSERT_EQUAL(State::RUNNING, signifier.returnMostSignificant(states));
    states.push_back(State::PAUSED);
    CPPUNIT_ASSERT_EQUAL(State::PAUSED, signifier.returnMostSignificant(states));
    states.push_back(State::HEATING);
    CPPUNIT_ASSERT_EQUAL(State::HEATING, signifier.returnMostSignificant(states));
    states.push_back(State::INCREASING);
    CPPUNIT_ASSERT_EQUAL(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::COOLING);
    CPPUNIT_ASSERT_EQUAL(State::COOLING, signifier.returnMostSignificant(states));
    states.push_back(State::DECREASING);
    CPPUNIT_ASSERT_EQUAL(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::MOVING);
    CPPUNIT_ASSERT_EQUAL(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    CPPUNIT_ASSERT_EQUAL(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::INTERLOCKED);
    CPPUNIT_ASSERT_EQUAL(State::INTERLOCKED, signifier.returnMostSignificant(states));
    states.push_back(State::ERROR);
    CPPUNIT_ASSERT_EQUAL(State::ERROR, signifier.returnMostSignificant(states));
    states.push_back(State::INIT);
    CPPUNIT_ASSERT_EQUAL(State::INIT, signifier.returnMostSignificant(states));
    states.push_back(State::UNKNOWN);
    CPPUNIT_ASSERT_EQUAL(State::UNKNOWN, signifier.returnMostSignificant(states));
}


void States_Test::testStatesSignifierActiveDecreasing() {
    std::vector<State> states{State::DISABLED, State::ON, State::STOPPED};
    auto signifier = StateSignifier(State::ACTIVE,      // staticMoreSignificant
                                    State::DECREASING); // changingMoreSignificant
    CPPUNIT_ASSERT_EQUAL(State::ON, signifier.returnMostSignificant(states));
    states.push_back(State::RUNNING);
    CPPUNIT_ASSERT_EQUAL(State::RUNNING, signifier.returnMostSignificant(states));
    states.push_back(State::PAUSED);
    CPPUNIT_ASSERT_EQUAL(State::PAUSED, signifier.returnMostSignificant(states));
    states.push_back(State::HEATING);
    CPPUNIT_ASSERT_EQUAL(signifier.returnMostSignificant(states), State::HEATING);
    states.push_back(State::INCREASING);
    CPPUNIT_ASSERT_EQUAL(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::COOLING);
    CPPUNIT_ASSERT_EQUAL(State::COOLING, signifier.returnMostSignificant(states));
    states.push_back(State::DECREASING);
    CPPUNIT_ASSERT_EQUAL(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::MOVING);
    CPPUNIT_ASSERT_EQUAL(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    CPPUNIT_ASSERT_EQUAL(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::INTERLOCKED);
    CPPUNIT_ASSERT_EQUAL(State::INTERLOCKED, signifier.returnMostSignificant(states));
    states.push_back(State::ERROR);
    CPPUNIT_ASSERT_EQUAL(State::ERROR, signifier.returnMostSignificant(states));
    states.push_back(State::INIT);
    CPPUNIT_ASSERT_EQUAL(State::INIT, signifier.returnMostSignificant(states));
    states.push_back(State::UNKNOWN);
    CPPUNIT_ASSERT_EQUAL(State::UNKNOWN, signifier.returnMostSignificant(states));
}


void States_Test::testStatesSignifierPassiveIncreasing() {
    std::vector<State> states{State::DISABLED, State::ON, State::STOPPED};
    auto signifier = StateSignifier(State::PASSIVE,     // staticMoreSignificant
                                    State::INCREASING); // changingMoreSignificant
    CPPUNIT_ASSERT_EQUAL(State::STOPPED, signifier.returnMostSignificant(states));
    states.push_back(State::RUNNING);
    CPPUNIT_ASSERT_EQUAL(State::RUNNING, signifier.returnMostSignificant(states));
    states.push_back(State::PAUSED);
    CPPUNIT_ASSERT_EQUAL(State::PAUSED, signifier.returnMostSignificant(states));
    states.push_back(State::COOLING);
    CPPUNIT_ASSERT_EQUAL(signifier.returnMostSignificant(states), State::COOLING);
    states.push_back(State::DECREASING);
    CPPUNIT_ASSERT_EQUAL(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::HEATING);
    CPPUNIT_ASSERT_EQUAL(State::HEATING, signifier.returnMostSignificant(states));
    states.push_back(State::INCREASING);
    CPPUNIT_ASSERT_EQUAL(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::MOVING);
    CPPUNIT_ASSERT_EQUAL(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    CPPUNIT_ASSERT_EQUAL(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::INTERLOCKED);
    CPPUNIT_ASSERT_EQUAL(State::INTERLOCKED, signifier.returnMostSignificant(states));
    states.push_back(State::ERROR);
    CPPUNIT_ASSERT_EQUAL(State::ERROR, signifier.returnMostSignificant(states));
    states.push_back(State::INIT);
    CPPUNIT_ASSERT_EQUAL(State::INIT, signifier.returnMostSignificant(states));
    states.push_back(State::UNKNOWN);
    CPPUNIT_ASSERT_EQUAL(State::UNKNOWN, signifier.returnMostSignificant(states));
}


void States_Test::testStatesSignifierActiveIncreasing() {
    std::vector<State> states{State::DISABLED, State::ON, State::STOPPED};
    auto signifier = StateSignifier(State::ACTIVE,      // staticMoreSignificant
                                    State::INCREASING); // changingMoreSignificant
    CPPUNIT_ASSERT_EQUAL(State::ON, signifier.returnMostSignificant(states));
    states.push_back(State::RUNNING);
    CPPUNIT_ASSERT_EQUAL(State::RUNNING, signifier.returnMostSignificant(states));
    states.push_back(State::PAUSED);
    CPPUNIT_ASSERT_EQUAL(State::PAUSED, signifier.returnMostSignificant(states));
    states.push_back(State::COOLING);
    CPPUNIT_ASSERT_EQUAL(signifier.returnMostSignificant(states), State::COOLING);
    states.push_back(State::DECREASING);
    CPPUNIT_ASSERT_EQUAL(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::HEATING);
    CPPUNIT_ASSERT_EQUAL(State::HEATING, signifier.returnMostSignificant(states));
    states.push_back(State::INCREASING);
    CPPUNIT_ASSERT_EQUAL(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::MOVING);
    CPPUNIT_ASSERT_EQUAL(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    CPPUNIT_ASSERT_EQUAL(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::INTERLOCKED);
    CPPUNIT_ASSERT_EQUAL(State::INTERLOCKED, signifier.returnMostSignificant(states));
    states.push_back(State::ERROR);
    CPPUNIT_ASSERT_EQUAL(State::ERROR, signifier.returnMostSignificant(states));
    states.push_back(State::INIT);
    CPPUNIT_ASSERT_EQUAL(State::INIT, signifier.returnMostSignificant(states));
    states.push_back(State::UNKNOWN);
    CPPUNIT_ASSERT_EQUAL(State::UNKNOWN, signifier.returnMostSignificant(states));
}


void States_Test::testAcquiringChangingOnPassive() {
    std::vector<State> states{State::ON, State::OFF};
    auto signifier = StateSignifier(State::PASSIVE,     // staticMoreSignificant
                                    State::DECREASING); // changingMoreSignificant
    CPPUNIT_ASSERT_EQUAL(State::OFF, signifier.returnMostSignificant(states));
    states.push_back(State::ACQUIRING);
    CPPUNIT_ASSERT_EQUAL(State::ACQUIRING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    CPPUNIT_ASSERT_EQUAL(State::CHANGING, signifier.returnMostSignificant(states));
}


void States_Test::testAcquiringChangingOnActive() {
    std::vector<State> states{State::ON, State::OFF};
    auto signifier = StateSignifier(State::ACTIVE,      // staticMoreSignificant
                                    State::DECREASING); // changingMoreSignificant

    CPPUNIT_ASSERT_EQUAL(State::ON, signifier.returnMostSignificant(states));
    states.push_back(State::ACQUIRING);
    CPPUNIT_ASSERT_EQUAL(State::ACQUIRING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    CPPUNIT_ASSERT_EQUAL(State::CHANGING, signifier.returnMostSignificant(states));
}


void States_Test::testStatesSignifierNonDefList() {
    std::vector<State> trumpList{State::INTERLOCKED, State::UNKNOWN, State::KNOWN};
    std::vector<State> states{State::DISABLED, State::CHANGING, State::ON,      State::DECREASING,
                              State::RUNNING,  State::PAUSED,   State::UNKNOWN, State::INTERLOCKED};
    auto signifier = StateSignifier(trumpList);
    CPPUNIT_ASSERT_EQUAL(State::CHANGING, signifier.returnMostSignificant(states));
}
