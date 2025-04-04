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
 * File:   FileInputOutput_Test.cc
 * Author: heisenb
 *
 * Created on March 7, 2013, 11:06 AM
 */

#include "FileInputOutput_Test.hh"

#include <filesystem>
#include <karabo/data/schema/NodeElement.hh>
#include <karabo/data/schema/VectorElement.hh>
#include <karabo/data/types/Hash.hh>
#include <karabo/io/FileTools.hh>
#include <karabo/util/TimeProfiler.hh>

#include "TestPathSetup.hh"

using namespace std;
using namespace karabo::data;
using namespace karabo::util;
using namespace karabo::io;

CPPUNIT_TEST_SUITE_REGISTRATION(FileInputOutput_Test);


struct MySchema {
    KARABO_CLASSINFO(MySchema, "TestXsd", "1.0");


    static void expectedParameters(karabo::data::Schema& expected) {
        STRING_ELEMENT(expected)
              .key("exampleKey1")
              .tags("hardware, poll")
              .displayedName("Example key 1")
              .description("Example key 1 description")
              .options("Radio,Air Condition,Navigation", ",")
              .assignmentOptional()
              .defaultValue("Navigation")
              .expertAccess()
              .commit();

        NODE_ELEMENT(expected)
              .key("MyNodeElement")
              .tags("myNode")
              .displayedName("MyNodeElem")
              .description("Description of my node elem")
              .commit();

        DOUBLE_ELEMENT(expected)
              .key("MyNodeElement.a")
              .tags("myNode")
              .displayedName("MyNodeElem_A")
              .description("Description of my node elem A")
              .assignmentMandatory()
              .commit();

        STRING_ELEMENT(expected)
              .key("MyNodeElement.b")
              .tags("myNode")
              .displayedName("MyNodeElem_B")
              .description("Description of my node elem B")
              .assignmentMandatory()
              .commit();

        INT64_ELEMENT(expected)
              .key("exampleKey5")
              .alias("exampleAlias5")
              .tags("h/w; d.m.y", ";")
              .displayedName("Example key 5")
              .description("Example key 5 description")
              .readOnly()
              .initialValue(1442244)
              .commit();

        INT64_ELEMENT(expected)
              .key("exampleKeyINTERNAL")
              .displayedName("INTERNAL")
              .description("Example key INTERNAL")
              .assignmentInternal();

        // test for CHOICE element
        CHOICE_ELEMENT(expected)
              .key("shapes")
              .displayedName("shapesAsChoice")
              .description("Description of Choice-element shapes")
              .assignmentOptional()
              .defaultValue("circle")
              .commit();

        NODE_ELEMENT(expected)
              .key("shapes.circle")
              .tags("shape")
              .displayedName("Circle")
              .description("Description of circle")
              .commit();

        INT32_ELEMENT(expected)
              .key("shapes.circle.radius")
              .tags("shape")
              .displayedName("radius")
              .description("Radius of circle")
              .minInc(5)
              .maxExc(10)
              .assignmentOptional()
              .defaultValue(5)
              .commit();

        INT32_ELEMENT(expected)
              .key("shapes.circle.color")
              .tags("shape")
              .displayedName("color")
              .description("Color of circle")
              .minExc(2)
              .maxInc(20)
              .assignmentOptional()
              .defaultValue(5)
              .commit();

        NODE_ELEMENT(expected)
              .key("shapes.circle.newnode")
              .tags("shape")
              .displayedName("NewNodeOfCircle")
              .description("Description of NEW NODE of circle")
              .commit();

        INT32_ELEMENT(expected)
              .key("shapes.circle.newnode.mynewint")
              .tags("shape")
              .displayedName("MyNewInt")
              .description("Descr of shapes circle newnode MyNewInt")
              .assignmentOptional()
              .defaultValue(555)
              .commit();

        NODE_ELEMENT(expected)
              .key("shapes.rectangle")
              .tags("shape")
              .displayedName("rectangle")
              .description("Description of rectangle")
              .commit();

        DOUBLE_ELEMENT(expected)
              .key("shapes.rectangle.square")
              .tags("shape")
              .displayedName("square")
              .description("Description of square of rectangle")
              .assignmentOptional()
              .noDefaultValue()
              .commit();


        vector<string> vecStr;
        vecStr.push_back("first line");
        vecStr.push_back("second line");
        VECTOR_STRING_ELEMENT(expected)
              .key("strVector")
              .displayedName("myVectorString")
              .assignmentOptional()
              .defaultValue(vecStr)
              .reconfigurable()
              .commit();

        vector<int> vecInt;
        vecInt.push_back(5);
        vecInt.push_back(15);
        VECTOR_INT32_ELEMENT(expected)
              .key("intVector")
              .displayedName("MyVectorInt")
              .minSize(2)
              .maxSize(5)
              .assignmentOptional()
              .defaultValue(vecInt)
              .reconfigurable()
              .commit();

        INT32_ELEMENT(expected)
              .key("SimpleElem")
              .displayedName("SimpleElem")
              .description("Description of SimpleElem")
              .unit(Unit::METER)
              .metricPrefix(MetricPrefix::MILLI)
              .readOnly()
              .commit();
    }
};


