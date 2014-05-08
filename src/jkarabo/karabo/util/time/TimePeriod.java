/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.time;

import karabo.util.Attributes;
import karabo.util.Hash;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class TimePeriod {

    private boolean m_Open;
    private Epochstamp m_Start;
    private Epochstamp m_Stop;

    public TimePeriod() {
        m_Open = false;
    }

    /**
     * Constructs a time period from Hash. Start and stop timestamps are stored
     * under the two reserved keys "KRB_start" and "KRB_stop", respectively.
     *
     * @param hash Hash object ("KRB_start", any, "KRB_stop", any)
     */
    public TimePeriod(Hash hash) {
        m_Start = Epochstamp.fromHashAttributes(hash.getAttributes("KRB_start"));
        m_Stop = Epochstamp.fromHashAttributes(hash.getAttributes("KRB_stop"));
        m_Open = false;
        if (hash.has("KRB_open")) {
            hash.get("KRB_open");
        }
    }

    /**
     * Constructs a time period from two given timestamps
     *
     * @param start Epochstamp object
     * @param stop Epochstamp object
     */
    public TimePeriod(Epochstamp start, Epochstamp stop) {
        m_Start = start;
        m_Stop = stop;
        m_Open = false;
    }

    public TimePeriod(long nanostart, long nanostop) {
        
    }
    /**
     * Return the time duration (i.e. length) of a time period.
     *
     * @return TimeDuration object
     */
    public TimeDuration getDuration() {
        if (m_Open) {
            return new TimeDuration(-1, -1);
        }
        return m_Stop.minus(m_Start);
    }

    /**
     * Get the start (resp. stop) timestamp
     *
     * @return Epochstamp object
     */
    public Epochstamp getStart() {
        return m_Start;

    }

    public Epochstamp getStop() {
        return m_Stop;

    }

    /**
     * Set the start (resp. stop) timestamp. By default, it set it to the
     * current system epoch timestamp.
     *
     * @param stamp Epochstamp (by default, use the current system timestamp)
     */
    public void start(Epochstamp stamp) {
        m_Start = stamp;
        m_Open = true;
    }

    public void start() {
        start(new Epochstamp());
    }

    public void stop(Epochstamp stamp) {
        m_Open = false;
        m_Stop = stamp;
    }

    public void stop() {
        stop(new Epochstamp());
    }

    /**
     * Check if period is still open (i.e. not yet stopped)
     *
     * @return bool
     */
    public boolean isOpen() {
        return m_Open;
    }

    /**
     * Check if time point (timestamp) is before, within, or after a time
     * period.
     *
     * @param tm Epochstamp object
     * @return bool
     */
    public boolean before(Epochstamp tm) {
        return !(m_Open || m_Stop.isGreater(tm));
    }

    public boolean contain(Epochstamp tm) {
        return !(before(tm) || after(tm));
    }

    public boolean after(Epochstamp tm) {
        return (m_Start.isGreaterOrEqual(tm));
    }

    /**
     * Serialize time period to and from Hash object.
     *
     * @param hash Hash object
     */
    public void fromHash(Hash hash) {
        m_Start = Epochstamp.fromHashAttributes(hash.getAttributes("KRB_start"));
        m_Stop = Epochstamp.fromHashAttributes(hash.getAttributes("KRB_stop"));
        m_Open = false;
        if (hash.has("KRB_open")) {
            m_Open = hash.get("KRB_open");
        }
    }

    public void toHash(Hash hash) {
        Attributes now = new Attributes();
        hash.set("KRB_start", "");
        m_Start.toHashAttributes(now);
        hash.setAttributes("KRB_start", now);

        hash.set("KRB_stop", "");
        m_Stop.toHashAttributes(now);
        hash.setAttributes("KRB_stop", now);

        hash.set("KRB_open", m_Open);
    }
    
//            operator karabo::util::Hash() {
//                karabo::util::Hash hash;
//                toHash(hash);
//                return hash;
//            }
}
