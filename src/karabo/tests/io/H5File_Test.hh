/* 
 * File:   H5File_Test.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on February 20, 2013, 2:33 PM
 */

#ifndef H5FILE_TEST_HH
#define	H5FILE_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/util/Configurator.hh>
#include <karabo/util/Dims.hh>

class H5File_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(H5File_Test);
    CPPUNIT_TEST(testBufferWrite);
    CPPUNIT_TEST(testBufferRead);
    //    CPPUNIT_TEST(testVectorBufferWrite);
    CPPUNIT_TEST(testWrite);
    CPPUNIT_TEST(testRead);
    //    CPPUNIT_TEST(testReadTable);
    CPPUNIT_TEST_SUITE_END();

public:

    KARABO_CLASSINFO(H5File_Test, "H5File_Test", "1.0");

    H5File_Test();
    virtual ~H5File_Test();
    void setUp();
    void tearDown();

private:

    size_t m_numberOfRecords;

    size_t m_v3Size;
    std::vector<std::string> m_v3;


    const size_t m_maxRec;
    karabo::util::Dims m_dimsVec;
    std::vector<int> m_a1;
    std::vector<std::string> m_a2;
    std::vector<bool> m_a3;

    bool m_reportTime;

    void testWrite();
    void testBufferWrite();
    void testBufferRead();
    //void testVectorBufferWrite();
    void testRead();
    void testReadTable();
};

#endif	/* H5FILE_TEST_HH */

