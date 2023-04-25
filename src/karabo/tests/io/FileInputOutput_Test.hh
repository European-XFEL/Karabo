/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   FileInputOutput_Test.hh
 * Author: heisenb
 *
 * Created on March 7, 2013, 11:06 AM
 */

#ifndef FILEINPUTOUTPUT_TEST_HH
#define FILEINPUTOUTPUT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>

class FileInputOutput_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(FileInputOutput_Test);
    CPPUNIT_TEST(writeTextFile);
    CPPUNIT_TEST(readTextFile);
    CPPUNIT_TEST(writeTextSchema);
    CPPUNIT_TEST(readTextSchema);
    CPPUNIT_TEST(writeSequenceToTextFile);
    CPPUNIT_TEST(readSequenceFromTextFile);
    CPPUNIT_TEST(writeBinaryFile);
    CPPUNIT_TEST(readBinaryFile);
    CPPUNIT_TEST(writeBinarySchema);
    CPPUNIT_TEST(readBinarySchema);
    CPPUNIT_TEST(writeSequenceToBinaryFile);
    CPPUNIT_TEST(readSequenceFromBinaryFile);
    CPPUNIT_TEST(writeHdf5File);
    CPPUNIT_TEST(readHdf5File);
    CPPUNIT_TEST(writeSequenceToHdf5File);
    CPPUNIT_TEST(readSequenceFromHdf5File);
    CPPUNIT_TEST_SUITE_END();


   public:
    FileInputOutput_Test();
    virtual ~FileInputOutput_Test();
    void setUp();
    void tearDown();

   private:
    karabo::util::Schema m_schema;
    karabo::util::Hash m_rootedHash;
    karabo::util::Hash m_bigHash;
    karabo::util::Hash m_unrootedHash;
    karabo::util::Hash m_withSchemaHash;
    bool m_canCleanUp;

    void writeTextFile();
    void readTextFile();
    void writeTextSchema();
    void readTextSchema();
    void writeSequenceToTextFile();
    void readSequenceFromTextFile();
    void writeBinaryFile();
    void readBinaryFile();
    void writeBinarySchema();
    void readBinarySchema();
    void writeSequenceToBinaryFile();
    void readSequenceFromBinaryFile();
    void writeHdf5File();
    void readHdf5File();
    void writeSequenceToHdf5File();
    void readSequenceFromHdf5File();
};

#endif /* FILEINPUTOUTPUT_TEST_HH */
