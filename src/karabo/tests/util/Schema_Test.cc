/* 
 * File:   Schema_Test.cc
 * Author: irinak
 * 
 * Created on September 28, 2012, 1:14 PM
 */

#include "Schema_Test.hh"

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
        GraphicsRenderer::Pointer p = GraphicsRenderer::create("GraphicsRenderer", Hash("shapes.Circle.radius", 0.5, "color", "red", "antiAlias", "true"));
        //cout << Configurator<GraphicsRenderer>::getSchema("GraphicsRenderer"); 

    } catch (karabo::util::Exception e) {
        cout << e << endl;
    }
}

void Schema_Test::setUp() {
    try {
        m_schema = Schema("MyTest", Schema::AssemblyRules(READ|WRITE|INIT));
        TestStruct1::expectedParameters(m_schema);
        cout << m_schema;
    } catch (karabo::util::Exception e) {
        cout << e << endl;
    }
}

void Schema_Test::testGetRootName() {
    CPPUNIT_ASSERT(m_schema.getRootName() == "MyTest");
}

void Schema_Test::testPerKeyFunctionality() {

    std::vector<std::string> keys = m_schema.getParameters();
    
    for (size_t i = 0; i < keys.size(); ++i) {

        if (keys[i] == "exampleKey1"){
            int nodeType = m_schema.getNodeType(keys[i]);
            CPPUNIT_ASSERT(nodeType == 1);
            
            int assignment = m_schema.getAssignment(keys[i]);
            CPPUNIT_ASSERT(assignment == 0);
            CPPUNIT_ASSERT(assignment == Schema::OPTIONAL_PARAM);
            
            bool hasAssignment = m_schema.hasAssignment(keys[i]);
            CPPUNIT_ASSERT(hasAssignment == true);
            CPPUNIT_ASSERT(m_schema.isAssignmentOptional(keys[i]) == true);
            
            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == true);
            string defaultValue = m_schema.getDefaultValue<string>(keys[i]);
            CPPUNIT_ASSERT(defaultValue == "Some default string");
            
            CPPUNIT_ASSERT(m_schema.hasAccessMode(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAccessReconfigurable(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.getAccessMode(keys[i]) == 4);
        }
        
        if (keys[i] == "exampleKey2"){
            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == true);     
            int defaultValue = m_schema.getDefaultValue<int>(keys[i]);
            CPPUNIT_ASSERT(defaultValue == -10);
            
            CPPUNIT_ASSERT(m_schema.hasAccessMode(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAccessInitOnly(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.getAccessMode(keys[i]) == 1);
        }
        
        if (keys[i] == "exampleKey3"){
            CPPUNIT_ASSERT(m_schema.getNodeType(keys[i]) == 1);
            CPPUNIT_ASSERT(m_schema.getAssignment(keys[i]) == 1);
            CPPUNIT_ASSERT(m_schema.getAssignment(keys[i]) == Schema::MANDATORY_PARAM);
            CPPUNIT_ASSERT(m_schema.isAssignmentMandatory(keys[i]) == true);
            
            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == false);
        }

        if (keys[i] == "exampleKey4"){
            CPPUNIT_ASSERT(m_schema.getNodeType(keys[i]) == 1);
            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == false);
            
            CPPUNIT_ASSERT(m_schema.getAssignment(keys[i]) == 2);
            CPPUNIT_ASSERT(m_schema.getAssignment(keys[i]) == Schema::INTERNAL_PARAM);
            CPPUNIT_ASSERT(m_schema.isAssignmentInternal(keys[i]) == true);
            
            CPPUNIT_ASSERT(m_schema.hasAccessMode(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAccessInitOnly(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.getAccessMode(keys[i]) == 1);
        }
        
        if (keys[i] == "exampleKey5"){
            CPPUNIT_ASSERT(m_schema.getNodeType(keys[i]) == 1);
            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == true);
            long long int defaultValue = m_schema.getDefaultValue<long long int>(keys[i]);
            CPPUNIT_ASSERT(defaultValue == 1442244);
            
            CPPUNIT_ASSERT(m_schema.hasAssignment(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAssignmentOptional(keys[i]) == true);
            
            CPPUNIT_ASSERT(m_schema.hasAccessMode(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAccessReadOnly(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.getAccessMode(keys[i]) == 2);
            CPPUNIT_ASSERT(m_schema.getAccessMode(keys[i]) == READ);
        }
    }
}
