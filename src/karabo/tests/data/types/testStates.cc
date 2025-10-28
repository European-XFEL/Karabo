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
 * File:   States_Test.hh
 * Author: haufs
 *
 * Created on July 8, 2016, 10:08 AM
 */

#include <gtest/gtest.h>

#include "karabo/data/types/State.hh"
#include "karabo/data/types/StateSignifier.hh"

using namespace karabo::data;
using namespace std;


TEST(TestStates, testStringRoundTrip) {
    const State& s = State::CLOSED;
    const std::string& sstr = s.name();
    const State& s2 = State::fromString(sstr);
    EXPECT_TRUE(s == s2);
}


TEST(TestStates, testSignifier) {
    std::vector<State> s;
    s.push_back(State::DISABLED);
    s.push_back(State::COOLED);
    s.push_back(State::DECREASING);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::DECREASING);
    s.push_back(State::RAMPING_UP);
    EXPECT_TRUE(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::RAMPING_UP);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::DECREASING);
    s.push_back(State::INTERLOCKED);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::INTERLOCKED);
    s.push_back(State::UNKNOWN);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::UNKNOWN);
}


TEST(TestStates, testSignifierInitTrump) {
    std::vector<State> s;
    s.push_back(State::INIT);
    s.push_back(State::RUNNING);
    s.push_back(State::CHANGING);
    s.push_back(State::NORMAL);
    s.push_back(State::PASSIVE);
    s.push_back(State::ACTIVE);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::INIT);
}


TEST(TestStates, testInterlockTrump) {
    std::vector<State> s;
    s.push_back(State::RUNNING);
    s.push_back(State::CHANGING);
    s.push_back(State::NORMAL);
    s.push_back(State::PASSIVE);
    s.push_back(State::ACTIVE);
    s.push_back(State::INTERLOCKED);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::INTERLOCKED);
}


TEST(TestStates, testSignifierNonDefaultList) {
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
    EXPECT_EQ(State::CHANGING, StateSignifier(trumpList).returnMostSignificant(s));
}


TEST(TestStates, testRunningTrumpActivePassive) {
    std::vector<State> s;
    s.push_back(State::DISABLED);
    s.push_back(State::RUNNING);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::RUNNING);
    s.push_back(State::ACTIVE);
    s.push_back(State::PASSIVE);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::RUNNING);
    s.push_back(State::PAUSED);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::PAUSED);
}


TEST(TestStates, testChainStatesPassive) {
    std::vector<State> s;
    s.push_back(State::ON);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::ON);
    s.push_back(State::STOPPED);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::STOPPED);
    s.push_back(State::ACQUIRING);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::ACQUIRING);
    s.push_back(State::MOVING);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::MOVING);
    s.push_back(State::INTERLOCKED);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::INTERLOCKED);
    s.push_back(State::ERROR);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::ERROR);
    s.push_back(State::INIT);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::INIT);
    s.push_back(State::UNKNOWN);
    EXPECT_TRUE(StateSignifier().returnMostSignificant(s) == State::UNKNOWN);
}


TEST(TestStates, testChainStatesActive) {
    std::vector<State> s;

    s.push_back(State::ON);
    EXPECT_TRUE(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::ON);
    s.push_back(State::STOPPED);
    EXPECT_TRUE(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::ON);
    s.push_back(State::ACQUIRING);
    EXPECT_TRUE(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::ACQUIRING);
    s.push_back(State::MOVING);
    EXPECT_TRUE(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::MOVING);
    s.push_back(State::INTERLOCKED);
    EXPECT_TRUE(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::INTERLOCKED);
    s.push_back(State::ERROR);
    EXPECT_TRUE(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::ERROR);
    s.push_back(State::INIT);
    EXPECT_TRUE(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::INIT);
    s.push_back(State::UNKNOWN);
    EXPECT_TRUE(StateSignifier(State::ACTIVE, State::INCREASING).returnMostSignificant(s) == State::UNKNOWN);
}


