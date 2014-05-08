/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.time;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Locale;
import java.util.TimeZone;
import java.util.logging.Level;
import java.util.logging.Logger;
import karabo.util.Attributes;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Epochstamp {

    private Date m_date;

    public final void now() {
        m_date = new Date();
    }

    public Epochstamp() {
        now();
    }

    public Epochstamp(long seconds, long fractions) {
        long millis = seconds * 1000 + fractions / TIME_UNITS.MILLISEC.duration();
        m_date.setTime(millis);
    }

    public Epochstamp(long millis) {
        now();
        m_date.setTime(millis);
//        m_seconds = millis / 1000;
//        m_fractionalSeconds = (millis % 1000) * TIME_UNITS.MILLISEC.duration();
    }

    public Epochstamp(Date date) {
        m_date = date;
//        long now = date.getTime();
//        m_seconds = now / 1000;
//        m_fractionalSeconds = (now % 1000) * TIME_UNITS.MILLISEC.duration();
    }

    public long getSeconds() {
        return m_date.getTime() / 1000;
    }

    public long getFractionalSeconds() {
        return (m_date.getTime() % 1000) * TIME_UNITS.MILLISEC.duration();
    }

    public Epochstamp copy(Epochstamp other) {
        m_date = other.m_date;
//        m_fractionalSeconds = other.m_fractionalSeconds;
        return this;
    }

    public boolean equals(Epochstamp other) {
        return (m_date.getTime() == other.m_date.getTime());
    }

    public long compareTo(Epochstamp other) {
//        if (m_seconds != other.m_seconds) {
//            return m_seconds - other.m_seconds;
//        }
//        return m_fractionalSeconds - other.m_fractionalSeconds;
        return m_date.getTime() - other.m_date.getTime();
    }

    public boolean isEquals(Epochstamp other) {
        return equals(other);
    }

    public boolean isNotEquals(Epochstamp other) {
        return !equals(other);
    }

    public boolean isLesser(Epochstamp other) {
        return compareTo(other) < 0;
    }

    public boolean isLesserOrEqual(Epochstamp other) {
        return compareTo(other) <= 0;
    }

    public boolean isGreater(Epochstamp other) {
        return compareTo(other) > 0;
    }

    public boolean isGreaterOrEqual(Epochstamp other) {
        return compareTo(other) >= 0;
    }

    public TimeDuration minus(Epochstamp other) {
        long millis = m_date.getTime() - other.m_date.getTime();
        return new TimeDuration(millis / 1000, (millis % 1000) * TIME_UNITS.MILLISEC.duration());
    }

    public Epochstamp minusEquals(TimeDuration duration) {
        long millis = m_date.getTime();
        millis -= (duration.getTotalSeconds() * 1000 + duration.getFractions(TIME_UNITS.MILLISEC));
        m_date.setTime(millis);
        return this;
    }

    public Epochstamp plusEquals(TimeDuration duration) {
        long millis = m_date.getTime();
        millis += (duration.getTotalSeconds() * 1000 + duration.getFractions(TIME_UNITS.MILLISEC));
        m_date.setTime(millis);
        return this;
    }

    public Epochstamp plus(TimeDuration duration) {
        Epochstamp result = new Epochstamp(m_date.getTime());
        result.plusEquals(duration);
        return result;
    }

    public Epochstamp minus(TimeDuration duration) {
        Epochstamp result = new Epochstamp(m_date.getTime());
        result.minusEquals(duration);
        return result;
    }

    public TimeDuration elapsed(Epochstamp other) {
        if (this.isLesser(other)) {
            return other.minus(this);
        } else {
            return this.minus(other);
        }
    }

    public TimeDuration elapsed() {
        Epochstamp other = new Epochstamp();
        return elapsed(other);
    }

    public String toIso8601(TIME_UNITS precision) {
        SimpleDateFormat formatted = new SimpleDateFormat("yyyyMMdd'T'HHmmss.SSS");
        return formatted.format(m_date);
    }

    public String toIso8601() {
        return toIso8601(TIME_UNITS.MILLISEC);
    }

    @Override
    public String toString() {
        return m_date.toString();
    }

    public String toFormattedString(String fmt) {
        SimpleDateFormat formatted = new SimpleDateFormat(fmt);
        return formatted.format(m_date);
    }

    public static Epochstamp fromIso8601(String timePoint) {
        try {
            Date d = new SimpleDateFormat("yyyyMMdd'T'HHmmss.SSS", Locale.US).parse(timePoint);
            return new Epochstamp(d);
        } catch (ParseException ex) {
            throw new RuntimeException("Date Parsing Exception: " + ex);
        }
    }

    public static boolean hashAttributesContainTimeInformation(Attributes attributes) {
        return attributes.has("sec") && attributes.has("frac");
    }

    public static Epochstamp fromHashAttributes(Attributes attributes) {
        long millis = 0;
        try {
            millis = ((long) attributes.get("sec")) * 1000 + ((long) attributes.get("frac")) / TIME_UNITS.MILLISEC.duration();
        } catch (Exception e) {
            throw new RuntimeException("Re-throw Attribute Access Exception: " + e);
        }
        Date date = new Date(millis);
        return new Epochstamp(date);
    }

    public void toHashAttributes(Attributes attributes) {
        long millis = m_date.getTime();
        attributes.set("sec", millis / 1000);
        attributes.set("frac", (millis % 1000) * TIME_UNITS.MILLISEC.duration());
    }
}
