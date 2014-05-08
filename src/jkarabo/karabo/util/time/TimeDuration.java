/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.time;

import java.math.BigDecimal;
import karabo.util.Hash;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class TimeDuration {

    private long m_Seconds;
    private long m_Fractions;
    private static String DEFAULT_FORMAT;
    static {
        DEFAULT_FORMAT = "%S.%N";
    }

    public TimeDuration() {
        m_Seconds = 0;
        m_Fractions = 0;
    }

    public TimeDuration(Hash hash) {
        m_Seconds = hash.get("seconds");
        m_Fractions = hash.get("fractions");
    }

    public TimeDuration(long seconds, long fractions) {
        m_Seconds = seconds;
        m_Fractions = fractions;
    }

    public TimeDuration(int days, int hours, int minutes, long seconds, long fractions) {
        m_Seconds = days * TIME_UNITS.DAY.duration() + hours * TIME_UNITS.HOUR.duration() + minutes * TIME_UNITS.MINUTE.duration() + seconds;
        m_Fractions = fractions;
    }

    public TimeDuration(TimeDuration other) {
        this(other.m_Seconds, other.m_Fractions);
    }

    public TimeDuration set(long seconds, long fractions) {
        m_Seconds = seconds;
        m_Fractions = fractions;
        return this;
    }

    public TimeDuration set(int days, int hours, int minutes, long seconds, long fractions) {
        m_Seconds = days * TIME_UNITS.DAY.duration() + hours * TIME_UNITS.HOUR.duration() + minutes * TIME_UNITS.MINUTE.duration() + seconds;
        m_Fractions = fractions;
        return this;
    }

    public boolean isNull() {
        return (m_Seconds == 0) && (m_Fractions == 0);
    }

    public long getDays() {
        return m_Seconds / TIME_UNITS.DAY.duration();
    }

    public long getHours() {
        return (m_Seconds / TIME_UNITS.HOUR.duration()) % 24;

    }

    public long getTotalHours() {
        return m_Seconds / TIME_UNITS.HOUR.duration();
    }

    public long getMinutes() {
        return (m_Seconds / TIME_UNITS.MINUTE.duration()) % 60;
    }

    public long getTotalMinutes() {
        return m_Seconds / TIME_UNITS.MINUTE.duration();
    }

    public long getSeconds() {
        return m_Seconds % 60;
    }

    public long getTotalSeconds() {
        return m_Seconds;
    }

    public long getFractions(TIME_UNITS unit) {
        return m_Fractions / unit.duration();
    }

    public long getFractions() {
        return m_Fractions / TIME_UNITS.NANOSEC.duration();
    }
    
    public String format(String fmt) {
        StringBuilder oss = new StringBuilder();
        //String newLine = System.getProperty("line.separator");
        boolean percent = false;
        for (Character c : fmt.toCharArray()) {
            if (c == '%') {
                if (!percent)
                    percent = true;
                else {
                    percent = false;
                    oss.append('%');
                }
            } else if (percent) {
                percent = false;
                switch(c) {
                    case 'd':
                    case 'D':
                        oss.append(getDays());
                        break;
                    case 'H':
                        oss.append(String.format("%02d", getHours()));
                        break;
                    case 'M':
                        oss.append(String.format("%02d", getMinutes()));
                        break;
                    case 'S':
                        oss.append(String.format("%02d", getSeconds()));
                        break;
                    case 'h':
                        oss.append(String.format("%d", getHours()));
                        break;
                    case 'm':
                        oss.append(String.format("%d", getMinutes()));
                        break;
                    case 's':
                        oss.append(String.format("%d", getSeconds()));
                        break;
                    case 'l':
                        oss.append(String.format("%d", getFractions(TIME_UNITS.MILLISEC)));
                        break;
                    case 'u':
                        oss.append(String.format("%d", getFractions(TIME_UNITS.MICROSEC)));
                        break;
                    case 'n':
                        oss.append(String.format("%d", getFractions(TIME_UNITS.NANOSEC)));
                        break;
                    case 'p':
                        oss.append(String.format("%d", getFractions(TIME_UNITS.PICOSEC)));
                        break;
                    case 'f':
                        oss.append(String.format("%d", getFractions(TIME_UNITS.FEMTOSEC)));
                        break;
                    case 'a':
                        oss.append(String.format("%d", getFractions(TIME_UNITS.ATTOSEC)));
                        break;
                    case 'L':
                        oss.append(String.format("%03d", getFractions(TIME_UNITS.MILLISEC)));
                        break;
                    case 'U':
                        oss.append(String.format("%06d", getFractions(TIME_UNITS.MICROSEC)));
                        break;
                    case 'N':
                        oss.append(String.format("%09d", getFractions(TIME_UNITS.NANOSEC)));
                        break;
                    case 'P':
                        oss.append(String.format("%012d", getFractions(TIME_UNITS.PICOSEC)));
                        break;
                    case 'F':
                        oss.append(String.format("%015d", getFractions(TIME_UNITS.FEMTOSEC)));
                        break;
                    case 'A':
                        oss.append(String.format("%018d", getFractions(TIME_UNITS.ATTOSEC)));
                        break;
                    default:
                        throw new RuntimeException("TimeDuration format is not recognized: " + fmt);
                }
            } else {
                oss.append(c);
            }
        }
        return oss.toString();
    }

    public boolean equals(TimeDuration other) { // operator ==
        return (m_Seconds == other.m_Seconds) && (m_Fractions == other.m_Fractions);
    }

    public long comparesTo(TimeDuration other) {
        if (m_Seconds != other.m_Seconds) {
            return m_Seconds - other.m_Seconds;
        }
        return m_Fractions - other.m_Fractions;
    }

    public boolean isEqual(TimeDuration other) { // operator ==
        return equals(other);
    }

    public boolean isNotEqual(TimeDuration other) {// operator !=
        return !equals(other);
    }

    public boolean isLesser(TimeDuration other) { // operator <
        return comparesTo(other) < 0;
    }

    public boolean isLesserOrEqual(TimeDuration other) { // operator <=
        return comparesTo(other) <= 0;
    }

    public boolean isGreater(TimeDuration other) { // operator >
        return comparesTo(other) > 0;
    }

    public boolean isGreaterOrEqual(TimeDuration other) { // operator >
        return comparesTo(other) >= 0;
    }

    public TimeDuration plusEquals(TimeDuration other) {
        m_Seconds += other.m_Seconds;
        m_Fractions += other.m_Fractions;
        if (m_Fractions > TIME_UNITS.ATTOSEC.duration()) {
            m_Seconds += 1;
            m_Fractions -= TIME_UNITS.ATTOSEC.duration();
        }
        return this;
    }

    public TimeDuration plus(TimeDuration other) {
        TimeDuration result = new TimeDuration(this);
        return result.plusEquals(other);
    }

    public TimeDuration minusEquals(TimeDuration other) {
        m_Seconds -= other.m_Seconds;
        if (m_Fractions < other.m_Fractions) {
            m_Fractions = (TIME_UNITS.ATTOSEC.duration() + m_Fractions) - other.m_Fractions;
            m_Seconds -= 1;
        } else {
            m_Fractions -= other.m_Fractions;
        }
        return this;
    }

    public TimeDuration minus(TimeDuration other) {
        TimeDuration result = new TimeDuration(this);
        return result.minusEquals(other);
    }

    public double divide(TimeDuration other) {
        BigDecimal eighteenth = new BigDecimal("0.000000000000000001");
        BigDecimal thisValue = BigDecimal.valueOf(m_Seconds);
        BigDecimal fraction = BigDecimal.valueOf(m_Fractions);
        thisValue.add(fraction.multiply(eighteenth));
        BigDecimal otherValue = BigDecimal.valueOf(other.m_Seconds);
        BigDecimal otherFraction = BigDecimal.valueOf(other.m_Fractions);
        otherValue.add(otherFraction.multiply(eighteenth));
        return (thisValue.divide(otherValue)).doubleValue();
    }

    public void fromHash(Hash hash) {
        m_Seconds = hash.get("seconds");
        m_Fractions = hash.get("fractions");
    }

    public void toHash(Hash hash) {
        hash.set("seconds", getSeconds());
        hash.set("fractions", getFractions(TIME_UNITS.ATTOSEC));
    }

    public static void setDefaultFormat(String fmt) {
        DEFAULT_FORMAT = fmt;
    }

    @Override
    public String toString() {
        return format(DEFAULT_FORMAT);
    }

    public static String toString(TimeDuration duration) {
        return duration.toString();
    }
}
