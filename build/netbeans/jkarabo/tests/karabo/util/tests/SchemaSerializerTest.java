/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.logging.Level;
import java.util.logging.Logger;
import karabo.util.AccessType;
import karabo.util.BinarySerializer;
import karabo.util.BinarySerializerSchema;
import karabo.util.ClassInfo;
import karabo.util.Hash;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Registrator;
import karabo.util.Schema;
import karabo.util.TextSerializer;
import karabo.util.TextSerializerSchema;
import karabo.util.Units;
import karabo.util.vectors.VectorInteger;
import karabo.util.vectors.VectorString;
import org.junit.After;
import org.junit.AfterClass;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

@KARABO_CLASSINFO(classId = "TestSchemaSerializer", version = "1.0")
class TestSchemaSerializer extends ClassInfo {

    public static void expectedParameters(Schema expected) {

        STRING_ELEMENT(expected).key("exampleKey1")
                .tags("hardware, poll")
                .displayedName("Example key 1")
                .description("Example key 1 description")
                .options("Radio,Air Condition,Navigation", ",")
                .assignmentOptional().defaultValue("Navigation")
                .advanced()
                .commit();

        NODE_ELEMENT(expected).key("MyNodeElement")
                .tags("myNode")
                .displayedName("MyNodeElem")
                .description("Description of my node elem")
                .commit();

        DOUBLE_ELEMENT(expected).key("MyNodeElement.a")
                .tags("myNode")
                .displayedName("MyNodeElem_A")
                .description("Description of my node elem A")
                .assignmentMandatory()
                .commit();

        STRING_ELEMENT(expected).key("MyNodeElement.b")
                .tags("myNode")
                .displayedName("MyNodeElem_B")
                .description("Description of my node elem B")
                .assignmentMandatory()
                .commit();

        INT64_ELEMENT(expected).key("exampleKey5")
                .alias("exampleAlias5")
                .tags("h/w; d.m.y", ";")
                .displayedName("Example key 5")
                .description("Example key 5 description")
                .readOnly()
                .initialValue(1442244L)
                .commit();

        INT64_ELEMENT(expected).key("exampleKeyINTERNAL")
                .displayedName("INTERNAL")
                .description("Example key INTERNAL")
                .assignmentInternal();

        //test for CHOICE element
        CHOICE_ELEMENT(expected).key("shapes")
                .displayedName("shapesAsChoice")
                .description("Description of Choice-element shapes")
                .assignmentOptional().defaultValue("circle")
                .commit();

        //or: test for LIST element (min/max appears only in 'annotation')
        LIST_ELEMENT(expected).key("shapes")
                .displayedName("shapesAsList")
                .description("Description of List-element shapes")
                .min(2)
                .max(5)
                .assignmentMandatory()
                .commit();

        NODE_ELEMENT(expected).key("shapes.circle")
                .tags("shape")
                .displayedName("Circle")
                .description("Description of circle")
                .commit();

        INT32_ELEMENT(expected).key("shapes.circle.radius")
                .tags("shape")
                .displayedName("radius")
                .description("Radius of circle")
                .minInc(5)
                .maxExc(10)
                .assignmentOptional().defaultValue(5)
                .commit();

        INT32_ELEMENT(expected).key("shapes.circle.color")
                .tags("shape")
                .displayedName("color")
                .description("Color of circle")
                .minExc(2)
                .maxInc(20)
                .assignmentOptional().defaultValue(5)
                .commit();

        NODE_ELEMENT(expected).key("shapes.circle.newnode")
                .tags("shape")
                .displayedName("NewNodeOfCircle")
                .description("Description of NEW NODE of circle")
                .commit();

        INT32_ELEMENT(expected).key("shapes.circle.newnode.mynewint")
                .tags("shape")
                .displayedName("MyNewInt")
                .description("Descr of shapes circle newnode MyNewInt")
                .assignmentOptional().defaultValue(555)
                .commit();

        NODE_ELEMENT(expected).key("shapes.rectangle")
                .tags("shape")
                .displayedName("rectangle")
                .description("Description of rectangle")
                .commit();

        DOUBLE_ELEMENT(expected).key("shapes.rectangle.square")
                .tags("shape")
                .displayedName("square")
                .description("Description of square of rectangle")
                .assignmentOptional().noDefaultValue()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("strVector")
                .displayedName("myVectorString")
                .assignmentOptional().defaultValue(new VectorString(Arrays.asList("first line", "second line")))
                .reconfigurable()
                .commit();

        VECTOR_INT32_ELEMENT(expected).key("intVector")
                .displayedName("MyVectorInt")
                .reconfigurable()
                .minSize(2)
                .maxSize(5)
                .assignmentOptional().defaultValue(new VectorInteger(Arrays.asList(5, 15)))
                .commit();

        INT32_ELEMENT(expected).key("SimpleElem")
                .displayedName("SimpleElem")
                .description("Description of SimpleElem")
                .unit(Units.Unit.METER)
                .metricPrefix(Units.MetricPrefix.MILLI)
                .readOnly()
                .alarmHigh(7)
                .alarmLow(2)
                .warnHigh(1)
                .warnLow(0)
                .commit();

    }
}

