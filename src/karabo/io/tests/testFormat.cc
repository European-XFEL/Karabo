/*
 * $Id: testFormat.cc 6764 2012-07-18 09:29:46Z heisenb $
 *
 * File:   testFormat.cc
 * Author: <irina.kozlova@xfel.eu>
 *
 * Created on September 14, 2010, 02:14PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <fstream>
#include <assert.h>
#include <iosfwd>
#include <boost/any.hpp>

#include "../Reader.hh"
#include "../Writer.hh"
#include "../Format.hh"

using namespace std;
using namespace exfel::io;
using namespace exfel::util;

int testFormat(int argc, char** argv) {
  // we need to know the path to run directory in the package
  string runDir;
  if (argc == 2) {
    runDir = argv[1];
    runDir += "/";
  }

  try {

    {
      cout << "TEST 1" << endl;
      cout << "Reading file run/parseLibConfig_ReadOnly.libconfig ... " << endl;
      Hash input, config;

      input.setFromPath("TextFile.filename", runDir + "parseLibConfig_ReadOnly.libconfig");
      input.setFromPath("TextFile.format.LibConfig");

      Reader<Hash>::Pointer in = Reader<Hash>::create(input);
      in->read(config);
      cout << " ... created configuration object : " << endl;
      cout << config << endl; //for visualisation, can be removed later

      assert(config.has("application") == 1);
      assert(config.has("columns") == 0);
      //'application' is root, containing one element
      assert(config.size() == 1);

      Hash u;
      config.get("application", u);
      //'application' contains element 'window' is true
      assert(u.has("window") == 1);
      //'application' contains element 'misc' is true
      assert(u.has("misc") == 1);

      Hash u1;
      u.get("window", u1);
      //'window' contains 'title' is true
      assert(u1.has("title") == 1);
      //'window' contains 'chapter' is false
      assert(u1.has("chapter") == 0);
      //'window' contains three elements
      assert(u1.size() == 3);

      //Find the number of elements with given key.
      //For map the result will be either 0 (not present) or 1 (present).
      //In the current example: element 'pos' is present
      assert(u1.count("pos") == 1);

      //Find value of key 'title'
      string str = u1.get<string > ("title");
      assert(str == "My Application");

      //Key 'chapter' does not exist in 'window', PARAMETER_EXCEPTION
      try {
        string str1 = u1.get<string > ("chapter");
        cout << "chapter : " << str1 << endl;
      } catch (const CastException&) {
        assert(false);
      } catch (const ParameterException&) {
        assert(true);
      }

      string titleValue = config.getFromPath<string > ("application.window.title");
      assert(titleValue == "My Application");

      int wValue = config.getFromPath<int>("application.window.size.w");
      assert(wValue == 640);
      
      cout << "Assertions checked." << endl;

      //Find value of 'application.misc.bitmask'
      //int bitmaskValue = ((config.get<Hash > ("application")).get<Hash > ("misc")).getNumeric<int>("bitmask");

      //Assume object 'config' is known (as above).
      //Create generatedLibConfig_tmp.conf in libconfig format from this object

      cout<<"Generating LibConfig file from the object ..."<<endl;
      Hash output;
      output.setFromPath("TextFile.filename", runDir + "generatedLibConfig_tmp.libconfig");
      Writer<Hash>::Pointer out = Writer<Hash>::create(output);

      //add complex type:
      cout<<"adding complex Type ..."<<endl;
      config.set("complexType", complex<float>(5.4, 8.2));

      out->write(config);
      cout<<"Result file: generatedLibConfig_tmp.libconfig"<<endl;
    }

    {
      cout << "TEST 2" << endl;
      cout << "Reading file run/parseXML_ReadOnly.xml ... " << endl;
      Hash input, config;

      input.setFromPath("TextFile.filename", runDir + "parseXML_ReadOnly.xml");
      input.setFromPath("TextFile.format.Xml");

      Reader<Hash>::Pointer in = Reader<Hash>::create(input);
      in->read(config);
      cout << " ... created configuration object : " << endl;
      cout << config << endl;

      assert(config.has("application") == 1);
      assert(config.has("columns") == 0);
      //'application' is root, containing one element:
      assert(config.size() == 1);

      Hash u;
      config.get("application", u);
      //'application' contains element 'window' is true
      assert(u.has("window") == 1);
      //'application' contains element 'misc' is true
      assert(u.has("misc") == 1);

      Hash u1;
      u.get("window", u1);
      //'window' contains 'title' is true
      assert(u1.has("title") == 1);
      //'window' contains 'chapter' is false
      assert(u1.has("chapter") == 0);
      //'window' contains three elements
      assert(u1.size() == 3);

      //Find the number of elements with given key.
      //For map the result will be either 0 (not present) or 1 (present).
      //In the current example: element 'pos' is present
      assert(u1.count("pos") == 1);

      //Find value of key 'title'
      string str = u1.get<string > ("title");
      assert(str == "My Application");

      //Key 'chapter' does not exist in 'window', PARAMETER_EXCEPTION
      try {
        string str1 = u1.get<string > ("chapter");
        cout << "chapter : " << str1 << endl;
      } catch (const CastException&) {
        assert(false);
      } catch (const ParameterException&) {
        assert(true);
      }

      string titleValue = config.getFromPath<string > ("application.window.title");
      assert(titleValue == "My Application");

      int wValue = config.getFromPath<int>("application.window.size.w");
      assert(wValue == 640);

      Hash misc;
      u.get("misc", misc);
      assert(string("VECTOR_INT32") == misc.getTypeAsString("vectint"));
      assert(string("VECTOR_STRING") == misc.getTypeAsString("columns"));
      assert(misc.has("emptyelem") == 1);
      cout << "Assertions checked." << endl;
    }
    
    {
        cout << "TEST 3" << endl;
        Schema s = Reader<Hash>::initialParameters("TextFile");
        //cout << s << endl << endl;
        Hash h ("this.is.a.test", 5, "schema", s);
        cout << h;
        Format<Hash>::Pointer f = Format<Hash>::create("Xml");
        string ss = f->serialize(h);
        //cout << ss << endl;
        Hash h1 = f->unserialize(ss);
        //cout << h1 << endl;
        //cout << h1.get<Schema>("schema") << endl;
        
    }
    
    
  } catch (const Exception& e) {
     cout << e;
  }

  return 0;
}
