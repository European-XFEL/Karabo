/* 
 * File:   FileInputOutput_Test.cc
 * Author: heisenb
 * 
 * Created on March 7, 2013, 11:06 AM
 */

#include <boost/filesystem/path.hpp>

#include <karabo/io/FileTools.hh>
#include <karabo/util/Profiler.hh>

#include "TestPathSetup.hh"
#include "FileInputOutput_Test.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::io;

CPPUNIT_TEST_SUITE_REGISTRATION(FileInputOutput_Test);


FileInputOutput_Test::FileInputOutput_Test() {
}


FileInputOutput_Test::~FileInputOutput_Test() {
}


void FileInputOutput_Test::setUp() {
    Hash rooted("dom.b.c", 1, "dom.b.d", vector<int>(5, 1), "dom.b.e", vector<Hash > (2, Hash("a", 1)), "a.d", std::complex<double>(1.2, 4.2));
    rooted.setAttribute("dom", "a1", true);
    rooted.setAttribute("dom", "a2", 3.4);
    rooted.setAttribute("dom.b", "b1", "3");
    rooted.setAttribute("dom.b.c", "c1", 2);
    rooted.setAttribute("dom.b.c", "c2", vector<string > (3, "bla"));
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



    Hash unrooted("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", std::vector<unsigned long long > (5, 5), "F.f.f.f.f", Hash("x.y.z", 99));
    unrooted.setAttribute("F.f.f", "attr1", true);
    m_unrootedHash = unrooted;
}


void FileInputOutput_Test::tearDown() {

}


void FileInputOutput_Test::writeTextFile() {
    Profiler p("writeTextFile");
    // Using the Factory interface
    Output<Hash>::Pointer out = Output<Hash>::create("TextFile", Hash("filename", resourcePath("file1.xml")));
    out->write(m_rootedHash);

    p.start("bigHash");
    out = Output<Hash>::create("TextFile", Hash("filename", resourcePath("file2.xml"), "format.Xml.indentation", -1));
    out->write(m_bigHash);
    p.stop("bigHash");
    double time = HighResolutionTimer::time2double(p.getTime("bigHash"));
    if (false) clog << "writing big Hash (text) took " << time << " [s]" << endl;

    out = Output<Hash>::create("TextFile", Hash("filename", resourcePath("file3.xml"), "format.Xml.indentation", 0, "format.Xml.writeDataTypes", false));
    out->write(m_unrootedHash);

    // Using the FileTools interface
    saveToFile(m_rootedHash, resourcePath("file1a.xml"));

    saveToFile(m_bigHash, resourcePath("file2a.xml"), Hash("format.Xml.indentation", -1));

    saveToFile(m_unrootedHash, resourcePath("file3a.xml"), Hash("format.Xml.indentation", 0, "format.Xml.writeDataTypes", false));


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

    // Using the FileTools interface
    Hash h1a;
    loadFromFile(h1a, resourcePath("file1a.xml"));

    Hash h2a;
    loadFromFile(h2a, resourcePath("file2a.xml"));

    Hash h3a;
    loadFromFile(h3a, resourcePath("file3a.xml"));

//    clog << "h2 (xml)\n" << h2 << endl;

    CPPUNIT_ASSERT(karabo::util::similar(h1, m_rootedHash));
    CPPUNIT_ASSERT(karabo::util::similar(h1, h1a));
    CPPUNIT_ASSERT(karabo::util::similar(h2, m_bigHash));

// TODO: This has to be fixed (vector<Hash> attributes)
//    CPPUNIT_ASSERT(h2.getAttribute<int>("a.c", "k5") == 123);
//    vector<bool> vecBoolAttr = h2.getAttribute < vector<bool> >("a.c", "k6");
//    vector<bool> refVecBoolAttr(4, true);
//    for (size_t i = 0; i < refVecBoolAttr.size(); ++i) {
//        CPPUNIT_ASSERT(refVecBoolAttr[i] == vecBoolAttr[i]);
//    }

    CPPUNIT_ASSERT(karabo::util::similar(h2, h2a));

    CPPUNIT_ASSERT(h3.get<string>("a.b.c") == "1");
    CPPUNIT_ASSERT(h3a.get<string>("a.b.c") == "1");
}


