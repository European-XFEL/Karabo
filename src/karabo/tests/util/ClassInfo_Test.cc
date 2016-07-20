/* 
 * $Id$
 * 
 * Author: <irina.kozlova@xfel.eu>
 * 
 * Created on October 29, 2012, 12:38 PM
 */

#include "ClassInfo_Test.hh"

using namespace std;
using namespace karabo::util;

CPPUNIT_TEST_SUITE_REGISTRATION(ClassInfo_Test);


ClassInfo_Test::ClassInfo_Test() {
}


ClassInfo_Test::~ClassInfo_Test() {
}


void ClassInfo_Test::setUp() {

    Hash c;
    c.setFromPath("BobbyCar.name", "Auto");
    c.setFromPath("BobbyCar.equipment", "Radio");

    //Rectangle>>>>>>>>
    c.setFromPath("BobbyCar.shape.Rectangle.name", "top rectangle");
    c.setFromPath("BobbyCar.shape.Rectangle.a", 2.0);
    c.setFromPath("BobbyCar.shape.Rectangle.b", 3.0);
    c.setFromPath("BobbyCar.shape.Rectangle.position", 11.5);
    //<<<<<<<<Rectangle

    //Circle >>>>>
    //c.setFromPath("BobbyCar.shape.Circle.name", "top circle");
    //c.setFromPath("BobbyCar.shape.Circle.radius", 15);
    //<<<<<< Circle

    c.setFromPath("BobbyCar.MyCircle.name", "MyCircleInBobbycar");

    vp = Vehicle::create(c);
    vp->start();

}


void ClassInfo_Test::testGetClassId() {

    string cId = vp->getClassInfo().getClassId();
    CPPUNIT_ASSERT(cId == "BobbyCar");

}


void ClassInfo_Test::testGetClassName() {

    string cName = vp->getClassInfo().getClassName();
    CPPUNIT_ASSERT(cName == "BobbyCar");

}


void ClassInfo_Test::testGetNamespace() {

    string cNamespace = vp->getClassInfo().getNamespace();
    CPPUNIT_ASSERT(cNamespace == "karabo::util");

}


void ClassInfo_Test::testAnotherUsage() {

    ClassInfo classInfo = BobbyCar::classInfo();
    string classId = classInfo.getClassId();
    string className = classInfo.getClassName();
    string nameSpace = classInfo.getNamespace();

    CPPUNIT_ASSERT(classId == "BobbyCar");
    CPPUNIT_ASSERT(className == "BobbyCar");
    CPPUNIT_ASSERT(nameSpace == "karabo::util");

}


void ClassInfo_Test::testGetRegisteredKeys() {

    vector<string> keys = Factory<Vehicle>::getRegisteredKeys();
    cout << "\n Registered keys for Factory<Vehicle> " << endl;
    for (size_t i = 0; i < keys.size(); ++i) {
        cout << keys[i] << endl;
    }
    CPPUNIT_ASSERT(keys[0] == "BobbyCar");

}


void ClassInfo_Test::testExpectedParameters() {
    Schema expected = Vehicle::expectedParameters();
    vector<string> keys = expected.getKeysAsVector();
    CPPUNIT_ASSERT(keys[0] == "BobbyCar");
}