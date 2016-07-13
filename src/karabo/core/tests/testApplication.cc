/*
 * $Id: testApplication.cc 4862 2011-12-13 17:19:32Z irinak@DESY.DE $
 *
 * File:   testApplication.cc
 * Author: <irina.kozlova@xfel.eu>
 *
 * Created on October 15, 2010, 3:19 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <fstream>
#include <assert.h>
#include <iosfwd>

#include <exfel/io/Reader.hh>
#include <exfel/io/Writer.hh>
#include <exfel/util/PluginLoader.hh>
#include "../Application.hh"

using namespace std;
using namespace exfel::io;
using namespace exfel::util;
using namespace exfel::core;


int testApplication(int argc, char** argv) {
    // we need to know the path to run directory in the package
    string runDir;
    if (argc == 2) {
        runDir = argv[1];
        runDir += "/";
    }

    try {
        cout << " TEST 1 " << endl;

        Hash input;
        input.setFromPath("TextFile.filename", runDir + "expected.xsd");
        input.setFromPath("TextFile.format.Xsd.indentation", 1);
        Writer<Schema>::Pointer out = Writer<Schema>::create(input);

        cout << "ClassInfo ClassId:   " << out->getClassInfo().getClassId() << endl;
        cout << "ClassInfo ClassName: " << out->getClassInfo().getClassName() << endl;
        cout << "ClassInfo Namespace: " << out->getClassInfo().getNamespace() << endl;
        cout << "ClassInfo LogCategory: " << out->getClassInfo().getLogCategory() << endl;

        cout << "2 ClassInfo ClassId:   " << Writer<Schema>::classInfo().getClassId() << endl;
        cout << "2 ClassInfo ClassName: " << Writer<Schema>::classInfo().getClassName() << endl;
        cout << "2 ClassInfo Namespace: " << Writer<Schema>::classInfo().getNamespace() << endl;
        cout << "2 ClassInfo LogCategory: " << Writer<Schema>::classInfo().getLogCategory() << endl;

        Schema expected = Application::expectedParameters("Application");
        cout << "see intermediate representation in: fileExpected.txt" << endl;
        ofstream myfile;
        myfile.open("fileExpected.txt");
        myfile << expected;
        myfile.close();

        out->write(expected);
        cout << "Result: expected.xsd" << endl;

        //or just run it with all default values (without 'indentation', fileExpected.txt, ...)
        //ConfigWriterPointer out = ConfigWriterFactory::create(Hash("TextFile.filename", runDir + "expected.xsd"));
        //out->write(ApplicationFactory::expectedParameters("Application"));

        cout << ">>>>>> TESTS for function help() <<<<<<<" << endl;

        Application::help();
        Application::help("Application");
        Application::help("Application.Logger"); //.Logger (SINGLE_ELEMENT)
        Application::help("Application.modules"); //.modules (NON_EMPTY_LIST)

        Application::help("Application.Logger.appenders"); //.appenders (LIST)

        Application::help("Application.Logger.appenders.Ostream"); //Ostream
        Application::help("Application.Logger.appenders.Ostream.layout"); //Ostream  .layout (CHOICE)

        Application::help("Application.Logger.appenders.Ostream.abc"); //"No such element 'abc' exists."

        Application::help("Application.Logger.appenders.abc"); //"The following element 'abc ' is not a sub-element of 'appenders' "

    } catch (const Exception& e) {
        cout << e;
        return 1;
    }
    return 0;
}