void FileInputOutput_Test::writeSequenceToTextFile() {

    // Using the Factory interface
    Output<Hash>::Pointer out = Output<Hash>::create("TextFile", Hash("filename", resourcePath("seqfile1.xml"), "enableAppendMode", true));
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
        CPPUNIT_ASSERT(karabo::util::similar(h1, m_rootedHash));
    }
}


void FileInputOutput_Test::writeBinaryFile() {

    Profiler p("writeBinaryFile");

    // Using the Factory interface
    Output<Hash>::Pointer out = Output<Hash>::create("BinaryFile", Hash("filename", resourcePath("file1.bin")));
    out->write(m_rootedHash);

    p.start("bigHash");
    out = Output<Hash>::create("BinaryFile", Hash("filename", resourcePath("file2.bin")));
    out->write(m_bigHash);
    p.stop("bigHash");
    double time = HighResolutionTimer::time2double(p.getTime("bigHash"));
    if (false) clog << "writing big Hash (binary) took " << time << " [s]" << endl;

    out = Output<Hash>::create("BinaryFile", Hash("filename", resourcePath("file3.bin")));
    out->write(m_unrootedHash);

    // Using the FileTools interface
    saveToFile(m_rootedHash, resourcePath("file1a.bin"));

    saveToFile(m_bigHash, resourcePath("file2a.bin"));

    saveToFile(m_unrootedHash, resourcePath("file3a.bin"));


}


void FileInputOutput_Test::readBinaryFile() {

    // Using the Factory interface
    Input<Hash>::Pointer in = Input<Hash>::create("BinaryFile", Hash("filename", resourcePath("file1.bin"), "format", "Bin"));
    Hash h1;
    in->read(h1);

    in = Input<Hash>::create("BinaryFile", Hash("filename", resourcePath("file2.bin"), "format", "Bin"));
    Hash h2;
    in->read(h2);
    //    cout << h2 << endl;

    in = Input<Hash>::create("BinaryFile", Hash("filename", resourcePath("file3.bin")));
    Hash h3;
    in->read(h3);

    // Using the FileTools interface
    Hash h1a;
    loadFromFile(h1a, resourcePath("file1a.bin"));

    Hash h2a;
    loadFromFile(h2a, resourcePath("file2a.bin"));

    Hash h3a;
    loadFromFile(h3a, resourcePath("file3a.bin"));

//    clog << "h2 (binary)\n" << h2 << endl;

    CPPUNIT_ASSERT(karabo::util::similar(h1, m_rootedHash));
    CPPUNIT_ASSERT(karabo::util::similar(h1, h1a));
    CPPUNIT_ASSERT(karabo::util::similar(h2, m_bigHash));

    CPPUNIT_ASSERT(h2.getAttribute<int>("a.c", "k5") == 123);
    vector<bool> vecBoolAttr = h2.getAttribute < vector<bool> >("a.c", "k6");
    vector<bool> refVecBoolAttr(4, true);
    for (size_t i = 0; i < refVecBoolAttr.size(); ++i) {
        CPPUNIT_ASSERT(refVecBoolAttr[i] == vecBoolAttr[i]);
    }

    CPPUNIT_ASSERT(karabo::util::similar(h2, h2a));

    CPPUNIT_ASSERT(h3.get<int>("a.b.c") == 1);
    CPPUNIT_ASSERT(h3a.get<int>("a.b.c") == 1);
}


void FileInputOutput_Test::writeSequenceToBinaryFile() {

    // Using the Factory interface
    Output<Hash>::Pointer out = Output<Hash>::create("BinaryFile", Hash("filename", resourcePath("seqfile1.bin"), "enableAppendMode", true));
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
        CPPUNIT_ASSERT(karabo::util::similar(h1, m_rootedHash));
    }
}


