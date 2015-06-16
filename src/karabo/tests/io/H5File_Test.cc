/* 
 * Author: <krzysztof.wrona@xfel.eu>
 * 
 * Created on February 20, 2013, 2:33 PM
 */

//#define KARABO_ENABLE_TRACE_LOG 

#include "H5File_Test.hh"
#include "karabo/util/ArrayTools.hh"
#include "TestPathSetup.hh"
#include "karabo/io/TextSerializer.hh"
#include "karabo/io/HashXmlSerializer.hh"
#include "HashXmlSerializer_Test.hh"
#include "karabo/io/h5/Group.hh"
#include "karabo/io/h5/FixedLengthArray.hh"
#include "Hdf5_Test.hh"
#include "karabo/tests/util/Dims_Test.hh"
#include <karabo/util/Hash.hh>

#include <karabo/io/h5/File.hh>
#include <karabo/io/h5/Element.hh>
#include <karabo/io/h5/Scalar.hh>
#include <karabo/util/TimeProfiler.hh>

#include <karabo/log/Tracer.hh>

// for memcopy
#include <cstring>

using namespace karabo::util;
using namespace karabo::io::h5;
using namespace karabo::io;
using namespace krb_log4cpp;

CPPUNIT_TEST_SUITE_REGISTRATION(H5File_Test);


H5File_Test::H5File_Test() : m_maxRec(100), m_testBufferWriteSuccess(false) {

    karabo::log::Tracer tr;
    tr.disableAll();
    tr.enable("karabo.io.h5");
    //tr.disable("karabo.io.h5.DatasetWriter");
    //    tr.enable("karabo.io.h5.Table");
    //    tr.enable("karabo.io.h5.Table.saveTableFormatAsAttribute");
    //    tr.enable("karabo.io.h5.Table.openNew");
    //    tr.enable("karabo.io.h5.Table.openReadOnly");
    //    tr.enable("H5File_Test.testReadTable");
    //    tr.enable("H5File_Test.testBufferWrite");
    //    tr.enable("H5File_Test.testBufferRead");
    //    tr.enable("H5File_Test.testRead");
    //    tr.enable("H5File_Test.testWrite");
    tr.enable("H5File_Test");
    tr.reconfigure();



    m_reportTime = false; //true;
    m_numberOfRecords = 2; //512;
}


H5File_Test::~H5File_Test() {
}


void H5File_Test::setUp() {
    {
        m_v3Size = 5;
        ostringstream oss;
        oss << "vecString";
        m_v3 = vector<string>(m_v3Size, "");
        for (size_t i = 0; i < m_v3.size(); ++i) {
            oss << " " << i;
            m_v3[i] = oss.str();
        }

        m_v4Size = 6;
        m_v4 = vector<complex<float> >(m_v4Size);
        for (size_t i = 0; i < m_v4.size(); ++i) {
            m_v4[i] = complex<float>(i + 0.1, i + 0.3);
        }

    }



    {
        // used in: testBufferWrite and testBufferRead        
        m_dimsVec = Dims(2, 5);

        m_dimsVecA1 = Dims(2, 5);
        m_a1.resize(m_maxRec * m_dimsVecA1.size(), 0);
        for (size_t j = 0; j < m_maxRec * m_dimsVec.size(); ++j) {
            m_a1[j] = j % 1000000;
        }


        m_dimsVecA2 = Dims(2, 5);
        m_a2.resize(m_maxRec * m_dimsVecA2.size(), "");
        for (size_t j = 0; j < m_maxRec * m_dimsVec.size(); ++j) {
            ostringstream oss;
            oss << "[Hi " << j % 1000000 << "]";
            m_a2[j] = oss.str();
        }

        m_dimsVecA3 = Dims(2, 5);
        m_a3.resize(m_maxRec * m_dimsVecA3.size(), false);
        for (size_t j = 0; j < m_maxRec * m_dimsVec.size(); ++j) {
            if (j % 3) m_a3[j] = true;
        }

        m_dimsVecA4 = Dims(2, 5);
        m_a4.resize(m_maxRec * m_dimsVecA4.size());
        for (size_t j = 0; j < m_maxRec * m_dimsVec.size(); ++j) {
            m_a4[j] = std::complex<float>(j + 0.2, j + 0.4);
        }

        m_dimsVecA5 = Dims(2, 5);
        m_a5.resize(m_maxRec * m_dimsVecA5.size());
        for (size_t j = 0; j < m_maxRec * m_dimsVec.size(); ++j) {
            m_a5[j] = std::complex<double>(j + 0.2, j + 0.4);
        }

    }

}


void H5File_Test::tearDown() {
}


void H5File_Test::testWrite() {


    try {

        Hash data;

        data.set("instrument.a", 100);
        data.set("instrument.b", static_cast<float> (10));
        data.set("instrument.c", "abc");

        Hash::Attributes a;


        vector<bool> vAttr(5, true);
        vAttr[3] = false;
        // a.set("att1", true);
        a.set("att2", 2345);
        //  a.set("att3", vAttr); 
        //   a.set("att4", "J");
        data.setAttributes("instrument.c", a);

        data.set("instrument.d", true).setAttribute("att1", 123);
        data.set("instrument.e", static_cast<char> (58));
        complex<float> cf(14.0, 18.2);
        data.set("instrument.f", cf);

        size_t s = 4 * 32 * 256;
        //s = 1024 * 1024;
        vector<unsigned short> v0(s);
        for (size_t i = 0; i < v0.size(); ++i) {
            v0[i] = static_cast<unsigned short> (i % 16);
        }

        data.set("instrument.LPD.image", &v0[0]).setAttribute("dims", Dims(4, 32, 256).toVector());

        data.set("instrument.OTHER.image1", &v0[0]).setAttribute("dims", Dims(128, 256).toVector());

        addPointerToHash(data, "instrument.OTHER.image2", &v0[0], Dims(128, 256));




        s = 12;
        vector<double> v1(s);
        for (size_t i = 0; i < v1.size(); ++i) {
            v1[i] = i * 1.0 + 0.1234;
        }
        data.set("vectors.double", v1).setAttribute("dims", Dims(3, 4).toVector());

        s = 10;
        vector<bool> v2(s, false);
        for (size_t i = 0; i < v2.size(); ++i) {
            if (i % 2) v2[i] = true;
        }
        data.set("vectors.bool", v2).setAttribute("dims", Dims(2, 5).toVector());
        data.set("vectors.str", m_v3).setAttribute("dims", Dims(5).toVector());
        data.set("vectors.cf", m_v4).setAttribute("dims", Dims(6).toVector());



        //discover format from the Hash data        
        Format::Pointer dataFormat = Format::discover(data);




        // add additional data elements

        Hash i32el(
                   "h5path", "experimental",
                   "h5name", "test",
                   "key", "experimental.test",
                   "compressionLevel", 9
                   );

        h5::Element::Pointer e1 = h5::Element::create("INT32", i32el);

        data.set("experimental.test", 198);
        dataFormat->addElement(e1);


        data.set("abecadlo.wer", 1006u);

        Hash uel(
                 "h5path", "experimental",
                 "h5name", "test23",
                 "key", "abecadlo.wer",
                 "compressionLevel", 9
                 );
        h5::Element::Pointer e2 = h5::Element::create("UINT32", uel);
        dataFormat->addElement(e2);



        //        dataFormat->removeElement("instrument/e");
        //        dataFormat->removeElement("instrument/d");
        //        dataFormat->removeElement("instrument/c");
        //        dataFormat->removeElement("instrument/b");
        //        dataFormat->removeElement("instrument/a");
        //        dataFormat->removeElement("vectors.image");
        //        dataFormat->removeElement("vectors.bool");
        //        dataFormat->removeElement("vectors.str");


        //        clog << "config 2: " << dataFormat->getConfig() << endl;

        File file(resourcePath("file.h5"));
        file.open(File::TRUNCATE);


        Table::Pointer t = file.createTable("/abc", dataFormat);


        t->writeAttributes(data);
        for (size_t i = 0; i < m_numberOfRecords; ++i)
            t->write(data, i);

        file.close();



    } catch (karabo::util::Exception& ex) {
        clog << ex.detailedMsg() << endl;
        CPPUNIT_FAIL(ex.detailedMsg());
    }



}