/**
 *
 * @author Sergey Esenov <serguei.essenov@xfel.eu>
 */
public class SchemaSerializerTest {

    public SchemaSerializerTest() {
    }

    @BeforeClass
    public static void setUpClass() {
        Registrator.registerAll();
    }

    @AfterClass
    public static void tearDownClass() {
    }

    @Before
    public void setUp() {
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
    public void testSchemaBinarySerializer() {
        Schema testSchema = new Schema("TestSchema", new Schema.AssemblyRules(AccessType.INIT_READ_WRITE));
        TestSchemaSerializer.expectedParameters(testSchema);

        BinarySerializer<Schema> serializer = BinarySerializerSchema.create(BinarySerializerSchema.class, "Bin", new Hash());

        try {
            ByteBuffer archive = serializer.save(testSchema);
            //System.out.println("*** archive1.size=" + archive1.capacity());
            Schema inputSchema = serializer.load(archive.array(), 0, archive.capacity());
            //System.out.println("*** input schema ***\n" + inputSchema);
            // Check whether alias maps got re-established
            assertTrue(inputSchema.keyHasAlias("exampleKey5") == true);
            assertTrue(inputSchema.aliasHasKey("exampleAlias5") == true);
            assertTrue("exampleKey5".equals(inputSchema.getKeyFromAlias("exampleAlias5")));
            assertTrue("exampleAlias5".equals(inputSchema.<String>getAliasFromKey("exampleKey5")));

        } catch (IOException ex) {
            Logger.getLogger(SchemaSerializerTest.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
    
    @Test
    public void testSchemaXmlSerializer() {
        Schema testSchema = new Schema("TestSchema", new Schema.AssemblyRules(AccessType.INIT_READ_WRITE));
        TestSchemaSerializer.expectedParameters(testSchema);
        
        TextSerializer<Schema> textSerializer = TextSerializerSchema.create(TextSerializerSchema.class, "Xml", new Hash());

        try {
            String archive = textSerializer.save(testSchema);
            //System.out.println("*** archived Schema ...\n" + archive);
            Schema inputSchema = textSerializer.load(archive);
            //System.out.println("*** input schema ***\n" + inputSchema);
            assertTrue(inputSchema.keyHasAlias("exampleKey5") == true);
            assertTrue(inputSchema.aliasHasKey("exampleAlias5") == true);
            assertTrue("exampleKey5".equals(inputSchema.getKeyFromAlias("exampleAlias5")));
            assertTrue("exampleAlias5".equals(inputSchema.<String>getAliasFromKey("exampleKey5")));
        } catch (IOException ex) {
            Logger.getLogger(SchemaSerializerTest.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}
