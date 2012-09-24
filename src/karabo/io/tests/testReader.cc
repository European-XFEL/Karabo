/*
 * $Id: testReader.cc 6873 2012-07-30 15:11:08Z irinak $
 *
 * File:   sample.cc
 * Author: <your.email@xfel.eu>
 *
 * Created on August,2010,02:14PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <fstream>
#include <cassert>
#include <iosfwd>
#include <vector>
#include <exfel/util/Test.hh>
#include <boost/filesystem.hpp>

#include "../Reader.hh"
#include "../Writer.hh"
#include "../StringStreamWriter.hh"
#include "../StringStreamReader.hh"

#include "ParameterCheck.hh"
#include <exfel/util/PluginLoader.hh>
#include <exfel/util/String.hh>

using namespace std;
using namespace exfel::io;
using namespace exfel::util;

int testReader(int argc, char** argv) {

    try {

        Test t;
        TEST_INIT(t, argc, argv);
        cout << t << endl;

        //        {
        //            cout << "\nTEST 1. Writing Schema into file (LibConfig format)" << endl;
        //            // This Reader<Hash> really reads a Hash and is asked for its expected parameters
        //            Schema expected = Reader<Hash>::expectedParameters("TextFile");
        //            Hash input;
        //            input.setFromPath("TextFile.filename", t.file("expected.conf"));
        //            input.setFromPath("TextFile.format.LibConfig");
        //            Writer<Hash>::Pointer out = Writer<Hash>::create(input);
        //            out->write(expected);
        //        }

        {
            cout << "\nTEST 2A. Writing Schema into File (Xsd format)" << endl;
            Schema schemaForTest = Reader<Hash>::expectedParameters("TextFile");
            Hash input;
            input.setFromPath("TextFile.filename", t.file("expected.xsd"));
            input.setFromPath("TextFile.format.Xsd");
            //input.setFromPath("TextFile.format.Xsd.indentation", -1);
            input.setFromPath("TextFile.format.Xsd.indentation", 3);
            cout <<  "Check  format: \n" << input << endl;
            
            Writer<Schema>::Pointer out = Writer<Schema>::create(input);
            out->write(schemaForTest);
            cout << "Schema is now written into file " << input.getFromPath<string>("TextFile.filename") << endl;

            cout << "\nTEST 2B. Writing Schema into Stream (Xsd format)" << endl;
            string outputString;
            Hash inStream;
            inStream.setFromPath("StringStream.format.Xsd.indentation", -1);
            inStream.setFromPath("StringStream.stringPointer", &outputString);
            cout <<  "Check  format: \n" << inStream << endl;                
       
            Writer<Schema>::Pointer writerSchema = Writer<Schema>::create(inStream);

            writerSchema->write(schemaForTest);
            cout << "Result: outputString.length()  = " << outputString.length() << endl;
            //can be just printed out :
            //cout << "Result: outputString  = \n" << outputString << endl;
        }
        {
            cout << "\nTesting create(const string& clasId, const Hash& parameters)" << endl;
            Reader<Hash>::Pointer inpt1 = Reader<Hash>::create("TextFile", Hash("filename", t.file("xmlForReading.xml")));
            Hash configFromXmlFile;
            inpt1->read(configFromXmlFile);
            cout << "Created object 'configFromXmlFile': \n" << configFromXmlFile << endl;
        }

        {
            cout << "\nTesting function 'serialize', Format<Hash> " << endl;
            Format<Hash>::Pointer fh = Format<Hash>::create("Xml");
            Hash sample;
            sample.setFromPath("a.b.c.d", 7);
            sample.setFromPath("a.b.c.f", 5);
            string hashToString = fh->serialize(sample);
            cout << "Serialized (string):\n" << hashToString << endl;
        }
        {
            cout << "\nTesting function 'unserialize', Format<Hash> " << endl;
            Format<Hash>::Pointer fh = Format<Hash>::create("Xml");
            string strSample = "<?xml version=\"1.0\" ?><a xmlns=\"http://xfel.eu/config\"><b><c><d>7</d><f>5</f></c></b></a>";
            Hash stringToHash = fh->unserialize(strSample);
            cout << "Unserialized (hash):\n" << stringToHash << endl;
        }

        {
            cout << "\nTEST Vectors" << endl;
            Schema expected = ParameterCheck::expectedParameters("ParameterCheck");
            cout << "Get expected parameters of 'ParameterCheck' (see io/src/tests/ParameterCheck.cc)" << endl;
            cout << "Schema 'expected' : \n" << expected << endl;
            Hash input;
            input.setFromPath("TextFile.filename", t.file("expectedParamCheck.xsd"));
            Writer<Schema>::Pointer out = Writer<Schema>::create(input);
            out->write(expected);
            cout << "resulted XML Schema: expectedParamCheck.xsd" << endl;

            Hash conf, outputXml, outputLibConf;
            conf.setFromPath("ParameterCheck.vectorString", vector<string > (3, "Hallo"));
            conf.setFromPath("ParameterCheck.vectorBool", deque<bool>(5, true));
            conf.setFromPath("ParameterCheck.vectorInt32", vector<int>(8, 5));
            conf.setFromPath("ParameterCheck.vectorDouble", vector<double>(5, 2.7));

            conf.setFromPath("ParameterCheck.valueUInt8", (unsigned char) 2);
            conf.setFromPath("ParameterCheck.valueInt8", (signed char) 'b');
            conf.setFromPath("ParameterCheck.valueChar", (char) 'b');
            conf.setFromPath("ParameterCheck.valueInt8t", (int8_t) 20);
            conf.setFromPath("ParameterCheck.vectorUInt8", vector<unsigned char> (3, 255));
            conf.setFromPath("ParameterCheck.vectorInt8", vector<signed char> (3, 'a'));
            conf.setFromPath("ParameterCheck.vectorCHAR", vector<char> (5, 'c'));

            boost::filesystem::path pathFile("/path/to/file.txt");
            conf.setFromPath("ParameterCheck.filepath", pathFile);

            cout << "conf: \n" << conf << endl;

            outputXml.setFromPath("TextFile.filename", t.file("resultParamCheckXML.xml"));
            outputXml.setFromPath("TextFile.format.Xml");
            outputXml.setFromPath("TextFile.format.Xml.printDataType", true);
            Writer<Hash>::Pointer out1 = Writer<Hash>::create(outputXml);
            out1->write(conf);
            cout << "created XML file: resultParamCheckXML.xml" << endl;

#if defined(_WIN32)
#else
            outputLibConf.setFromPath("TextFile.filename", t.file("resultParamCheckLibConf.libconfig"));
            Writer<Hash>::Pointer out2 = Writer<Hash>::create(outputLibConf);
            out2->write(conf);
            cout << "created LibConfig file: resultParamCheckLibConf.libconfig" << endl;
#endif

            //&&&&&&&&&&&&&&&Read XML from file and create object
            cout << "&&&&& Read XML file and create object &&&&&" << endl;
            Hash configFromXmlFile, checkFromXml;
            checkFromXml.setFromPath("TextFile.filename", t.file("resultParamCheckXML.xml"));
            Reader<Hash>::Pointer inpt1 = Reader<Hash>::create(checkFromXml);
            inpt1->read(configFromXmlFile);
            cout << "Read from file " << checkFromXml.getFromPath<string > ("TextFile.filename") << "; Created object 'configFromXmlFile': \n" << configFromXmlFile << endl;

        }

        //    {
        //      Hash input, configuration;
        //      //cout << ConfigReaderFactory::expectedParameters("TextFile");
        //      input.setFromPath("TextFile.filename", t.file("test.libconfig"));
        //      //input.setFromPath("TextFile.format.LibConfig");
        //      //input.setFromPath("TextFile.format[1].Xml");
        //      ConfigReaderPointer in = ConfigReaderFactory::create(input);
        //      in->read(configuration);
        //      cout << configuration << endl;
        //    }

        {
            cout << "TEST 3A" << endl;
            Hash input, output, configuration;
#if defined(_WIN32)
            cout << "TEST 3A skipped" << endl;
#else
            input.setFromPath("TextFile.filename", t.file("test.conf"));
            input.setFromPath("TextFile.format.LibConfig");
            Reader<Hash>::Pointer in = Reader<Hash>::create(input);
            in->read(configuration);
#endif

            cout << "TEST 3B" << endl;
            output.setFromPath("TextFile.filename", t.file("testConfig.xml"));
            //output.setFromPath("TextFile.format.Xml");
            output.setFromPath("TextFile.format.Xml.indentation", 4);
            Writer<Hash>::Pointer out = Writer<Hash>::create(output);
            out->write(configuration);
        }

        {
            cout << "\nTEST 4A" << endl;
            cout << "Reading file testConfig.xml ... " << endl;
            Hash input, output, configuration;
            input.setFromPath("TextFile.filename", t.file("testConfig.xml"));
            input.setFromPath("TextFile.format.Xml");
            Reader<Hash>::Pointer in = Reader<Hash>::create(input);
            in->read(configuration);
            cout << " ... created configuration object : " << endl;
            cout << configuration << endl;

            cout << "TEST 4B" << endl;
#if defined(_WIN32)
            cout << "TEST 4B skipped" << endl;
#else
            output.setFromPath("TextFile.filename", t.file("resultedLibConfig.conf"));
            output.setFromPath("TextFile.format.LibConfig");
            Writer<Hash>::Pointer out = Writer<Hash>::create(output);
            out->write(configuration);
            cout << " result:  resultedLibConfig.conf" << endl;
#endif
        }

        {
            cout << "\nTEST 5" << endl;
#if defined(_WIN32)
            cout << "TEST 5 skipped" << endl;
#else
            Hash config, configuration;
            config.setFromPath("TextFile.filename", t.file("xmlForReading.xml"));
            Reader<Hash>::Pointer in = Reader<Hash>::create(config);
            in->read(configuration);

            cout << "Reading file " << config.getFromPath<string > ("TextFile.filename") << ". Created object 'configuration': " << endl;
            cout << configuration << endl;

            config.setFromPath("TextFile.filename", t.file("newTestXml.xml"));
            //Default: simple data types will not be written into XML.
            //the following line reflects the default behavior:
            //config.setFromPath("TextFile.format.Xml.printDataType", false); //false or 0

            //In order to write simple data types as an atribute in XML-element,
            //for example: <elname dataType="STRING">Hallo</elname>
            //set the value of printDataType to true :
            config.setFromPath("TextFile.format.Xml.printDataType", true); //true or 1

            Writer<Hash>::Pointer out = Writer<Hash>::create(config);
            out->write(configuration);
            cout << "...from this 'configuration' object write again an XML-document newTestXml.xml" << endl;
#endif
        }

        //        {
        //            cout << "\nTEST 6" << endl;
        //            Hash c;
        //            Hash config;
        //            config.setFromPath("Motor1.name", "Beckhoff Motor");
        //            config.setFromPath("Motor1.Init.position", 10);
        //            config.setFromPath("Motor1.Init.velocity", 3.0);
        //            config.setFromPath("Motor1.array[0]", uint(0));
        //            config.setFromPath("Motor1.array[1]", uint(1));
        //            config.setFromPath("Motor1.array[2]", uint(122));
        //            config.setFromPath("Motor1.array[3]", uint(33));
        //            config.setFromPath("Motor1.array[4]", uint(4));
        //            config.setFromPath("Motor1.Init.sAxisName[0]", "Parrot is an exotic bird");
        //            config.setFromPath("Motor1.Init.sAxisName[1]", "Data processing");
        //            config.setFromPath("Motor1.Init.sAxisName[2]", "Data transmission as an asynchronous task!");
        //            config.setFromPath("Motor1.Init.a.b.number", double(123.45));
        //
        //            cout << "Source config ...\n" << config;
        //
        //            //c.setFromPath("TextFile.format.Plc");
        //            c.setFromPath("TextFile.filename", t.file("test.plc"));
        //            Writer<Hash>::Pointer out = Writer<Hash>::create(c);
        //            out->write(config);
        //
        //            cout << "result: test.plc" << endl;
        //        }
        //
        //        {
        //            cout << "\nTEST 7" << endl;
        //            Hash c;
        //            Hash config;
        //
        //            //c.setFromPath("TextFile.format.Plc");
        //            c.setFromPath("TextFile.filename", t.file("test.plc"));
        //            Reader<Hash>::Pointer in = Reader<Hash>::create(c);
        //            in->read(config);
        //
        //            cout << "Target config ...\n" << config;
        //            cout << "result: test.plc is read" << endl;
        //        }

        string plcstr;
        //        {
        //            cout << "\nTEST 8" << endl;
        //            
        //            Hash config;
        //            config.setFromPath("Motor1.name", "Beckhoff Motor");
        //            config.setFromPath("Motor1.Init.position", 10);
        //            config.setFromPath("Motor1.Init.velocity", 3.0);
        //            config.setFromPath("Motor1.array[0]", uint(0));
        //            config.setFromPath("Motor1.array[1]", uint(1));
        //            config.setFromPath("Motor1.array[2]", uint(122));
        //            config.setFromPath("Motor1.array[3]", uint(33));
        //            config.setFromPath("Motor1.array[4]", uint(4));
        //            config.setFromPath("Motor1.Init.sAxisName[0]", "Parrot is an exotic bird");
        //            config.setFromPath("Motor1.Init.sAxisName[1]", "Data processing");
        //            config.setFromPath("Motor1.Init.sAxisName[2]", "Data transmission as an asynchronous task!");
        //            config.setFromPath("Motor1.Init.a.b.number", double(123.45));
        //
        //            cout << "Source config ...\n" << config;
        //
        //            plcstr.clear();
        //            Hash c;
        //            c.setFromPath("StringStream.format.Plc");
        //            c.setFromPath("StringStream.stringPointer", &plcstr);
        //            Writer<Hash>::Pointer out = Writer<Hash>::create(c);
        //            out->write(config);
        //
        //            cout << "result: plcstr.length() = " << plcstr.length() << endl;
        //        }
        //
        //        {
        //            cout << "\nTEST 9" << endl;
        //
        //            Hash c;
        //            c.setFromPath("StringStream.format.Plc");
        //            c.setFromPath("StringStream.string", plcstr);  // plcstr contains encoded PLC message
        //            Hash config;
        //            Reader<Hash>::Pointer in = Reader<Hash>::create(c);
        //            in->read(config);   // <--- decoding into config
        //
        //            cout << "Target config ...\n" << config;
        //            cout << "result: plcstr read" << endl;
        //        }

        {
            cout << "\nTEST 10. Writing Binary format." << endl;
            Hash c;
            Hash hash;
            hash.setFromPath("Motor1.name", "Beckhoff Motor");
            hash.setFromPath("Motor1.Init.position", 10);
            hash.setFromPath("Motor1.Init.velocity", 3.0);
            hash.setFromPath("Motor1.array[0]", static_cast<unsigned int> (0));
            hash.setFromPath("Motor1.array[1]", static_cast<unsigned int> (1));
            hash.setFromPath("Motor1.array[2]", static_cast<unsigned int> (122));
            hash.setFromPath("Motor1.array[3]", static_cast<unsigned int> (33));
            hash.setFromPath("Motor1.array[4]", static_cast<unsigned int> (4));
            hash.setFromPath("Motor1.Init.sAxisName[0]", "Parrot is an exotic bird");
            hash.setFromPath("Motor1.Init.sAxisName[1]", "Data processing");
            hash.setFromPath("Motor1.Init.sAxisName[2]", "Data transmission as an asynchronous task!");
            hash.setFromPath("Motor1.Init.a.b.number", static_cast<double> (123.45));

            cout << "Source hash is ...\n" << hash << endl;

            plcstr.clear();
            c.setFromPath("StringStream.format.Bin");
            c.setFromPath("StringStream.stringPointer", &plcstr);
            Writer<Hash>::Pointer out = Writer<Hash>::create(c);
            out->write(hash);

            cout << "result: plcstr.length() = " << plcstr.length() << endl;
            cout << "--- End of TEST10" << endl;
        }

        {
            cout << "\nTEST 11. Reading Binary format" << endl;
            Hash c;
            Hash hash;

            c.setFromPath("StringStream.format.Bin");
            c.setFromPath("StringStream.string", plcstr); // plcstr contains encoded PLC message
            Reader<Hash>::Pointer in = Reader<Hash>::create(c);
            in->read(hash); // <--- decoding into hash

            cout << "Target hash ...\n" << hash;
            cout << "--- End of TEST11" << endl;
        }

        {
            cout << "\nTEST 12. Testing function help()" << endl;
            cout << "\n&&& Reader<Hash>::help() &&&" << endl;
            Reader<Hash>::help();

            cout << "\n&&& Reader<Hash>::help(\"StringStream\")  &&&" << endl;
            Reader<Hash>::help("StringStream");

            cout << "\n&&& Reader<Hash>::help(\"TextFile\")  &&&" << endl;
            cout << "EXPECTED PARAMS : \n" << Reader<Hash>::expectedParameters() << endl;
            Reader<Hash>::help("TextFile");

            cout << "\n&&& Reader<Hash>::help(\"TextFile.format\")  &&&" << endl;
            Reader<Hash>::help("TextFile.format");

            cout << "\n&&& Reader<Hash>::help(\"TextFile.format.Xml\")  &&&" << endl;
            Reader<Hash>::help("TextFile.format.Xml");

            Schema sh2 = Writer<Hash>::expectedParameters();
            cout << "\n&&& Writer<Hash>::expectedParameters().help() &&&" << endl;
            sh2.help();

            cout << "\n&&& Writer<Hash>::expectedParameters().help(\"TextFile.format\") &&&" << endl;
            sh2.help("TextFile.format");
            
            cout << "\n&&& Writer<Schema>::help()  &&&" << endl;
            Writer<Schema>::help();
    
            cout << "\n&&& Reader<Schema>::help()  &&&" << endl;
            Reader<Schema>::help();
            
            cout << "\n--- End of TEST12" << endl;
       }

        string binstr;
        {
            cout << "\nTEST 13. Writing Binary format.\nCreate sample hash containing other hashes and vector of hashes" << endl;
            // create a vector of hashes
            vector<Hash> vh;
            for (int count = 0; count < 5; count++) {
                Hash h("index", count, "text", "the text #" + String::toString(count));
                vh.push_back(h);
            }
            // create vector of bool's ==> deque of bool's
            deque<bool> val(10);
            for (deque<bool>::size_type i = 0; i < val.size(); i++) {
                if (i == 1 || i > 5) val[i] = false;
                else val[i] = true;
            }
            Hash bitstr("BitString", val);
            // put it into vector of hashes
            vh.push_back(bitstr);
            // create just one more hash
            vector<string> vs;
            vs.push_back("Sergey Esenov");
            vs.push_back("Nicola Coppola");
            vs.push_back("Andrea Parenti");
            vs.push_back("Burkhard Heisen");
            Hash embed("embedInt", 42, "embedFloat", 42.f, "embedDouble", 42., "names", vs);
            
            // create top level hash we want to serialize...
            Hash hash("Node1", vh, "Node2", embed);

            cout << "Sample hash is ...\n" << hash << endl;

            stringstream ss;
            binstr.clear();
            Hash c;
            c.setFromPath("StringStream.format.Bin");
            c.setFromPath("StringStream.stringPointer", &binstr);
            Writer<Hash>::Pointer out = Writer<Hash>::create(c);
            out->write(hash);

            cout << "result: binstr.length() = " << binstr.length() << endl << endl;
            cout << "--- End of TEST13" << endl;

        }

        {
            cout << "\nTEST 14. Reading Binary format." << endl;
            cout << "input:  binstr.length() = " << binstr.length() << endl;
            Hash hash;
            Hash c("StringStream.string", binstr, "StringStream.format.Bin", Hash());
            Reader<Hash>::Pointer in = Reader<Hash>::create(c);
            in->read(hash); // <--- decoding into hash

            cout << "Resulting hash is ...\n" << hash << endl;
            cout << "--- End of TEST14" << endl;
        }
    } catch (const Exception& e) {
        cout << e;
        RETHROW
    }
    return 0;
}
