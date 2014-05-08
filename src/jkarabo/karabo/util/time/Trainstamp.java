/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.time;

import karabo.util.Attributes;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Trainstamp {

    private long m_trainId;

    public Trainstamp() {
    }

    public Trainstamp(long trainId) {
        m_trainId = trainId;
    }

    public long getTrainId() {
        return m_trainId;
    }
    
    public static boolean hashAttributesContainTimeInformation(Attributes attributes) {
        return attributes.has("tid");
    }
    
    public static Trainstamp fromHashAttributes(Attributes attributes) {
        long tid;
        try {
            tid = (long) attributes.get("tid");
        } catch(Exception e) {
            throw new RuntimeException("Failed to access \"tid\" attribute: " + e);
        }
        return new Trainstamp(tid);
    }
    
    public void toHashAttributes(Attributes attributes) {
        attributes.set("tid", m_trainId);
    }
}
