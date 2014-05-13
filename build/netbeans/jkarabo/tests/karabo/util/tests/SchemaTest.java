/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests;

import karabo.util.AccessType;
import karabo.util.ClassInfo;
import static karabo.util.ClassInfo.KARABO_REGISTER_FOR_CONFIGURATION;
import karabo.util.Configurator;
import karabo.util.Hash;
import karabo.util.Schema;
import karabo.util.Units;
import karabo.util.tests.configurationtestclasses.Circle;
import karabo.util.tests.configurationtestclasses.EditableCircle;
import karabo.util.tests.configurationtestclasses.GraphicsRenderer;
import karabo.util.tests.configurationtestclasses.GraphicsRenderer1;
import karabo.util.tests.configurationtestclasses.OtherSchemaElements;
import karabo.util.tests.configurationtestclasses.Rectangle;
import karabo.util.tests.configurationtestclasses.Shape;
import karabo.util.tests.configurationtestclasses.TestStruct1;
import karabo.util.types.Types;
import karabo.util.vectors.VectorInteger;
import karabo.util.vectors.VectorString;
import org.junit.After;
import org.junit.AfterClass;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 *
 * @author Sergey Esenov <serguei.essenov at xfel.eu>
 */
public class SchemaTest {

    private Schema m_schema;

    public SchemaTest() {
    }

    @BeforeClass
    public static void setUpClass() {
    }

    @AfterClass
    public static void tearDownClass() {
    }

    @Before
    public void setUp() {
        try {
            m_schema = new Schema("MyTest");
            TestStruct1.expectedParameters(m_schema);
        } catch (Exception e) {
            System.out.println("Exception in test setUp(): " + e);
        }
    }

    @After
    public void tearDown() {
    }
    // TODO add test methods here.
    // The methods must be annotated with annotation @Test. For example:
    //
    // @Test
    // public void hello() {}

    @Test
    public void testBuildUp() {
        try {
            { // Registration of "Circle" class is explicit: call Configurator static method
                //                          Derived class
                KARABO_REGISTER_FOR_CONFIGURATION(Shape.class, Circle.class);
                //                                     Base class    classId
                Schema schema = Configurator.getSchema(Shape.class, "Circle");
                // or ...
                //                                            Derived class           Base class    classId
                // Schema schema = Configurator.registerClass(Circle.class).getSchema(Shape.class, "Circle");
                assertTrue(schema.isAccessInitOnly("shadowEnabled"));
                assertTrue(schema.isAccessInitOnly("radius"));
                assertTrue(schema.isLeaf("radius"));
            }
            { // Registration of "GraphicsRenderer1" class is implicit: call GraphicsRenderer1 static method
                Schema schema = new Schema("test");
                GraphicsRenderer1.expectedParameters(schema);
                assertTrue(schema.isAccessInitOnly("shapes.circle.radius"));
                assertTrue(schema.isLeaf("shapes.circle.radius"));
            }
            { // Registration of "GraphicsRenderer" class is explicit: call ClassInfo static method
                KARABO_REGISTER_FOR_CONFIGURATION(GraphicsRenderer.class);
                GraphicsRenderer gr = GraphicsRenderer.create(GraphicsRenderer.class, "GraphicsRenderer", new Hash("shapes.Circle.radius", 0.5, "color", "red", "antiAlias", "true"));
                Schema schema = Configurator.getSchema(GraphicsRenderer.class, "GraphicsRenderer");
                if (schema == null) {
                    System.out.println("schema is null!");
                }
//                    else {
//                    System.out.println(schema);
//                }
            }

        } catch (Exception e) {
            System.out.println("testBuildUp: " + e);
        }
    }

