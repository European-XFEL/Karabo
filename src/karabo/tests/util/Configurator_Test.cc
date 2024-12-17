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
 * File:   Configurator_Test.cc
 * Author: heisenb
 *
 * Created on January 28, 2013, 2:49 PM
 */

#include "Configurator_Test.hh"

#include <karabo/util/Configurator.hh>

#include "karabo/util/LeafElement.hh"
#include "karabo/util/NodeElement.hh"
#include "karabo/util/SimpleElement.hh"

using namespace karabo::util;

CPPUNIT_TEST_SUITE_REGISTRATION(Configurator_Test);

KARABO_REGISTER_FOR_CONFIGURATION(Base);
// Register also the constructor with an extra int flag:
KARABO_REGISTER_FOR_CONFIGURATION_ADDON(int, Base);
KARABO_REGISTER_FOR_CONFIGURATION(Aggregated);


void Base::expectedParameters(karabo::util::Schema& s) {
    NODE_ELEMENT(s).key("node").appendParametersOf<Aggregated>().commit();
}


Base::Base(const karabo::util::Hash& hash) {
    m_aggregated = Configurator<Aggregated>::createNode("node", hash);
}

Base::Base(const karabo::util::Hash& hash, int extra) : Base(hash) {
    m_extra0 = extra;
}

Base::~Base() {}


std::shared_ptr<Aggregated>& Base::getAggregated() {
    return m_aggregated;
}


void Aggregated::expectedParameters(karabo::util::Schema& s) {
    INT32_ELEMENT(s)
          .key("answer")
          .description("The answer")
          .displayedName("Answer")
          .assignmentOptional()
          .defaultValue(0)
          .commit();
}


Aggregated::Aggregated(const karabo::util::Hash& hash) : m_answer(hash.get<int>("answer")) {}


Aggregated::Aggregated(const int answer) : m_answer(answer) {}


Aggregated::~Aggregated() {}


int Aggregated::foo() const {
    return m_answer;
}

class Dev1 : public Base {
   public:
    KARABO_CLASSINFO(Dev1, "Dev1", "");

    static void expectedParameters(karabo::util::Schema& s) {
        INT32_ELEMENT(s).key("memberDev1").assignmentOptional().defaultValue(1).commit();
    }

    Dev1(const karabo::util::Hash& hash) : Base(hash) {}
    Dev1(const karabo::util::Hash& hash, int extra) : Base(hash), m_extra1(extra) {}

    virtual ~Dev1() {}

    virtual int getLevel() override {
        return Base::getLevel() + 1;
    }

    virtual int getExtra() override {
        return m_extra1;
    }

    int m_extra1 = -1;
};
KARABO_REGISTER_FOR_CONFIGURATION(Base, Dev1)
// Register also the constructor with an extra int flag:
KARABO_REGISTER_FOR_CONFIGURATION_ADDON(int, Base, Dev1);

class Dev2 : public Dev1 {
   public:
    KARABO_CLASSINFO(Dev2, "Dev2", "");

    static void expectedParameters(karabo::util::Schema& s) {
        INT32_ELEMENT(s).key("memberDev2").assignmentOptional().defaultValue(2).commit();
    }

    Dev2(const karabo::util::Hash& hash) : Dev1(hash) {}
    Dev2(const karabo::util::Hash& hash, int extra) : Dev1(hash), m_extra2(extra) {}

    virtual ~Dev2() {}

    virtual int getLevel() override {
        return Dev1::getLevel() + 1;
    }

    virtual int getExtra() override {
        return m_extra2;
    }

    int m_extra2 = -1;
};
KARABO_REGISTER_FOR_CONFIGURATION(Base, Dev1, Dev2)
// Register also the constructor with an extra int flag:
KARABO_REGISTER_FOR_CONFIGURATION_ADDON(int, Base, Dev1, Dev2);

class Dev3 : public Dev2 {
   public:
    KARABO_CLASSINFO(Dev3, "Dev3", "");

    static void expectedParameters(karabo::util::Schema& s) {
        INT32_ELEMENT(s).key("memberDev3").assignmentOptional().defaultValue(3).commit();
    }

    Dev3(const karabo::util::Hash& hash) : Dev2(hash) {}
    Dev3(const karabo::util::Hash& hash, int extra) : Dev2(hash), m_extra3(extra) {}

    virtual int getLevel() override {
        return Dev2::getLevel() + 1;
    }

    virtual int getExtra() override {
        return m_extra3;
    }
    int m_extra3 = -1;
};
KARABO_REGISTER_FOR_CONFIGURATION(Base, Dev1, Dev2, Dev3)
// Register also the constructor with an extra int flag:
KARABO_REGISTER_FOR_CONFIGURATION_ADDON(int, Base, Dev1, Dev2, Dev3);

class Dev4 : public Dev3 {
   public:
    KARABO_CLASSINFO(Dev4, "Dev4", "");

