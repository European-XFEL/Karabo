/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.time;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public enum TIME_UNITS {
    // Fractions

    ATTOSEC(1L), // Atto-second is the smallest time unit = highest resolution for time values.
    FEMTOSEC(1000L * ATTOSEC.duration()),
    PICOSEC(1000L * FEMTOSEC.duration()),
    NANOSEC(1000L * PICOSEC.duration()),
    MICROSEC(1000L * NANOSEC.duration()),
    MILLISEC(1000L * MICROSEC.duration()),
    ONESECOND(1000L * MILLISEC.duration()),
    SECOND(1L), // Base unit
    // Multiples
    MINUTE(60L * SECOND.duration()),
    HOUR(60L * MINUTE.duration()),
    DAY(24L * HOUR.duration());
    
    private final long m_unit;

    TIME_UNITS(long unit) {
        m_unit = unit;
    }

    public long duration() {
        return m_unit;
    }
}