    @Test
    public void testPaths() {
        KARABO_REGISTER_FOR_CONFIGURATION(Shape.class, EditableCircle.class); // intemediate classes auto-registered
        KARABO_REGISTER_FOR_CONFIGURATION(Shape.class, Rectangle.class);

        KARABO_REGISTER_FOR_CONFIGURATION(GraphicsRenderer.class);
        Schema schema = GraphicsRenderer.getSchema(GraphicsRenderer.class, "GraphicsRenderer");

        VectorString paths = schema.getPaths();
        //System.out.println("*** testPaths UNIT TEST => paths: " + paths);
        assertTrue(paths.contains("antiAlias"));
        assertTrue(paths.contains("color"));
        assertTrue(paths.contains("bold"));
        assertTrue(paths.contains("shapes.Shape.shadowEnabled"));
        assertTrue(paths.contains("shapes.Circle.shadowEnabled"));
        assertTrue(paths.contains("shapes.Circle.radius"));
        assertTrue(paths.contains("shapes.EditableCircle.shadowEnabled"));
        assertTrue(paths.contains("shapes.EditableCircle.radius"));
        assertTrue(paths.contains("shapes.Rectangle.shadowEnabled"));
        assertTrue(paths.contains("shapes.Rectangle.a"));
        assertTrue(paths.contains("shapes.Rectangle.b"));
        assertTrue(paths.contains("version"));
    }

    @Test
    public void testGetRequiredAccessLevel() {
        KARABO_REGISTER_FOR_CONFIGURATION(Shape.class, EditableCircle.class);
        KARABO_REGISTER_FOR_CONFIGURATION(Shape.class, Rectangle.class);

        KARABO_REGISTER_FOR_CONFIGURATION(GraphicsRenderer.class);
        Schema schema = GraphicsRenderer.getSchema(GraphicsRenderer.class, "GraphicsRenderer");
        
        assertTrue(schema.getRequiredAccessLevel("shapes") == Schema.AccessLevel.EXPERT);
        //all sub-elements of Node-element 'shapes' will have EXPERT level: 
        assertTrue(schema.getRequiredAccessLevel("shapes.Circle.shadowEnabled") == Schema.AccessLevel.EXPERT);
        assertTrue(schema.getRequiredAccessLevel("shapes.Circle") == Schema.AccessLevel.EXPERT);
        assertTrue(schema.getRequiredAccessLevel("shapes.Rectangle.b") == Schema.AccessLevel.EXPERT);

        //but sub-element 'shapes.Rectangle.a' with higher level will keep its ADMIN level
        assertTrue(schema.getRequiredAccessLevel("shapes.Rectangle.a") == Schema.AccessLevel.ADMIN);

        assertTrue(schema.getRequiredAccessLevel("antiAlias") == Schema.AccessLevel.EXPERT);
        assertTrue(schema.getRequiredAccessLevel("color") == Schema.AccessLevel.USER);

        //check requiredAccesLevel set on leaves-elements in expectedParameters
        assertTrue(m_schema.getRequiredAccessLevel("exampleKey1") == Schema.AccessLevel.USER);
        assertTrue(m_schema.getRequiredAccessLevel("exampleKey2") == Schema.AccessLevel.OPERATOR);
        assertTrue(m_schema.getRequiredAccessLevel("exampleKey3") == Schema.AccessLevel.EXPERT);
        assertTrue(m_schema.getRequiredAccessLevel("exampleKey4") == Schema.AccessLevel.ADMIN);

        //default for readOnly element - OBSERVER
        assertTrue(m_schema.getRequiredAccessLevel("exampleKey5") == Schema.AccessLevel.OBSERVER);

        //default for reconfigurable element - USER
        assertTrue(m_schema.getRequiredAccessLevel("sampleKey") == Schema.AccessLevel.USER);

        Schema ose = new Schema("OtherSchemaElements");
        OtherSchemaElements.expectedParameters(ose);

        //check default requiredAccessLevel by elements : slot, path, vector, image
//        assertTrue(ose.getRequiredAccessLevel("slotTest") == Schema.AccessLevel.USER); //SLOT    <-- Support not implemented yet!!!
        assertTrue(ose.getRequiredAccessLevel("filename") == Schema.AccessLevel.USER); //reconfigurable PATH
        assertTrue(ose.getRequiredAccessLevel("testfile") == Schema.AccessLevel.OBSERVER); //readOnly PATH
        assertTrue(ose.getRequiredAccessLevel("vecIntReconfig") == Schema.AccessLevel.USER); //reconfigurable VECTOR_INT32
        assertTrue(ose.getRequiredAccessLevel("vecInt") == Schema.AccessLevel.OBSERVER); //readOnly VECTOR_INT32
        assertTrue(ose.getRequiredAccessLevel("vecBool") == Schema.AccessLevel.USER); //init VECTOR_BOOL
//        assertTrue(ose.getRequiredAccessLevel("image") == Schema.AccessLevel.OBSERVER); //IMAGE  <-- Support not implemented yet!!!
    }

