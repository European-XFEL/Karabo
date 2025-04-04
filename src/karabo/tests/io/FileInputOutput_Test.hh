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
 * File:   FileInputOutput_Test.hh
 * Author: heisenb
 *
 * Created on March 7, 2013, 11:06 AM
 */

#ifndef FILEINPUTOUTPUT_TEST_HH
#define FILEINPUTOUTPUT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"

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
    CPPUNIT_TEST_SUITE_END();


   public:
    FileInputOutput_Test();
    virtual ~FileInputOutput_Test();
    void setUp();
    void tearDown();

   private:
    karabo::data::Schema m_schema;
    karabo::data::Hash m_rootedHash;
    karabo::data::Hash m_bigHash;
    karabo::data::Hash m_unrootedHash;
    karabo::data::Hash m_withSchemaHash;
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
};

#endif /* FILEINPUTOUTPUT_TEST_HH */
