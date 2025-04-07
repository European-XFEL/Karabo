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
 * File:   Configurator_Test.hh
 * Author: heisenb
 *
 * Created on January 28, 2013, 2:49 PM
 */

#ifndef KARABO_UTIL_CONFIGURATOR_TEST_HH
#define KARABO_UTIL_CONFIGURATOR_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/data/types/ClassInfo.hh"
#include "karabo/data/types/Schema.hh"

// Forward
class Aggregated;

class Base {
   public:
    KARABO_CLASSINFO(Base, "Base", "");

    static void expectedParameters(karabo::data::Schema& s);

    Base(const karabo::data::Hash& hash);
    Base(const karabo::data::Hash& hash, int extra);

    virtual ~Base();

    virtual int getLevel() {
        return 0;
    }
    virtual int getExtra() {
        return m_extra0;
    }

    std::shared_ptr<Aggregated>& getAggregated();

   private:
    std::shared_ptr<Aggregated> m_aggregated;
    int m_extra0 = -1;
};

class Aggregated {
   public:
    KARABO_CLASSINFO(Aggregated, "Aggregated", "");

    static void expectedParameters(karabo::data::Schema& s);

    Aggregated(const karabo::data::Hash& hash);

    Aggregated(const int answer);

    virtual ~Aggregated();

    int foo() const;

   private:
    int m_answer;
};

class Configurator_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Configurator_Test);
    CPPUNIT_TEST(testAggregated);
    CPPUNIT_TEST(testInheritanceChain);
    CPPUNIT_TEST(testInheritanceChainWithExtra);
    CPPUNIT_TEST_SUITE_END();

    void testAggregated();
    void testInheritanceChain();
    void testInheritanceChainWithExtra();
    void test();
};

#endif /* CONFIGURATOR_TEST_HH */