void H5File_Test::testRead() {

    try {

        //Define what you want to read from file.h5 which has been written by testWrite
        // we read:
        //    /abc/experimental/test23 into Hash element data("bla")  - UINT32
        //    /abc/vectors/double  into Hash element data("db")       - VECTOR_DOUBLE
        //    /abc/instrument/d    into Hash element data("d")        - BOOL
        //    /abc/vectors/bool    into Hash element data(("a.bools") - PTR_BOOL
        //    /abc/instrument/c    into Hash element data("c")        - STRING

        Format::Pointer format = Format::createEmptyFormat();
        Hash c1(
                "h5path", "experimental",
                "h5name", "test23",
                "key", "bla"
                );

        h5::Element::Pointer e1 = h5::Element::create("UINT32", c1);
        format->addElement(e1);

        Dims dims(3, 4);
        Hash c2;
        c2.set("h5path", "vectors");
        c2.set("h5name", "double");
        c2.set("key", "db");
        c2.set("dims", dims.toVector());


        h5::Element::Pointer e2 = h5::Element::create("VECTOR_DOUBLE", c2);
        format->addElement(e2);


        Hash c3(
                "h5path", "instrument",
                "h5name", "d",
                "key", "d"
                );

        h5::Element::Pointer e3 = h5::Element::create("BOOL", c3);
        format->addElement(e3);

        Dims boolDims(2, 5);
        Hash c4(
                "h5path", "vectors",
                "h5name", "bool",
                "key", "a.bools",
                "dims", boolDims.toVector()
                );

        h5::Element::Pointer e4 = h5::Element::create("VECTOR_BOOL", c4);
        format->addElement(e4);


        Hash c5(
                "h5path", "instrument",
                "h5name", "c",
                "attributes[0].INT32.h5name", "att2"
                );

        h5::Element::Pointer e5 = h5::Element::create("STRING", c5);
        format->addElement(e5);


        Dims stringsDims(5);
        Hash c6(
                "h5path", "vectors",
                "h5name", "str",
                "key", "strings",
                "dims", stringsDims.toVector()
                );

        h5::Element::Pointer e6 = h5::Element::create("VECTOR_STRING", c6);
        format->addElement(e6);

        Dims complexFloatDims(6);
        Hash c7(
                "h5path", "vectors",
                "h5name", "cf",
                "dims", complexFloatDims.toVector()
                );

        h5::Element::Pointer e7 = h5::Element::create("VECTOR_COMPLEX_FLOAT", c7);
        format->addElement(e7);



        // End of definition of what is read



        // Open the file in READONLY mode
        File file(resourcePath("file.h5"));
        file.open(File::READONLY);


        // Define the table to be read using defined format
        Table::Pointer table = file.getTable("/abc", format);

        // The following would read entire table data
        // Table::Pointer table = file.getTable("/abc");

        // Declare container for data
        Hash data;

        // Bind some variable to hash data
        // Note that hash element "d" is not bound

        // bla will contain data from /abc/experimental/test23
        unsigned int& bla = data.bindReference<unsigned int>("bla");

        // vecd will contain data from /abc/vectors/double
        // here we are responsible for memory management
        vector<double>& vecd = data.bindReference<vector<double> >("db");
        vecd.resize(dims.size());

        // bArray will contain data from /abc/vectors/bool
        // here we are responsible for memory management
        bool bArray[boolDims.size()];
        data.set("a.bools", &bArray[0]);

        vector<string>& strings = data.bindReference<vector<string> >("strings");
        strings.resize(stringsDims.size());

        vector<complex<float> >& cf = data.bindReference<vector<std::complex<float> > >("vectors.cf");
        cf.resize(complexFloatDims.size());

        // /abc/instrument/d will be accessible as Hash element with key "d"
        // because we do not use binding
        // Memory is managed by our API


        // Now bind the data Hash to table
        table->bind(data);

        // get number of records in the file
        size_t nRecords = table->size();

        KARABO_LOG_FRAMEWORK_TRACE_CF << "number of records: " << nRecords;
        CPPUNIT_ASSERT(nRecords == m_numberOfRecords);



        table->readAttributes(data);
        // read first record
        table->read(0);

        KARABO_LOG_FRAMEWORK_TRACE_CF << "after reading: ";

        KARABO_LOG_FRAMEWORK_TRACE_CF << "READ DATA instrument\n" << data.get<Hash>("instrument");

        // assert values
        CPPUNIT_ASSERT(bla == 1006);
        CPPUNIT_ASSERT(data.get<unsigned int>("bla") == 1006);
        vector<double>& vec = data.get< vector<double> >("db");
        for (size_t i = 0; i < dims.size(); ++i) {
            CPPUNIT_ASSERT(vec[i] - (i * 1.0 + 0.1234)  < 10e-16); 
            CPPUNIT_ASSERT((i * 1.0 + 0.1234) - vec[i]  < 10e-16);
            CPPUNIT_ASSERT(vecd[i] - (i * 1.0 + 0.1234) < 10e-16);
            CPPUNIT_ASSERT((i * 1.0 + 0.1234) - vecd[i] < 10e-16);
        }

        CPPUNIT_ASSERT(data.get<bool>("d") == true);

        for (size_t i = 0; i < boolDims.size(); ++i) {
            KARABO_LOG_FRAMEWORK_TRACE_CF << "bArray[" << i << "] = " << bArray[i];
            if (i % 2) CPPUNIT_ASSERT(bArray[i] == true);
            else CPPUNIT_ASSERT(bArray[i] == false);
        }

        KARABO_LOG_FRAMEWORK_TRACE_CF << "string reference value: abc, actual value: " << data; //.get<string>("instrument.c");
        CPPUNIT_ASSERT(data.get<string>("instrument.c") == "abc");

        //vector<string>& strings = data.get<vector<string> >("strings");

        for (size_t i = 0; i < stringsDims.size(); ++i) {
            KARABO_LOG_FRAMEWORK_TRACE_CF << "strings[" << i << "] = " << strings[i];
            CPPUNIT_ASSERT(strings[i] == m_v3[i]);
        }

        for (size_t i = 0; i < complexFloatDims.size(); ++i) {
            KARABO_LOG_FRAMEWORK_TRACE_CF << "complex[" << i << "] = " << cf[i];
            CPPUNIT_ASSERT(cf[i].real() == m_v4[i].real());
            CPPUNIT_ASSERT(cf[i].imag() == m_v4[i].imag());
        }





        //read second record
        table->read(1);

        file.close();

    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL(ex.detailedMsg());
    }

}


void H5File_Test::testReadTable() {

    try {
        // Open the file in READONLY mode
        File file(resourcePath("file.h5"));
        file.open(File::READONLY);


        // Define the table to be read using defined format 
        Table::Pointer table = file.getTable("/abc");


        Format::ConstPointer fp = table->getFormat();
        h5::Element::ConstPointer imageElement = fp->getElement("instrument.LPD.image");


        Hash data, attributes;
        boost::shared_array<unsigned short> image;
        size_t arraySize = 0;


        if (imageElement->getMemoryType() == Types::VECTOR_UINT16) {
            //clog << "image is an array" << endl;
            arraySize = imageElement->getDims().size();
            image = boost::shared_array<unsigned short>(new unsigned short[arraySize]);
            data.set("instrument.LPD.image", image.get());
        }




        // Declare container for data

        table->readAttributes(attributes);
        //KARABO_LOG_FRAMEWORK_TRACE_CF << "Attributes:\n" << attributes;


        table->bind(data);
        table->read(0);


        //        size_t printSize = (arraySize < 6 ? arraySize : 6);
        //        for (size_t i = 0; i < printSize; ++i) {
        //            clog << "image[" << i << "] = " << image[i] << endl;
        //        }

        float b = data.get<float>("instrument.b");
        KARABO_LOG_FRAMEWORK_TRACE_CF << "data(\"instrument.b\"): " << b;
        CPPUNIT_ASSERT(b > 9.99999);
        CPPUNIT_ASSERT(b < 10.00001);
        CPPUNIT_ASSERT(data.get<int>("instrument.a") == 100);
        CPPUNIT_ASSERT(data.get<string>("instrument.c") == "abc");
        CPPUNIT_ASSERT(data.get<bool>("instrument.d") == true);

        //TODO  CPPUNIT_ASSERT( data.getAttribute<int>("instrument.d","att1") == 123 );
        CPPUNIT_ASSERT(data.get<char>("instrument.e") == 58);

        file.close();
    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL(ex.detailedMsg());
    }


}


void H5File_Test::testBuffer() {
    try {
        testBufferWrite();
        testBufferRead();
    } catch (...) {
        CPPUNIT_FAIL("TestBuffer failed");
    }

}


