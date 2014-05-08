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
import karabo.util.vectors.VectorHash;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
@KARABO_CLASSINFO(classId = "AbstractInput", version = "1.0")
public class AbstractInput extends ClassInfo {

    private InputHandler m_handlers = null;
    private String m_instanceId;

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(AbstractInput.class, AbstractInput.class);
        //System.out.println("AbstractInput class static registration");
    }

    public static void expectedParameters(Schema expected) {
    }

    public AbstractInput() {
    }

    public AbstractInput(Hash configuration) {
    }

    public void reconfigure(Hash configuration) {
    }

    public void setInstanceId(String id) {
        m_instanceId = id;
    }

    public String getInstanceId() {
        return m_instanceId;
    }

    public void setInputHandlerType(String handlerType) {
        String capitalType = handlerType.toUpperCase();
        if (capitalType == "JAVA") {
            m_handlers = InputHandler.create(InputHandler.class, "JavaInputHandler");
        } else {
            throw new RuntimeException("Handler type " + handlerType + " is not supported.  Supported type (case-insensitive) is Java");
        }
    }

    public InputHandler getInputHandler() {
        return m_handlers;
    }

    public void registerIOEventHandler(Object ioEventHandler) {
        if (m_handlers == null) {
            throw new RuntimeException("Handler storage not initialized: call 'setInputHandlerType' first.");
        }
        m_handlers.registerIOEventHandler(ioEventHandler);
    }

    public void registerEndOfStreamEventHandler(Object endOfStreamEventHandler) {
        if (m_handlers == null) {
            throw new RuntimeException("Handler storage not initialized: call 'setInputHandlerType' first.");
        }
        m_handlers.registerEndOfStreamEventHandler(endOfStreamEventHandler);
    }

    public boolean needsDeviceConnection() {
        return false;
    }

    public VectorHash getConnectedOutputChannels() {
        return new VectorHash();
    }

    public void connectNow(Hash outputChannelInfo) {
    }

    public boolean canCompute() {
        return true;
    }

    public void update() {
    }

    public void setEndOfStream() {
    }

    public boolean respondsToEndOfStream() {
        return true;
    }

    protected void triggerIOEvent() {
        if (m_handlers != null) {
            m_handlers.triggerIOEvent();
        }
    }

    protected void triggerEndOfStreamEvent() {
        if (m_handlers != null) {
            m_handlers.triggerEndOfStreamEvent();
        }
    }
}
