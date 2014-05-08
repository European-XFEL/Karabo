/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.time;

import karabo.util.Attributes;
import karabo.util.time.Trainstamp;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Timestamp {

    private Epochstamp m_epochstamp;
    private Trainstamp m_trainstamp;

    public Timestamp() {
    }

    public Timestamp(Epochstamp e, Trainstamp t) {
    }

    public long getSeconds() {
        return m_epochstamp.getSeconds();
    }

    public long getFractionalSeconds() {
        return m_epochstamp.getFractionalSeconds();
    }

    public long getTrainId() {
        return 0;
        //return m_trainstamp.getTrainId();
    }

    public static boolean hashAttributesContainTimeInformation(Attributes attributes) {
        return (Epochstamp.hashAttributesContainTimeInformation(attributes) && Trainstamp.hashAttributesContainTimeInformation(attributes));
    }

    /**
     * Creates an Timestamp from three Hash attributes This function throws in
     * case the attributes do no provide the correct information
     *
     * @param attributes Hash attributes
     * @return Timestamp object
     */
    public static Timestamp fromHashAttributes(Attributes attributes) {
        return new Timestamp();
        //return new Timestamp(Epochstamp.fromHashAttributes(attributes), Trainstamp.fromHashAttributes(attributes));
    }

    /**
     * Creates an EpochStamp from an ISO 8601 formatted string (or other set of
     * predefined formats)
     *
     * @param timePoint ISO 8601 formatted string (see formats locale to more
     * information)
     * @return EpochStamp object
     */
    public static Epochstamp fromIso8601(String timePoint) {
        return Epochstamp.fromIso8601(timePoint);
    }

    /**
     * Generates a string (respecting ISO-8601) for object time for INTERNAL
     * usage ("%Y%m%dT%H%M%S%f" to "20121225T132536.789333[123456789123]")
     * "yyyyMMddTHHmmss.f"
     *
     * @param precision - Indicates the precision of the fractional seconds
     * (e.g. MILLISEC, MICROSEC, NANOSEC, PICOSEC, FEMTOSEC, ATTOSEC)
     * @return ISO 8601 formatted string (extended or compact)
     */
    public String toIso8601(TIME_UNITS precision) {
        return toIso8601(precision);
    }
    
    public String toIso8601() {
        return this.toIso8601(TIME_UNITS.MICROSEC);
    }

    /**
     * Formats to specified format time stored in the object
     *
     * @param format The format of the time point (visit strftime for more info:
     * http://www.cplusplus.com/reference/ctime/strftime/)
     * @return formated string
     */
    public String toFormattedString(String format) {
        return "";
    }

    public String toFormattedString() {
        return toFormattedString("yyyy-MM-dd HH:mm:ss");
    }

    /**
     * Formats as Hash attributes
     *
     * @param attributes container to which the time point information is added
     */
    public void toHashAttributes(Attributes attributes) {
        m_epochstamp.toHashAttributes(attributes);
        m_trainstamp.toHashAttributes(attributes);
    }
}