void H5File_Test::testBufferWrite() {

    try {

        Hash data;

        TimeProfiler p("VectorBufferWrite");
        p.open();
        p.startPeriod("format");
        Format::Pointer format = Format::createEmptyFormat();

        {
            Hash h1(
                    "h5path", "",
                    "h5name", "mercury"
                    );
            h5::Element::Pointer e1 = h5::Element::create("INT32", h1);
            format->addElement(e1);
        }

        {
            Hash h2(
                    "h5path", "",
                    "h5name", "venus"
                    );
            h5::Element::Pointer e2 = h5::Element::create("UINT16", h2);
            format->addElement(e2);
        }

        {
            Hash h3(
                    "h5path", "",
                    "h5name", "earth"
                    );
            h5::Element::Pointer e3 = h5::Element::create("FLOAT", h3);
            format->addElement(e3);
        }

        {
            Hash h4(
                    "h5path", "",
                    "h5name", "mars"
                    );
            h5::Element::Pointer e4 = h5::Element::create("BOOL", h4);
            format->addElement(e4);
        }

        {

            Hash h5(
                    "h5path", "",
                    "h5name", "jupiter"
                    );
            h5::Element::Pointer e5 = h5::Element::create("STRING", h5);
            format->addElement(e5);
        }

        {
            Hash h1(
                    "h5path", "",
                    "h5name", "neptun"
                    );
            h5::Element::Pointer e1 = h5::Element::create("COMPLEX_FLOAT", h1);
            format->addElement(e1);
        }

        {
            Hash h1(
                    "h5path", "vector",
                    "h5name", "a1",
                    "dims", m_dimsVecA1.toVector()
                    );
            h5::Element::Pointer e1 = h5::Element::create("VECTOR_INT32", h1);
            format->addElement(e1);
        }

        {
            Hash h1(
                    "h5path", "vector",
                    "h5name", "a2",
                    "dims", m_dimsVecA2.toVector()
                    );
            h5::Element::Pointer e1 = h5::Element::create("VECTOR_STRING", h1);
            format->addElement(e1);
        }

        {
            Hash h1(
                    "h5path", "vector",
                    "h5name", "a3",
                    "dims", m_dimsVecA3.toVector()
                    );
            h5::Element::Pointer e1 = h5::Element::create("VECTOR_BOOL", h1);
            format->addElement(e1);
        }

        {
            Hash h1(
                    "h5path", "vector",
                    "h5name", "a4",
                    "dims", m_dimsVecA4.toVector()
                    );
            h5::Element::Pointer e1 = h5::Element::create("VECTOR_COMPLEX_FLOAT", h1);
            format->addElement(e1);
        }

        {
            Hash h1(
                    "h5path", "vector",
                    "h5name", "a5",
                    "dims", m_dimsVecA5.toVector()
                    );
            h5::Element::Pointer e1 = h5::Element::create("VECTOR_COMPLEX_DOUBLE", h1);
            format->addElement(e1);
        }


        p.stopPeriod("format");


        //data.set("vectors.image", v0).setAttribute("dims", Dims(1024, 1024).toVector());

        p.startPeriod("initialize");

        //size_t maxIterations = 100;
        size_t maxIterations = 2;

        //TODO: update size for vectors
        unsigned long long totalSize = maxIterations * m_maxRec * (4 + 2 + 4 + 1 + 12) / 1000000;

        vector<int> mercury(m_maxRec, 1);
        vector<unsigned short> venus(m_maxRec, 2);
        vector<float> earth(m_maxRec, 3);
        bool mars[m_maxRec];
        vector<std::string> jupiter(m_maxRec, "Hello 000000");
        vector<std::complex<float> > neptun(m_maxRec);


        for (size_t i = 0; i < m_maxRec; ++i) {
            mercury[i] = i + 1000;
            venus[i] = i;
            earth[i] = i * 2.5;

            mars[i] = true;
            if (i % 2) mars[i] = false;

            ostringstream oss;
            oss << "Hello " << std::setw(6) << std::setfill('0') << i;
            jupiter[i] = oss.str();

            neptun[i] = complex<float>(i + 0.1, i + 0.2);
        }


        p.stopPeriod("initialize");
        p.startPeriod("create");


        string filename = "/dev/shm/file3.h5";
        filename = resourcePath("file3.h5");
        File file(filename);
        file.open(File::TRUNCATE);

        Table::Pointer t = file.createTable("/planets", format);



        bool exists = file.hasTable("/planets");
        //cerr << "/planets " << exists << endl;
        CPPUNIT_ASSERT(exists == true);
                        
        exists = file.hasTable("/planet");
        //cerr << "/planet " << exists << endl;
        CPPUNIT_ASSERT(exists == false);
                       
        exists = file.hasTable("planets");
        //cerr << "planets " << exists << endl;
        CPPUNIT_ASSERT(exists == false);
                

        p.stopPeriod("create");
        p.startPeriod("write0");

        int i = 0, l = 23;
        data.set("mercury", &mercury[i]);
        data.set("venus", &venus[i]);
        data.set("earth", &earth[i]);
        //we cannot use vector of bool this way
        data.set("mars", &mars[i]);
        data.set("jupiter", &jupiter[i]);
        data.set("neptun", &neptun[i]);
        data.set("vector.a1", m_a1);
        data.set("vector.a2", m_a2);
        data.set("vector.a3", m_a3);
        data.set("vector.a4", m_a4);
        data.set("vector.a5", m_a5);
        t->write(data, i, l);

        i = i + l;
        l = 5;
        data.set("mercury", &mercury[i]);
        data.set("venus", &venus[i]);
        data.set("earth", &earth[i]);
        data.set("mars", &mars[i]);
        data.set("neptun", &neptun[i]);
        t->write(data, i, l);

        // 5 records are intentionally skipped
        // should appear in file with zeros ( recordId=28, len=5)
        i = i + l + 5;
        l = 6;
        data.set("mercury", &mercury[i]);
        data.set("venus", &venus[i]);
        data.set("earth", &earth[i]);
        data.set("mars", &mars[i]);
        data.set("neptun", &neptun[i]);
        t->write(data, i, l);


        i = i + l;
        l = 3;
        data.set("mercury", &mercury[i]);
        data.set("venus", &venus[i]);
        data.set("earth", &earth[i]);
        data.set("mars", &mars[i]);
        data.set("neptun", &neptun[i]);
        t->write(data, i, l);

        i = i + l;
        l = 8;
        data.set("mercury", &mercury[i]);
        data.set("venus", &venus[i]);
        data.set("earth", &earth[i]);
        data.set("mars", &mars[i]);
        data.set("neptun", &neptun[i]);
        t->write(data, i, l);

        i = i + l;
        l = 25;
        data.set("mercury", &mercury[i]);
        data.set("venus", &venus[i]);
        data.set("earth", &earth[i]);
        data.set("mars", &mars[i]);
        data.set("neptun", &neptun[i]);
        t->write(data, i, l);


        i = i + l;
        l = 5;
        data.set("mercury", &mercury[i]);
        data.set("venus", &venus[i]);
        data.set("earth", &earth[i]);
        data.set("mars", &mars[i]);
        data.set("neptun", &neptun[i]);
        t->write(data, i, l);

        //    for (int j = 0; j < 4; ++j) {
        //        data.set("mercury", mercury[i+j]);
        //        data.set("venus", venus[i+j]);
        //        data.set("earth", earth[i+j]);
        //        t->write(data, i+j);
        //    }

        i = i + l;
        l = 19;
        data.set("mercury", &mercury[i]);
        data.set("venus", &venus[i]);
        data.set("earth", &earth[i]);
        data.set("mars", &mars[i]);
        data.set("neptun", &neptun[i]);
        t->write(data, i, l);

        i = i + l;
        l = 1;
        KARABO_LOG_FRAMEWORK_TRACE_CF << " i=" << i << " l=" << l;
        data.set("mercury", &mercury[i]);
        data.set("venus", &venus[i]);
        data.set("earth", &earth[i]);
        data.set("mars", &mars[i]);
        data.set("neptun", &neptun[i]);
        t->write(data, i, l);


        p.stopPeriod("write0");
        p.startPeriod("write");


        data.set("mercury", &mercury[0]);
        data.set("venus", &venus[0]);
        data.set("earth", &earth[0]);
        data.set("mars", &mars[0]);
        data.set("jupiter", &jupiter[0]);
        data.set("neptun", &neptun[0]);
        data.set("vector.a1", m_a1);
        data.set("vector.a2", m_a2);
        data.set("vector.a3", m_a3);
        data.set("vector.a4", m_a4);

        for (size_t j = 0; j < maxIterations; ++j) {
            t->write(data, m_maxRec*j, m_maxRec);
        }
        p.stopPeriod("write");

        p.startPeriod("close");
        file.close();
        p.stopPeriod("close");
        p.close();
        TimeDuration formatTime = p.getPeriod("format").getDuration();
        TimeDuration initializeTime = p.getPeriod("initialize").getDuration();
        TimeDuration createTime = p.getPeriod("create").getDuration();
        TimeDuration writeTime = p.getPeriod("write").getDuration();
        TimeDuration closeTime = p.getPeriod("close").getDuration();

        if (m_reportTime) {
            clog << endl;
            clog << "file: " << filename << endl;
            clog << "initialize data                  : " << initializeTime << " [s]" << endl;
            clog << "format                           : " << formatTime << " [s]" << endl;
            clog << "open/prepare file                : " << createTime << " [s]" << endl;
            clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
            clog << "written data size                : " << totalSize << " [MB]" << endl;
            clog << "writing speed                    : " << totalSize / double(writeTime) << " [MB/s]" << endl; //TODO
            clog << "close                            : " << closeTime << " [s]" << endl;
            clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
            clog << "write+close(flush to disk) speed : " << totalSize / double(writeTime + closeTime) << " [MB/s]" << endl; //TODO
        }


    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL(ex.detailedMsg());
    }

    m_testBufferWriteSuccess = true;
}


void H5File_Test::testBufferRead() {

    if (!m_testBufferWriteSuccess) {
        CPPUNIT_FAIL("Test testBufferRead not run due to a failure in testBufferWrite");
    }

    try {


        //TimeProfiler p("VectorBufferRead");

        //p.startPeriod("format");
        //        Format::Pointer format = Format::createEmptyFormat();
        //        {
        //            Hash h1(
        //                    "h5path", "",
        //                    "h5name", "mercury"
        //                    );
        //            h5::Element::Pointer e1 = h5::Element::create("INT32", h1);
        //            format->addElement(e1);
        //        }
        //
        //        {
        //            Hash h1(
        //                    "h5path", "vector",
        //                    "h5name", "a1",
        //                    "dims", m_dimsVecA1.toVector()
        //                    );
        //            h5::Element::Pointer e1 = h5::Element::create("VECTOR_INT32", h1);
        //            format->addElement(e1);
        //        }
        //
        //        {
        //            Hash h1(
        //                    "h5path", "vector",
        //                    "h5name", "a2",
        //                    "dims", m_dimsVecA2.toVector()
        //                    );
        //            h5::Element::Pointer e1 = h5::Element::create("VECTOR_STRING", h1);
        //            format->addElement(e1);
        //        }
        //
        //        {
        //            Hash h1(
        //                    "h5path", "vector",
        //                    "h5name", "a3",
        //                    "dims", m_dimsVecA3.toVector()
        //                    );
        //            h5::Element::Pointer e1 = h5::Element::create("VECTOR_BOOL", h1);
        //            format->addElement(e1);
        //        }
        //        
        //        {
        //            Hash h1(
        //                    "h5path", "vector",
        //                    "h5name", "a4",
        //                    "dims", m_dimsVecA4.toVector()
        //                    );
        //            h5::Element::Pointer e1 = h5::Element::create("VECTOR_COMPLEX_FLOAT", h1);
        //            format->addElement(e1);
        //        }
        //

        string filename = "/dev/shm/file3.h5";
        filename = resourcePath("file3.h5");
        File file(filename);
        file.open(File::READONLY);

        Table::Pointer t = file.getTable("/planets");

        Hash data;
        const size_t bufLen = 100;

        vector<int>& mercuryBuffer = data.bindReference<vector<int> >("mercury");
        mercuryBuffer.resize(bufLen);



        t->bind(data, bufLen);

        vector<int>& a1 = data.get< vector<int> >("vector.a1");
        vector<string>& a2 = data.get< vector<string> >("vector.a2");
        vector<bool>& a3 = data.get < vector<bool> >("vector.a3");
        vector<complex<float> >& a4 = data.get < vector<complex<float> > >("vector.a4");
        vector<complex<double> >& a5 = data.get < vector<complex<double> > >("vector.a5");

        size_t numReadRecords = t->read(0, bufLen);

        for (size_t i = 0; i < mercuryBuffer.size(); ++i) {
            KARABO_LOG_FRAMEWORK_TRACE_CF << "mercuryBuffer[" << i << "] = " << mercuryBuffer[i];
        }

        size_t m = 0;
        for (size_t j = 0; j < numReadRecords; ++j) {
            KARABO_LOG_FRAMEWORK_TRACE_CF << "{" << j << "}: ";
            for (size_t k = 0; k < m_dimsVecA1.extentIn(0); ++k) {
                for (size_t l = 0; l < m_dimsVecA1.extentIn(1); ++l) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "a1: [" << k << "," << l << "]=" << a1[m];
                    CPPUNIT_ASSERT(a1[m] == m_a1[m]);
                    m++;
                }
            }
            m = 0;
            for (size_t k = 0; k < m_dimsVecA2.extentIn(0); ++k) {
                for (size_t l = 0; l < m_dimsVecA2.extentIn(1); ++l) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "a2: [" << k << "," << l << "]=" << a2[m];
                    CPPUNIT_ASSERT(a2[m] == m_a2[m]);
                    m++;
                }
            }
            m = 0;
            for (size_t k = 0; k < m_dimsVecA3.extentIn(0); ++k) {
                for (size_t l = 0; l < m_dimsVecA3.extentIn(1); ++l) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "a3: [" << k << "," << l << "]=" << a3[m];
                    CPPUNIT_ASSERT(a3[m] == m_a3[m]);
                    m++;
                }
            }
            m = 0;
            for (size_t k = 0; k < m_dimsVecA4.extentIn(0); ++k) {
                for (size_t l = 0; l < m_dimsVecA4.extentIn(1); ++l) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "a4: [" << k << "," << l << "]=" << a4[m];
                    CPPUNIT_ASSERT(a4[m] == m_a4[m]);
                    m++;
                }
            }
            m = 0;
            for (size_t k = 0; k < m_dimsVecA5.extentIn(0); ++k) {
                for (size_t l = 0; l < m_dimsVecA5.extentIn(1); ++l) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "a5: [" << k << "," << l << "]=" << a5[m];
                    CPPUNIT_ASSERT(a5[m] == m_a5[m]);
                    m++;
                }
            }
        }


        file.close();
    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_ASSERT(true == false);
    }

}