    static void expectedParameters(karabo::util::Schema& s) {
        INT32_ELEMENT(s).key("memberDev4").assignmentOptional().defaultValue(4).commit();
    }

    Dev4(const karabo::util::Hash& hash) : Dev3(hash) {}
    Dev4(const karabo::util::Hash& hash, int extra) : Dev3(hash), m_extra4(extra) {}

    virtual int getLevel() override {
        return Dev3::getLevel() + 1;
    }

    virtual int getExtra() override {
        return m_extra4;
    }

    int m_extra4 = -1;
};
KARABO_REGISTER_FOR_CONFIGURATION(Base, Dev1, Dev2, Dev3, Dev4)
// Register also the constructor with an extra int flag:
KARABO_REGISTER_FOR_CONFIGURATION_ADDON(int, Base, Dev1, Dev2, Dev3, Dev4);

class Dev5 : public Dev4 {
   public:
    KARABO_CLASSINFO(Dev5, "Dev5", "");

    static void expectedParameters(karabo::util::Schema& s) {
        INT32_ELEMENT(s).key("memberDev5").assignmentOptional().defaultValue(5).commit();
    }

    Dev5(const karabo::util::Hash& hash) : Dev4(hash) {}
    Dev5(const karabo::util::Hash& hash, int extra) : Dev4(hash), m_extra5(extra) {}

    virtual int getLevel() override {
        return Dev4::getLevel() + 1;
    }

    virtual int getExtra() override {
        return m_extra5;
    }

    int m_extra5 = -1;
};
KARABO_REGISTER_FOR_CONFIGURATION(Base, Dev1, Dev2, Dev3, Dev4, Dev5)
// Register also the constructor with an extra int flag:
KARABO_REGISTER_FOR_CONFIGURATION_ADDON(int, Base, Dev1, Dev2, Dev3, Dev4, Dev5);

class Dev6 : public Dev5 {
   public:
    KARABO_CLASSINFO(Dev6, "Dev6", "");

    static void expectedParameters(karabo::util::Schema& s) {
        INT32_ELEMENT(s).key("memberDev6").assignmentOptional().defaultValue(6).commit();
    }

    Dev6(const karabo::util::Hash& hash) : Dev5(hash) {}
    Dev6(const karabo::util::Hash& hash, int extra) : Dev5(hash), m_extra6(extra) {}

    virtual int getLevel() override {
        return Dev5::getLevel() + 1;
    }

    virtual int getExtra() override {
        return m_extra6;
    }

    int m_extra6 = -1;
};
KARABO_REGISTER_FOR_CONFIGURATION(Base, Dev1, Dev2, Dev3, Dev4, Dev5, Dev6)
// Register also the constructor with an extra int flag:
KARABO_REGISTER_FOR_CONFIGURATION_ADDON(int, Base, Dev1, Dev2, Dev3, Dev4, Dev5, Dev6);


void Configurator_Test::testAggregated() {
    {
        // Test to construct Base from Aggregated params
        Hash config("node.answer", 42);
        Base::Pointer b = Configurator<Base>::create("Base", config);
        CPPUNIT_ASSERT(b->getAggregated()->foo() == 42);
    }

    {
        // Test to construct Base from Aggregated object
        Hash config("node", Aggregated::Pointer(new Aggregated(42)));
        Base::Pointer b = Configurator<Base>::create("Base", config);
        CPPUNIT_ASSERT(b->getAggregated()->foo() == 42);
    }
}

