/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Created on Oct 4, 2012, 5:30:35 PM
 */

#include "ReaderWriter_Test.hh"
#include <iostream>
#include <fstream>

using namespace karabo::util;
using namespace karabo::io;
using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION(ReaderWriter_Test);


ReaderWriter_Test::ReaderWriter_Test() {
}


ReaderWriter_Test::~ReaderWriter_Test() {
}


void ReaderWriter_Test::setUp() {

    //Define 'hash', using setFromPath function  
    //used in the test 'testWritingReadingBinaryFormat'
    hash.setFromPath("Motor.name", "Beckhoff Motor");
    hash.setFromPath("Motor.Init.position", 10);
    hash.setFromPath("Motor.Init.velocity", 3.0);
    hash.setFromPath("Motor.array[0]", static_cast<unsigned int> (0));
    hash.setFromPath("Motor.array[1]", static_cast<unsigned int> (1));
    hash.setFromPath("Motor.array[2]", static_cast<unsigned int> (122));
    hash.setFromPath("Motor.array[3]", static_cast<unsigned int> (33));
    hash.setFromPath("Motor.array[4]", static_cast<unsigned int> (4));
    hash.setFromPath("Motor.Init.sAxisName[0]", "Parrot is an exotic bird");
    hash.setFromPath("Motor.Init.sAxisName[1]", "Data processing");
    hash.setFromPath("Motor.Init.sAxisName[2]", "Data transmission as an asynchronous task!");
    hash.setFromPath("Motor.Init.a.b.number", static_cast<double> (123.45));

    //Define 'runDir' directory containing all needed resources
    runDir = string(TESTPATH) + string("io/resources/");

}


void ReaderWriter_Test::testWritingReadingBinaryFormat() {
    string plcstr;
    plcstr.clear();
    //TEST 1. WritingBinaryFormat >>>>>>
    Hash c;
    plcstr.clear();
    c.setFromPath("StringStream.format.Bin");
    c.setFromPath("StringStream.stringPointer", &plcstr);
    Writer<Hash>::Pointer out = Writer<Hash>::create(c);
    out->write(hash);
    CPPUNIT_ASSERT(plcstr.length() == 289);

    //TEST 2. ReadingBinaryFormat >>>>>>>
    Hash c2;
    Hash hash2;
    c2.setFromPath("StringStream.format.Bin");
    c2.setFromPath("StringStream.string", plcstr); // plcstr contains encoded PLC message
    Reader<Hash>::Pointer in = Reader<Hash>::create(c2);
    in->read(hash2); // <--- decoding 'plcstr' into hash 'hash2'

    //check that original 'hash' and decoded 'hash2' are identical
    for (Hash::const_iterator it = hash.begin(); it != hash.end(); it++) {
        if (!hash2.identical(it)) {
            CPPUNIT_ASSERT(false);
        }
    }
    for (Hash::const_iterator it = hash2.begin(); it != hash2.end(); it++) {
        if (!hash.identical(it)) {
            CPPUNIT_ASSERT(false);
        }
    }
    CPPUNIT_ASSERT(true);
}


