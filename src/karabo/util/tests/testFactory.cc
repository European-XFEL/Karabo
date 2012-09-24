//
// $Id: testFactory.cc 7034 2012-08-20 12:44:54Z coppola $
//
// Copyright (C) 2009 European XFEL GmbH Hamburg. All rights reserved.
//
// Author: <krzysztof.wrona@xfel.eu>
//
//

#include <iostream>
#include <cassert>
#include <string>

#include "../Exception.hh"
#include "../Schema.hh"
#include "../Factory.hh"
#include "../Test.hh"

#include "Shape.hh"
#include "Vehicle.hh"
#include "Motor.hh"

using namespace std;
using namespace exfel::util;

int testFactory(int argc, char** argv) {

  try {

    Test t;
    TEST_INIT(t, argc, argv);

    cout << t << endl;



    //    {
    //      Schema expected =  Factory<Vehicle>::readParameters();
    //      cout << "READ:" << endl;
    //      cout << expected << endl;
    //    }
    //
    {

      Schema expected = Vehicle::expectedParameters();
      cout << "Vehicle::expectedParameters():" << endl;
      cout << expected << endl;
      cout << "============================================================\n";
    }
    
     {

      Schema expected = Vehicle::expectedParameters("BobbyCar");
      assert(expected.aliasIsOfType<int>("color") == true);
      assert(expected.key2alias<int>("color") == 1);
      assert(expected.alias2key(1) == "color");
      assert(expected.aliasIsOfType<string>("shape.Rectangle.position") == false);
      assert(expected.key2alias<vector<int> >("shape.Rectangle.position").size() == 4);
      //assert(expected.alias2key(vector<int>(4,1)) == "shape.Rectangle.position");
      assert(expected.hasKey("color") == true);
      assert(expected.hasKey("shape.Rectangle.position") == true);
      assert(expected.hasKey("shape.Rectangle") == true);
      assert(expected.hasKey("definitelyNotAValidKey") == false);
      cout << "============================================================\n";
    }

    {
      cout << "START READ:" << endl;
      //Schema master;
      Schema expected = Vehicle::expectedParameters(READ);
      cout << "READ:" << endl;
      cout << expected << endl;
      cout << "END READ:" << endl;
      cout << "============================================================\n";
    }
    //    {
    //      Schema master;
    //      Schema& expected = master.initParameterDescription("Test", WRITE);
    //      Vehicle::expectedParameters(expected);
    //      cout << "WRITE:" << endl;
    //      cout << expected << endl;
    //    }
    //

//    {
//      cout << "TEST" <<endl;
//      Hash c;
//      c.setFromPath("SpecializedBobbyCar.name", "ThisSummer");
//      c.setFromPath("SpecializedBobbyCar.shape.Rectangle.name", "MySpecialRectangle");
//      c.setFromPath("SpecializedBobbyCar.MyCircle.name", "MySpecialCircleInBobbycar");
//      Vehicle::Pointer vP = Vehicle::create(c);
//      vP->start();
//      cout << "============================================================\n";
//    }

    {
      cout << "TEST" <<endl;
      Hash c;
      c.setFromPath("ForwardSpecializedBobbyCar.name", "ThisSummer");
      c.setFromPath("ForwardSpecializedBobbyCar.shape.Rectangle.name", "MySpecialRectangle");
      c.setFromPath("ForwardSpecializedBobbyCar.MyCircle.name", "MySpecialCircleInBobbycar");
      //c.setFromPath("ForwardSpecializedBobbyCar.MyCircle.runningDirection", "forward");
      Vehicle::Pointer vP = Vehicle::create(c);
      vP->start();
      cout << "============================================================\n";
    }
    
    {
      cout << "TEST" <<endl;
      Hash c;
      c.setFromPath("BobbyCar.name", "Summer");
      c.setFromPath("BobbyCar.shape.Rectangle.name", "MyRectangle");
      c.setFromPath("BobbyCar.MyCircle.name", "MyCircleInBobbycar");
      Vehicle::Pointer vP = Vehicle::create(c);
      vP->start();
      cout << "============================================================\n";
    }
    {
      Hash c;
      c.setFromPath("Car.name", "Apple");
      c.setFromPath("Car.idPair", pair<int,int>(1,1));
      Vehicle::Pointer vP = Vehicle::create(c);
      vP->start();
      cout << "============================================================\n";
    }
    
    
    { cout << "----- TESTING FUNCTION 'HELP' -----" <<endl;
      
      cout<<"--------- Test 1. ---------\n";
      
      cout<<"\n Vehicle::help() \n";
      Vehicle::help();
      
      cout<<"\n Vehicle::help(\"BoobbyCar\") \n";
      Vehicle::help("BobbyCar");
      
      cout << "\n Vehicle::help(\"BobbyCar.shape\") \n";
      Vehicle::help("BobbyCar.shape"); //complex element CHOICE
       
      cout << "\n Vehicle::help(\"BobbyCar.MyCircle\")  \n";
      Vehicle::help("BobbyCar.MyCircle"); //SINGLE_ELEMENT MyCircle
    
      cout << "\n Vehicle::help(\"BobbyCar.shape.Circle.name\")  \n";
      Vehicle::help("BobbyCar.shape.Circle.name");
      
      cout << "\n Vehicle::help(\"BobbyCar.shape.Circle.abc\")  \n";
      Vehicle::help("BobbyCar.shape.Circle.abc");
      
      cout << "\n Vehicle::help(\"BobbyCar.shape.abc\")  \n";
      Vehicle::help("BobbyCar.shape.abc");
      
      //other tests
      //Vehicle::help("BobbyCar.shape.Rectangle");
      //Vehicle::help("BobbyCar.shape.Rectangle.position");
      //Vehicle::help("BobbyCar.abc");
      
      cout<<"--------- Test 1b.  Vehicle::help(Motor)  ---------\n";
      Vehicle::help("Motor");
      
      cout<<"--------- Test 2 ---------\n";
      Schema expectParams = Vehicle::expectedParameters(READ|WRITE|INIT);
      expectParams.help("ForwardSpecializedBobbyCar");
      //expectParams.help("SpecializedBobbyCar");
      expectParams.help("BobbyCar");
      expectParams.help("Car");
      expectParams.help("Motor");
      
      cout<<"--------- Test 3 --------\n";
      cout<<"Vehicle::initialParameters().help(BobbyCar)" << endl;
      Schema initParams = Vehicle::initialParameters();
      initParams.help("BobbyCar");
      
      cout<<"--------- Test 4 ---------\n";
      Vehicle::monitorableParameters().help("Motor");
      
      cout<<"--------- Test 5 ---------\n";
      Vehicle::reconfigurableParameters().help("Car");
      
      cout<<"--------- Test 6 ---------\n";
      Schema sh = ConfigurableShape::expectedParameters();
      cout << "ConfigurableShape::expectedParameters():\n" << sh << endl;
      ConfigurableShape::help("Circle");
      ConfigurableShape::help("Rectangle");

      cout<<"--------- Test 7 --------\n";
      cout<<"Vehicle::initialParameters().help(SpecializedBobbyCar)" << endl;
      //initParams = Vehicle::initialParameters();
      //initParams.help("SpecializedBobbyCar");

      cout<<"--------- Test 8 --------\n";
      cout<<"Vehicle::initialParameters().help(ForwardSpecializedBobbyCar)" << endl;
      initParams.help("ForwardSpecializedBobbyCar");

      cout << "============================================================\n";
    }

  } catch (const Exception& e) {
    cout << e;
    RETHROW;
  }

  return 0;
}