TEST(TestStates, testComparisons) {
    EXPECT_TRUE(State::CHANGING.isDerivedFrom(State::NORMAL)); // direct parentage
    EXPECT_TRUE(
          !State::NORMAL.isDerivedFrom(State::CHANGING));     // direct parentage the other way round should not compare
    EXPECT_TRUE(State::RUNNING.isDerivedFrom(State::NORMAL)); // direct parentage
    EXPECT_TRUE(!State::CHANGING.isDerivedFrom(State::ERROR)); // no parentage
    EXPECT_TRUE(!State::ERROR.isDerivedFrom(State::CHANGING)); // the other way round
    EXPECT_TRUE(State::HEATED.isDerivedFrom(State::NORMAL));   // longer list of ancestors
    EXPECT_TRUE(!State::KNOWN.isDerivedFrom(
          State::INCREASING)); // longer list of ancestors the other way round should not compare
    EXPECT_TRUE(State::PAUSED.isDerivedFrom(State::DISABLED));

    const State state(State::fromString("ON"));
    EXPECT_TRUE(state == State::ON);
    EXPECT_TRUE(state != State::INIT);
}


TEST(TestStates, testStatesSignifierDefault) {
    std::vector<State> states{State::DISABLED, State::ON, State::STOPPED};
    auto signifier = StateSignifier(State::PASSIVE,     // staticMoreSignificant
                                    State::DECREASING); // changingMoreSignificant
    EXPECT_EQ(State::STOPPED, signifier.returnMostSignificant(states));
    states.push_back(State::RUNNING);
    EXPECT_EQ(State::RUNNING, signifier.returnMostSignificant(states));
    states.push_back(State::PAUSED);
    EXPECT_EQ(State::PAUSED, signifier.returnMostSignificant(states));
    states.push_back(State::HEATING);
    EXPECT_EQ(State::HEATING, signifier.returnMostSignificant(states));
    states.push_back(State::INCREASING);
    EXPECT_EQ(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::COOLING);
    EXPECT_EQ(State::COOLING, signifier.returnMostSignificant(states));
    states.push_back(State::DECREASING);
    EXPECT_EQ(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::MOVING);
    EXPECT_EQ(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    EXPECT_EQ(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::INTERLOCKED);
    EXPECT_EQ(State::INTERLOCKED, signifier.returnMostSignificant(states));
    states.push_back(State::ERROR);
    EXPECT_EQ(State::ERROR, signifier.returnMostSignificant(states));
    states.push_back(State::INIT);
    EXPECT_EQ(State::INIT, signifier.returnMostSignificant(states));
    states.push_back(State::UNKNOWN);
    EXPECT_EQ(State::UNKNOWN, signifier.returnMostSignificant(states));
}


TEST(TestStates, testStatesSignifierActiveDecreasing) {
    std::vector<State> states{State::DISABLED, State::ON, State::STOPPED};
    auto signifier = StateSignifier(State::ACTIVE,      // staticMoreSignificant
                                    State::DECREASING); // changingMoreSignificant
    EXPECT_EQ(State::ON, signifier.returnMostSignificant(states));
    states.push_back(State::RUNNING);
    EXPECT_EQ(State::RUNNING, signifier.returnMostSignificant(states));
    states.push_back(State::PAUSED);
    EXPECT_EQ(State::PAUSED, signifier.returnMostSignificant(states));
    states.push_back(State::HEATING);
    EXPECT_EQ(signifier.returnMostSignificant(states), State::HEATING);
    states.push_back(State::INCREASING);
    EXPECT_EQ(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::COOLING);
    EXPECT_EQ(State::COOLING, signifier.returnMostSignificant(states));
    states.push_back(State::DECREASING);
    EXPECT_EQ(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::MOVING);
    EXPECT_EQ(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    EXPECT_EQ(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::INTERLOCKED);
    EXPECT_EQ(State::INTERLOCKED, signifier.returnMostSignificant(states));
    states.push_back(State::ERROR);
    EXPECT_EQ(State::ERROR, signifier.returnMostSignificant(states));
    states.push_back(State::INIT);
    EXPECT_EQ(State::INIT, signifier.returnMostSignificant(states));
    states.push_back(State::UNKNOWN);
    EXPECT_EQ(State::UNKNOWN, signifier.returnMostSignificant(states));
}


TEST(TestStates, testStatesSignifierPassiveIncreasing) {
    std::vector<State> states{State::DISABLED, State::ON, State::STOPPED};
    auto signifier = StateSignifier(State::PASSIVE,     // staticMoreSignificant
                                    State::INCREASING); // changingMoreSignificant
    EXPECT_EQ(State::STOPPED, signifier.returnMostSignificant(states));
    states.push_back(State::RUNNING);
    EXPECT_EQ(State::RUNNING, signifier.returnMostSignificant(states));
    states.push_back(State::PAUSED);
    EXPECT_EQ(State::PAUSED, signifier.returnMostSignificant(states));
    states.push_back(State::COOLING);
    EXPECT_EQ(signifier.returnMostSignificant(states), State::COOLING);
    states.push_back(State::DECREASING);
    EXPECT_EQ(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::HEATING);
    EXPECT_EQ(State::HEATING, signifier.returnMostSignificant(states));
    states.push_back(State::INCREASING);
    EXPECT_EQ(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::MOVING);
    EXPECT_EQ(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    EXPECT_EQ(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::INTERLOCKED);
    EXPECT_EQ(State::INTERLOCKED, signifier.returnMostSignificant(states));
    states.push_back(State::ERROR);
    EXPECT_EQ(State::ERROR, signifier.returnMostSignificant(states));
    states.push_back(State::INIT);
    EXPECT_EQ(State::INIT, signifier.returnMostSignificant(states));
    states.push_back(State::UNKNOWN);
    EXPECT_EQ(State::UNKNOWN, signifier.returnMostSignificant(states));
}


TEST(TestStates, testStatesSignifierActiveIncreasing) {
    std::vector<State> states{State::DISABLED, State::ON, State::STOPPED};
    auto signifier = StateSignifier(State::ACTIVE,      // staticMoreSignificant
                                    State::INCREASING); // changingMoreSignificant
    EXPECT_EQ(State::ON, signifier.returnMostSignificant(states));
    states.push_back(State::RUNNING);
    EXPECT_EQ(State::RUNNING, signifier.returnMostSignificant(states));
    states.push_back(State::PAUSED);
    EXPECT_EQ(State::PAUSED, signifier.returnMostSignificant(states));
    states.push_back(State::COOLING);
    EXPECT_EQ(signifier.returnMostSignificant(states), State::COOLING);
    states.push_back(State::DECREASING);
    EXPECT_EQ(State::DECREASING, signifier.returnMostSignificant(states));
    states.push_back(State::HEATING);
    EXPECT_EQ(State::HEATING, signifier.returnMostSignificant(states));
    states.push_back(State::INCREASING);
    EXPECT_EQ(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::MOVING);
    EXPECT_EQ(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    EXPECT_EQ(State::INCREASING, signifier.returnMostSignificant(states));
    states.push_back(State::INTERLOCKED);
    EXPECT_EQ(State::INTERLOCKED, signifier.returnMostSignificant(states));
    states.push_back(State::ERROR);
    EXPECT_EQ(State::ERROR, signifier.returnMostSignificant(states));
    states.push_back(State::INIT);
    EXPECT_EQ(State::INIT, signifier.returnMostSignificant(states));
    states.push_back(State::UNKNOWN);
    EXPECT_EQ(State::UNKNOWN, signifier.returnMostSignificant(states));
}


TEST(TestStates, testAcquiringChangingOnPassive) {
    std::vector<State> states{State::ON, State::OFF};
    auto signifier = StateSignifier(State::PASSIVE,     // staticMoreSignificant
                                    State::DECREASING); // changingMoreSignificant
    EXPECT_EQ(State::OFF, signifier.returnMostSignificant(states));
    states.push_back(State::ACQUIRING);
    EXPECT_EQ(State::ACQUIRING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    EXPECT_EQ(State::CHANGING, signifier.returnMostSignificant(states));
}


TEST(TestStates, testAcquiringChangingOnActive) {
    std::vector<State> states{State::ON, State::OFF};
    auto signifier = StateSignifier(State::ACTIVE,      // staticMoreSignificant
                                    State::DECREASING); // changingMoreSignificant

    EXPECT_EQ(State::ON, signifier.returnMostSignificant(states));
    states.push_back(State::ACQUIRING);
    EXPECT_EQ(State::ACQUIRING, signifier.returnMostSignificant(states));
    states.push_back(State::CHANGING);
    EXPECT_EQ(State::CHANGING, signifier.returnMostSignificant(states));
}


TEST(TestStates, testStatesSignifierNonDefList) {
    std::vector<State> trumpList{State::INTERLOCKED, State::UNKNOWN, State::KNOWN};
    std::vector<State> states{State::DISABLED, State::CHANGING, State::ON,      State::DECREASING,
                              State::RUNNING,  State::PAUSED,   State::UNKNOWN, State::INTERLOCKED};
    auto signifier = StateSignifier(trumpList);
    EXPECT_EQ(State::CHANGING, signifier.returnMostSignificant(states));
}