void ReaderWriter_Test::testWritingReadingXmlFormat() {

    //TEST 3. ReadingXmlFormat >>>>>>
    //read from file  'xmlForReading_readonly.xml' (stored in 'resources' directory) XML-Format 
    //and create configuration object 'configuration'
    Hash configuration, conf, confw;

    conf.setFromPath("TextFile.filename", runDir + "xmlForReading_readonly.xml");

    Reader<Hash>::Pointer in = Reader<Hash>::create(conf);
    in->read(configuration);

    //if needed uncomment for check:
    //cout << "Reading file " << conf.getFromPath<string > ("TextFile.filename") << ". Created object 'configuration': " << endl;
    //cout << configuration << endl;

    //TEST 4. Check created configuration object 'configuration' >>>>>>
    CPPUNIT_ASSERT(configuration.has("application")); //contains element 'application'
    CPPUNIT_ASSERT(configuration.size() == 1); //contains one element ('application')
    CPPUNIT_ASSERT(!configuration.has("columns")); //contains no element 'columns'

    Hash u = configuration.get<Hash>("application");
    CPPUNIT_ASSERT(u.has("window")); //'application' contains element 'windows'
    std::vector<int> vecInt = u.getFromPath<std::vector<int> >("misc.vectint");
    CPPUNIT_ASSERT(vecInt[0] == 5);

    //TEST 5. WritingXmlFormat >>>>>>
    //write configuration object 'configuration' into newly created file 'newTestXml.xml'

    //Default: simple data types will not be written into XML.
    //the following line reflects the default behavior:
    //conf.setFromPath("TextFile.format.Xml.printDataType", false); //false or 0

    //In order to write simple data types as an attribute in XML-element,
    //for example: <myname dataType="STRING">Hallo</myname>
    //set the value of parameter 'printDataType' to 'true' :
    confw.setFromPath("TextFile.format.Xml.printDataType", true); //true or 1

    string fnameNew = runDir + "newTestXml.xml";
    boost::filesystem::path filePathNew(fnameNew.c_str());
    confw.setFromPath("TextFile.filename", filePathNew);

    Writer<Hash>::Pointer out = Writer<Hash>::create(confw);
    out->write(configuration);

    //Compare files:  'etalonXML_readonly.xml' with 
    //the newly created file 'newTestXml.xml' (that was created from the 'configuration' object 
    //and contains all 'dataTypes' written as an attribute for each xml-element; 
    //therefore it is slightly different from the input file for reading 'xmlForReading_readonly.xml')   
    ifstream newFile(fnameNew.c_str());

    string fnameEtalon = runDir + "etalonXML_readonly.xml";
    ifstream etalonFile(fnameEtalon.c_str());

    if (etalonFile.is_open()) {
        if (newFile.is_open()) {

            while (etalonFile.good()) {
                while (newFile.good()) {
                    string etalonLine, newLine;

                    getline(etalonFile, etalonLine);

                    getline(newFile, newLine);

                    if (!etalonLine.compare(newLine) == 0) {
                        CPPUNIT_ASSERT(false);
                    }
                }
            }
            etalonFile.close();
            newFile.close();
        } else {
            cout << "Error opening file " << fnameNew << endl;
        }
    } else {
        cout << "Error opening etalon file " << fnameEtalon << endl;
    }

}


void ReaderWriter_Test::testWritingSchema() {

    //This test takes as a test-schema the expected parameters of 'TextFileReader' class   
    Schema schemaForTest = Reader<Hash>::expectedParameters("TextFile");

    //TEST 6. Writing Schema into Stream (Xsd format).   
    //Test-schema will be written into xsd-file 'expectedParamsTextFile.xsd'

    Hash input;
    string fileName = runDir + "expectedParamsTextFile.xsd";
    input.setFromPath("TextFile.filename", fileName);
    input.setFromPath("TextFile.format.Xsd");
    input.setFromPath("TextFile.format.Xsd.indentation", 3);

    //if needed uncomment for check:
    //cout << "Check  format: \n" << input << endl;

    Writer<Schema>::Pointer out = Writer<Schema>::create(input);
    out->write(schemaForTest);

    //if needed uncomment for check:
    //cout << "Schema is now written into file " << input.getFromPath<string > ("TextFile.filename") << endl;

    string checkFileName = input.getFromPath<string > ("TextFile.filename");
    CPPUNIT_ASSERT(fileName.compare(checkFileName) == 0);

    ifstream newFile(fileName.c_str());

    string fnameEtalon = runDir + "etalonXSD_readonly.xsd";
    ifstream etalonFile(fnameEtalon.c_str());

    if (etalonFile.is_open()) {
        if (newFile.is_open()) {

            while (etalonFile.good()) {
                while (newFile.good()) {
                    string etalonLine, newLine;

                    getline(etalonFile, etalonLine);

                    getline(newFile, newLine);

                    if (!etalonLine.compare(newLine) == 0) {
                        CPPUNIT_ASSERT(false);
                    }
                }
            }
            etalonFile.close();
            newFile.close();
        } else {
            cout << "Error opening file " << fileName << endl;
        }
    } else {
        cout << "Error opening etalon file " << fnameEtalon << endl;
    }

    //TEST 7. Writing Schema into Stream (Xsd format).
    //Test-schema will be written into xsd-file 'expectedParamsTextFile.xsd'

    string outputString;

    Hash inStream;
    inStream.setFromPath("StringStream.format.Xsd.indentation", -1);
    inStream.setFromPath("StringStream.stringPointer", &outputString);

    //if needed uncomment for check:
    //cout << "Check  format: \n" << inStream << endl;

    Writer<Schema>::Pointer writerSchema = Writer<Schema>::create(inStream);

    writerSchema->write(schemaForTest);
    CPPUNIT_ASSERT(outputString.length() == 3623);
}