    @Test
    public void testSetRequiredAccessLevel() {
        assertTrue(true);
    }

    @Test
    public void testGetRootName() {
        assertTrue("MyTest".equals(m_schema.getRootName()));
    }

    @Test
    public void testGetTags() {
        assertTrue("hardware".equals(m_schema.getTags("exampleKey1").get(0)));
        assertTrue("poll".equals(m_schema.getTags("exampleKey1").get(1)));
        assertTrue("hardware".equals(m_schema.getTags("exampleKey2").get(0)));
        assertTrue("poll".equals(m_schema.getTags("exampleKey2").get(1)));
        assertTrue("hardware".equals(m_schema.getTags("exampleKey3").get(0)));
        assertTrue("set".equals(m_schema.getTags("exampleKey3").get(1)));
        assertTrue("software".equals(m_schema.getTags("exampleKey4").get(0)));
        assertTrue("h/w".equals(m_schema.getTags("exampleKey5").get(0)));
        assertTrue("d.m.y".equals(m_schema.getTags("exampleKey5").get(1)));
    }

    @Test
    public void testGetNodeType() {
        Schema.NodeType nodeType = m_schema.getNodeType("exampleKey1");
        assertTrue(nodeType == Schema.NodeType.LEAF);

        assertTrue(m_schema.getNodeType("exampleKey5") == Schema.NodeType.LEAF);
    }

    @Test
    public void testGetValueType() {
        Types.ReferenceType valueType = m_schema.getValueType("exampleKey1");
        assertTrue(valueType == Types.ReferenceType.STRING);

        assertTrue(m_schema.getValueType("exampleKey2") == Types.ReferenceType.INT32);
        assertTrue(m_schema.getValueType("exampleKey3") == Types.ReferenceType.INT32);
        assertTrue(m_schema.getValueType("exampleKey4") == Types.ReferenceType.FLOAT);
        assertTrue(m_schema.getValueType("exampleKey5") == Types.ReferenceType.INT64);
    }

    @Test
    public void testKeyHasAlias() {
        assertFalse(m_schema.keyHasAlias("exampleKey1"));
        assertTrue(m_schema.keyHasAlias("exampleKey2"));
        assertTrue(m_schema.keyHasAlias("exampleKey3"));
        assertTrue(m_schema.keyHasAlias("exampleKey4"));
        assertTrue(m_schema.keyHasAlias("exampleKey5"));
    }

    @Test
    public void testAliasHasKey() {
        assertTrue(m_schema.aliasHasKey(10) == true);
        assertTrue(m_schema.aliasHasKey(5.5) == true);
        assertTrue(m_schema.aliasHasKey("exampleAlias4") == true);

        VectorInteger vecIntAlias = new VectorInteger();
        vecIntAlias.add(10);
        vecIntAlias.add(20);
        vecIntAlias.add(30);
        assertTrue(m_schema.aliasHasKey(vecIntAlias));

        assertFalse(m_schema.aliasHasKey(7));
    }

    @Test
    public void testGetKeyFromAlias() {
        assertTrue("exampleKey2".equals(m_schema.getKeyFromAlias(10)));
        assertTrue("exampleKey3".equals(m_schema.getKeyFromAlias(5.5)));
        assertTrue("exampleKey4".equals(m_schema.getKeyFromAlias("exampleAlias4")));

        VectorInteger vecIntAlias = new VectorInteger();
        vecIntAlias.add(10);
        vecIntAlias.add(20);
        vecIntAlias.add(30);
        assertTrue("exampleKey5".equals(m_schema.getKeyFromAlias(vecIntAlias)));
    }

    @Test
    public void testGetAliasAsString() {
        assertTrue("10".equals(m_schema.getAliasAsString("exampleKey2")));
        assertTrue("5.5".equals(m_schema.getAliasAsString("exampleKey3")));
        assertTrue("exampleAlias4".equals(m_schema.getAliasAsString("exampleKey4")));

        String aliasStr = m_schema.getAliasAsString("exampleKey5");
        assertTrue("10,20,30".equals(aliasStr));
    }

