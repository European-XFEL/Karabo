/*
 * $Id: testHash.cc 5333 2012-03-02 08:47:36Z heisenb $
 *
 * File:   testHash.hh
 * Author: <your.email@xfel.eu>
 *
 * Created on August 13, 2010, 3:28 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <cstdlib>
#include <iostream>
#include <assert.h>
#include <string>
#include <deque>
#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include "../Exception.hh"
#include "../Schema.hh"
#include "../SimpleElement.hh"
#include "../ComplexElement.hh"
#include "../VectorElement.hh"

using namespace std;
using namespace exfel::util;
using namespace boost::assign;

class SomeCustomElement {
private:
    ComplexElement m_outerElement;

    SimpleElement<unsigned int> m_myUnsigned;
    SimpleElement<std::string> m_myString;
    SimpleElement<float> m_myFloat;
    SimpleElement<int > m_myInt;
    SimpleElement<double > m_myDouble;
    VectorElement<bool, std::deque> m_myVectorBool;

public:

    SomeCustomElement(Schema& expected) : m_outerElement(ComplexElement(expected)), m_myVectorBool(VectorElement<bool, deque>(expected)) {

        m_outerElement.reconfigureAndRead();

        m_myUnsigned.key("myUnsigned")
                .displayedName("MyUnsigned")
                .description("My Unsigned represents an index")
                .assignmentOptional().noDefaultValue()
                .readOnly();

        m_myString.key("myString")
                .displayedName("MyString")
                .description("My String represents a title")
                .assignmentOptional().defaultValue("Add your title here")
                .reconfigurable();

        m_myFloat.key("myFloat")
                .displayedName("MyFloat")
                .description("My Float represents something wet")
                .assignmentOptional().defaultValue(7.77)
                .reconfigurable();

        m_myInt.key("myInt")
                .displayedName("MyInt")
                .description("My Int represents for instance a temperature")
                .assignmentOptional().defaultValue(7)
                .reconfigurable();

        m_myDouble.key("myDouble")
                .displayedName("MyDouble")
                .description("My Double represents duplication")
                .assignmentOptional().defaultValue(0.00000007777)
                .reconfigurable();

        m_myVectorBool.key("status")
                .displayedName("OutputStatus")
                .description("A bit string which shows the status.")
                .assignmentOptional().noDefaultValue()
                .readOnly();
    }

    SomeCustomElement& key(const std::string& name) {
        m_outerElement.key(name);
        return *this;
    }

    SomeCustomElement& displayedName(const std::string& displayedName) {
        m_outerElement.displayedName(displayedName);
        return *this;
    }

    SomeCustomElement& description(const std::string& desc) {
        m_outerElement.description(desc);
        return *this;
    }

    SomeCustomElement& assignmentMandatory() {
        m_outerElement.assignmentMandatory();
        return *this;
    }

    SomeCustomElement& assignmentOptional() {
        m_outerElement.assignmentOptional();
        return *this;
    }

    SomeCustomElement& assignmentInternal() {
        m_outerElement.assignmentInternal();
        return *this;
    }

    SomeCustomElement& initAndRead() {
        m_outerElement.initAndRead();
        return *this;
    }

    SomeCustomElement& reconfigurable() {
        m_outerElement.reconfigurable();
        return *this;
    }

    SomeCustomElement& readOnly() {
        m_outerElement.readOnly();
        return *this;
    }

    SomeCustomElement& init() {
        m_outerElement.init();
        return *this;
    }

    void commit() {
        m_outerElement.assignmentOptional().reconfigureAndRead();
        Schema& innerElement = m_outerElement.commit();
        m_myUnsigned.commit(innerElement);
        m_myString.commit(innerElement);
        m_myInt.commit(innerElement);
        m_myFloat.commit(innerElement);
        m_myDouble.commit(innerElement);
        m_myVectorBool.commit(innerElement);
    }
};

class SomeClass {
public:

    static void expectedParameters(Schema& expected) {

        STRING_ELEMENT(expected).key("exampleKey1")
                .displayedName("Example key 1")
                .description("Example key 1 description")
                .assignmentOptional().defaultValue("Some default string")
                .reconfigurable()
                .commit();

        UINT32_ELEMENT(expected).key("exampleKey2")
                .displayedName("Example key 2")
                .description("Example key 2 description")
                .assignmentOptional().defaultValue(10)
                .reconfigurable()
                .commit();

        SomeCustomElement(expected).key("exampleKey3")
                .displayedName("Example key 3")
                .description("Example key 3 description")
                .assignmentOptional()
                .readOnly()
                .commit();
    }
};

static void expectedParameters(Schema& expected) {

    STRING_ELEMENT(expected).key("exampleKey1")
            .displayedName("Example key 1")
            .description("Example key 1 description")
            .assignmentOptional().defaultValue("Some default string")
            .reconfigurable()
            .commit();

    UINT32_ELEMENT(expected).key("exampleKey2")
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .assignmentOptional().defaultValue(10)
            .reconfigurable()
            .commit();

    UINT32_ELEMENT(expected).key("exampleKey3")
            .displayedName("Example key 3")
            .description("Example key 3 description")
            .assignmentOptional().defaultValue(20)
            .reconfigurable()
            .commit();

    FLOAT_ELEMENT(expected).key("exampleKey4").alias("exampleAlias4")
            .displayedName("Example key 4")
            .description("Example key 4 description")
            .assignmentOptional().defaultValue(0)
            .readOnly()
            .commit();

    INT32_ELEMENT(expected).key("exampleKey5").alias("exampleAlias5")
            .displayedName("Example key 5")
            .description("Example key 5 description")
            .assignmentOptional().defaultValue(0)
            .readOnly()
            .commit();

}

void setAdditionalParameters(Schema& expected) {

    STRING_ELEMENT(expected).key("testKey1")
            .displayedName("Test key 1")
            .description("Test key 1 description")
            .assignmentOptional().defaultValue("Some default string")
            .reconfigurable()
            .commit();

    UINT32_ELEMENT(expected).key("testKey2")
            .displayedName("Test key 2")
            .description("Test key 2 description")
            .assignmentOptional().defaultValue(10000)
            .reconfigurable()
            .commit();

    UINT32_ELEMENT(expected).key("testKey3")
            .displayedName("Test key 3")
            .description("Test key 3 description")
            .assignmentOptional().defaultValue(2000)
            .reconfigurable()
            .commit();

    FLOAT_ELEMENT(expected).key("testKey4").alias("testAlias4")
            .displayedName("Test key 4")
            .description("Test key 4 description")
            .assignmentOptional().defaultValue(0)
            .readOnly()
            .commit();

    INT32_ELEMENT(expected).key("testKey5").alias("testAlias5")
            .displayedName("Test key 5")
            .description("Test key 5 description")
            .assignmentOptional().defaultValue(0)
            .readOnly()
            .commit();

}

int testSchema(int argc, char** argv) {

    cout << "Running Test: testSchema..." << endl;

    {
        cout << "\nTEST 1\n";
        Schema expected;
        Schema& tmp = expected.initParameterDescription("TextFile", READ | WRITE | INIT);
        expectedParameters(tmp);
        cout << "Expected parameters ...\n" << expected << '\n';

        Schema schema;
        Schema& tmp2 = schema.initParameterDescription("Bla", READ | WRITE | INIT);
        setAdditionalParameters(tmp2);
        cout << "Additional parameters are...\n" << schema << endl;

        expected.addExternalSchema(schema);
        cout << "Expected parameters after adding...\n" << expected << '\n';
        cout << "END TEST 1\n";

        cout << " Casted Hash " << endl;

        cout << *(static_cast<Hash*>(&schema)) << endl;

        
        Schema comp;
        Schema& tmp3 = comp.initParameterDescription("TextFile", READ | WRITE | INIT);
        cout << "======================  COMPLEX_ELEMENT  ==================" << endl;
        SomeClass::expectedParameters(tmp3);
        cout << "Expected parameters with complex elements...\n" << comp << '\n';
    }

    return 0;
}
