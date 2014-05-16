/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.io.tests;

import java.util.Arrays;
import karabo.io.Input;
import karabo.io.InputHash;
import karabo.io.Output;
import karabo.io.OutputHash;
import karabo.util.Hash;
import karabo.util.Registrator;
import karabo.util.vectors.VectorDouble;
import karabo.util.vectors.VectorHash;
import karabo.util.vectors.VectorInteger;
import org.junit.After;
import org.junit.AfterClass;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 *
 * @author Sergey Esenov <serguei.essenov@xfel.eu>
 */
public class HashXmlFileTest {

    public HashXmlFileTest() {
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
    public void xmlFileInputOutput() {
        {
            Hash rooted = new Hash("abc", 12345, "aFloat", 3.1415F, "aString", "Some vector as attribute");
            rooted.setAttribute("aString", "aVector", new VectorDouble(Arrays.asList(3.1415, 2.87, 4.12, 3.62, 8.12, 2.78)));
            rooted.setAttribute("aString", "aNumber", 2.783467333);

            //VectorString vBinarySerializer = Configurator.getRegisteredClasses(OutputHash.class);
            //System.out.println("Registry of base class OutputHash");
            //for (String name : vBinarySerializer) {
            //    System.out.println("\tClass " + name);
            //}
            
            Output<Hash> out = OutputHash.create(OutputHash.class, "TextFile", new Hash("filename", "tests/resources/file1.xml"));
            //TextFileOutputHash out = new TextFileOutputHash(new Hash("filename", "file1.xml"));
            out.write(rooted);
            Input<Hash> inp = InputHash.create(InputHash.class, "TextFile", new Hash("filename", "tests/resources/file1.xml"));
            Hash hash = inp.read();   // inp.<Hash>read();
            assertTrue(hash.get("abc").equals(hash.get("abc")));
            assertTrue(hash.get("aFloat").equals(hash.get("aFloat")));

    //        System.out.println("hash2 is ...\n" + hash);
            Input<Hash> inp2a = InputHash.create(InputHash.class, "TextFile", new Hash("filename", "tests/resources/file2a.xml"));
            Hash h2a = inp2a.read();   // inp2a.<Hash>read();
            //System.out.println("h2a...\n" + h2a);
        }
        {
            Hash unrooted = new Hash("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", new VectorInteger("5,5,5,5,5"), "F.f.f.f.f", new Hash("x.y.z", 99));
            unrooted.setAttribute("F.f.f", "attr1", true);

            Output<Hash> out = OutputHash.create(OutputHash.class, "TextFile", new Hash("filename", "tests/resources/file3.xml", "format.Xml.indentation", 0, "format.Xml.writeDataTypes", false));
            out.write(unrooted);

            Input<Hash> inp = InputHash.create(InputHash.class, "TextFile", new Hash("filename", "tests/resources/file3.xml"));
            Hash hash = inp.read();
            //System.out.println("xmlFileInputOutput: hash is ...\n" + hash);
        }
        {
            VectorHash vh = new VectorHash();
            vh.add(new Hash("b.c", 2.0, "c", 3.f));
            vh.add(new Hash("d.e", "4", "e.f.g.h", new VectorInteger("5,5,5,5,5")));
            Hash unrooted = new Hash("a.b.c", 1, "b", vh, "F.f.f.f.f", new Hash("x.y.z", 99));
            unrooted.setAttribute("F.f.f", "attr1", true);

            Output<Hash> out = OutputHash.create(OutputHash.class, "TextFile", new Hash("filename", "tests/resources/file4.xml", "format.Xml.indentation", 2, "format.Xml.writeDataTypes", false));
            out.write(unrooted);

            Input<Hash> inp = InputHash.create(InputHash.class, "TextFile", new Hash("filename", "tests/resources/file4.xml"));
            Hash hash = inp.read();
            //System.out.println("xmlFileInputOutput: hash is ...\n" + hash);
        }
    }
}