void Configurator_Test::testInheritanceChain() {
    // Test that we support inheritance chains of up to 7 classes
    // - creation from factory
    // - schema assembled from each level
    {
        // We can create a Base (with its aggregate inside).
        Base::Pointer b = Configurator<Base>::create("Base", Hash("node.answer", 44));
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(0, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(-1, b->getExtra()); // default, since no use of int constructor
        CPPUNIT_ASSERT_EQUAL(44, b->getAggregated()->foo());
        // Its schema contains the expected path
        Schema schema = Configurator<Base>::getSchema("Base");
        CPPUNIT_ASSERT_EQUAL(1ul, schema.getPaths().size());
        CPPUNIT_ASSERT_EQUAL(std::string("node.answer"), schema.getPaths()[0]);
    }
    {
        // We can create a Dev1 (with Base's aggregate inside).
        Base::Pointer b = Configurator<Base>::create("Dev1", Hash("node.answer", 45));
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(1, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(-1, b->getExtra()); // default
        CPPUNIT_ASSERT_EQUAL(45, b->getAggregated()->foo());
        // Its schema contains the expected paths, including the inherited
        Schema schema = Configurator<Base>::getSchema("Dev1");
        CPPUNIT_ASSERT_EQUAL(2ul, schema.getPaths().size());
        CPPUNIT_ASSERT_EQUAL(std::string("node.answer"), schema.getPaths()[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev1"), schema.getPaths()[1]);
    }
    {
        // We can create a Dev2 (with Base's aggregate inside).
        Base::Pointer b = Configurator<Base>::create("Dev2", Hash("node.answer", 46));
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(2, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(-1, b->getExtra()); // default
        CPPUNIT_ASSERT_EQUAL(46, b->getAggregated()->foo());
        // Its schema contains the expected paths, including the inherited
        Schema schema = Configurator<Base>::getSchema("Dev2");
        CPPUNIT_ASSERT_EQUAL(3ul, schema.getPaths().size());
        CPPUNIT_ASSERT_EQUAL(std::string("node.answer"), schema.getPaths()[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev1"), schema.getPaths()[1]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev2"), schema.getPaths()[2]);
    }
    {
        // We can create a Dev3
        Base::Pointer b = Configurator<Base>::create("Dev3", Hash());
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(3, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(-1, b->getExtra()); // default
        // Its schema contains the expected paths, including the inherited
        Schema schema = Configurator<Base>::getSchema("Dev3");
        CPPUNIT_ASSERT_EQUAL(4ul, schema.getPaths().size());
        CPPUNIT_ASSERT_EQUAL(std::string("node.answer"), schema.getPaths()[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev1"), schema.getPaths()[1]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev2"), schema.getPaths()[2]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev3"), schema.getPaths()[3]);
    }
    {
        // We can create a Dev4
        Base::Pointer b = Configurator<Base>::create("Dev4", Hash());
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(4, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(-1, b->getExtra()); // default
        // Its schema contains the expected paths, including the inherited
        Schema schema = Configurator<Base>::getSchema("Dev4");
        CPPUNIT_ASSERT_EQUAL(5ul, schema.getPaths().size());
        CPPUNIT_ASSERT_EQUAL(std::string("node.answer"), schema.getPaths()[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev1"), schema.getPaths()[1]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev2"), schema.getPaths()[2]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev3"), schema.getPaths()[3]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev4"), schema.getPaths()[4]);
    }
    {
        // We can create a Dev5
        Base::Pointer b = Configurator<Base>::create("Dev5", Hash());
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(5, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(-1, b->getExtra()); // default
        // Its schema contains the expected paths, including the inherited
        Schema schema = Configurator<Base>::getSchema("Dev5");
        CPPUNIT_ASSERT_EQUAL(6ul, schema.getPaths().size());
        CPPUNIT_ASSERT_EQUAL(std::string("node.answer"), schema.getPaths()[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev1"), schema.getPaths()[1]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev2"), schema.getPaths()[2]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev3"), schema.getPaths()[3]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev4"), schema.getPaths()[4]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev5"), schema.getPaths()[5]);
    }
    {
        // We can create a Dev6
        Base::Pointer b = Configurator<Base>::create("Dev6", Hash());
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(6, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(-1, b->getExtra()); // default
        // Its schema contains the expected paths, including the inherited
        Schema schema = Configurator<Base>::getSchema("Dev6");
        CPPUNIT_ASSERT_EQUAL(7ul, schema.getPaths().size());
        CPPUNIT_ASSERT_EQUAL(std::string("node.answer"), schema.getPaths()[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev1"), schema.getPaths()[1]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev2"), schema.getPaths()[2]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev3"), schema.getPaths()[3]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev4"), schema.getPaths()[4]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev5"), schema.getPaths()[5]);
        CPPUNIT_ASSERT_EQUAL(std::string("memberDev6"), schema.getPaths()[6]);
    }
}

void Configurator_Test::testInheritanceChainWithExtra() {
    // Test that we support inheritance chains of up to 7 classes
    // - creation from factory with extra constructor argument
    {
        // We can create a Base with an int
        Base::Pointer b = Configurator<Base>::create("Base", Hash(), -1234);
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(0, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(-1234, b->getExtra()); // as specified above
    }
    {
        // We can create a Dev1 with an int
        Base::Pointer b = Configurator<Base>::create("Dev1", Hash(), 11);
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(1, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(11, b->getExtra()); // as specified above
    }
    {
        // We can create a Dev2 with an int
        Base::Pointer b = Configurator<Base>::create("Dev2", Hash(), 22);
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(2, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(22, b->getExtra()); // as specified above
    }
    {
        // We can create a Dev3 with an int
        Base::Pointer b = Configurator<Base>::create("Dev3", Hash(), 33);
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(3, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(33, b->getExtra()); // as specified above
    }
    {
        // We can create a Dev4 with an int
        Base::Pointer b = Configurator<Base>::create("Dev4", Hash(), 44);
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(4, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(44, b->getExtra()); // as specified above
    }
    {
        // We can create a Dev5 with an int
        Base::Pointer b = Configurator<Base>::create("Dev5", Hash(), 55);
        CPPUNIT_ASSERT(b != nullptr);
        CPPUNIT_ASSERT_EQUAL(5, b->getLevel());
        CPPUNIT_ASSERT_EQUAL(55, b->getExtra()); // as specified above
    }
}
