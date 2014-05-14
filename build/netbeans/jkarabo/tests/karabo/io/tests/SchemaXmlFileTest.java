/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.io.tests;

import java.util.Arrays;
import karabo.io.Input;
import karabo.io.InputSchema;
import karabo.io.Output;
import karabo.io.OutputSchema;
import karabo.util.AccessType;
import karabo.util.ClassInfo;
import karabo.util.Configurator;
import karabo.util.Hash;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Registrator;
import karabo.util.Schema;
import karabo.util.Units;
import karabo.util.vectors.VectorInteger;
import karabo.util.vectors.VectorString;
import org.junit.After;
import org.junit.AfterClass;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

@KARABO_CLASSINFO(classId = "TestSchemaSerializer2", version = "1.0")
class TestSchemaSerializer2 extends ClassInfo {

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
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
public class SchemaXmlFileTest {
    
    public SchemaXmlFileTest() {
        Registrator.registerAll();
    }
    
    @BeforeClass
    public static void setUpClass() {
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
    public void textFileInputOutput() {
        Schema testSchema = new Schema("TestSchema", new Schema.AssemblyRules(AccessType.INIT_READ_WRITE));
        TestSchemaSerializer2.expectedParameters(testSchema);

        VectorString vBinarySerializer = Configurator.getRegisteredClasses(InputSchema.class);
        //System.out.println("Registry of base class InputSchema");
        for (String name : vBinarySerializer) {
            //System.out.println("\tClass " + name);
        }
        Output<Schema> out = OutputSchema.create(OutputSchema.class, "TextFile", new Hash("filename", "tests/resources/file2.xml"));
        out.write(testSchema);
        //System.out.println("Java TestSchema is saved.");

        Input<Schema> inp = InputSchema.create(InputSchema.class, "TextFile", new Hash("filename", "tests/resources/file2.xml"));
        Schema schema2 = inp.read();   // inp.<Schema>read();
        
        assertTrue(testSchema.getDisplayedName("intVector").equals(schema2.getDisplayedName("intVector")));
        assertTrue(testSchema.getDescription("exampleKey5").equals(schema2.getDescription("exampleKey5")));
        //System.out.println("schema2 is ...\n" + schema2);
        //System.out.println("Java TestSchema is loaded.");
        
        Input<Schema> inp2 = InputSchema.create(InputSchema.class, "TextFile", new Hash("filename", "tests/resources/testschema.xml"));
        Schema cppSchema = inp2.read();   // inp.<Schema>read();
        //System.out.println("C++ TestSchema is loaded.");
    }
}
