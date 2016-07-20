/* 
 * File:   Hdf5File_Test.cc
 * Author: Krzysztof Wrona <krzysztof.wrona@xfel.eu>
 * 
 * Created on November 27, 2012, 1:15 PM
 */

#include "Hdf5File_Test.hh"
#include <karabo/io/File.hh>
#include <karabo/io/ArrayView.hh>
#include <karabo/io/ArrayDimensions.hh>

#include <karabo/io/Writer.hh>
#include <karabo/io/Reader.hh>

using namespace std;
using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::io::hdf5;


CPPUNIT_TEST_SUITE_REGISTRATION(Hdf5File_Test);


Hdf5File_Test::Hdf5File_Test() : arraySize(6), vecSize(10) {

    va.resize(arraySize);
    vb.resize(arraySize);
    vc.resize(arraySize);
    vd.resize(arraySize);
    ve.resize(arraySize);
    vf.resize(arraySize);
    vg.resize(arraySize);
    vh.resize(arraySize);
    vo.resize(arraySize);
    vp.resize(arraySize);
    vx.resize(arraySize);
    vs.resize(arraySize);

    aaArr = boost::shared_array<signed char>(new signed char[arraySize]);
    abArr = boost::shared_array<short>(new short[arraySize]);
    acArr = boost::shared_array<int>(new int[arraySize]);
    adArr = boost::shared_array<long long>(new long long[arraySize]);
    aeArr = boost::shared_array<unsigned char>(new unsigned char[arraySize]);
    afArr = boost::shared_array<unsigned short>(new unsigned short[arraySize]);
    agArr = boost::shared_array<unsigned int>(new unsigned int[arraySize]);
    ahArr = boost::shared_array<unsigned long long>(new unsigned long long[arraySize]);
    aoArr = boost::shared_array<float>(new float[arraySize]);
    apArr = boost::shared_array<double>(new double [arraySize]);
    axArr = boost::shared_array<bool>(new bool [arraySize]);
    asArr = boost::shared_array<string > (new string [arraySize]);

    runDir = string(TESTPATH) + string("io/resources/");
}


Hdf5File_Test::~Hdf5File_Test() {
}


void Hdf5File_Test::setUp() {

}


void Hdf5File_Test::tearDown() {

}


