/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Created on Oct 4, 2012, 5:30:35 PM
 */

#ifndef READERWRITER_TEST_HH
#define	READERWRITER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/io/Reader.hh>
#include <karabo/io/Writer.hh>
#include <karabo/io/StringStreamWriter.hh>
#include <karabo/io/StringStreamReader.hh>
#include <karabo/io/SchemaXsdFormat.hh>
#include <karabo/util/Hash.hh>
#include <boost/filesystem/path.hpp>

class ReaderWriter_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(ReaderWriter_Test);

    CPPUNIT_TEST(testWritingReadingBinaryFormat);
    CPPUNIT_TEST(testWritingReadingXmlFormat);
    CPPUNIT_TEST(testWritingSchema);

    CPPUNIT_TEST_SUITE_END();

public:
    ReaderWriter_Test();
    virtual ~ReaderWriter_Test();
    void setUp();
    karabo::util::Hash hash;
    std::string runDir;

private:
    void testWritingReadingBinaryFormat();
    void testWritingReadingXmlFormat();
    void testWritingSchema();

};

#endif	/* READERWRITER_TEST_HH */