void H5File_Test::testVectorOfHashes() {


    Hash data;
    vector<Hash> data1(3);
    {
        Hash h1;
        h1.set("ha1", 10);
        h1.set("ha2", "abc");
        h1.set("ha3", 10.2);
        data1[0] = h1;
    }

    {
        Hash h1;
        h1.set("hb1", 10.1);
        h1.set("hb2", 102);
        h1.set("hb3", 103);
        data1[1] = h1;
    }

    {
        Hash h1;
        h1.set("hc1", 190u);
        h1.set("hc2", 191uLL);
        h1.set("hc3", 192LL);
        data1[2] = h1;
    }



    data.set("x.y", Hash());
    data.set("c", Hash());
    data.set("a.vector", data1);
    data.set("x.y.z.a.m", 20);
    data.set("b.v[0].p", "Hello");
    data.set("b.v[0].q", 1000);
    data.set("b.v[1].q", 1000);
    data.set("b.v[2].q", 1000);
    data.set("b.v[3].q", 1000);
    data.set("b.v[4].q", 1000);
    data.set("b.v[5].q", 1000);
    data.set("b.v[6].q", 1000);
    data.set("b.v[7].q", 1000);
    data.set("b.v[8].q", 1000);
    data.set("b.v[9].q", 1000);
    data.set("b.v[10].q", 1000);
    data.set("b.v[11].q", 1000);
    data.set("c.d.e", Hash());

    //    Hash h2("vh1", data1);
    //    data.set("hvh", h2);

    //    int a = data.get<int>("vector[0].ha1");
    //    clog << "a=" << a << endl;



    try {

        Format::Pointer dataFormat = Format::discover(data);
        File file(resourcePath("file4.h5"));
        file.open(File::TRUNCATE);

        Table::Pointer t = file.createTable("/abc", dataFormat);
        for (size_t i = 0; i < m_numberOfRecords; ++i) {
            t->write(data, i);
        }

        Hash rdata;

        t->bind(rdata);

        t->read(0);

        file.close();

        // compare original and read Hash using xml serializer
        // this way we depend on the properly functional XML serializer but we can easily
        // compare two Hash'es including order of elements
        // This test may fail if XML serializer test fails

        Hash c("Xml.indentation", 1, "Xml.writeDataTypes", false);
        TextSerializer<Hash>::Pointer s = TextSerializer<Hash>::create(c);
        string sdata, srdata;
        s->save(data, sdata);
        s->save(rdata, srdata);

        //        clog << " data\n" << sdata << endl;
        //        clog << "rdata\n" << srdata << endl;
        CPPUNIT_ASSERT(sdata == srdata);


    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL(ex.detailedMsg());
    }

}


void H5File_Test::testWriteFailure() {

    try {

        Hash data;

        data.set("instrument.a", 100);
        data.set("instrument.b", static_cast<float> (10));
        data.set("instrument.c", "abc");

        File file(resourcePath("fileFail.h5"));
        file.open(File::TRUNCATE);


        //discover format from the Hash data
        Format::Pointer dataFormat = Format::discover(data);


        Table::Pointer t = file.createTable("/base", dataFormat);


        //        t->writeAttributes(data);

        t->write(data, 0);
        t->write(data, 1);
        //force error
        data.erase("instrument.b");

        t->write(data, 2);
        data.set("instrument.b", static_cast<float> (22));
        t->write(data, 3);
        file.close();


    } catch (Exception& ex) {
        CPPUNIT_FAIL("Error");
    }
}


void H5File_Test::testManyTables() {
    TimeProfiler p("ManyTables");
    p.open();

    try {

        Hash data, d1, d2, d3, d4;


        int n = 5;
        //        int rec = 100;

        //        float totalSize = rec * (4 + 4 + 8 + 2) * n / 1024;
        for (int i = 0; i < n; ++i) {
            data.set(toString(i), i);
            data.set("id" + toString(i), (unsigned char) 0);
        }



        p.startPeriod("format");
        Format::Pointer dataFormat = Format::discover(data);
        Hash h("h5name", "3");
        h5::Element::Pointer e = h5::Element::create("VLARRAY_INT32", h);
        dataFormat->replaceElement("3", e);
        p.stopPeriod("format");

        //        for (int i = 0; i < n; ++i) {
        //            d1.set(toString(i), vector<int>(rec, i));
        //            d2.set(toString(i), vector<float> (rec, i));
        //            d3.set(toString(i), vector<double> (rec, i));
        //            d4.set(toString(i), vector<unsigned short> (rec, i));
        //        }
        //
        //        data.set("d1", d1);
        //        data.set("d2", d2);
        //        data.set("d3", d3);
        //        data.set("d4", d4);
        //
        //
        string filename = "/dev/shm/fileManyTables.h5";
        filename = resourcePath("fileManyTables.h5");
        File file(filename);
        file.open(File::TRUNCATE);

        p.startPeriod("create");


        Table::Pointer t = file.createTable("/monitor/motor1", dataFormat);

        data.set("3", 234);

        //        clog << "File structure is created" << endl;
        p.stopPeriod("create");
        //
        //        //        t->writeAttributes(data);
        p.startPeriod("write");
        h5::Element::Pointer e3 = t->getFormat()->getElement("3");
        h5::Element::Pointer eid3 = t->getFormat()->getElement("id3");

        vector<int> v(2, 0);
        v[0] = 234;
        v[1] = 88;
        data.clear();
        data.set("3", &v[0]).setAttribute("size", 2);
        e3->write(data, 0);
        unsigned char id3 = 0;
        id3 = id3 | 1; // rec 0 -> 2^0 = 1
        id3 = id3 | 8; // rec 3 -> 2^3 =   8
        //        clog << "id3: " << oct << (int) id3 << endl;
        data.set("id3", id3);
        eid3->write(data, 0);


        //        //        for (int i = 0; i < rec; ++i) {
        //        //            t->write(data, i);
        //        //        }
        //
        //        int m = 1;
        //        for (int i = 0; i < m; ++i) {
        //            t->write(data, i*rec, rec);
        //        }
        //        totalSize *= m;
        //
        p.stopPeriod("write");

        p.startPeriod("close");
        file.close();
        p.stopPeriod("close");
        p.close();
        TimeDuration formatTime = p.getPeriod("format").getDuration();
        //        TimeDuration discoverTime = p.getPeriod("discover").getDuration();
        TimeDuration createTime = p.getPeriod("create").getDuration();
        TimeDuration writeTime = p.getPeriod("write").getDuration();
        TimeDuration closeTime = p.getPeriod("close").getDuration();


        if (false) {
            clog << endl;
            clog << "file: " << filename << endl;
            clog << "format                           : " << formatTime << " [s]" << endl;
            clog << "open/prepare file                : " << createTime << " [s]" << endl;
            clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
            //            clog << "written data size                : " << totalSize << " [kB]" << endl;
            //            clog << "writing speed                    : " << totalSize / double(writeTime) << " [kB/s]" << endl;
            clog << "close                            : " << closeTime << " [s]" << endl;
            clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
            //            clog << "write+close(flush to disk) speed : " << totalSize / double(writeTime + closeTime) << " [kB/s]" << endl;
        }


    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Error");
    }

}