void Hdf5File_Test::testWrite() {

    hdf5::File file(runDir + "writer.h5");
    file.open(hdf5::File::TRUNCATE);
    vector<Hash> data(vecSize);


    //Note for ArrayViews
    // For each data[i] ArrayViews are identical
    for (size_t i = 0; i < vecSize; ++i) {
        compute(data[i], i);
    }



    DataFormat::Pointer dataFormat;
    Hash dfc;
    bool discoverConfig = true;
    if (discoverConfig) {
        try {
            dataFormat = DataFormat::discoverFromData(data[0]);
        } catch (...) {
            KARABO_RETHROW;
        }
        dfc = dataFormat->getConfig();
        tracer << "dataFormatConfig: " << endl << dfc << endl;
        Writer<Hash>::Pointer wc = Writer<Hash>::create(Hash("TextFile.filename", runDir + "writer.xml"));
        wc->write(dfc);
    } else {
        Reader<Hash>::Pointer rc = Reader<Hash>::create(Hash("TextFile.filename", runDir + "writerConv.xml"));
        rc->read(dfc);
    }
    dataFormat = DataFormat::create(dfc);


    tracer << "-----" << endl << data[0] << endl << "-----";



    Table::Pointer table = file.createTable("/test", dataFormat);

    tracer << "table created " << endl;
    for (size_t i = 0; i < vecSize; ++i) {
        table->append(data[i]);
    }
    file.close();

    file.open(hdf5::File::READONLY);

    Table::Pointer tableRead = file.getTable("/test");

    std::vector<Hash> readData;
    readData.resize(vecSize);


    for (size_t i = 0; i < vecSize; ++i) {
        readData[i].setFromPath("vectors.va", vector<signed char>(arraySize));
        readData[i].setFromPath("vectors.vb", vector<short>(arraySize));
        readData[i].setFromPath("vectors.vc", vector<int>(arraySize));
        readData[i].setFromPath("vectors.vd", vector<long long>(arraySize));
        readData[i].setFromPath("vectors.ve", vector<unsigned char >(arraySize));
        readData[i].setFromPath("vectors.vf", vector<unsigned short >(arraySize));
        readData[i].setFromPath("vectors.vg", vector<unsigned int>(arraySize));
        readData[i].setFromPath("vectors.vh", vector<unsigned long long >(arraySize));
        readData[i].setFromPath("vectors.vo", vector<float>(arraySize));
        readData[i].setFromPath("vectors.vp", vector<double >(arraySize));
        readData[i].setFromPath("vectors.vs", vector<string >(arraySize));
        readData[i].setFromPath("deque.vx", deque<bool >(arraySize));
        tableRead->allocate(readData[i]);
        tableRead->read(readData[i], i);
    }

    for (size_t i = 0; i < vecSize; ++i) {

        tracer << "------- readData[" << i << "] ----------" << endl;

        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.a"));
        CPPUNIT_ASSERT(readData[i].getFromPath<signed char>("scalars.a") == (signed char) i);
        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.b"));
        CPPUNIT_ASSERT(readData[i].getFromPath<short>("scalars.b") == (short) i);
        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.c"));
        CPPUNIT_ASSERT(readData[i].getFromPath<int>("scalars.c") == (int) i);
        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.d"));
        CPPUNIT_ASSERT(readData[i].getFromPath<long long>("scalars.d") == (long long) i);
        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.e"));
        CPPUNIT_ASSERT(readData[i].getFromPath<unsigned char>("scalars.e") == (unsigned char) i);
        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.f"));
        CPPUNIT_ASSERT(readData[i].getFromPath<unsigned short>("scalars.f") == (signed short) i);
        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.g"));
        CPPUNIT_ASSERT(readData[i].getFromPath<unsigned int>("scalars.g") == (signed int) i);
        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.h"));
        CPPUNIT_ASSERT(readData[i].getFromPath<unsigned long long>("scalars.h") == (signed long) i);
        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.o"));
        CPPUNIT_ASSERT(readData[i].getFromPath<float>("scalars.o") == (float) i);
        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.p"));
        CPPUNIT_ASSERT(readData[i].getFromPath<double>("scalars.p") == (double) i);

        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.s"));
        std::ostringstream str;
        str << "Hello " << i << " World!!! ";
        string s = str.str();
        string s1 = readData[i].getFromPath<std::string > ("scalars.s");
        CPPUNIT_ASSERT(s == s1);

        CPPUNIT_ASSERT(readData[i].hasFromPath("scalars.x"));
        if (i % 2) {
            CPPUNIT_ASSERT(readData[i].getFromPath<bool>("scalars.x") == true);
        } else {
            CPPUNIT_ASSERT(readData[i].getFromPath<bool>("scalars.x") == false);
        }



        assertArrayView<signed char>("va", readData[i]);
        assertArrayView<signed short>("vb", readData[i]);
        assertArrayView<signed int>("vc", readData[i]);
        assertArrayView<signed long long>("vd", readData[i]);
        assertArrayView<unsigned char>("ve", readData[i]);
        assertArrayView<unsigned short>("vf", readData[i]);
        assertArrayView<unsigned int>("vg", readData[i]);
        assertArrayView<unsigned long long>("vh", readData[i]);
        assertArrayView<float>("vo", readData[i]);
        assertArrayView<double>("vp", readData[i]);
        assertStringArrayView("vs", i, readData[i]);
        assertBoolArrayView("vx", i, readData[i]);

        assertVector<signed char>("va", i, readData[i]);
        assertVector<signed short>("vb", i, readData[i]);
        assertVector<signed int>("vc", i, readData[i]);
        assertVector<signed long long>("vd", i, readData[i]);
        assertVector<unsigned char>("ve", i, readData[i]);
        assertVector<unsigned short>("vf", i, readData[i]);
        assertVector<unsigned int>("vg", i, readData[i]);
        assertVector<unsigned long long>("vh", i, readData[i]);
        assertVector<float>("vo", i, readData[i]);
        assertVector<double>("vp", i, readData[i]);
        assertStringVector("vs", i, readData[i]);
        assertBoolVector("vx", i, readData[i]);


        CPPUNIT_ASSERT(readData[i].hasFromPath("vectors.nonexisting") == false);
    }


}


