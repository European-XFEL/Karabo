/* 
 * File:   Hdf5File_Test.hh
 * Author: Krzysztof Wrona <krzysztof.wrona@xfel.eu>
 *
 * Created on November 27, 2012, 1:15 PM
 */

#ifndef HDF5FILE_TEST_HH
#define	HDF5FILE_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <vector>
#include <deque>
#include <karabo/io/hdf5/File.hh>
#include <karabo/io/ArrayView.hh>
#include <karabo/io/ArrayDimensions.hh>
#include <karabo/io/ioProfiler.hh>

class Hdf5File_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(Hdf5File_Test);
    CPPUNIT_TEST(testWrite);

    CPPUNIT_TEST_SUITE_END();

public:
    Hdf5File_Test();
    virtual ~Hdf5File_Test();
    void setUp();
    void tearDown();

private:

    void testWrite();


    void compute(karabo::util::Hash& rec, int idx);

    template<class T>
    void assertArrayView(std::string name, const karabo::util::Hash& h, const T& dummy = 0) {

        CPPUNIT_ASSERT(h.hasFromPath("arrayView." + name));
        const karabo::io::ArrayView<T>& rva = h.getFromPath < karabo::io::ArrayView<T> > ("arrayView." + name);
        const karabo::io::ArrayDimensions adim = rva.getDims();
        CPPUNIT_ASSERT(adim.getRank() == 1);
        CPPUNIT_ASSERT(adim[0] == arraySize);
        for (size_t j = 0; j < arraySize; ++j) {
            tracer << "rva[" << j << "]: " << rva[j] << std::endl;
            CPPUNIT_ASSERT(rva[j] == (vecSize - 1 + j));
        }
    }

    void assertStringArrayView(std::string name, int i, const karabo::util::Hash& h) {

        CPPUNIT_ASSERT(h.hasFromPath("arrayView." + name));
        const karabo::io::ArrayView<std::string>& rva = h.getFromPath < karabo::io::ArrayView<std::string> > ("arrayView." + name);
        const karabo::io::ArrayDimensions adim = rva.getDims();
        CPPUNIT_ASSERT(adim.getRank() == 1);
        CPPUNIT_ASSERT(adim[0] == arraySize);
        for (size_t j = 0; j < arraySize; ++j) {
            std::ostringstream ref;
            tracer << "rva[" << j << "]: " << rva[j] << std::endl;
            ref << "Hello " << (vecSize - 1) << "[" << j << "]" << " from me";
            for (size_t k = 0; k < j; ++k) {
                ref << k;
            }
            tracer << "arrayView<string> rva[" << j << "]: " << rva[j] << " reference: " << ref.str() << std::endl;
            CPPUNIT_ASSERT(rva[j] == ref.str());
        }
    }

    void assertBoolArrayView(std::string name, int i, const karabo::util::Hash& h) {

        CPPUNIT_ASSERT(h.hasFromPath("arrayView." + name));
        const karabo::io::ArrayView<bool>& rva = h.getFromPath < karabo::io::ArrayView<bool> > ("arrayView." + name);
        const karabo::io::ArrayDimensions adim = rva.getDims();
        CPPUNIT_ASSERT(adim.getRank() == 1);
        CPPUNIT_ASSERT(adim[0] == arraySize);

        for (size_t j = 0; j < arraySize; ++j) {
            tracer << "rva[" << j << "]: " << (int) (rva[j]) << std::endl;
            if (j % 2) {
                CPPUNIT_ASSERT(rva[j] == true);
            } else {
                CPPUNIT_ASSERT(rva[j] == false);
            }
        }
    }

    template<class T>
    void assertVector(std::string name, int i, const karabo::util::Hash& h, const T& dummy = 0) {

        CPPUNIT_ASSERT(h.hasFromPath("vectors." + name));
        const std::vector<T>& rva = h.getFromPath < std::vector<T> > ("vectors." + name);
        CPPUNIT_ASSERT(rva.size() == arraySize);
        for (size_t j = 0; j < arraySize; ++j) {
            tracer << "rva[" << j << "]: " << (int) (rva[j]) << std::endl;
            CPPUNIT_ASSERT(rva[j] == i + j);
        }
    }

    void assertStringVector(std::string name, int i, const karabo::util::Hash& h) {

        CPPUNIT_ASSERT(h.hasFromPath("vectors." + name));
        tracer << "vectors." + name << std::endl;
        const std::vector<std::string>& rva = h.getFromPath < std::vector<std::string> > ("vectors." + name);
        CPPUNIT_ASSERT(rva.size() == arraySize);
        for (size_t j = 0; j < arraySize; ++j) {
            std::ostringstream ref;
            tracer << "rva[" << j << "]: " << rva[j] << std::endl;
            ref << "Hello " << i << "[" << j << "]" << " from me";
            for (size_t k = 0; k < j; ++k) {
                ref << k;
            }
            tracer << "rva[" << j << "]: " << rva[j] << std::endl;
            CPPUNIT_ASSERT(rva[j] == ref.str());
        }
    }

    void assertBoolVector(std::string name, int i, const karabo::util::Hash& h) {

        CPPUNIT_ASSERT(h.hasFromPath("deque." + name));
        const std::deque<bool>& rva = h.getFromPath < std::deque<bool> > ("deque." + name);
        CPPUNIT_ASSERT(rva.size() == arraySize);
        for (size_t j = 0; j < arraySize; ++j) {
            tracer << "rva[" << j << "]: " << (int) (rva[j]) << std::endl;
            if (j % 2) {
                CPPUNIT_ASSERT(rva[j] == true);
            } else {
                CPPUNIT_ASSERT(rva[j] == false);
            }



        }
    }

    std::string runDir;

    const size_t arraySize;
    size_t vecSize;


    std::vector<signed char> va;
    std::vector<short> vb;
    std::vector<int> vc;
    std::vector<long long > vd;
    std::vector<unsigned char> ve;
    std::vector<unsigned short> vf;
    std::vector<unsigned int> vg;
    std::vector<unsigned long long> vh;
    std::vector<float> vo;
    std::vector<double> vp;
    std::deque<bool> vx;
    std::vector<std::string> vs;

    boost::shared_array<signed char> aaArr;
    boost::shared_array<short> abArr;
    boost::shared_array<int> acArr;
    boost::shared_array<long long> adArr;
    boost::shared_array<unsigned char> aeArr;
    boost::shared_array<unsigned short> afArr;
    boost::shared_array<unsigned int> agArr;
    boost::shared_array<unsigned long long> ahArr;
    boost::shared_array<float> aoArr;
    boost::shared_array<double> apArr;
    boost::shared_array<bool> axArr;
    boost::shared_array<std::string> asArr;



};

#endif	/* HDF5FILE_TEST_HH */