FileInputOutput_Test::FileInputOutput_Test() {}


FileInputOutput_Test::~FileInputOutput_Test() {}


void FileInputOutput_Test::setUp() {
    m_canCleanUp = false;
    Hash rooted("dom.b.c", 1, "dom.b.d", vector<int>(5, 1), "dom.b.e", vector<Hash>(2, Hash("a", 1)), "a.d",
                std::complex<double>(1.2, 4.2));
    rooted.setAttribute("dom", "a1", true);
    rooted.setAttribute("dom", "a2", 3.4);
    rooted.setAttribute("dom.b", "b1", "3");
    rooted.setAttribute("dom.b.c", "c1", 2);
    rooted.setAttribute("dom.b.c", "c2", vector<string>(3, "bla"));
    m_rootedHash = rooted;


    // Hash big("a.b", std::vector<double>(20 * 1024 * 1024, 1.0));
    Hash big("a.b", std::vector<double>(1000, 1.0));

    vector<Hash>& tmp = big.bindReference<vector<Hash> >("a.c");
    tmp.resize(10000);
    for (size_t i = 0; i < tmp.size(); ++i) {
        tmp[i] = m_rootedHash;
    }
    m_bigHash = big;
    m_bigHash.setAttribute("a.c", "k5", 123);
    m_bigHash.setAttribute("a.c", "k6", vector<bool>(4, true));
    m_bigHash.setAttribute("a.c", "k7", vector<unsigned char>(5, 1));


    Hash unrooted("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", std::vector<unsigned long long>(5, 5),
                  "F.f.f.f.f", Hash("x.y.z", 99));
    unrooted.setAttribute("F.f.f", "attr1", true);
    m_unrootedHash = unrooted;

    Schema testSchema("TestSchema", Schema::AssemblyRules(READ | WRITE | INIT));
    MySchema::expectedParameters(testSchema);
    m_schema = testSchema;

    vector<Hash>& vec = m_withSchemaHash.bindReference<vector<Hash> >("a.v");
    vec.resize(4);
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i] = Hash("schema", testSchema);
    }
    m_withSchemaHash.set("a.x", "Hello");
    m_withSchemaHash.set("a.y", testSchema);
    m_withSchemaHash.set("a.z", 25);
}


void FileInputOutput_Test::tearDown() {
    if (m_canCleanUp) {
        if (std::filesystem::exists(resourcePath("folder"))) {
            std::filesystem::remove_all(resourcePath("folder"));
        }
        if (std::filesystem::exists(resourcePath("/tmp/folder/"))) {
            std::filesystem::remove_all(resourcePath("/tmp/folder/"));
        }
    }
}


