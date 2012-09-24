/*
 * $Id: testIntrospection.cc 4864 2011-12-14 10:18:58Z irinak@DESY.DE $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include "../Test.hh"
#include "Vehicle.hh"
#include "BobbyCar.hh"

using namespace std;
using namespace exfel::util;

int testIntrospection(int argc, char** argv) {

  try {

    Test t;
    TEST_INIT(t, argc, argv);

    cout << t << endl;
    // use t.file("filename"); to access file

    vector<string> keys = Factory<Vehicle>::getRegisteredKeys();

    cout << endl << "Registered keys for Factory<Vehicle> " << endl;
    for (size_t i = 0; i < keys.size(); ++i) {
      cout << keys[i] << endl;
    }
    cout << endl;

    Schema ex;
    Schema& tmp = ex.initParameterDescription("bla");
    BobbyCar::expectedParameters(tmp);
    cout << ex << endl;

    Hash c;
    c.setFromPath("BobbyCar.name", "Winter");
    c.setFromPath("BobbyCar.shape.Rectangle.name", "top rectangle" );
    c.setFromPath("BobbyCar.MyCircle.name", "Circle name" );

    Vehicle::Pointer vp = Vehicle::create(c);
    vp->start();

    {
      string cId = BobbyCar::classInfo().getClassId();
      cout << "BobbyCar::classInfo()->getClassId() : " << cId << endl;
      assert(cId == "BobbyCar");
    }
    {
      string cNamespace = BobbyCar::classInfo().getNamespace();
      cout << "BobbyCar::classInfo()->getNamespace() : " << cNamespace << endl;
      assert(cNamespace == "exfel::util");
    }
    {
      string cName = BobbyCar::classInfo().getClassName();
      cout << "BobbyCar::classInfo()->getClassName() : " << cName << endl;
      assert(cName == "BobbyCar");
    }


    {
      string cId = vp->getClassInfo().getClassId();
      cout << "vp->getClassInfo()->getClassId() : " << cId << endl;
      assert(cId == "BobbyCar");
    }
    {
      string cNamespace = vp->getClassInfo().getNamespace();
      cout << "vp->getClassInfo()->getNamespace() : " << cNamespace << endl;
      assert(cNamespace == "exfel::util");
    }
    {
      string cName = vp->getClassInfo().getClassName();
      cout << "vp->getClassInfo()->getClassName() : " << cName << endl;
      assert(cName == "BobbyCar");
    }


    cout << "BobbyCar::classInfo()->getClassName() : " << BobbyCar::classInfo().getClassName() << endl;
    cout << "BobbyCar::classInfo()->getNamespace() : " << BobbyCar::classInfo().getNamespace() << endl;
    cout << "vp->getClassInfo()->getClassId() : " << vp->getClassInfo().getClassId() << endl;
    cout << "vp->getClassInfo()->getClassName() : " << vp->getClassInfo().getClassName() << endl;
    exfel::util::ClassInfo classInfo = BobbyCar::classInfo();
    cout << "another usage: " << classInfo.getClassId() << endl;


  } catch (Exception e) {
    cout << e;
    RETHROW
  }

  return 0;
}