void H5File_Test::testManyGroups() {
    TimeProfiler p("ManyGroups");
    p.open();
    Format::createEmptyFormat();

    KARABO_LOG_FRAMEWORK_TRACE_CF << "start ManyGroups";
    try {

        Hash d1, d2, d3, d4;

        int n = 25; //00; //10000; //0; //5000; //500;//5000;//20000;//25000;
        size_t rec = 100;
        int m = 1;
        float totalSize = rec * (4 + 4 + 8 + 2) * n / 1024;
        size_t num_prop = n * 4;
        size_t num_rec = 0;
        bool attr = true;
        for (int i = 0; i < n; ++i /*i+=2*/) {

            d1.set(toString(i), i);
            //            d1.set(toString(i+1), static_cast<unsigned long long>(i));

            d2.set(toString(i), static_cast<float> (i));
            //            d2.set(toString(i+1), static_cast<string>(toString(i)));

            d3.set(toString(i), static_cast<double> (i));
            //            d3.set(toString(i+1), static_cast<unsigned int> (i));

            d4.set(toString(i), static_cast<unsigned short> (i));
            //            d4.set(toString(i+1), static_cast<vector<unsigned short> > (1000000)).
            //            setAttribute("dims", Dims(10,100,2000,5).toVector());
            if (attr) {
                d1.setAttribute(toString(i), "unit", 2);
                //d1.setAttribute(toString(i), "a", 234);
                d2.setAttribute(toString(i), "unit", 3);
                //d2.setAttribute(toString(i), "a", 235);
                d3.setAttribute(toString(i), "unit", 4);
                //d3.setAttribute(toString(i), "a", 236);
                d4.setAttribute(toString(i), "unit", 5);
                // d4.setAttribute(toString(i), "a", 237);
            }
        }

        //        vector<unsigned char>& status = d1.bindReference<vector<unsigned char> >("status");0
        //        status.resize(n*200, 1);
        //        d1.setAttribute("status","dims",Dims(200,n).toVector());


        p.startPeriod("format");

        FormatDiscoveryPolicy::Pointer policy = FormatDiscoveryPolicy::create("Policy", Hash("chunkSize", static_cast<unsigned long long> (rec), "compressionLevel", 9));
        Format::Pointer dataFormat1 = Format::discover(d1, policy);
        Format::Pointer dataFormat2 = Format::discover(d2, policy);
        Format::Pointer dataFormat3 = Format::discover(d3, policy);
        Format::Pointer dataFormat4 = Format::discover(d4, policy);

        {
            Hash h(
                   "h5path", "c5",
                   "h5name", "test"
                   );

            h5::Element::Pointer e = h5::Element::create("INT32", h);
            dataFormat1->addElement(e);
        }
        //        {
        //            Hash h(
        //                   "h5path", "c5",
        //                   "h5name", "test"
        //                   );
        //
        //            h5::Element::Pointer e = h5::Element::create("UINT64ATTR", h);
        //            dataFormat1->addElement(e);
        //        }

        p.stopPeriod("format");


        string filename = "/dev/shm/fileManyGroups.h5";
        filename = resourcePath("fileManyGroups.h5");
        File file(filename);
        file.open(File::TRUNCATE);
        KARABO_LOG_FRAMEWORK_TRACE_CF << "File is open";


        p.startPeriod("create");
        Table::Pointer t1 = file.createTable("/base/c1", dataFormat1);
        //        file.reportOpenObjects();
        Table::Pointer t2 = file.createTable("/base/c2", dataFormat2);
        //        file.reportOpenObjects();
        Table::Pointer t3 = file.createTable("/base/c3", dataFormat3);
        //        file.reportOpenObjects();
        Table::Pointer t4 = file.createTable("/base/c4", dataFormat4);
        //        file.reportOpenObjects();
        KARABO_LOG_FRAMEWORK_TRACE_CF << "File structure is created";
        p.stopPeriod("create");

        p.startPeriod("attribute");
        if (attr) {
            //            clog << "write attributes" << endl;
            //            file.reportOpenObjects();
            p.startPeriod("attribute1");
            t1->writeAttributes(d1);
            p.stopPeriod("attribute1");
            //            double attTime1 = HighResolutionTimer::time2double(p.getTime("attribute1"));
            //            clog << "t1: " << attTime1 << endl;
            //            file.reportOpenObjects();
            p.startPeriod("attribute2");
            t2->writeAttributes(d2);
            p.stopPeriod("attribute2");
            //            double attTime2 = HighResolutionTimer::time2double(p.getTime("attribute2"));
            //            clog << "t2: " << attTime2 << endl;
            //            file.reportOpenObjects();
            p.startPeriod("attribute3");
            t3->writeAttributes(d3);
            p.stopPeriod("attribute3");
            //            double attTime3 = HighResolutionTimer::time2double(p.getTime("attribute3"));
            //            clog << "t3: " << attTime3 << endl;
            //            file.reportOpenObjects();
            p.startPeriod("attribute4");
            t4->writeAttributes(d4);
            p.stopPeriod("attribute4");
            //            double attTime4 = HighResolutionTimer::time2double(p.getTime("attribute4"));
            //            clog << "t4: " << attTime4 << endl;
            //            file.reportOpenObjects();
            //            clog << "Attributes have been written" << endl;
        }
        p.stopPeriod("attribute");
        #define WRITE
        #ifdef WRITE
        for (int i = 0; i < n; ++i) {
            vector<int>& v1 = d1.bindReference< vector<int> >(toString(i));
            v1.resize(rec, i);
            for (size_t k = 0; k < v1.size(); ++k) {
                v1[k] = k;
            }
            vector<float>& v2 = d2.bindReference < vector<float> >(toString(i));
            v2.resize(rec, i);
            vector<double>& v3 = d3.bindReference < vector<double> >(toString(i));
            v3.resize(rec, i);
            vector<unsigned short>& v4 = d4.bindReference < vector<unsigned short> >(toString(i));
            v4.resize(rec, i);
        }

        vector<int>& v5 = d1.bindReference< vector<int> >("c5.test");
        v5.resize(rec, 8);
        d1.getNode("c5.test").setAttribute("aa", vector<unsigned long long>());
        vector<unsigned long long>& a5 = d1.getNode("c5.test").getAttribute< vector<unsigned long long> >("aa");
        a5.resize(rec, 23);

        //            vector<unsigned char>& status1 = d1.bindReference<vector<unsigned char> >("status");
        //            status1.resize(200*n*rec, 1);




        p.startPeriod("write");

        //        int m = 10; 
        //        for (int i = 0; i < m*rec; ++i) {
        //            t1->write(d1, i);
        //            t2->write(d2, i);
        //            t3->write(d3, i);
        //            t4->write(d4, i);
        //        }


        num_rec = m*rec;
        for (int i = 0; i < m; ++i) {
            for (size_t j = 0; j < static_cast<size_t> (n); ++j) {
                vector<int>& v1 = d1.get< vector<int> >(toString(j));
                for (size_t k = 0; k < v1.size(); ++k) {
                    v1[k] = k + j;
                }
            }

            t1->write(d1, i*rec, rec);
            t2->write(d2, i*rec, rec);
            t3->write(d3, i*rec, rec);
            t4->write(d4, i*rec, rec);
        }
        totalSize *= m;

        p.stopPeriod("write");
        //        clog << "---report before closing--" << endl;
        //        file.reportOpenObjects();
        p.startPeriod("close");
        file.closeTable(t1);
        p.stopPeriod("close");
        //        clog << "-----" << endl;
        //        file.reportOpenObjects();
        p.startPeriod("close");
        file.closeTable(t2);
        p.stopPeriod("close");
        //        clog << "-----" << endl;
        //        file.reportOpenObjects();
        p.startPeriod("close");
        file.closeTable(t3);
        p.stopPeriod("close");
        //        clog << "-----" << endl;
        //        file.reportOpenObjects();
        p.startPeriod("close");
        file.closeTable(t4);
        p.stopPeriod("close");
        //        clog << "-----" << endl;
        //        file.reportOpenObjects();
        p.startPeriod("close");
        file.close();
        p.stopPeriod("close");

        #endif

        #define READ 1
        #ifdef READ
        {
            file.open(File::READONLY);
            //clog << "a" << endl;
            p.startPeriod("open");
            Table::Pointer t1 = file.getTable("/base/c1");
            Table::Pointer t2 = file.getTable("/base/c2");
            Table::Pointer t3 = file.getTable("/base/c3");
            Table::Pointer t4 = file.getTable("/base/c4");
            KARABO_LOG_FRAMEWORK_TRACE_CF << "File structure is open";
            p.stopPeriod("open");
            //clog << "a" << endl;
            Hash rd1, rd2, rd3, rd4;
            p.startPeriod("bind");
            t1->bind(rd1, rec);
            t2->bind(rd2, rec);
            t3->bind(rd3, rec);
            t4->bind(rd4, rec);
            p.stopPeriod("bind");


            p.startPeriod("readAttr");
            Hash a1, a2, a3, a4;
            //            t1->readAttributes(a1);
            //            t2->readAttributes(a2);
            //            t3->readAttributes(a3);
            //            t4->readAttributes(a4);            
            p.stopPeriod("readAttr");

            p.startPeriod("read");
            t1->read(0, rec);
            t2->read(0, rec);
            t3->read(0, rec);
            t4->read(0, rec);
            p.stopPeriod("read");



            //            clog << "---report before closing--" << endl;
            //            file.reportOpenObjects();
            p.startPeriod("close1");
            file.closeTable(t1);
            p.stopPeriod("close1");
            //            clog << "-----" << endl;
            //            file.reportOpenObjects();
            p.startPeriod("close1");
            file.closeTable(t2);
            p.stopPeriod("close1");
            //            clog << "-----" << endl;
            //            file.reportOpenObjects();
            p.startPeriod("close1");
            file.closeTable(t3);
            p.stopPeriod("close1");
            //            clog << "-----" << endl;
            //            file.reportOpenObjects();
            p.startPeriod("close1");
            file.closeTable(t4);
            p.stopPeriod("close1");
            //            clog << "-----" << endl;
            //            file.reportOpenObjects();
            p.startPeriod("close1");
            file.close();
            p.stopPeriod("close1");



            for (int i = 0; i < n; ++i) {
                vector<int>& v1 = d1.get<vector<int> >(toString(i));
                vector<float>& v2 = d2.get<vector<float> >(toString(i));
                vector<double>& v3 = d3.get<vector<double> >(toString(i));
                vector<unsigned short>& v4 = d4.get<vector<unsigned short> >(toString(i));

                vector<int>& rv1 = rd1.get<vector<int> >(toString(i));
                vector<float>& rv2 = rd2.get<vector<float> >(toString(i));
                vector<double>& rv3 = rd3.get<vector<double> >(toString(i));
                vector<unsigned short>& rv4 = rd4.get<vector<unsigned short> >(toString(i));
                CPPUNIT_ASSERT(v1.size() == rec);
                CPPUNIT_ASSERT(v2.size() == rec);
                CPPUNIT_ASSERT(v3.size() == rec);
                CPPUNIT_ASSERT(v4.size() == rec);
                CPPUNIT_ASSERT(rv1.size() == rec);
                CPPUNIT_ASSERT(rv2.size() == rec);
                CPPUNIT_ASSERT(rv3.size() == rec);
                CPPUNIT_ASSERT(rv4.size() == rec);
                for (size_t k = 0; k < rec; ++k) {
                    CPPUNIT_ASSERT(rv1[k] == v1[k]);
                    CPPUNIT_ASSERT(rv2[k] == v2[k]);
                    CPPUNIT_ASSERT(rv3[k] == v3[k]);
                    CPPUNIT_ASSERT(rv4[k] == v4[k]);
                }
            }
        }
        #endif
        p.close();
        TimeDuration formatTime = p.getPeriod("format").getDuration();
        TimeDuration createTime = p.getPeriod("create").getDuration();
        TimeDuration attributeTime = p.getPeriod("attribute").getDuration();
        TimeDuration writeTime = p.getPeriod("write").getDuration();
        TimeDuration closeTime = p.getPeriod("close").getDuration();
        TimeDuration openTime = p.getPeriod("open").getDuration();
        TimeDuration bindTime = p.getPeriod("bind").getDuration();
        TimeDuration readTime = p.getPeriod("read").getDuration();
        TimeDuration readAttrTime = p.getPeriod("readAttr").getDuration();
        TimeDuration close1Time = p.getPeriod("close1").getDuration();

        if (false) {
            clog << endl;
            clog << "file: " << filename << endl;
            clog << "number of properties             : " << num_prop << endl;
            clog << "number of records                : " << num_rec << endl;
            clog << "format                           : " << formatTime << " [s]" << endl;
            clog << "open/prepare file                : " << createTime << " [s]" << endl;
            clog << "write attributes                 : " << attributeTime << " [s]" << endl;
            clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
            clog << "written data size                : " << totalSize << " [kB]" << endl;
            clog << "writing speed                    : " << totalSize / double(writeTime) << " [kB/s]" << endl; //TODO
            clog << "close                            : " << closeTime << " [s]" << endl;
            clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
            clog << "write+close(flush to disk) speed : " << totalSize / double(writeTime + closeTime) << " [kB/s]" << endl; //TODO
            clog << "Total write time                 : " << formatTime + createTime + attributeTime + writeTime + closeTime << " [s]" << endl;
            clog << "open for reading                 : " << openTime << " [s]" << endl;
            clog << "bind                             : " << bindTime << " [s]" << endl;
            clog << "read                             : " << readTime << " [s]" << endl;
            clog << "read attributes                  : " << readAttrTime << " [s]" << endl;
            clog << "close reading                    : " << close1Time << " [s]" << endl;
            clog << "Total (open/bind/read/close) time: " << openTime + bindTime + readTime + close1Time << " [s]" << endl;
        }


    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Error");
    }

}