void FileInputOutput_Test::writeTextFile() {
    TimeProfiler p("writeTextFile");
    p.open();
    // Using the Factory interface
    Output<Hash>::Pointer out = Output<Hash>::create("TextFile", Hash("filename", resourcePath("file1.xml")));
    out->write(m_rootedHash);

    p.startPeriod("bigHash");
    out = Output<Hash>::create("TextFile", Hash("filename", resourcePath("file2.xml"), "format.Xml.indentation", -1));
    out->write(m_bigHash);
    p.stopPeriod("bigHash"); // p.stopPeriod();
    p.close();
    if (false) clog << "writing big Hash (text) took " << p.getPeriod("bigHash").getDuration() << " [s]" << endl;

    out = Output<Hash>::create("TextFile", Hash("filename", resourcePath("file3.xml"), "format.Xml.indentation", 0,
                                                "format.Xml.writeDataTypes", false));
    out->write(m_unrootedHash);

    out = Output<Hash>::create("TextFile", Hash("filename", resourcePath("file4.xml"), "format.Xml.indentation", 0,
                                                "format.Xml.writeDataTypes", true));
    out->write(m_withSchemaHash);

    // Using the FileTools interface
    saveToFile(m_rootedHash, resourcePath("file1a.xml"));

    saveToFile(m_bigHash, resourcePath("file2a.xml"), Hash("format.Xml.indentation", -1));

    saveToFile(m_unrootedHash, resourcePath("file3a.xml"),
               Hash("format.Xml.indentation", 0, "format.Xml.writeDataTypes", false));

    saveToFile(m_withSchemaHash, resourcePath("file4a.xml"),
               Hash("format.Xml.indentation", 0, "format.Xml.writeDataTypes", true));

    // Check different folder levels
    if (std::filesystem::exists(resourcePath("folder/"))) {
        CPPUNIT_FAIL("'folder' already exists!");
    }
    saveToFile(m_rootedHash, resourcePath("folder/file5a.xml"));

    if (std::filesystem::exists(resourcePath("/tmp/folder/"))) {
        CPPUNIT_FAIL("'/tmp/folder' already exists!");
    }
    saveToFile(m_rootedHash, "/tmp/folder/file6a.xml");
}


void FileInputOutput_Test::readTextFile() {
    // Using the Factory interface
    Input<Hash>::Pointer in = Input<Hash>::create("TextFile", Hash("filename", resourcePath("file1.xml")));
    Hash h1;
    in->read(h1);

    in = Input<Hash>::create("TextFile", Hash("filename", resourcePath("file2.xml"), "format", "Xml"));
    Hash h2;
    in->read(h2);

    in = Input<Hash>::create("TextFile", Hash("filename", resourcePath("file3.xml")));
    Hash h3;
    in->read(h3);

    in = Input<Hash>::create("TextFile", Hash("filename", resourcePath("file4.xml")));
    Hash h4;
    in->read(h4);


    // Using the FileTools interface
    Hash h1a;
    loadFromFile(h1a, resourcePath("file1a.xml"));

    Hash h2a;
    loadFromFile(h2a, resourcePath("file2a.xml"));

    Hash h3a;
    loadFromFile(h3a, resourcePath("file3a.xml"));

    Hash h4a;
    loadFromFile(h4a, resourcePath("file4a.xml"));

    Hash h5a;
    loadFromFile(h5a, resourcePath("folder/file5a.xml"));

    Hash h6a;
    loadFromFile(h6a, "/tmp/folder/file6a.xml");

    //    clog << "h2 (xml)\n" << h2 << endl;

    CPPUNIT_ASSERT(karabo::data::similar(h1, m_rootedHash));
    CPPUNIT_ASSERT(karabo::data::similar(h1, h1a));
    CPPUNIT_ASSERT(karabo::data::similar(h2, m_bigHash));

    // TODO: This has to be fixed (vector<Hash> attributes)
    //    CPPUNIT_ASSERT(h2.getAttribute<int>("a.c", "k5") == 123);
    //    vector<bool> vecBoolAttr = h2.getAttribute < vector<bool> >("a.c", "k6");
    //    vector<bool> refVecBoolAttr(4, true);
    //    for (size_t i = 0; i < refVecBoolAttr.size(); ++i) {
    //        CPPUNIT_ASSERT(refVecBoolAttr[i] == vecBoolAttr[i]);
    //    }

    CPPUNIT_ASSERT(karabo::data::similar(h2, h2a));

    // for h3 data types have been not serialized so we cannot use "similar" function
    CPPUNIT_ASSERT(h3.get<string>("a.b.c") == "1");
    CPPUNIT_ASSERT(h3a.get<string>("a.b.c") == "1");

    CPPUNIT_ASSERT(karabo::data::similar(h4, m_withSchemaHash));
    CPPUNIT_ASSERT(karabo::data::similar(h4, h4a));

    CPPUNIT_ASSERT(karabo::data::similar(h5a, m_rootedHash));
    CPPUNIT_ASSERT(karabo::data::similar(h6a, m_rootedHash));
    m_canCleanUp = true;
}


