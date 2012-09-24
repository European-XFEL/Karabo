/*
 * $Id: testSample.cc 5324 2012-03-01 14:12:02Z heisenb $
 *
 * Author: <your.email@xfel.eu>
 *
 * Created on March,2011,05:54AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <iostream>
#include <cassert>
#include <string>
#include <exfel/util/Test.hh>
#include <exfel/util/Factory.hh>
#include <exfel/core/Module.hh>
#include <exfel/io/Reader.hh>
#include <exfel/io/Writer.hh>
#include "../ModulePy.hh"

using namespace std;
using namespace exfel::util;
using namespace exfel::core;
using namespace exfel::io;

int testSample(int argc, char** argv) {

  try {
    
    Test t;
    TEST_INIT(t, argc, argv);

    cout << t << endl;
    Py_Initialize();
    // TODO Find another module for future testing
    // use t.file("filename"); to access file
    
    {
    cout << "TEST 3" << endl; 
    Schema expectParamModule = Module::expectedParameters();
    cout<<"expectedParameters of Module :\n"<<expectParamModule<<endl;
    
    Schema initParamModule = Module::initialParameters();
    cout<<"initialParameters of Module : \n"<< initParamModule << endl;
    
    Schema expectParamModulePy = Module::expectedParameters("ModulePy");
    cout<<"expectedParameters of ModulePy :\n"<<expectParamModulePy<<endl;
    }
    
    {
    cout << "TEST 4" << endl;
    Hash conf, conf2;
    conf.setFromPath("ModulePy.python.Multiplication.a", 7);
    conf.setFromPath("ModulePy.python.Multiplication.b", 5);
    conf2 = conf.getFromPath<Hash>("ModulePy");
    cout << conf2 <<endl;
   
    //Module::Pointer moduleT;
    //moduleT = Module::create(conf);   
    //moduleT->compute();
    }

  } catch (Exception e) {
    cout << e;
    RETHROW
  } 

  return 0;
}