void H5File_Test::testVLWrite() {

    TimeProfiler p("VLWrite");
    p.open();
    Hash data;
    try {


        Format::Pointer dataFormat = Format::createEmptyFormat();

        Hash h1(
                "h5path", "experimental",
                "h5name", "test"
                );

        h5::Element::Pointer e1 = h5::Element::create("VLARRAY_FLOAT", h1);
        dataFormat->addElement(e1);

        string filename = "/dev/shm/fileVL.h5";
        filename = resourcePath("fileVL.h5");
        File file(filename);
        file.open(File::TRUNCATE);



        p.startPeriod("create");
        Table::Pointer t = file.createTable("/base", dataFormat);
        p.stopPeriod("create");

        //        t->writeAttributes(data);
        p.startPeriod("write");
        //        for (int i = 0; i < rec; ++i) {

        {
            vector<float> v(20, 2);
            for (size_t i = 0; i < 20; ++i) {
                v[i] = i;
            }
            data.set("experimental.test", v).setAttribute("size", 20);
            t->write(data, 0);
        }
        {
            vector<float> v(3, 8);
            data.set("experimental.test", &v[0]).setAttribute("size", 3);
            t->write(data, 1);
        }
        {

            vector<float> v(90, 24);
            data.set("experimental.test", v).setAttribute("size", Dims(10, 30, 50).toVector());
            t->write(data, 2, 3);
        }

        {
            Hash rdata, rbdata;
            vector<float>& r1 = rdata.bindReference<vector<float> >("experimental.test");

            {
                t->bind(rdata);

                t->read(0);
                vector<float>& a = rdata.get<vector<float> >("experimental.test");
                clog << "size: " << a.size() << endl;
                clog << "rdata:\n" << rdata << endl;

                t->bind(rbdata, 2);
                t->read(1, 2);
                const vector<unsigned long long>& sizes = rbdata.getNode("experimental.test").getAttribute<vector<unsigned long long> >("size");
                clog << "size (0): " << sizes[0] << endl;
                clog << "size (1): " << sizes[1] << endl;

                //vector<float>& a = rdata.get<vector<float> >("experimental.test");
                clog << "rdata:\n" << rbdata << endl;

                t->bind(rdata);
                t->read(3);
                clog << "size: " << r1.size() << endl;
                clog << "rdata:\n" << rdata << endl;

                t->read(4);
                clog << "size: " << r1.size() << endl;
                clog << "rdata:\n" << rdata << endl;

            }
        }
        p.stopPeriod("write");

        p.startPeriod("close");
        file.close();
        p.stopPeriod("close");
        p.close();

    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Error");
    }

}


