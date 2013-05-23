/* 
 * Author: <krzysztof.wrona@xfel.eu>
 * 
 * Created on February 20, 2013, 2:33 PM
 */

#include "H5File_Test.hh"
#include "karabo/util/ArrayTools.hh"
#include "TestPathSetup.hh"
#include "karabo/io/TextSerializer.hh"
#include "karabo/io/HashXmlSerializer.hh"
#include "HashXmlSerializer_Test.hh"
#include "karabo/io/h5/Group.hh"
#include "karabo/io/h5/FixedLengthArray.hh"
#include "Hdf5_Test.hh"
#include <karabo/util/Hash.hh>

#include <karabo/io/h5/File.hh>
#include <karabo/io/h5/Element.hh>
#include <karabo/io/h5/Scalar.hh>
#include <karabo/util/Profiler.hh>

#include <karabo/log/Tracer.hh>

using namespace karabo::util;
using namespace karabo::io::h5;
using namespace karabo::io;
using namespace log4cpp;

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
    tr.enable("H5File_Test.testReadTable");
    tr.enable("H5File_Test.testBufferWrite");
    tr.enable("H5File_Test.testBufferRead");
    tr.enable("H5File_Test.testRead");
    tr.enable("H5File_Test.testWrite");
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


        Table::Pointer t = file.createTable("/abc", dataFormat, 1);


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
            CPPUNIT_ASSERT(vec[i] == i * 1.0 + 0.1234);
            CPPUNIT_ASSERT(vecd[i] == i * 1.0 + 0.1234);
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

        Profiler p("VectorBufferWrite");

        p.start("format");
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


        p.stop("format");


        //data.set("vectors.image", v0).setAttribute("dims", Dims(1024, 1024).toVector());

        p.start("initialize");

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


        p.stop("initialize");
        p.start("create");


        string filename = "/dev/shm/file3.h5";
        filename = resourcePath("file3.h5");
        File file(filename);
        file.open(File::TRUNCATE);

        Table::Pointer t = file.createTable("/planets", format, 100);

        p.stop("create");
        p.start("write0");

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


        p.stop("write0");
        p.start("write");


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
        p.stop("write");

        p.start("close");
        file.close();
        p.stop("close");

        double formatTime = HighResolutionTimer::time2double(p.getTime("format"));
        double initializeTime = HighResolutionTimer::time2double(p.getTime("initialize"));
        double createTime = HighResolutionTimer::time2double(p.getTime("create"));
        double writeTime = HighResolutionTimer::time2double(p.getTime("write"));
        double closeTime = HighResolutionTimer::time2double(p.getTime("close"));

        if (m_reportTime) {
            clog << endl;
            clog << "file: " << filename << endl;
            clog << "initialize data                  : " << initializeTime << " [s]" << endl;
            clog << "format                           : " << formatTime << " [s]" << endl;
            clog << "open/prepare file                : " << createTime << " [s]" << endl;
            clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
            clog << "written data size                : " << totalSize << " [MB]" << endl;
            clog << "writing speed                    : " << totalSize / writeTime << " [MB/s]" << endl;
            clog << "close                            : " << closeTime << " [s]" << endl;
            clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
            clog << "write+close(flush to disk) speed : " << totalSize / (writeTime + closeTime) << " [MB/s]" << endl;
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


        Profiler p("VectorBufferRead");

        p.start("format");
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

        Table::Pointer t = file.createTable("/abc", dataFormat, 1);
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


        Table::Pointer t = file.createTable("/base", dataFormat, 1);


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


void H5File_Test::testManyGroups() {
    Profiler p("ManyGroups");

  
    try {

        Hash data, d1, d2, d3, d4;

        int n = 500;
        int rec = 100;
        float totalSize = rec * (4 + 4 + 8 + 2) * n / 1024;
        for (int i = 0; i < n; ++i) {
            d1.set(toString(i), i);
            d2.set(toString(i), static_cast<float> (i));
            d3.set(toString(i), static_cast<double> (i));
            d4.set(toString(i), static_cast<unsigned short> (i));
        }

        data.set("d1", d1);
        data.set("d2", d2);
        data.set("d3", d3);
        data.set("d4", d4);
        
        p.start("format");
        Format::Pointer dataFormat = Format::discover(data);
        vector<string> el;
        dataFormat->getElementsNames(el);
        for (size_t i = 0; i < el.size(); ++i) {
            dataFormat->getElement(el[i])->setCompressionLevel(0);
        }
        p.stop("format");

        for (int i = 0; i < n; ++i) {
            d1.set(toString(i), vector<int>(rec, i));
            d2.set(toString(i), vector<float> (rec, i));
            d3.set(toString(i), vector<double> (rec, i));
            d4.set(toString(i), vector<unsigned short> (rec, i));
        }

        data.set("d1", d1);
        data.set("d2", d2);
        data.set("d3", d3);
        data.set("d4", d4);


        string filename = "/dev/shm/fileMany.h5";
        filename   = resourcePath("fileMany.h5");
        File file(filename);
        file.open(File::TRUNCATE);
        KARABO_LOG_FRAMEWORK_TRACE_CF << "File is open";


        p.start("create");
        Table::Pointer t = file.createTable("/base", dataFormat, 100);
        KARABO_LOG_FRAMEWORK_TRACE_CF << "File structure is created";
        p.stop("create");

        //        t->writeAttributes(data);
        p.start("write");
        //        for (int i = 0; i < rec; ++i) {
        //            t->write(data, i);
        //        }

        int m = 1;
        for (int i = 0; i < m; ++i) {
            t->write(data, i*rec, rec);
        }
        totalSize *= m;

        p.stop("write");

        p.start("close");
        file.close();
        p.stop("close");

        double formatTime = HighResolutionTimer::time2double(p.getTime("format"));
        double discoverTime = HighResolutionTimer::time2double(p.getTime("discover"));
        double createTime = HighResolutionTimer::time2double(p.getTime("create"));
        double writeTime = HighResolutionTimer::time2double(p.getTime("write"));
        double closeTime = HighResolutionTimer::time2double(p.getTime("close"));

        if (false) {
            clog << endl;
            clog << "file: " << filename << endl;
            //          clog << "initialize data                  : " << initializeTime << " [s]" << endl;
            clog << "discover                           : " << discoverTime << " [s]" << endl;
            clog << "format                           : " << formatTime << " [s]" << endl;
            clog << "open/prepare file                : " << createTime << " [s]" << endl;
            clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
            clog << "written data size                : " << totalSize << " [kB]" << endl;
            clog << "writing speed                    : " << totalSize / writeTime << " [kB/s]" << endl;
            clog << "close                            : " << closeTime << " [s]" << endl;
            clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
            clog << "write+close(flush to disk) speed : " << totalSize / (writeTime + closeTime) << " [kB/s]" << endl;
        }


    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Error");
    }

}


void H5File_Test::testManyTables() {
    Profiler p("ManyTable");

    KARABO_LOG_FRAMEWORK_TRACE_CF << "start ManyTables";
    try {

        Hash d1, d2, d3, d4;

        int n = 500; //5000; //500;//5000;//20000;//25000;
        int rec = 200;
        float totalSize = rec * (4 + 4 + 8 + 2) * n / 1024;
        for (int i = 0; i < n; ++i) {
            d1.set(toString(i), i);
            d2.set(toString(i), static_cast<float> (i));
            d3.set(toString(i), static_cast<double> (i));
            d4.set(toString(i), static_cast<unsigned short> (i));
            //    d1.setAttribute(toString(i), "unit", "mA");
            //    d1.setAttribute(toString(i), "a", 234);
            //    d2.setAttribute(toString(i), "unit", "mA"); 
            //    d2.setAttribute(toString(i), "a", 234);
            //    d3.setAttribute(toString(i), "unit", "mA");
            //    d3.setAttribute(toString(i), "a", 234);
            //    d4.setAttribute(toString(i), "unit", "mA");
            //    d4.setAttribute(toString(i), "a", 23            
        }

        //        vector<unsigned char>& status = d1.bindReference<vector<unsigned char> >("status");
        //        status.resize(n*200, 1);
        //        d1.setAttribute("status","dims",Dims(200,n).toVector());


        p.start("format");

        FormatDiscoveryPolicy::Pointer policy = FormatDiscoveryPolicy::create(Hash("Policy.chunkSize", rec));
        Format::Pointer dataFormat1 = Format::discover(d1, policy);
        Format::Pointer dataFormat2 = Format::discover(d2, policy);
        Format::Pointer dataFormat3 = Format::discover(d3, policy);
        Format::Pointer dataFormat4 = Format::discover(d4, policy);

        p.stop("format");


        //        Hash c1, c2, c3, c4;
        //        p.start("discover");
        //        Format::discoverFromHash(d1, c1);
        //        Format::discoverFromHash(d2, c2);
        //        Format::discoverFromHash(d3, c3);
        //        Format::discoverFromHash(d4, c4);
        //        p.stop("discover");
        //
        //        //        data.set("d1", d1);
        //        //        data.set("d2", d2);
        //        //        data.set("d3", d3);
        //        //        data.set("d4", d4);
        //
        //
        //        p.start("format");
        //        Format::Pointer dataFormat1 = Format::createFormat(c1, false);
        //        //        vector<string> el;
        //        //        dataFormat1->getElementsNames(el);
        //        //        for (size_t i = 0; i < el.size(); ++i) {
        //        //            dataFormat1->getElement(el[i])->setCompressionLevel(6);
        //        //        }
        //        Format::Pointer dataFormat2 = Format::createFormat(c2, false);
        //
        //        //        dataFormat2->getElementsNames(el);
        //        //        for (size_t i = 0; i < el.size(); ++i) {
        //        //            dataFormat2->getElement(el[i])->setCompressionLevel(6);
        //        //        }
        //        Format::Pointer dataFormat3 = Format::createFormat(c3, false);
        //
        //        //        dataFormat3->getElementsNames(el);
        //        //        for (size_t i = 0; i < el.size(); ++i) {
        //        //            dataFormat3->getElement(el[i])->setCompressionLevel(6);
        //        //        }
        //        Format::Pointer dataFormat4 = Format::createFormat(c4, false);
        //
        //        //        dataFormat4->getElementsNames(el);
        //        //        for (size_t i = 0; i < el.size(); ++i) {
        //        //            dataFormat4->getElement(el[i])->setCompressionLevel(6);
        //        //        }

        p.stop("format");



        string filename = "/dev/shm/fileManyTables.h5";
        filename   = resourcePath("fileManyTables.h5");
        File file(filename);
        file.open(File::TRUNCATE);
        KARABO_LOG_FRAMEWORK_TRACE_CF << "File is open";


        p.start("create");
        size_t chunk = rec;
        Table::Pointer t1 = file.createTable("/base/c1", dataFormat1, chunk);
        Table::Pointer t2 = file.createTable("/base/c2", dataFormat2, chunk);
        Table::Pointer t3 = file.createTable("/base/c3", dataFormat3, chunk);
        Table::Pointer t4 = file.createTable("/base/c4", dataFormat4, chunk);
        KARABO_LOG_FRAMEWORK_TRACE_CF << "File structure is created";
        p.stop("create");

        //        p.start("attribute");
        //        t1->writeAttributes(d1);
        //        t2->writeAttributes(d2);
        //        t3->writeAttributes(d3);
        //        t4->writeAttributes(d4);
        //        p.stop("attribute");


        for (int i = 0; i < n; ++i) {
            d1.set(toString(i), vector<int>(rec, i));
            d2.set(toString(i), vector<float> (rec, i));
            d3.set(toString(i), vector<double> (rec, i));
            d4.set(toString(i), vector<unsigned short> (rec, i));
        }
        //            vector<unsigned char>& status1 = d1.bindReference<vector<unsigned char> >("status");
        //            status1.resize(200*n*rec, 1);




        p.start("write");

        //        int m = 10; 
        //        for (int i = 0; i < m*rec; ++i) {
        //            t1->write(d1, i);
        //            t2->write(d2, i);
        //            t3->write(d3, i);
        //            t4->write(d4, i);
        //        }

        int m = 10;
        for (int i = 0; i < m; ++i) {
            t1->write(d1, i*rec, rec);
            t2->write(d2, i*rec, rec);
            t3->write(d3, i*rec, rec);
            t4->write(d4, i*rec, rec);
        }
        totalSize *= m;

        p.stop("write");

        p.start("close");
        file.close();
        p.stop("close");

        double formatTime = HighResolutionTimer::time2double(p.getTime("format"));
        double createTime = HighResolutionTimer::time2double(p.getTime("create"));
        double attributeTime = HighResolutionTimer::time2double(p.getTime("attribute"));
        double writeTime = HighResolutionTimer::time2double(p.getTime("write"));
        double closeTime = HighResolutionTimer::time2double(p.getTime("close"));

        if (false) {
            clog << endl;
            //            clog << "file: " << filename << endl;
            //          clog << "initialize data                  : " << initializeTime << " [s]" << endl;
            clog << "format                           : " << formatTime << " [s]" << endl;
            clog << "open/prepare file                : " << createTime << " [s]" << endl;
            clog << "write attributes                 : " << attributeTime << " [s]" << endl;
            clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
            clog << "written data size                : " << totalSize << " [kB]" << endl;
            clog << "writing speed                    : " << totalSize / writeTime << " [kB/s]" << endl;
            clog << "close                            : " << closeTime << " [s]" << endl;
            clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
            clog << "write+close(flush to disk) speed : " << totalSize / (writeTime + closeTime) << " [kB/s]" << endl;
            clog << "Total time                       : " << formatTime + createTime + attributeTime + writeTime + closeTime << " [s]" << endl;
        }


    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Error");
    }

}


void H5File_Test::testVLWrite() {

    Profiler p("VLWrite");
    Hash data;
    try {


        Format::Pointer dataFormat = Format::createEmptyFormat();

        Hash h1(
                "h5path", "experimental",
                "h5name", "test"
                );

        h5::Element::Pointer e1 = h5::Element::create("VLARRAY_INT32", h1);

        vector<int> v(20, 2);
        data.set("experimental.test", &v[0]).setAttribute("size", 20);
        dataFormat->addElement(e1);

        string filename = "/dev/shm/fileVL.h5";
        filename = resourcePath("fileVL.h5");
        File file(filename);
        file.open(File::TRUNCATE);



        p.start("create");
        Table::Pointer t = file.createTable("/base", dataFormat, 1);
        p.stop("create");

        //        t->writeAttributes(data);
        p.start("write");
        //        for (int i = 0; i < rec; ++i) {

        {
            vector<int> v(20, 2);
            data.set("experimental.test", &v[0]).setAttribute("size", 20);
            t->write(data, 0);
        }
        {
            vector<int> v(3, 8);
            data.set("experimental.test", &v[0]).setAttribute("size", 3);
            t->write(data, 1);
        }
        {

            vector<int> v(9, 24);
            data.set("experimental.test", &v[0]).setAttribute("size", 9);
            t->write(data, 2);
        }
        //        }
        p.stop("write");

        p.start("close");
        file.close();
        p.stop("close");

    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_FAIL("Error");
    }

}