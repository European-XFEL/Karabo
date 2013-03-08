/* 
 * File:   FileInputOutput_Test.cc
 * Author: heisenb
 * 
 * Created on March 7, 2013, 11:06 AM
 */

#include <boost/filesystem/path.hpp>

#include <karabo/io/FileTools.hh>

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
    Hash rooted("a.b.c", 1, "a.b.d", vector<int>(5, 1), "a.b.e", vector<Hash > (2, Hash("a", 1)), "a.d", std::complex<double>(1.2, 4.2));
    rooted.setAttribute("a", "a1", true);
    rooted.setAttribute("a", "a2", 3.4);
    rooted.setAttribute("a.b", "b1", "3");
    rooted.setAttribute("a.b.c", "c1", 2);
    rooted.setAttribute("a.b.c", "c2", vector<string > (3, "bla"));
    m_rootedHash = rooted;


    Hash big("a.b", std::vector<double>(1000, 1.0));
    vector<Hash>& tmp = big.bindReference<vector<Hash> >("a.c");
    tmp.resize(10000);
    for (size_t i = 0; i < tmp.size(); ++i) {
        tmp[i] = m_rootedHash;
    }
    m_bigHash = big;


    Hash unrooted("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", std::vector<unsigned long long > (5, 5), "F.f.f.f.f", Hash("x.y.z", 99));
    unrooted.setAttribute("F.f.f", "attr1", true);
    m_unrootedHash = unrooted;
}

void FileInputOutput_Test::tearDown() {
   
}

void FileInputOutput_Test::writeTextFile() {
    
    // Using the Factory interface
    Output<Hash>::Pointer out = Output<Hash>::create("TextFile", Hash("filename", resourcePath("file1.xml")));
    out->write(m_rootedHash);
    
    out = Output<Hash>::create("TextFile", Hash("filename", resourcePath("file2.xml"), "format.Xml.indentation", -1));
    out->write(m_bigHash);
    
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
    
    CPPUNIT_ASSERT(karabo::util::similar(h1, m_rootedHash));
    CPPUNIT_ASSERT(karabo::util::similar(h1, h1a));
    CPPUNIT_ASSERT(karabo::util::similar(h2, m_bigHash));
    CPPUNIT_ASSERT(karabo::util::similar(h2, h2a));
    
    CPPUNIT_ASSERT(h3.get<string>("a.b.c") == "1");
    CPPUNIT_ASSERT(h3a.get<string>("a.b.c") == "1");
}

void FileInputOutput_Test::writeBinaryFile() {
    
    // Using the Factory interface
    Output<Hash>::Pointer out = Output<Hash>::create("BinaryFile", Hash("filename", resourcePath("file1.bin")));
    out->write(m_rootedHash);
    
    out = Output<Hash>::create("BinaryFile", Hash("filename", resourcePath("file2.bin")));
    out->write(m_bigHash);
    
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
    
    CPPUNIT_ASSERT(karabo::util::similar(h1, m_rootedHash));
    CPPUNIT_ASSERT(karabo::util::similar(h1, h1a));
    CPPUNIT_ASSERT(karabo::util::similar(h2, m_bigHash));
    CPPUNIT_ASSERT(karabo::util::similar(h2, h2a));
    
    CPPUNIT_ASSERT(h3.get<int>("a.b.c") == 1);
    CPPUNIT_ASSERT(h3a.get<int>("a.b.c") == 1);
}
