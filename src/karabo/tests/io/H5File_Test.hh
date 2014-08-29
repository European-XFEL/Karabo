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
#include <karabo/io/h5/Format.hh>
#include <complex>

class H5File_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(H5File_Test);

    CPPUNIT_TEST(testBuffer);

    CPPUNIT_TEST(testWrite);
    CPPUNIT_TEST(testWriteFailure);
            

    CPPUNIT_TEST(testRead);
    CPPUNIT_TEST(testReadTable);
    CPPUNIT_TEST(testVectorOfHashes);

    CPPUNIT_TEST(testManyGroups);
    CPPUNIT_TEST(testManyTables);
//        CPPUNIT_TEST(testVLWrite);
    CPPUNIT_TEST(testTrainFormat);
    CPPUNIT_TEST(testClose);
    CPPUNIT_TEST(testArray);

//    CPPUNIT_TEST(testExternalHdf5);
            
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

    size_t m_v4Size;
    std::vector<std::complex<float> > m_v4;





    const size_t m_maxRec;
    karabo::util::Dims m_dimsVec;

    karabo::util::Dims m_dimsVecA1;
    std::vector<int> m_a1;

    karabo::util::Dims m_dimsVecA2;
    std::vector<std::string> m_a2;

    karabo::util::Dims m_dimsVecA3;
    std::vector<bool> m_a3;

    karabo::util::Dims m_dimsVecA4;
    std::vector<std::complex<float> > m_a4;

    karabo::util::Dims m_dimsVecA5;
    std::vector<std::complex<double> > m_a5;

    bool m_testBufferWriteSuccess;

    bool m_reportTime;

    void testWrite();
    void testWriteFailure();
    void testVLWrite();

    void testBufferWrite();
    void testBufferRead();
    void testBuffer();
    void testManyGroups();
    void testManyTables();

    void testRead();
    void testReadTable();

    void testClose();
    void testVectorOfHashes();

    void testArray();
    void testExternalHdf5();
    
    void testTrainFormat();
    uint64_t fillTrainBuffer(std::vector<char>& buffer, size_t bufferLen, const karabo::util::Hash& dataset,
                             unsigned long long trainId, unsigned short imageCount);

    karabo::io::h5::Format::Pointer trainFormatImages(const karabo::util::Hash& dataset);
    karabo::io::h5::Format::Pointer trainFormatDescriptors();
    karabo::io::h5::Format::Pointer trainFormatTrainData(unsigned short detectorDataBlockSize);
};

#endif	/* H5FILE_TEST_HH */

