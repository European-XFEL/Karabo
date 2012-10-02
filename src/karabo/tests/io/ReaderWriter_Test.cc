/*
 * File:   ReaderWriter_Test.cc
 * Author: irinak
 *
 * Created on Oct 1, 2012, 5:32:32 PM
 */

#include "ReaderWriter_Test.hh"

using namespace karabo::util;
using namespace karabo::io;
using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION(ReaderWriter_Test);

ReaderWriter_Test::ReaderWriter_Test() {
}

ReaderWriter_Test::~ReaderWriter_Test() {
}

void ReaderWriter_Test::testWritingReadingBinaryFormat() {
    string plcstr;

    //WritingBinaryFormat >>>>>>
    Hash hash;
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

    Hash c;
    plcstr.clear();
    c.setFromPath("StringStream.format.Bin");
    c.setFromPath("StringStream.stringPointer", &plcstr);
    Writer<Hash>::Pointer out = Writer<Hash>::create(c);
    out->write(hash);
    CPPUNIT_ASSERT(plcstr.length() == 289);

    //ReadingBinaryFormat >>>>>>>
    Hash c2;
    Hash hash2;

    c2.setFromPath("StringStream.format.Bin");
    c2.setFromPath("StringStream.string", plcstr); // plcstr contains encoded PLC message
    Reader<Hash>::Pointer in = Reader<Hash>::create(c2);
    in->read(hash2); // <--- decoding into hash

    //check that original 'hash' and decoded 'hash2' are identical
    bool flag = true;
    for (Hash::const_iterator it = hash.begin(); it != hash.end(); it++){
        if( !hash2.identical(it) ) {
            flag = false;
            CPPUNIT_ASSERT(flag);
        }
    }
    for (Hash::const_iterator it = hash2.begin(); it != hash2.end(); it++){
        if( !hash.identical(it) ) {
            flag = false;
            CPPUNIT_ASSERT(flag);
        }
    }
    CPPUNIT_ASSERT(flag);
}

void ReaderWriter_Test::testWriting() {
    Schema schemaForTest = Reader<Hash>::expectedParameters("TextFile");
    Hash input;
    string fileName = "expected.xsd";
    input.setFromPath("TextFile.filename", fileName);
    input.setFromPath("TextFile.format.Xsd");
    //input.setFromPath("TextFile.format.Xsd.indentation", -1);
    input.setFromPath("TextFile.format.Xsd.indentation", 3);
    cout << "Check  format: \n" << input << endl;

    Writer<Schema>::Pointer out = Writer<Schema>::create(input);
    out->write(schemaForTest);
    cout << "Schema is now written into file " << input.getFromPath<string > ("TextFile.filename") << endl;

    string checkFileName = input.getFromPath<string > ("TextFile.filename");
    CPPUNIT_ASSERT(fileName.compare(checkFileName) == 0);

    cout << "\nTEST 2B. Writing Schema into Stream (Xsd format)" << endl;

    string outputString;

    Hash inStream;
    inStream.setFromPath("StringStream.format.Xsd.indentation", -1);
    inStream.setFromPath("StringStream.stringPointer", &outputString);
    cout << "Check  format: \n" << inStream << endl;

    Writer<Schema>::Pointer writerSchema = Writer<Schema>::create(inStream);

    writerSchema->write(schemaForTest);
    CPPUNIT_ASSERT(outputString.length() == 3623);
    cout << "Result: outputString.length()  = " << outputString.length() << endl;
}


