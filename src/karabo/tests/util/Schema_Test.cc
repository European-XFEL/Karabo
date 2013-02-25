/* 
 * File:   Schema_Test.cc
 * Author: irinak
 * 
 * Created on September 28, 2012, 1:14 PM
 */

#include "Schema_Test.hh"
#include "Configurator_Test.hh"

using namespace std;
using namespace karabo::util;
using namespace configurationTest;

CPPUNIT_TEST_SUITE_REGISTRATION(Schema_Test);

Schema_Test::Schema_Test() {
}

Schema_Test::~Schema_Test() {
}


void Schema_Test::testBuildUp() {
    cout << endl << endl;
    try {
        {
        Schema schema = Configurator<Shape>::getSchema("Circle");
        cout << schema << endl;
        CPPUNIT_ASSERT(schema.isAccessInitOnly("shadowEnabled") == true);
        CPPUNIT_ASSERT(schema.isAccessInitOnly("radius") == true);
        CPPUNIT_ASSERT(schema.isLeaf("radius") == true);
        }
        {
        Schema schema("test");
        GraphicsRenderer1::expectedParameters(schema);
        cout << schema << endl;
        }
        //GraphicsRenderer::Pointer p = GraphicsRenderer::create("GraphicsRenderer", Hash("shapes.Circle.radius", 0.1,"color", "red", "antiAlias", true));
        //cout << Configurator<GraphicsRenderer>::getSchema("GraphicsRenderer"); 
        
        
        
        
        
    } catch (karabo::util::Exception e) {
        cout << e << endl;
    }
}

void Schema_Test::setUp() {
    try {
        
        m_schema = Schema("MyTest");
        TestStruct1::expectedParameters(m_schema);
        cout << m_schema;
    } catch (karabo::util::Exception e) {
        cout << e << endl;
    }
}

void Schema_Test::testGetRootName() {
    CPPUNIT_ASSERT(m_schema.getRootName() == "MyTest");
}