void FileInputOutput_Test::writeHdf5File() {

    Profiler p("writeHdf5File");

    try {

        Output<Hash>::Pointer out = Output<Hash>::create("Hdf5File", Hash("filename", resourcePath("fileS1.h5")));
        out->write(m_rootedHash);


        p.start("bigHash");
        out = Output<Hash>::create("Hdf5File", Hash("filename", resourcePath("fileS2.h5")));
        out->write(m_bigHash);
        p.stop("bigHash");
        double time = HighResolutionTimer::time2double(p.getTime("bigHash"));
        if (false) clog << "writing big Hash (Hdf5) took " << time << " [s]" << endl;

        out = Output<Hash>::create("Hdf5File", Hash("filename", resourcePath("fileS3.h5")));
        out->write(m_unrootedHash);

        // Using the FileTools interface
        saveToFile(m_rootedHash, resourcePath("fileS1a.h5"));

        saveToFile(m_bigHash, resourcePath("fileS2a.h5"));

        saveToFile(m_unrootedHash, resourcePath("fileS3a.h5"));
    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Hdf5 test failed");
    }

}


void FileInputOutput_Test::readHdf5File() {

    Hash h1, h2, h3, h1a, h2a, h3a;
    try {
        // Using the Factory interface
        Input<Hash>::Pointer in = Input<Hash>::create("Hdf5File", Hash("filename", resourcePath("fileS1.h5")));
        in->read(h1);

        in = Input<Hash>::create("Hdf5File", Hash("filename", resourcePath("fileS2.h5")));
        in->read(h2);

        in = Input<Hash>::create("Hdf5File", Hash("filename", resourcePath("fileS3.h5")));
        in->read(h3);

        // Using the FileTools interface
        loadFromFile(h1a, resourcePath("fileS1a.h5"));

        loadFromFile(h2a, resourcePath("fileS2a.h5"));

        loadFromFile(h3a, resourcePath("fileS3a.h5"));

    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Hdf5 test failed");
    }

//    clog << "h2(hdf5)\n" << h2 << endl;
//    clog << "ref\n" << m_bigHash << endl;
    CPPUNIT_ASSERT(karabo::util::similar(h1, m_rootedHash));
    CPPUNIT_ASSERT(karabo::util::similar(h1, h1a));
    CPPUNIT_ASSERT(karabo::util::similar(h2, m_bigHash));
    CPPUNIT_ASSERT(karabo::util::similar(h2, h2a));

    CPPUNIT_ASSERT(h2.getAttribute<int>("a.c", "k5") == 123);
    vector<bool> vecBoolAttr = h2.getAttribute < vector<bool> >("a.c", "k6");
    vector<bool> refVecBoolAttr(4, true);
    for (size_t i = 0; i < refVecBoolAttr.size(); ++i) {
        CPPUNIT_ASSERT(refVecBoolAttr[i] == vecBoolAttr[i]);
    }
    CPPUNIT_ASSERT(karabo::util::similar(h2, h2a));

    CPPUNIT_ASSERT(h3.get<int>("a.b.c") == 1);
    CPPUNIT_ASSERT(h3a.get<int>("a.b.c") == 1);
}


void FileInputOutput_Test::writeSequenceToHdf5File() {

    // Using the Factory interface
    Output<Hash>::Pointer out = Output<Hash>::create("Hdf5File", Hash("filename", resourcePath("seqfile1.h5"), "enableAppendMode", true));
    for (size_t i = 0; i < 10; ++i) {
        out->write(m_rootedHash);
    }
    out->update(); // Necessary call to indicate completion of sequence writing
}


void FileInputOutput_Test::readSequenceFromHdf5File() {
    // Using the Factory interface
    Input<Hash>::Pointer in = Input<Hash>::create("Hdf5File", Hash("filename", resourcePath("seqfile1.h5")));
    Hash h1;
    CPPUNIT_ASSERT(in->size() == 10);
    for (size_t i = 0; i < in->size(); ++i) {
        in->read(h1, i);
        CPPUNIT_ASSERT(karabo::util::similar(h1, m_rootedHash));
    }
}
