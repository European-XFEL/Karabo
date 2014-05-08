/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.io;

import karabo.util.ClassInfo;
import karabo.util.KARABO_CLASSINFO;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
@KARABO_CLASSINFO(classId = "InputHandler", version = "1.0")
public abstract class InputHandler extends ClassInfo {

    public abstract void registerIOEventHandler(Object eventHandler);

    public abstract void registerEndOfStreamEventHandler(Object endOfStreamEventHandler);

    public abstract void triggerIOEvent();

    public abstract void triggerEndOfStreamEvent();
}
