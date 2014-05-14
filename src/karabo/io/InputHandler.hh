/* 
 * File:   InputHandler.hh
 * Author: esenov
 *
 * Created on September 17, 2013, 4:49 PM
 */

#ifndef KARABO_IO_INPUTHANDLER_HH
#define	KARABO_IO_INPUTHANDLER_HH

#include <karabo/util/Configurator.hh>


namespace karabo {
    namespace io {

        class InputHandler {
        public:
            
            KARABO_CLASSINFO(InputHandler, "InputHandler", "1.0")
            
            virtual void registerIOEventHandler(const boost::any& eventHandler) = 0;
            virtual void registerEndOfStreamEventHandler(const boost::any& endOfStreamEventHandler) = 0;
            virtual void triggerIOEvent() = 0;
            virtual void triggerEndOfStreamEvent() = 0;
        };
    }
}

#endif	/* KARABO_IO_INPUTHANDLER_HH */