    @Test
    public void testGetAccessMode() {
        AccessType accessModeKey1 = m_schema.getAccessMode("exampleKey1");
        assertTrue(accessModeKey1 == AccessType.WRITE);

        assertTrue(m_schema.getAccessMode("exampleKey2") == AccessType.INIT);
        assertTrue(m_schema.getAccessMode("exampleKey3") == AccessType.WRITE);
        assertTrue(m_schema.getAccessMode("exampleKey4") == AccessType.INIT);
        assertTrue(m_schema.getAccessMode("exampleKey5") == AccessType.READ);
    }

    @Test
    public void testGetAssignment() {
        Schema.AssignmentType assignment = m_schema.getAssignment("exampleKey1");
        assertTrue(assignment == Schema.AssignmentType.OPTIONAL_PARAM);

        assertTrue(m_schema.getAssignment("exampleKey2") == Schema.AssignmentType.OPTIONAL_PARAM);
        assertTrue(m_schema.getAssignment("exampleKey3") == Schema.AssignmentType.MANDATORY_PARAM);
        assertTrue(m_schema.getAssignment("exampleKey4") == Schema.AssignmentType.INTERNAL_PARAM);
        assertTrue(m_schema.getAssignment("exampleKey5") == Schema.AssignmentType.OPTIONAL_PARAM);
    }

    @Test
    public void testGetOptions() {
        VectorString options = m_schema.getOptions("exampleKey1");
        assertTrue("Radio".equals(options.get(0)));
        assertTrue("Air Condition".equals(options.get(1)));
        assertTrue("Navigation".equals(options.get(2)));

        assertTrue("5".equals(m_schema.getOptions("exampleKey2").get(0)));
        assertTrue("25".equals(m_schema.getOptions("exampleKey2").get(1)));
        assertTrue("10".equals(m_schema.getOptions("exampleKey2").get(2)));

        assertTrue("1.11".equals(m_schema.getOptions("exampleKey4").get(0)));
        assertTrue("-2.22".equals(m_schema.getOptions("exampleKey4").get(1)));
        assertTrue("5.55".equals(m_schema.getOptions("exampleKey4").get(2)));
    }

    @Test
    public void testGetDefaultValue() {
        String defaultValueKey1 = m_schema.getDefaultValue("exampleKey1");
        assertTrue("Navigation".equals(defaultValueKey1));

        int defaultValueKey2 = m_schema.getDefaultValue("exampleKey2");
        assertTrue(defaultValueKey2 == 10);
//        String defaultValueAsString2 = m_schema.getDefaultValue("exampleKey2");
//        assertTrue("10".equals(defaultValueAsString2));

        long defaultValue = m_schema.getDefaultValue("exampleKey5");
        assertTrue(defaultValue == 1442244);
//        String defaultValueAsString5 = m_schema.getDefaultValue("exampleKey5");
//        assertTrue("1442244".equals(defaultValueAsString5));

//        assertTrue(m_schema.getDefaultValue("sampleKey") == 10); // Was set from string, but maintains correct data typing

        assertTrue(m_schema.getDefaultValue("sampleKey") == "10");
//        assertTrue(m_schema.getDefaultValue("sampleKey") == 10);

        //readOnly element has default value which is equal to string "0"
        //(does not matter what is the type of the element itself)
        assertTrue(m_schema.hasDefaultValue("sampleKey2") == true);
        assertTrue(m_schema.getDefaultValue("sampleKey2") == "0");
    }

    @Test
    public void testGetAllowedStates() {
        VectorString allowedStates = m_schema.getAllowedStates("exampleKey3");
        assertTrue("AllOk.Started".equals(allowedStates.get(0)));
        assertTrue("AllOk.Stopped".equals(allowedStates.get(1)));
        assertTrue("AllOk.Run.On".equals(m_schema.getAllowedStates("exampleKey3").get(2)));
        assertTrue("NewState".equals(m_schema.getAllowedStates("exampleKey3").get(3)));
    }