void H5File_Test::testTrainFormat() {


    try {
        size_t bufLen = 1024 * 1024 * 1024;
        vector<char> buffer(bufLen, 0);


        Hash dataset;

        // dataset
        dataset.set("majorTrainFormatVersion", static_cast<unsigned char> (1));
        dataset.set("minorTrainFormatVersion", static_cast<unsigned char> (0));
        dataset.set("moduleWidth", static_cast<unsigned short> (512));
        dataset.set("moduleHeight", static_cast<unsigned short> (128));
        dataset.set("numModules", static_cast<unsigned short> (16));
        dataset.set("checksumType", static_cast<unsigned char> (0));
        dataset.set("checksumSize", static_cast<unsigned char> (4));
        dataset.set("encoding", static_cast<unsigned char> (1));
        dataset.set("detectorDataSize", static_cast<unsigned short> (4 * 1024));



        unsigned short numModules = dataset.get<unsigned short>("numModules");
        unsigned short moduleWidth = dataset.get<unsigned short>("moduleWidth");
        unsigned short moduleHeight = dataset.get<unsigned short>("moduleHeight");
        unsigned char checksumSize = dataset.get<unsigned char>("checksumSize");
        unsigned short detectorDataSize = dataset.get<unsigned short>("detectorDataSize");

        TimeProfiler p("train");
        p.open();
        // 
        string filename = "/dev/shm/train.h5";
        filename = resourcePath("train.h5");
        File file(filename);
        file.open(File::TRUNCATE);
        KARABO_LOG_FRAMEWORK_TRACE_CF << "File " << filename << " is open";



        p.startPeriod("discoverDataset");
        h5::Format::Pointer formatConfiguration = h5::Format::discover(dataset);
        //        clog << Epochstamp().elpased(t10) << endl;
        formatConfiguration->removeElement("checksumType");
        formatConfiguration->removeElement("checksumSize");
        //formatConfiguration->removeElement("encoding");
        //    clog << Epochstamp().elpased(t10) << endl;
        p.stopPeriod("discoverDataset");

        //clog << "discoverDataset: " << p.getPeriod("discoverDataset").getDuration() << endl;
        //p.startPeriod("createConfiguration");

        p.startPeriod("configuration");
        Table::Pointer lpdTableConfiguration = file.createTable("/instrument/LPD1/configuration", formatConfiguration);
        lpdTableConfiguration->write(dataset, 0);
        p.stopPeriod("configuration");
        //clog << "configuration: " << p.getPeriod("configuration").getDuration() << endl;
        //clog << Epochstamp().elpased(t1) << endl;
        //t1.now();

        Epochstamp t1;
        h5::Format::Pointer formatTrainData = trainFormatTrainData(detectorDataSize);
        p.startPeriod("trainData");
        Table::Pointer lpdTableTrainData = file.createTable("/instrument/LPD1/train", formatTrainData);
        p.stopPeriod("trainData");
        //clog << "trainData: " << p.getPeriod("trainData").getDuration() << endl;

        //        clog << Epochstamp().elpased(t1) << endl;

        //        p.startPeriod("createImages");
        h5::Format::Pointer formatImages = trainFormatImages(dataset);
        Table::Pointer lpdTableImages = file.createTable("/instrument/LPD1/images", formatImages);
        //        p.stopPeriod();

        h5::Format::Pointer formatDescriptors = trainFormatDescriptors();
        Table::Pointer lpdTableDescriptors = file.createTable("/instrument/LPD1/descriptors", formatDescriptors);


        p.startPeriod("writeData");
        size_t idx = 0;
        for (int i = 0; i < 4; ++i) {

            uint64_t trainLength = fillTrainBuffer(buffer, bufLen, dataset, i, i * 2 + 4);


            char* ptr = &buffer[0];


            Hash trainData;

            Hash& header = trainData.bindReference<Hash>("header");
            header.set("trainId", *reinterpret_cast<unsigned long long*> (ptr));
            header.set("dataId", *reinterpret_cast<unsigned long long*> (ptr + 8));
            header.set("linkId", *reinterpret_cast<unsigned long long*> (ptr + 16));
            header.set("imageCount", *reinterpret_cast<unsigned short*> (ptr + 24));


            uint64_t imageCount = trainData.get<unsigned short>("header.imageCount");
            unsigned long long offsetDescriptors = trainLength - (8 + checksumSize) - detectorDataSize - (imageCount * 16);
            unsigned long long offsetTrailer = trainLength - (8 + checksumSize);
            unsigned long long offsetDetectorDataBlock = offsetTrailer - detectorDataSize;

            Hash& images = trainData.bindReference<Hash>("images");
            images.set("image", reinterpret_cast<unsigned short*> (ptr + 26));
            vector<unsigned long long>& imageTrainIds = images.bindReference<vector<unsigned long long> >("trainId");
            imageTrainIds.resize(imageCount, header.get<unsigned long long>("trainId"));
            images.set("bunchNumber", reinterpret_cast<unsigned long long*> (ptr + offsetDescriptors + imageCount * sizeof (uint16_t)));

            Hash& descriptors = trainData.bindReference<Hash>("descriptors");
            vector<unsigned long long>& trainIds = descriptors.bindReference<vector<unsigned long long> >("trainId");
            trainIds.resize(imageCount, header.get<unsigned long long>("trainId"));

            descriptors.set("storageCellNumber", reinterpret_cast<unsigned short*> (ptr + offsetDescriptors));
            descriptors.set("bunchNumber", reinterpret_cast<unsigned long long*> (ptr + offsetDescriptors + imageCount * sizeof (uint16_t)));
            descriptors.set("status", reinterpret_cast<unsigned short*> (ptr + offsetDescriptors + imageCount * sizeof (uint16_t)
                            + imageCount * sizeof (uint64_t)));
            descriptors.set("length", reinterpret_cast<unsigned int*> (ptr + offsetDescriptors + imageCount * sizeof (uint16_t)
                            + imageCount * sizeof (uint64_t) + imageCount * sizeof (uint16_t)));


            trainData.set("detectorDataBlock", reinterpret_cast<char*> (ptr + offsetDetectorDataBlock));

            Hash& trailer = trainData.bindReference<Hash>("trailer");
            trailer.set("checksum", *reinterpret_cast<uint32_t*> (ptr + offsetTrailer));
            trailer.set("status", *reinterpret_cast<unsigned long long*> (ptr + offsetTrailer + checksumSize));

            //            KARABO_LOG_FRAMEWORK_TRACE_CF << "trainData:" << endl << trainData;


            if (i == 0) {
                Hash attr;
                attr.set("header.trainId", static_cast<unsigned long long> (0));
                attr.setAttribute("header.trainId", "description", "Unique train number within the facility");
                attr.set("header.dataId", static_cast<unsigned long long> (0));
                attr.setAttribute("header.dataId", "description", "FPGA train data processing level");
                attr.set("header.linkId", static_cast<unsigned long long> (0));
                attr.setAttribute("header.linkId", "description", "Identifies the FEM module name");
                attr.set("header.imageCount", static_cast<unsigned short> (0));
                attr.setAttribute("header.imageCount", "description", "Number of images collected for the train and sent out from the detector");
                attr.set("images.image", static_cast<unsigned short*> (0));
                attr.setAttribute("images.image", "dims", Dims(moduleWidth, moduleHeight, numModules).toVector());

                lpdTableTrainData->writeAttributes(attr);
                lpdTableImages->writeAttributes(attr);
            }

            lpdTableTrainData->write(trainData, i);
            lpdTableImages->write(trainData, idx, imageCount);
            lpdTableDescriptors->write(trainData.get<Hash>("descriptors"), idx, imageCount);

            idx += imageCount;
        }

        p.stopPeriod("writeData");
        p.close();
        //clog << "writeData: " << p.getPeriod("writeData").getDuration() << endl;
        file.close();

    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Error");
    }

}


uint64_t H5File_Test::fillTrainBuffer(std::vector<char>& buffer, size_t bufLen, const Hash& dataset,
                                      unsigned long long trainId, unsigned short imageCount) {

    unsigned short numModules = dataset.get<unsigned short>("numModules");
    unsigned short moduleWidth = dataset.get<unsigned short>("moduleWidth");
    unsigned short moduleHeight = dataset.get<unsigned short>("moduleHeight");
    //    unsigned char checksumType = dataset.get<unsigned char>("checksumType");
    unsigned char checksumSize = dataset.get<unsigned char>("checksumSize");
    //    unsigned char encoding = dataset.get<unsigned char>("encoding");
    unsigned short detectorDataSize = dataset.get<unsigned short>("detectorDataSize");




    // the rest of header  parameters fixed in this example
    unsigned long long dataId = 1;
    unsigned long long linkId = 1;



    typedef unsigned short Pixel;
    // single image
    size_t imageSize = numModules * moduleHeight * moduleWidth;
    KARABO_LOG_FRAMEWORK_TRACE_CF << "imageSize: " << imageSize;
    vector<Pixel> image(imageSize, 0);
    for (size_t i = 0; i < image.size(); ++i) {
        image[i] = (i % moduleWidth) + 100 * (i / (moduleHeight * moduleWidth));
        if (i < 200) {
            KARABO_LOG_FRAMEWORK_TRACE_CF << "[" << i << "]: " << image[i] << " i%W: " << i % moduleWidth << " i/H*W: " << (i / (moduleHeight * moduleWidth));
        }
    }





    // descriptors        
    vector<unsigned short> storageCellNumber(imageCount, 0);
    vector<unsigned long long> bunchNumber(imageCount, 0);
    vector<unsigned short> status(imageCount, 0);
    vector<unsigned int> length(imageCount, 0);

    unsigned long long imageBlockSize = 0;
    for (size_t j = 0; j < imageCount; ++j) {
        storageCellNumber[j] = j;
        bunchNumber[j] = j;
        status[j] = 0;
        length[j] = moduleWidth * moduleHeight * numModules * sizeof (Pixel);
        imageBlockSize += length[j];
    }

    vector<char> detectorData(detectorDataSize, 'A');

    unsigned int checksum = 0;

    unsigned long long trainStatus = 0;

    /////////////////////////////////////////////////////////



    unsigned long long trainLength = 0; // trainLength in bytes - it is known from the receiver.
    // here we need to calculate it according to the train format definition (see document: Train Builder Data Format)
    trainLength = 26 /*header size*/ + imageBlockSize /*image block size*/ + imageCount * 16 /*descriptor block size*/
            + detectorDataSize /*detector specific*/ + 8 + checksumSize /*trailer*/;




    char *ptr = &buffer[0];

    //header
    memcpy(ptr, (char*) &trainId, sizeof (trainId));
    memcpy(ptr + 8, (char*) &dataId, sizeof (dataId));
    memcpy(ptr + 16, (char*) &linkId, sizeof (linkId));
    memcpy(ptr + 24, (char*) &imageCount, sizeof (imageCount));

    // images
    for (size_t j = 0; j < imageCount; ++j) {
        //        for (int k = 0; k < 10; ++k) {
        //            image[k] = j;
        //        }
        KARABO_LOG_FRAMEWORK_TRACE_CF << "pos = ptr + " << (j * image.size() * sizeof (uint16_t));
        memcpy(ptr + 26 + (j * image.size() * sizeof (uint16_t)), (char*) &image[0], imageSize * sizeof (uint16_t));
    }

    //descriptors

    unsigned long long offsetDescriptors = trainLength - (8 + checksumSize) - detectorDataSize - (imageCount * 16);

    size_t lenStorageCellNumber = imageCount * sizeof (storageCellNumber[0]);
    size_t lenBunchNumber = imageCount * sizeof (bunchNumber[0]);
    size_t lenStatus = imageCount * sizeof (status[0]);
    size_t lenLength = imageCount * sizeof (length[0]);

    KARABO_LOG_FRAMEWORK_TRACE_CF << "sizeof storageCellNumber[]: " << lenStorageCellNumber;
    KARABO_LOG_FRAMEWORK_TRACE_CF << "sizeof bunchNumber[]: " << lenBunchNumber;
    KARABO_LOG_FRAMEWORK_TRACE_CF << "sizeof lenStatus[]: " << lenStatus;
    KARABO_LOG_FRAMEWORK_TRACE_CF << "sizeof length[]: " << lenLength;

    memcpy(ptr + offsetDescriptors,
           (char*) &storageCellNumber[0], lenStorageCellNumber);
    memcpy(ptr + offsetDescriptors + lenStorageCellNumber,
           (char*) &bunchNumber[0], lenBunchNumber);
    memcpy(ptr + offsetDescriptors + lenStorageCellNumber + lenBunchNumber,
           (char*) &status[0], lenStatus);
    memcpy(ptr + offsetDescriptors + lenStorageCellNumber + lenBunchNumber + lenStatus,
           (char*) &length[0], lenLength);


    //detector specific block
    unsigned long long offsetTrailer = trainLength - (8 + checksumSize);
    unsigned long long offsetDetectorDataBlock = offsetTrailer - detectorDataSize;

    memcpy(ptr + offsetDetectorDataBlock, &detectorData[0], detectorDataSize);


    memcpy(ptr + offsetTrailer, (char*) &checksum, checksumSize);
    memcpy(ptr + offsetTrailer + checksumSize, (char*) &trainStatus, 8);


    return trainLength;
}