void Hdf5File_Test::compute(Hash& rec, int idx) {


    signed char a = idx;
    short b = idx;
    int c = idx;
    long long d = idx;
    unsigned char e = idx;
    unsigned short f = idx;
    unsigned int g = idx;
    unsigned long long h = idx;
    float o = idx;
    double p = idx;
    bool x = false;
    if (idx % 2) x = true;
    std::ostringstream str;
    str << "Hello " << idx << " World!!! ";
    string s = str.str();

    //char a = 'a'+ (idx%20);

    for (size_t i = 0; i < arraySize; ++i) {
        va[i] = i + idx;
        vb[i] = i + idx;
        vc[i] = i + idx;
        vd[i] = i + idx;
        ve[i] = i + idx;
        vf[i] = i + idx;
        vg[i] = i + idx;
        vh[i] = i + idx;
        vo[i] = i + idx;
        vp[i] = i + idx;
        vx[i] = false;
        if (i % 2) vx[i] = true;
        tracer << "original vx[" << i << "] = " << vx[i] << endl;
        axArr[i] = vx[i];
        std::ostringstream str;
        str << "Hello " << idx << "[" << i << "]" << " from me";
        for (size_t j = 0; j < i; ++j) {
            str << j;
        }
        vs[i] = str.str();
    }

    rec.setFromPath("scalars.a", a);
    rec.setFromPath("scalars.b", b);
    rec.setFromPath("scalars.c", c);
    rec.setFromPath("scalars.d", d);
    rec.setFromPath("scalars.e", e);
    rec.setFromPath("scalars.f", f);
    rec.setFromPath("scalars.g", g);
    rec.setFromPath("scalars.h", h);
    rec.setFromPath("scalars.o", o);
    rec.setFromPath("scalars.p", p);
    rec.setFromPath("scalars.x", x);
    rec.setFromPath("scalars.s", s);

    //ArrayView<int> avc(acArr.get(), 2, 3);


    // Note that arrayViews store pointers to first element of vectors
    // The effect is that subsequent calls to compute method will modify values stored in 
    // in vectors and values are changed also for ArrayViews from the previous calls
    ArrayView<signed char> vaView(va);
    rec.setFromPath("arrayView.va", vaView);
    rec.setFromPath("arrayView.vb", ArrayView<short>(vb));
    rec.setFromPath("arrayView.vc", ArrayView<int>(vc));
    rec.setFromPath("arrayView.vd", ArrayView<long long>(vd));
    rec.setFromPath("arrayView.ve", ArrayView<unsigned char>(ve));
    rec.setFromPath("arrayView.vf", ArrayView<unsigned short>(vf));
    rec.setFromPath("arrayView.vg", ArrayView<unsigned int>(vg));
    rec.setFromPath("arrayView.vh", ArrayView<unsigned long long>(vh));
    rec.setFromPath("arrayView.vo", ArrayView<float>(vo));
    rec.setFromPath("arrayView.vp", ArrayView<double>(vp));
    rec.setFromPath("arrayView.vs", ArrayView<std::string > (vs));
    rec.setFromPath("arrayView.vx", ArrayView<bool>(axArr.get(), arraySize));


    rec.setFromPath("vectors.va", va);
    rec.setFromPath("vectors.vb", vb);
    rec.setFromPath("vectors.vc", vc);
    rec.setFromPath("vectors.vd", vd);
    rec.setFromPath("vectors.ve", ve);
    rec.setFromPath("vectors.vf", vf);
    rec.setFromPath("vectors.vg", vg);
    rec.setFromPath("vectors.vh", vh);
    rec.setFromPath("vectors.vo", vo);
    rec.setFromPath("vectors.vp", vp);
    rec.setFromPath("vectors.vs", vs);
    rec.setFromPath("deque.vx", vx);

}


