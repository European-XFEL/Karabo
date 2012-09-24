/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include "../Test.hh"

#include "Motor.hh"
#include "Vehicle.hh"


using namespace std;
using namespace exfel::util;

int testMotor(int argc, char** argv) {

  try {

    Test t;
    TEST_INIT(t, argc, argv);

    cout << t << endl;
    // use t.file("filename"); to access file
    {

//      MasterConfig expected = Vehicle::expectedParameters(INIT);

      //cout << "Vehicle initial parameters\n" << Vehicle::initialParameters();

      Hash c;
      c.setFromPath("Motor.position", 0);
      c.setFromPath("Motor.offset", 20);
      Vehicle::Pointer v = Vehicle::create(c);
      v->start();
      Hash rc;
      rc.setFromPath("Motor.velocity", (float)2.5);
      v->reconfigure(rc);
      
      v->start();




  //    Vehicle::expectedParameters(expected);
      //cout << expected << endl;
    }




  } catch (Exception e) {
    cout << e;
    RETHROW
  }

  return 0;
}

