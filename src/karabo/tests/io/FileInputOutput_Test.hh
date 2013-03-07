/* 
 * File:   FileInputOutput_Test.hh
 * Author: heisenb
 *
 * Created on March 7, 2013, 11:06 AM
 */

#ifndef FILEINPUTOUTPUT_TEST_HH
#define	FILEINPUTOUTPUT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class FileInputOutput_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(FileInputOutput_Test);
    CPPUNIT_TEST(writeTextFile);
    CPPUNIT_TEST(readTextFile);
    CPPUNIT_TEST(writeBinaryFile);
    CPPUNIT_TEST(readBinaryFile);
    CPPUNIT_TEST_SUITE_END();

public:
    FileInputOutput_Test();
    virtual ~FileInputOutput_Test();
    void setUp();
    void tearDown();

private:
    
    karabo::util::Hash m_rootedHash;
    karabo::util::Hash m_bigHash;
    karabo::util::Hash m_unrootedHash;
    
    void writeTextFile();
    void readTextFile();
    void writeBinaryFile();
    void readBinaryFile();
};

#endif	/* FILEINPUTOUTPUT_TEST_HH */

