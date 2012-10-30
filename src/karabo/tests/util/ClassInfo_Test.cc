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

    Schema ex;
    Schema& tmp = ex.initParameterDescription("test");

    BobbyCar::expectedParameters(tmp);

    Hash c;
    c.setFromPath("BobbyCar.name", "Auto");
    c.setFromPath("BobbyCar.equipment", "Radio");
    c.setFromPath("BobbyCar.shape.Circle.name", "MyCircle");
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