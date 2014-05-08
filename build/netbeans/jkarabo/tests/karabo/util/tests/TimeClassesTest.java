/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.logging.Level;
import java.util.logging.Logger;
import karabo.util.time.Epochstamp;
import karabo.util.time.TIME_UNITS;
import karabo.util.time.TimeDuration;
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
public class TimeClassesTest {

    public TimeClassesTest() {
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
    public void testTimeDuration() {
        TimeDuration td1 = new TimeDuration(312345, 987608822111000123L);
        TimeDuration td2 = new TimeDuration(43216, 8756234019273449L);
        TimeDuration td3 = td1.plus(td2);
//        System.out.println("td1 = " + td1);
//        assertTrue("45.987608822".equals(td1.toString()));
//        assertTrue("16.008756234".equals(td2.toString()));
//        assertTrue("02.996365056".equals(td3.toString()));
        assertTrue(td1.getDays() == 3);
        assertTrue(td1.getSeconds() == 45);
        assertTrue(td3.getMinutes() == 46);
        assertTrue(td3.format("%U").equals(Long.toString(td3.getFractions(TIME_UNITS.MICROSEC))));
    }

    @Test
    public void testEpochstamp() {
        
        SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
        Date date = new Date();
        try {
            date = formatter.parse("2013-10-07 23:15:47.396");
        } catch (ParseException ex) {
            throw new RuntimeException("testEpochstamp Parse Exception: " + ex);
        }
        Epochstamp t1 = new Epochstamp(date);

        
        Epochstamp t2 = t1.plus(new TimeDuration(2, 0));
//        System.out.println("t1 = " + t1.toIso8601());
//        System.out.println("t2 = " + t2.toIso8601());
        TimeDuration.setDefaultFormat("%s.%U");
        TimeDuration d = t2.minus(t1);
//        System.out.println("d: " + d);
        assertTrue("2.000000".equals(d.toString()));

        t2.plusEquals(new TimeDuration(0, 1000000000000000L));
        assertTrue("2.001000".equals((t2.minus(t1)).toString()));

        Epochstamp t3 = t2.plus(new TimeDuration(5, 0));
        Epochstamp t4 = t3.plus(d);
        t4.minusEquals(new TimeDuration(0, 2000000000000000L));
        TimeDuration e = t4.elapsed(t1);
//        System.out.println("t4.elapsed(t1) : " + e);
        assertTrue("8.999000".equals(e.toString()));
        
        String timePoint = t4.toIso8601();
        Epochstamp t5 = Epochstamp.fromIso8601(timePoint);
        assertTrue(t4.isEquals(t5));
    }
}