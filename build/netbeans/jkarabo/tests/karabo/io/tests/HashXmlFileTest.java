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
        Hash hash = new Hash("abc", 12345, "aFloat", 3.1415F, "aString", "Some vector as attribute");
        hash.setAttribute("aString", "aVector", new VectorDouble(Arrays.asList(3.1415, 2.87, 4.12, 3.62, 8.12, 2.78)));
        hash.setAttribute("aString", "aNumber", 2.783467333);

//        VectorString vBinarySerializer = Configurator.getRegisteredClasses(OutputHash.class);
//        System.out.println("Registry of base class OutputHash");
//        for (String name : vBinarySerializer) {
//            System.out.println("\tClass " + name);
//        }
        
        Output<Hash> out = OutputHash.create(OutputHash.class, "TextFile", new Hash("filename", "tests/resources/file1.xml"));
        //TextFileOutputHash out = new TextFileOutputHash(new Hash("filename", "file1.xml"));
        out.write(hash);
        Input<Hash> inp = InputHash.create(InputHash.class, "TextFile", new Hash("filename", "tests/resources/file1.xml"));
        Hash hash2 = inp.read();   // inp.<Hash>read();
        assertTrue(hash.get("abc").equals(hash2.get("abc")));
        assertTrue(hash.get("aFloat").equals(hash2.get("aFloat")));
       
//        System.out.println("hash2 is ...\n" + hash2);
        Input<Hash> inp2a = InputHash.create(InputHash.class, "TextFile", new Hash("filename", "tests/resources/file2a.xml"));
        Hash hash2a = inp2a.read();   // inp2a.<Hash>read();
        //System.out.println("hash2a...\n" + hash2a);
    }
}
