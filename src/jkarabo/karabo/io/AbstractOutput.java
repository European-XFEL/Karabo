/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.io;

import karabo.util.ClassInfo;
import karabo.util.Hash;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Schema;

/**
 *
 * @author esenov
 */
@KARABO_CLASSINFO(classId = "AbstractOutput", version = "1.0")
public class AbstractOutput extends ClassInfo {

    protected String m_instanceId;
    protected OutputHandler m_handlers;

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(AbstractOutput.class, AbstractOutput.class);
        //System.out.println("AbstractOutput class static registration");
    }

    public static void expectedParameters(Schema expected) {
    }

    public AbstractOutput() {
    }

    public AbstractOutput(Hash configuration) {
    }

    public void setInstanceId(String id) {
        m_instanceId = id;
    }

    public String getInstanceId() {
        return m_instanceId;
    }

    public void setOutputHandlerType(String handlerType) {
        String capitalType = handlerType.toUpperCase();
        if (capitalType == "JAVA") {
            m_handlers = OutputHandler.create(OutputHandler.class, "JavaOutputHandler");
        } else {
            throw new RuntimeException("Handler type " + handlerType + " is not supported.  Supported types (case-insensitive) is Java");
        }
    }

    public OutputHandler getOutputHandler() {
        return m_handlers;
    }

    public void registerIOEventHandler(Object ioEventHandler) {
        if (m_handlers == null) {
            throw new RuntimeException("Handler storage not initialized: call 'setOutputHandler' first.");
        }
        m_handlers.registerIOEventHandler(ioEventHandler);
    }

    public Hash getInformation() {
        return new Hash();
    }

    public void update() {
    }

    public void signalEndOfStream() {
    }

    public boolean canCompute() {
        return true;
    }

    protected void triggerIOEvent() {
        if (m_handlers != null) {
            m_handlers.triggerIOEvent();
        }
    }
}
