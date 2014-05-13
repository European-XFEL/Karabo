/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;
import karabo.util.Hash;
import karabo.util.Registrator;
import karabo.util.TextSerializer;
import karabo.util.TextSerializerHash;
import org.junit.After;
import org.junit.AfterClass;
import static org.junit.Assert.*;
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
}
