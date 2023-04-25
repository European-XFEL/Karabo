/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Serializable_Test.h
 * Author: heisenb
 *
 * Created on August 5, 2016, 12:13 PM
 */

#ifndef SERIALIZABLE_TEST_H
#define SERIALIZABLE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/util/Hash.hh>

class FancyData : protected karabo::util::Hash {
   public:
    typedef karabo::util::Hash type;

    KARABO_CLASSINFO(FancyData, "FancyData", "1.0");

    void setScalar(const int value);

    const int& getScalar() const;
};

class Serializable_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Serializable_Test);
    CPPUNIT_TEST(testMethod);

    CPPUNIT_TEST_SUITE_END();

   public:
    Serializable_Test();
    virtual ~Serializable_Test();

   private:
    void testMethod();
};

#endif /* SERIALIZABLE_TEST_H */