    @Test
    public void testGetUnit() {
        Units.Unit units = m_schema.getUnit("exampleKey2");
        assertTrue(units == Units.Unit.METER);

        String unitName = m_schema.getUnitName("exampleKey2");
        assertTrue("meter".equals(unitName));

        String unitSymbol = m_schema.getUnitSymbol("exampleKey2");
        assertTrue("m".equals(unitSymbol));
    }

    @Test
    public void testGetMetricPrefix() {
        assertTrue(m_schema.getMetricPrefix("exampleKey2") == Units.MetricPrefix.MILLI);
        assertTrue("milli".equals(m_schema.getMetricPrefixName("exampleKey2")));
        assertTrue("m".equals(m_schema.getMetricPrefixSymbol("exampleKey2")));
    }

    @Test
    public void testGetMinIncMaxInc() {

        int minInc = m_schema.getMinInc("exampleKey2");
//        String minIncStr = m_schema.getMinInc("exampleKey2");
        assertTrue(minInc == 5);
//        assertTrue(minIncStr == "5");


        int maxInc = m_schema.getMaxInc("exampleKey2");
//        String maxIncStr = m_schema.getMaxInc("exampleKey2");
        assertTrue(maxInc == 25);
//        assertTrue(maxIncStr == "25");
    }

    @Test
    public void testGetMinExcMaxExc() {

        int minExc = m_schema.getMinExc("exampleKey3");
//        String minExcStr = m_schema.getMinExc("exampleKey3");
        assertTrue(minExc == 10);
//        assertTrue(minExcStr == "10");

        int maxExc = m_schema.getMaxExc("exampleKey3");
//        String maxExcStr = m_schema.getMaxExc("exampleKey3");
        assertTrue(maxExc == 20);
//        assertTrue(maxExcStr == "20");

    }

    @Test
    public void testGetAlarmLowAlarmHigh() {
        assertTrue((int)m_schema.getAlarmLow("exampleKey5") == -20);
        assertTrue((int)m_schema.getAlarmHigh("exampleKey5") == 20);
    }

    @Test
    public void testGetWarnLowWarnHigh() {
        assertTrue((int)m_schema.getWarnLow("exampleKey5") == -10);
        assertTrue((int)m_schema.getWarnHigh("exampleKey5") == 10);
    }

    @Test
    public void testHasAlarmWarn() {
        assertTrue(m_schema.hasWarnLow("exampleKey5"));
        assertTrue(m_schema.hasWarnHigh("exampleKey5"));
        assertTrue(m_schema.hasAlarmLow("exampleKey5"));
        assertTrue(m_schema.hasAlarmHigh("exampleKey5"));
    }

    @Test
    public void testArchivePolicy() {
        Schema sch = new Schema("OtherSchemaElements");
        OtherSchemaElements.expectedParameters(sch);

        assertTrue(sch.hasArchivePolicy("testfile"));
        assertTrue(sch.hasArchivePolicy("vecInt"));
        assertTrue(sch.hasArchivePolicy("vecDouble"));

        assertTrue(sch.getArchivePolicy("testfile") == Schema.ArchivePolicy.EVERY_10MIN);
        assertTrue(sch.getArchivePolicy("vecInt") == Schema.ArchivePolicy.EVERY_EVENT);
        assertTrue(sch.getArchivePolicy("vecDouble") == Schema.ArchivePolicy.NO_ARCHIVING);

    }

    @Test
    public void testClassInfo() {
        {
            TestStruct1 ts1 = new TestStruct1();
            assertTrue("TestStruct1".equals(ts1.getClassName()));
            assertTrue("karabo.util.tests.configurationtestclasses".equals(ts1.getNamespace()));
            assertTrue("TestStruct1".equals(ts1.getClassId()));
            assertTrue("3.2".equals(ts1.getVersion()));
        }
        {
            ClassInfo ci = ClassInfo.classInfo(TestStruct1.class);
            assertTrue("TestStruct1".equals(ci.getClassName()));
            assertTrue("karabo.util.tests.configurationtestclasses".equals(ci.getNamespace()));
            assertTrue("TestStruct1".equals(ci.getClassId()));
            assertTrue("3.2".equals(ci.getVersion()));
        }
    }
}