void FileInputOutput_Test::writeTextSchema() {
    // Using the Factory interface
    Output<Schema>::Pointer out = Output<Schema>::create(
          "TextFile", Hash("filename", resourcePath("testschema.xml"), "format.Xml.indentation", 3));
    out->write(m_schema);

    // Using the FileTools interface
    saveToFile(m_schema, resourcePath("testschema2.xml"));
}


void FileInputOutput_Test::readTextSchema() {
    // Using the Factory interface
    Input<Schema>::Pointer in = Input<Schema>::create("TextFile", Hash("filename", resourcePath("testschema.xml")));
    Schema schema1;
    in->read(schema1);
    CPPUNIT_ASSERT(karabo::data::similar(schema1, m_schema));

    // Using the FileTools interface
    Schema schema2;
    loadFromFile(schema2, resourcePath("testschema2.xml"));
    CPPUNIT_ASSERT(karabo::data::similar(schema2, m_schema));
}


void FileInputOutput_Test::writeSequenceToTextFile() {
    // Using the Factory interface
    Output<Hash>::Pointer out =
          Output<Hash>::create("TextFile", Hash("filename", resourcePath("seqfile1.xml"), "enableAppendMode", true));
    for (size_t i = 0; i < 10; ++i) {
        out->write(m_rootedHash);
    }
    out->update(); // Necessary call to indicate completion of sequence writing
}


void FileInputOutput_Test::readSequenceFromTextFile() {
    // Using the Factory interface
    Input<Hash>::Pointer in = Input<Hash>::create("TextFile", Hash("filename", resourcePath("seqfile1.xml")));
    Hash h1;
    CPPUNIT_ASSERT(in->size() == 10);
    for (size_t i = 0; i < in->size(); ++i) {
        in->read(h1, i);
        CPPUNIT_ASSERT(karabo::data::similar(h1, m_rootedHash));
    }
}


void FileInputOutput_Test::writeBinaryFile() {
    TimeProfiler p("writeBinaryFile");
    p.open();
    // Using the Factory interface
    Output<Hash>::Pointer out = Output<Hash>::create("BinaryFile", Hash("filename", resourcePath("file1.bin")));
    out->write(m_rootedHash);

    p.startPeriod("bigHash");
    out = Output<Hash>::create("BinaryFile", Hash("filename", resourcePath("file2.bin")));
    out->write(m_bigHash);
    p.stopPeriod(); // p.stopPeriod("bigHash"); TODO
    p.close();
    if (false) clog << "writing big Hash (binary) took " << p.getPeriod("bigHash").getDuration() << " [s]" << endl;

    out = Output<Hash>::create("BinaryFile", Hash("filename", resourcePath("file3.bin")));
    out->write(m_unrootedHash);

    //    TODO: uncomment when schema serialization is done
    //    out = Output<Hash>::create("BinaryFile", Hash("filename", resourcePath("file4.bin")));
    //    out->write(m_withSchemaHash);


    // Using the FileTools interface
    saveToFile(m_rootedHash, resourcePath("file1a.bin"));

    saveToFile(m_bigHash, resourcePath("file2a.bin"));

    saveToFile(m_unrootedHash, resourcePath("file3a.bin"));
    //    TODO: uncomment when schema serialization is done
    //    saveToFile(m_withSchemaHash, resourcePath("file4a.bin"));
}


