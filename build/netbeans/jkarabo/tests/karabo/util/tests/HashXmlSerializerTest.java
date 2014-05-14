/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests;

import java.io.IOException;
import java.util.Arrays;
import java.util.logging.Level;
import java.util.logging.Logger;
import karabo.util.Hash;
import karabo.util.Registrator;
import karabo.util.TextSerializer;
import karabo.util.TextSerializerHash;
import karabo.util.vectors.VectorByte;
import karabo.util.vectors.VectorDouble;
import karabo.util.vectors.VectorFloat;
import karabo.util.vectors.VectorInteger;
import karabo.util.vectors.VectorShort;
import karabo.util.vectors.VectorString;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
public class HashXmlSerializerTest {

    public HashXmlSerializerTest() {
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
    public void testHashSimple() {
        Hash hash = new Hash("aByte", (byte) 61, "aInt", 12345, "aShort", (short) 12345, "aFloat", 12.345F,
                "aDouble", 123.45, "aString", "Some not so long string.");
        TextSerializer<Hash> serializer = TextSerializerHash.create(TextSerializerHash.class, "Xml", new Hash());
        try {
            String archive = serializer.<Hash>save(hash);
            //System.out.println(String.format("archive.content: 0x%x", new BigInteger(1, archive.array())));
            Hash hash2 = serializer.<Hash>load(archive);
            //System.out.println("Loaded hash is ...\n" + hash2);
            assert hash2.get("aByte").equals(hash.get("aByte"));
            assert hash2.get("aInt").equals(hash.get("aInt"));
            assert hash2.get("aShort").equals(hash.get("aShort"));
            assert hash2.get("aFloat").equals(hash.get("aFloat"));
            assert hash2.get("aDouble").equals(hash.get("aDouble"));
            assert hash2.get("aString").equals(hash.get("aString"));
            //assert hash2.get("aNone").equals(hash.get("aNone"));
        } catch (IOException ex) {
            Logger.getLogger(HashBinarySerializerTest.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
    
    @Test
    public void testHashVector() {
        Hash hash = new Hash();
        hash.set("vByte", new VectorByte(Arrays.asList((byte) 0x41, (byte) 0x61, (byte) 0x77)));
        hash.set("vInt",  new VectorInteger(Arrays.asList(0x41, 0x61, 0x77)));
        hash.set("vShort", new VectorShort(Arrays.asList((short) 0x41, (short) 0x61, (short) 0x77)));
        hash.set("vFloat",  new VectorFloat(Arrays.asList(4.1F, 61.44F, 77.777F)));
        hash.set("vDouble",  new VectorDouble(Arrays.asList(0.412345, 61.44, 77.777)));
        hash.set("vString",  new VectorString(Arrays.asList("a.412345", "b61.44", "c77.777")));
//        System.out.println("Saved hash is ...\n" + hash);
        TextSerializerHash serializer = TextSerializerHash.create(TextSerializerHash.class, "Xml", new Hash());
        try {
            String archive = serializer.save(hash);
            Hash hash2 = serializer.load(archive);
//            System.out.println("Loaded hash is ...\n" + hash2);
            assert hash2.get("vByte").equals(hash.get("vByte"));
            assert hash2.get("vInt").equals(hash.get("vInt"));
            assert hash2.get("vShort").equals(hash.get("vShort"));
            assert hash2.get("vFloat").equals(hash.get("vFloat"));
            assert hash2.get("vDouble").equals(hash.get("vDouble"));
            assert hash2.get("vString").equals(hash.get("vString"));
        } catch (IOException ex) {
            Logger.getLogger(HashBinarySerializerTest.class.getName()).log(Level.SEVERE, null, ex);
        }
        
    }
}
