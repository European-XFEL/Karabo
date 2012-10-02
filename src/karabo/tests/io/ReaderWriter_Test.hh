/*
 * File:   ReaderWriter_Test.hh
 * Author: irinak
 *
 * Created on Oct 1, 2012, 5:32:32 PM
 */

#ifndef READERWRITER_TEST_HH
#define	READERWRITER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/io/Reader.hh>
#include <karabo/io/Writer.hh>
#include <karabo/io/StringStreamWriter.hh>
#include <karabo/io/StringStreamReader.hh>
#include <karabo/util/Hash.hh>

class ReaderWriter_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(ReaderWriter_Test);

    CPPUNIT_TEST(testWritingReadingBinaryFormat);
    CPPUNIT_TEST(testWriting);

    CPPUNIT_TEST_SUITE_END();

public:
    ReaderWriter_Test();
    virtual ~ReaderWriter_Test();

private:
    void testWritingReadingBinaryFormat();
    void testWriting();

};

#endif	/* READERWRITER_TEST_HH */

