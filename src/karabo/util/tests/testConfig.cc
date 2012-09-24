//
// $Id: testConfig.cc 5009 2012-02-01 18:16:40Z heisenb $
//
// Copyright (C) 2009 European XFEL GmbH Hamburg. All rights reserved.
//
// Author: <your.email@xfel.eu>
//
//

#include <iostream>
#include <assert.h>
#include <string>
#include <boost/assign.hpp>

#include "../Exception.hh"
#include "../Schema.hh"

using namespace std;
using namespace exfel::util;
using namespace boost::assign;

int testConfig(int argc, char** argv) {

  cout << "Running Test: testConfig..." << endl;

  
  // Afer refactoring:
  
  // THE WHOLE TEST IS OUTDATED !!
  
//  
//  try {
//
//    // Setting up from constructor
//    {
//      Schema config("Simple", 3.2);
//      // C++ interprets the 3.2 above intrinsically as "double".
//      // If we want to get it back with get<> we have to be accurate in the type.
//      assert(config.get<double>("Simple") == 3.2);
//      // This does not work
//      try {
//        config.get<float>("Simple");
//        assert(true == false);
//      } catch (...) {
//        // Ok, expecting this exception here
//        //RETHROW
//      }
//      // If you want a more intelligent behaviour use this
//      try {
//        float aFloat = config.getNumeric<float>("Simple");
//        cout << aFloat << endl;
//        //assert(aFloat == 3.2);
//      } catch (...) {
//        RETHROW
//      }
//      Schema config1("A.b", "la", "B", 2, "C", vector<double>(5,2.5));
//      cout<< "TEST config1 :"<<endl;
//      cout << config1;
//
//      vector<int> vecint;
//      vecint += 1,5,33,100, 200, 555 ;
//      vector<double> vecdouble;
//      vecdouble += 1.5, 33.1, 200.2, 555.5 ;
//      deque<bool> vecbool;
//      vecbool += true, 1, false, 0 ;
//      Schema config2("VectInt", vecint, "VectDouble", vecdouble, "VectBool", vecbool);
//      cout<< "TEST config2 :"<<endl;
//      cout << config2;
//
//    }
//    
//    Schema config;
//
//    // Creating section as needed (like mkdir -p)
//    config.setFromPath("a.b.c.myInt", 44);
//    assert(config.getFromPath<int>("a.b.c.myInt") == 44);
//
//    config.setFromPath("a.b.c.myDouble", 5.0);
//    config.getFromPath<double>("a.b.c.myDouble");
//
//    config.setFromPath("a.b.c.myFloat", -5.7f);
//    config.getFromPath<float>("a.b.c.myFloat");
//
//    config.setFromPath("a.b.c.myFloatNext", 10.0e11f);
//    config.getFromPath<float>("a.b.c.myFloatNext");
//
////    config.setFromPath("a.b.c.myFloatInfin", 10.0e123f);
////    config.getFromPath<float>("a.b.c.myFloatInfin");
//
//    // Changing seperator in address
//    assert(config.getFromPath<int>("a&b&c&myInt", "&") == 44);
//    assert(config.getFromPath<int>("a/b/c/myInt", "/") == 44);
//    // Any values can be assigned, behavior is always map like -> no overwrite for different keys
//    config.setFromPath("a.b.c.myString", "Parrot");
//    const Schema& c = config.getFromPath<Schema > ("a.b.c");
//    assert(c.has("myInt"));
//    assert(c.has("myString"));
//    assert(c.get<string > ("myString") == "Parrot");
//    // Put empty config c1 under a.b
//    config.setFromPath("a.b.c1"); // => setFromPath("a.b.c1", Schema());
//    assert(c.has("myInt"));
//    assert(c.has("myString"));
//    assert(c.get<string > ("myString") == "Parrot");
//    // Copy of sub-tree
//    config.setFromPath<Schema > ("a.b.c1", config.getFromPath<Schema > ("a.b.c"));
//    // Can use all std::map functionality
//    config.getFromPath<Schema > ("a.b").erase("c");
//    const Schema& b = config.getFromPath<Schema > ("a.b");
//    assert(!b.has("c"));
//    const Schema& c1 = config.getFromPath<Schema > ("a.b.c1");
//    assert(c1.has("myInt"));
//    assert(c1.has("myString"));
//    assert(c1.get<string > ("myString") == "Parrot");
//
//    // Array stuff
//    config.setFromPath("shapes[0].circle.color", "blue");
//    config.setFromPath("shapes[ 1 ].circle.color", "red");
//    config.setFromPath("shapes[next].circle.color", "green");
//    const vector<Schema>& shapes = config.get<vector<Schema> >("shapes");
//    assert(shapes.size() == 3);
//    assert(shapes[0].has("circle"));
//    assert(shapes[0].getFromPath<string > ("circle.color") == "blue");
//    assert(config.getFromPath<string > ("shapes[ 0  ].circle.color") == "blue");
//    assert(config.getFromPath<string > ("shapes[1].circle.color") == "red");
//    assert(config.getFromPath<string > ("shapes[last].circle.color") == "green");
//    config.setFromPath("shapes[0].circle.color", "black");
//    assert(config.getFromPath<string > ("shapes[0].circle.color") == "black");
//    config.setFromPath("shapes[0].pi", 3.14);
//    assert(config.getFromPath<double>("shapes[0].pi") == 3.14);
//    assert(config.getFromPath<string>("shapes[0].circle.color") == "black");
//
//    config.setFromPath("shapes[0]", Schema("bla", "ALARM"));
//    cout << config << endl;
//    assert(config.getFromPath<Schema > ("shapes[0]").has("pi") == false);
//    config.setFromPath("shapes[last].circle.color", "black");
//    assert(config.getFromPath<string > ("shapes[last].circle.color") == "black");
//    assert(config.getFromPath<string > ("shapes[2].circle.color") == "black");
//    assert(config.getFromPath<string > ("shapes[ ].circle.color") == "black");
//    assert(config.getFromPath<string > ("shapes[ LAST ].circle.color") == "black");
//    Schema test = config.getFromPath<Schema > ("shapes[0]");
//    assert(test.has("bla") == true);
//    test.setFromPath("numbers[0]", 1);
//    test.setFromPath("numbers[1]", 3);
//    test.setFromPath("numbers[2]", 5);
//    assert(test.getFromPath < vector<int > >("numbers")[1] == 3);
//    assert(test.getFromPath<int>("numbers[1]") == 3);
//    assert(test.getFromPath<int>("numbers[]") == 5);
//    test.setFromPath("a[0].this.is.a.test", true);
//    assert(test.getFromPath<bool>("a[0].this.is.a.test") == true);
//    test.setFromPath("booleans[0]", true);
//    test.setFromPath("booleans[1]", true);
//    test.setFromPath("booleans[2]", false);
//    assert(test.getFromPath<bool>("booleans[1]") == true);
//    assert(test.getFromPath<bool>("booleans[2]") == false);
//    deque<bool> deqBool = test.getFromPath < deque<bool> >("booleans");
//    for (size_t i = 0; i < deqBool.size()-1; ++i) {
//      assert(deqBool[i] == true);
//    }
//    try {
//      Schema("this.is.not.supported", vector<bool>(3, true));
//      assert(true == false);
//    } catch (const NotSupportedException& e) {
//      // Ok, expected
//    }
//    test.setFromPath("Application.modules[0]");
//    Schema res = test.getFromPath<Schema>("Application.modules[0]");
//    test.setFromPath("Application.new[next].Filter.input");
//    //cout << res;
//
//  } catch (const Exception& e) {
//    cout << e;
//  }
  return 0;
}