void FileInputOutput_Test::readBinaryFile() {
    // Using the Factory interface
    Input<Hash>::Pointer in =
          Input<Hash>::create("BinaryFile", Hash("filename", resourcePath("file1.bin"), "format", "Bin"));
    Hash h1;
    in->read(h1);

    in = Input<Hash>::create("BinaryFile", Hash("filename", resourcePath("file2.bin"), "format", "Bin"));
    Hash h2;
    in->read(h2);

    in = Input<Hash>::create("BinaryFile", Hash("filename", resourcePath("file3.bin")));
    Hash h3;
    in->read(h3);

    //    TODO: uncomment when schema serialization is done
    //     in = Input<Hash>::create("BinaryFile", Hash("filename", resourcePath("file4.bin")));
    //     Hash h4;
    //     in->read(h4);


    // Using the FileTools interface
    Hash h1a;
    loadFromFile(h1a, resourcePath("file1a.bin"));

    Hash h2a;
    loadFromFile(h2a, resourcePath("file2a.bin"));

    Hash h3a;
    loadFromFile(h3a, resourcePath("file3a.bin"));

    //    TODO: uncomment when schema serialization is done
    //    Hash h4a;
    //    loadFromFile(h4a, resourcePath("file4a.bin"));


    //    clog << "h2 (binary)\n" << h2 << endl;
    CPPUNIT_ASSERT(karabo::data::similar(h1, m_rootedHash));
    CPPUNIT_ASSERT(karabo::data::similar(h1, h1a));
    CPPUNIT_ASSERT(karabo::data::similar(h2, m_bigHash));

    CPPUNIT_ASSERT(h2.getAttribute<int>("a.c", "k5") == 123);
    vector<bool> vecBoolAttr = h2.getAttribute<vector<bool> >("a.c", "k6");
    vector<bool> refVecBoolAttr(4, true);
    for (size_t i = 0; i < refVecBoolAttr.size(); ++i) {
        CPPUNIT_ASSERT(refVecBoolAttr[i] == vecBoolAttr[i]);
    }
    CPPUNIT_ASSERT(karabo::data::similar(h2, h2a));

    CPPUNIT_ASSERT(karabo::data::similar(h3, m_unrootedHash));
    CPPUNIT_ASSERT(karabo::data::similar(h3, h3a));

    //    TODO: uncomment when schema serialization is done
    //    CPPUNIT_ASSERT(karabo::data::similar(h4, m_withSchemaHash));
    //    CPPUNIT_ASSERT(karabo::data::similar(h4, h4a));
}


void FileInputOutput_Test::writeBinarySchema() {
    // Using the Factory interface
    Output<Schema>::Pointer out =
          Output<Schema>::create("BinaryFile", Hash("filename", resourcePath("testschema.bin")));
    out->write(m_schema);

    // Using the FileTools interface
    saveToFile(m_schema, resourcePath("testschema2.bin"));
}


void FileInputOutput_Test::readBinarySchema() {
    // Using the Factory interface
    Input<Schema>::Pointer in = Input<Schema>::create("BinaryFile", Hash("filename", resourcePath("testschema.bin")));
    Schema schema1;
    in->read(schema1);
    CPPUNIT_ASSERT(karabo::data::similar(schema1, m_schema));

    // Using the FileTools interface
    Schema schema2;
    loadFromFile(schema2, resourcePath("testschema2.bin"));
    CPPUNIT_ASSERT(karabo::data::similar(schema2, m_schema));
}


void FileInputOutput_Test::writeSequenceToBinaryFile() {
    // Using the Factory interface
    Output<Hash>::Pointer out =
          Output<Hash>::create("BinaryFile", Hash("filename", resourcePath("seqfile1.bin"), "enableAppendMode", true));
    for (size_t i = 0; i < 10; ++i) {
        out->write(m_rootedHash);
    }
    out->update(); // Necessary call to indicate completion of sequence writing
}


void FileInputOutput_Test::readSequenceFromBinaryFile() {
    // Using the Factory interface
    Input<Hash>::Pointer in = Input<Hash>::create("BinaryFile", Hash("filename", resourcePath("seqfile1.bin")));
    Hash h1;
    CPPUNIT_ASSERT(in->size() == 10);
    for (size_t i = 0; i < in->size(); ++i) {
        in->read(h1, i);
        CPPUNIT_ASSERT(karabo::data::similar(h1, m_rootedHash));
    }
}
