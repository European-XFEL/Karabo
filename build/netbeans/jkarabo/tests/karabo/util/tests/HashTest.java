/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests;

import java.util.ArrayList;
import karabo.util.Attributes;
import karabo.util.Hash;
import karabo.util.Node;
import karabo.util.vectors.VectorHash;
import karabo.util.types.Types;
import karabo.util.vectors.VectorString;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import static org.junit.Assert.*;

/**
 *
 * @author Sergey Esenov <serguei.essenov at xfel.eu>
 */
public class HashTest {

    public HashTest() {
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
    public void testConstructors() {
        {
            Hash h = new Hash();
            assertTrue(h.empty() == true);
            assertTrue(h.size() == 0);
        }
        {
            Hash h = new Hash("a", 1);
            assertTrue(h.empty() == false);
            assertTrue(h.size() == 1);
            assertTrue((int)h.get("a") == 1);
        }
        {
            Hash h = new Hash("a", 1, "b", 2.0);
            assertTrue(h.empty() == false);
            assertTrue(h.size() == 2);
            assertTrue((int)h.get("a") == 1);
            assertTrue((double) h.get("b") == 2.0);
        }
        {
            Hash h = new Hash("a", 1, "b", 2.0, "c", 3.f);
            assertTrue(h.empty() == false);
            assertTrue(h.size() == 3);
            assertTrue((int)h.get("a") == 1);
            assertTrue((double) h.get("b") == 2.0);
            assertTrue((float) h.get("c") == 3.0);
        }
        {
            Hash h = new Hash("a", 1, "b", 2.0, "c", 3.f, "d", "4");
            assertTrue(h.empty() == false);
            assertTrue(h.size() == 4);
            assertTrue((int)h.get("a") == 1);
            assertTrue((double) h.get("b") == 2.0);
            assertTrue((float) h.get("c") == 3.0);
            assertTrue("4".equals((String) h.get("d")));
        }
        {
            Hash h = new Hash("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", new Integer[]{5, 5, 5, 5, 5});
            assertTrue(h.empty() == false);
            assertTrue(h.size() == 5);
            assertTrue((int)h.get("a") == 1);
            assertTrue((double) h.get("b") == 2.0);
            assertTrue((float) h.get("c") == 3.0);
            assertTrue("4".equals((String) h.get("d")));
            assertTrue(((Integer[]) h.get("e"))[0] == 5);
        }
        {
            Hash h = new Hash("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", new Integer[]{5, 5, 5, 5, 5}, "f", new Hash("a", 6));
            assertTrue(h.empty() == false);
            assertTrue(h.size() == 6);
            assertTrue((int)h.get("a") == 1);
            assertTrue((double) h.get("b") == 2.0);
            assertTrue((float) h.get("c") == 3.0);
            assertTrue("4".equals((String) h.get("d")));
            assertTrue(((Integer[]) h.get("e"))[0] == 5);
            assertTrue((int)((Hash) h.get("f")).get("a") == 6);
            assertTrue((int)h.get("f.a") == 6);

        }
        {
            Hash h = new Hash("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", new Integer[]{5, 5, 5, 5, 5}, "F.f.f.f.f", new Hash("x.y.z", 99));
            assertTrue(h.empty() == false);
            assertTrue(h.size() == 6);
            assertTrue((int)h.get("a.b.c") == 1);
            assertTrue((double) h.get("b.c") == 2.0);
            assertTrue((float) h.get("c") == 3.0);
            assertTrue("4".equals((String) h.get("d.e")));
            assertTrue(((Integer[]) h.get("e.f.g.h"))[0] == 5);
            assertTrue((int)((Hash) h.get("F.f.f.f.f")).get("x.y.z") == 99);
            assertTrue((int)h.get("F.f.f.f.f.x.y.z") == 99);

            // Check 'flatten'
            Hash flat = h.flatten();

            assertTrue(flat.empty() == false);
            assertTrue(flat.size() == 6);
            assertTrue((int)flat.get("a.b.c", "") == 1);
            assertTrue((Double) flat.get("b.c", "") == 2.0);
            assertTrue((float) flat.get("c", "") == 3.0);
            assertTrue("4".equals((String) flat.get("d.e", "")));
            assertTrue(((Integer[]) flat.get("e.f.g.h", ""))[0] == 5);
            assertTrue((int)flat.get("F.f.f.f.f.x.y.z", "") == 99);

            Hash tree = flat.unflatten();

            assertTrue(tree.empty() == false);
            assertTrue(tree.size() == 6);
            assertTrue((int)tree.get("a.b.c") == 1);
            assertTrue((double) tree.get("b.c") == 2.0);
            assertTrue((float) tree.get("c") == 3.0);
            assertTrue("4".equals((String) tree.get("d.e")));
            assertTrue(((Integer[]) tree.get("e.f.g.h"))[0] == 5);
            assertTrue((int)((Hash) tree.get("F.f.f.f.f")).get("x.y.z") == 99);
            assertTrue((int)tree.get("F.f.f.f.f.x.y.z") == 99);
        }
    }

    @Test
    public void testGetSet() {
        {
            Hash h = new Hash();
            h.set("a.b.c1.d", 1);
            assertTrue(((Hash) h.get("a")).has("b"));
            assertTrue(((Hash) h.get("a.b")).has("c1"));
            assertTrue(((Hash) h.get("a.b.c1")).has("d"));
            assertTrue((int)h.get("a.b.c1.d") == 1);
            assertTrue(h.has("a.b.c1.d"));
            assertTrue(((Hash) h.get("a")).has("b.c1"));

            h.set("a.b.c2.d", "1");
            assertTrue(((Hash) h.get("a")).has("b"));
            assertTrue(((Hash) h.get("a.b")).has("c1"));
            assertTrue(((Hash) h.get("a.b")).has("c2"));
            assertTrue(((Hash) h.get("a.b")).has("c2.d"));
            assertTrue(((Hash) h.get("a.b")).is("c2.d", String.class));
            assertTrue(((Hash) h.get("a.b.c2")).has("d"));
            assertTrue("1".equals((String) h.get("a.b.c2.d")));

            //h.set("a.b[0].a", 1);
            h.set("a.b[0]", new Hash("a", 1));
            assertTrue(((Hash) h.get("a")).has("b"));
            assertTrue(((Hash) h.get("a")).size() == 1);
            assertTrue(h.is("a.b", VectorHash.class));
            assertTrue(((VectorHash) h.get("a.b")).size() == 1);
            assertTrue(((VectorHash) h.get("a.b")).get(0).size() == 1);
            assertTrue((int)((VectorHash) h.get("a.b")).get(0).get("a") == 1);
            assertTrue((int)h.get("a.b[0].a") == 1);

            h.set("a.b[2]", new Hash("a", "1"));
            assertTrue(((Hash) h.get("a")).has("b"));
            assertTrue(((Hash) h.get("a")).size() == 1);
            assertTrue(h.is("a.b", VectorHash.class));
            assertTrue(h.has("a.b") == true);
            assertTrue(((VectorHash) h.get("a.b")).size() == 3);
            assertTrue((int)h.get("a.b[0].a") == 1);
            assertTrue(((Hash) h.get("a.b[1]")).empty() == true);
            assertTrue("1".equals((String) h.get("a.b[2].a")));
            assertTrue(((VectorHash) h.get("a.b")).get(0).is("a", Integer.class));
            assertTrue(((VectorHash) h.get("a.b")).get(0).is("a", Types.ReferenceType.INT32));
            assertTrue(((VectorHash) h.get("a.b")).get(1).empty());
            assertTrue(((VectorHash) h.get("a.b")).get(2).is("a", String.class));
            assertTrue(((VectorHash) h.get("a.b")).get(2).is("a", Types.ReferenceType.STRING));

            assertTrue(((Hash) h.get("a")).is("b[0]", Hash.class));
            assertTrue(((Hash) h.get("a")).is("b[1]", Hash.class));
            assertTrue(((Hash) h.get("a")).is("b[2]", Hash.class));
            assertTrue(((Hash) h.get("a")).is("b[0]", Types.ReferenceType.HASH));
            assertTrue(((Hash) h.get("a")).is("b[1]", Types.ReferenceType.HASH));
            assertTrue(((Hash) h.get("a")).is("b[2]", Types.ReferenceType.HASH));
            assertFalse(((Hash) h.get("a.b[0]")).empty());
            assertTrue(((Hash) h.get("a.b[1]")).empty());
            assertFalse(((Hash) h.get("a.b[2]")).empty());
        }
        {
            Hash h = new Hash();
            h.set("a.b.c", 1);
            h.set("a.b.c", 2);
            assertTrue((Integer) h.get("a.b.c") == 2);
            assertTrue(((Hash) h.get("a")).is("b", Hash.class));
            assertTrue(h.is("a.b.c", Integer.class));
            assertTrue(h.is("a.b.c", Types.ReferenceType.INT32));
            assertTrue(h.has("a.b"));
            assertFalse(h.has("a.b.c.d"));
        }
        {
            Hash h = new Hash("x", new Hash("a", 1), "y", new Hash("a", 1));
            assertTrue((int)h.get("x.a") == 1);
            assertTrue((int)h.get("y.a") == 1);
        }
        {

            Hash h = new Hash("a[0]", new Hash("a", 1), "a[1]", new Hash("a", 1));
            assertTrue((int)h.get("a[0].a") == 1);
            assertTrue((int)h.get("a[1].a") == 1);
        }
        {
            Hash h = new Hash();
            h.set("x[0].y[0]", new Hash("a", 4.2, "b", "red", "c", true));
            h.set("x[1].y[0]", new Hash("a", 4.0, "b", "green", "c", false));
            assertTrue((Boolean) h.get("x[0].y[0].c"));
            assertFalse((Boolean) h.get("x[1].y[0].c"));
            assertTrue("red".equals((String) h.get("x[0].y[0].b")));
            assertTrue("green".equals((String) h.get("x[1].y[0].b")));
        }
        {
            Hash h1 = new Hash("a[0].b[0]", new Hash("a", 1));
            Hash h2 = new Hash("a[0].b[0]", new Hash("a", 2));

            h1.set("a[0]", h2);
            assertTrue((int)h1.get("a[0].a[0].b[0].a") == 2);
            h1.set("a", h2);
            assertTrue((int)h1.get("a.a[0].b[0].a") == 2);
        }
    }

    @Test
    public void testAttributes() {
        {
            Hash h = new Hash("a.b.a.b", 42);
            h.setAttribute("a", "attr1", "someValue");
            assertTrue("someValue".equals((String) h.getAttribute("a", "attr1")));

            h.setAttribute("a", "attr2", 42);
            assertTrue("someValue".equals((String) h.getAttribute("a", "attr1")));
            assertTrue((int) h.getAttribute("a", "attr2") == 42);

            h.setAttribute("a", "attr2", 43);
            assertTrue("someValue".equals((String) h.getAttribute("a", "attr1")));
            assertTrue((int) h.getAttribute("a", "attr2") == 43);

            h.setAttribute("a.b.a.b", "attr1", true);
            assertTrue((Boolean) h.getAttribute("a.b.a.b", "attr1"));

            Attributes attrs = h.getAttributes("a");
            assertTrue(attrs.size() == 2);
            assertTrue("someValue".equals((String) attrs.get("attr1")));
            assertTrue((int)attrs.get("attr2") == 43);

            //Node node = attrs.getNode("attr2");
            //assertTrue(node.getType() == Types::INT32);
        }
    }

    @Test
    public void testIteration() {

        Hash h = new Hash("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5, "order", 6);
        Attributes a = new Attributes("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5, "order", 6);

        {
            ArrayList<String> insertionOrder = new ArrayList<>();
            for (Node node : h.values()) {
                insertionOrder.add(node.getKey());
            }
            assertTrue("should".equals(insertionOrder.get(0)));
            assertTrue("be".equals(insertionOrder.get(1)));
            assertTrue("iterated".equals(insertionOrder.get(2)));
            assertTrue("in".equals(insertionOrder.get(3)));
            assertTrue("correct".equals(insertionOrder.get(4)));
            assertTrue("order".equals(insertionOrder.get(5)));
        }
        /*
         {
         std::vector<std::string> alphaNumericOrder;
         for (Hash::const_map_iterator it = h.mbegin(); it != h.mend(); ++it) {
         alphaNumericOrder.push_back(it->second.getKey());
         }
         assertTrue(alphaNumericOrder[0] == "be");
         assertTrue(alphaNumericOrder[1] == "correct");
         assertTrue(alphaNumericOrder[2] == "in");
         assertTrue(alphaNumericOrder[3] == "iterated");
         assertTrue(alphaNumericOrder[4] == "order");
         assertTrue(alphaNumericOrder[5] == "should");
         }

         h.set("be", "2"); // Has no effect on order

         {
         std::vector<std::string> insertionOrder;
         for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
         insertionOrder.push_back(it->getKey());
         }
         assertTrue(insertionOrder[0] == "should");
         assertTrue(insertionOrder[1] == "be");
         assertTrue(insertionOrder[2] == "iterated");
         assertTrue(insertionOrder[3] == "in");
         assertTrue(insertionOrder[4] == "correct");
         assertTrue(insertionOrder[5] == "order");
         }

         {
         std::vector<std::string> alphaNumericOrder;
         for (Hash::const_map_iterator it = h.mbegin(); it != h.mend(); ++it) {
         alphaNumericOrder.push_back(it->second.getKey());
         }
         assertTrue(alphaNumericOrder[0] == "be");
         assertTrue(alphaNumericOrder[1] == "correct");
         assertTrue(alphaNumericOrder[2] == "in");
         assertTrue(alphaNumericOrder[3] == "iterated");
         assertTrue(alphaNumericOrder[4] == "order");
         assertTrue(alphaNumericOrder[5] == "should");
         }

         h.erase("be"); // Remove
         h.set("be", "2"); // Must be last element in sequence now

         {
         std::vector<std::string> insertionOrder;
         for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
         insertionOrder.push_back(it->getKey());
         }
         assertTrue(insertionOrder[0] == "should");
         assertTrue(insertionOrder[1] == "iterated");
         assertTrue(insertionOrder[2] == "in");
         assertTrue(insertionOrder[3] == "correct");
         assertTrue(insertionOrder[4] == "order");
         assertTrue(insertionOrder[5] == "be");
         }

         {
         std::vector<std::string> alphaNumericOrder;
         for (Hash::const_map_iterator it = h.mbegin(); it != h.mend(); ++it) {
         alphaNumericOrder.push_back(it->second.getKey());
         }
         assertTrue(alphaNumericOrder[0] == "be");
         assertTrue(alphaNumericOrder[1] == "correct");
         assertTrue(alphaNumericOrder[2] == "in");
         assertTrue(alphaNumericOrder[3] == "iterated");
         assertTrue(alphaNumericOrder[4] == "order");
         assertTrue(alphaNumericOrder[5] == "should");
         }

         //  getKeys(...) to ...
         //         "set"
         {
         std::set<std::string> tmp; // create empty set
         h.getKeys(tmp); // fill set by keys
         std::set<std::string>::const_iterator it = tmp.begin();
         assertTrue(*it++ == "be");
         assertTrue(*it++ == "correct");
         assertTrue(*it++ == "in");
         assertTrue(*it++ == "iterated");
         assertTrue(*it++ == "order");
         assertTrue(*it++ == "should");
         }

         //         "vector"
         {
         std::vector<std::string> tmp; // create empty vector
         h.getKeys(tmp); // fill vector by keys
         std::vector<std::string>::const_iterator it = tmp.begin();
         assertTrue(*it++ == "should");
         assertTrue(*it++ == "iterated");
         assertTrue(*it++ == "in");
         assertTrue(*it++ == "correct");
         assertTrue(*it++ == "order");
         assertTrue(*it++ == "be");
         }

         //         "list"
         {
         std::list<std::string> tmp; // create empty list
         h.getKeys(tmp); // fill list by keys
         std::list<std::string>::const_iterator it = tmp.begin();
         assertTrue(*it++ == "should");
         assertTrue(*it++ == "iterated");
         assertTrue(*it++ == "in");
         assertTrue(*it++ == "correct");
         assertTrue(*it++ == "order");
         assertTrue(*it++ == "be");
         }

         //         "deque"
         {
         std::deque<std::string> tmp; // create empty queue
         h.getKeys(tmp); // fill deque by keys
         std::deque<std::string>::const_iterator it = tmp.begin();
         assertTrue(*it++ == "should");
         assertTrue(*it++ == "iterated");
         assertTrue(*it++ == "in");
         assertTrue(*it++ == "correct");
         assertTrue(*it++ == "order");
         assertTrue(*it++ == "be");
         }
         */
    }

    @Test
    public void testGetPaths() {
        {
            Hash h = new Hash();
            VectorString paths = h.getPaths();
            assertTrue(paths.size() == 0);
        }
    }

    @Test
    public void testMerge() {

        Hash h1 = new Hash("a", 1,
                "b", 2,
                "c.b[0].g", 3,
                "c.c[0].d", 4,
                "c.c[1]", new Hash("a.b.c", 6),
                "d.e", 7);

        Hash h2 = new Hash("a", 21,
                "b.c", 22,
                "c.b[0]", new Hash("key", "value"),
                "c.b[1].d", 24,
                "e", 27);

        h1.merge(h2);

        assertTrue(h1.has("a"));
        assertTrue((int)h1.get("a") == 21);
        assertTrue(h1.has("b"));
        assertTrue(!h1.has("c.b.d"));
        assertTrue(h1.has("c.b[0]"));
        assertTrue(h1.has("c.b[1]"));
        assertTrue(h1.has("c.b[2]"));
        assertTrue((int)h1.get("c.b[2].d") == 24);
        assertTrue(h1.has("c.c[0].d"));
        assertTrue(h1.has("c.c[1].a.b.c"));
        assertTrue(h1.has("d.e"));
        assertTrue(h1.has("e"));

        Hash h3 = new Hash(h1);

        assertTrue(Hash.similar(h1, h3));
        
        Hash h4 = new Hash().merge(h1);
        
        assertTrue(Hash.similar(h1, h4));
    }
}