karabo::io::h5::Format::Pointer H5File_Test::trainFormatTrainData(unsigned short detectorDataBlockSize) {

    Format::Pointer format = Format::createEmptyFormat();
    {
        Hash c(
               "h5path", "",
               "h5name", "trainId",
               "key", "header.trainId"
               );
        c.set("attributes[0].STRING.h5name", "description");
        h5::Element::Pointer e = h5::Element::create("UINT64", c);
        format->addElement(e);
    }

    {
        Hash c(
               "h5path", "",
               "h5name", "dataId",
               "key", "header.dataId"
               );
        c.set("attributes[0].STRING.h5name", "description");
        h5::Element::Pointer e = h5::Element::create("UINT64", c);
        format->addElement(e);
    }

    {
        Hash c(
               "h5path", "",
               "h5name", "linkId",
               "key", "header.linkId"
               );
        c.set("attributes[0].STRING.h5name", "description");
        h5::Element::Pointer e = h5::Element::create("UINT64", c);
        format->addElement(e);
    }

    {
        Hash c(
               "h5path", "",
               "h5name", "imageCount",
               "key", "header.imageCount"
               );
        c.set("attributes[0].STRING.h5name", "description");
        h5::Element::Pointer e = h5::Element::create("UINT16", c);
        format->addElement(e);
    }

    {
        Hash c(
               "h5path", "",
               "h5name", "status",
               "key", "trailer.status"
               );
        h5::Element::Pointer e = h5::Element::create("UINT64", c);
        format->addElement(e);
    }

    {
        Hash c(
               "h5path", "",
               "h5name", "detectorDataBlock",
               "key", "detectorDataBlock",
               "dims", Dims(detectorDataBlockSize).toVector(),
               "type", "PTR_CHAR"
               );
        h5::Element::Pointer e = h5::Element::create("VECTOR_CHAR", c);
        format->addElement(e);
    }

    return format;
}


karabo::io::h5::Format::Pointer H5File_Test::trainFormatDescriptors() {

    Format::Pointer format = Format::createEmptyFormat();

    {
        Hash c(
               "h5path", "",
               "h5name", "storageCellNumber"
               );
        h5::Element::Pointer e = h5::Element::create("UINT16", c);
        format->addElement(e);
    }

    {
        Hash c(
               "h5path", "",
               "h5name", "trainId"
               );
        h5::Element::Pointer e = h5::Element::create("UINT64", c);
        format->addElement(e);
    }

    {
        Hash c(
               "h5path", "",
               "h5name", "bunchNumber"
               );
        h5::Element::Pointer e = h5::Element::create("UINT64", c);
        format->addElement(e);
    }
    {
        Hash c(
               "h5path", "",
               "h5name", "status"
               );
        h5::Element::Pointer e = h5::Element::create("UINT16", c);
        format->addElement(e);
    }
    {
        Hash c(
               "h5path", "",
               "h5name", "length"
               );
        h5::Element::Pointer e = h5::Element::create("UINT32", c);
        format->addElement(e);
    }

    return format;
}


karabo::io::h5::Format::Pointer H5File_Test::trainFormatImages(const Hash& dataset) {

    Format::Pointer format = Format::createEmptyFormat();

    {


        Hash c(
               "h5path", "",
               "h5name", "image",
               "dims", Dims(dataset.get<unsigned short>("moduleWidth"), dataset.get<unsigned short>("moduleHeight"),
                            dataset.get<unsigned short>("numModules")).toVector(),
               "type", "PTR_UINT16",
               "key", "images.image"
               );

        // define "dims" attribute for the image as a vector of 3 elements: "module width", "module height", "number of modules"
        c.set("attributes[0].VECTOR_UINT64.h5name", "dims");
        c.set("attributes[0].VECTOR_UINT64.dims", "3");

        h5::Element::Pointer e = h5::Element::create("VECTOR_UINT16", c);
        format->addElement(e);
    }

    {
        Hash c(
               "h5path", "",
               "h5name", "trainId",
               "key", "images.trainId"

               );
        h5::Element::Pointer e = h5::Element::create("UINT64", c);
        format->addElement(e);
    }

    {
        Hash c(
               "h5path", "",
               "h5name", "bunchNumber",
               "key", "images.bunchNumber"
               );
        h5::Element::Pointer e = h5::Element::create("UINT64", c);
        format->addElement(e);
    }

    return format;
}


void H5File_Test::testClose() {

    try {
        string filename = "/dev/shm/close.h5";
        filename = resourcePath("close.h5");

        //filename = "/tmp/close.h5";
        Hash data("x", 123, "y", "abc", "z", vector<signed char>(100, 48));
        data.setAttribute("x", "a1", 987);
        data.setAttribute("x", "a2", vector<int>(3, 45));
        data.setAttribute("x", "a3", "textattr");
        data.setAttribute("x", "a4", true);
        std::vector<bool> vb(6, true);
        vb[1] = false;
        vb[2] = false;
        data.setAttribute("x", "a5", vb);
        vector<string> vs(4, "abcde");
        vs[2] = "aabbccddeeffgghhiijj";
        data.setAttribute("x", "a6", vs);


        Format::Pointer dataFormat = Format::discover(data);
        KARABO_LOG_FRAMEWORK_TRACE_CF << "File " << filename;


        //        for (unsigned long long i = 0; i < 100000000L; ++i) {
        for (unsigned long long i = 0; i < 1L; ++i) {

            //            clog << i << endl;


            File file(filename);
            file.open(File::TRUNCATE);
            KARABO_LOG_FRAMEWORK_TRACE_CF << "File " << filename << " " << i << " is open";
            Table::Pointer t = file.createTable("/a/b/c/d", dataFormat);
            t->writeAttributes(data);
            t->write(data, 0);
            t->write(data, 1);
            t->write(data, 2);
            t->write(data, 3);
            //            clog << i << " after write" << endl;
            //        file.closeTable(t);
            //            clog << i << " after table close" << endl;
            file.close();
            //            clog << i << " after file close" << endl;

            file.open(File::READONLY);
            KARABO_LOG_FRAMEWORK_TRACE_CF << "File " << filename << " " << i << " is open for reading";
            t = file.getTable("/a/b/c/d");
            Table::Pointer t1 = file.getTable("/a/b/c/d");
            Table::Pointer t2 = file.getTable("/a/b/c/d");
            Hash rAttr;
            t->readAttributes(rAttr);
            //            clog << rAttr << endl;
            //            clog << rAttr.getAttribute<bool>("x", "a4") << endl;
            Hash rdata;
            t->bind(rdata);
            t->read(0);
            t->read(1);
            t->read(2);
            t->read(3);

            t1->read(1);

            file.close();
        }
    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Error");
    }
}


void H5File_Test::testArray() {
    try {
        string filename = "/dev/shm/array.h5";

        filename = resourcePath("array.h5");
        const int arr[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        const int* ptr = &arr[0];
        std::pair<const int*, size_t> array(ptr, 12);
        Hash data("array", array);
        data.setAttribute("array", "dims", Dims(3, 4).toVector());

        KARABO_LOG_FRAMEWORK_TRACE_CF << "data " << data;
        KARABO_LOG_FRAMEWORK_TRACE_CF << "discover format";


        Format::Pointer dataFormat = Format::discover(data);
        KARABO_LOG_FRAMEWORK_TRACE_CF << "File " << filename;



        File file(filename);
        file.open(File::TRUNCATE);
        KARABO_LOG_FRAMEWORK_TRACE_CF << "File " << filename << " is open";
        Table::Pointer t = file.createTable("/a/b/c/d", dataFormat);
        t->writeAttributes(data);
        t->write(data, 0);
        t->write(data, 1);
        t->write(data, 2);
        t->write(data, 3);
        file.close();


        file.open(File::READONLY);
        KARABO_LOG_FRAMEWORK_TRACE_CF << "File " << filename << " is open for reading";

        Table::Pointer t1 = file.getTable("/a/b/c/d");
        CPPUNIT_ASSERT(t1->size() == 4);

        Hash rdata;
        t1->bind(rdata);
        for (size_t j = 0; j < 4; ++j) {
            t1->read(j);
            KARABO_LOG_FRAMEWORK_TRACE_CF << "record " << j << " rdata: " << rdata;
            vector<int>& rarr = rdata.get<vector<int> >("array");
            CPPUNIT_ASSERT(rarr.size() == 12);
            for (size_t i = 0; i < rarr.size(); ++i) {
                CPPUNIT_ASSERT(static_cast<size_t>(rarr[i]) == i);
            }
        }

        file.close();




    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Error in testArray");
    }
}


void H5File_Test::testExternalHdf5() {
    try {

        boost::filesystem::path filename1 = "/home/wrona/release/karaboPackageDevelopment/packages/testApplications/hdf5IO/hdf5images.h5";
        File file1(filename1);
        file1.open(File::READONLY);

        Format::Pointer df = Format::createEmptyFormat();

        Hash c(
               "h5path", "",
               "h5name", "Image24bitplane",
               "dims", Dims(227, 149, 3).toVector()
               );
        //"dims", Dims(3, 149, 227).toVector()

        h5::Element::Pointer e = h5::Element::create("VECTOR_UINT8", c);
        df->addElement(e);
        clog << c << endl;

        Table::Pointer table = file1.getTable("/", df, 1);

        Hash rdata;

        table->bind(rdata);
        table->read(0);

        clog << "image:" << endl << rdata << endl;

        file1.close();

    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Error");
    }

}