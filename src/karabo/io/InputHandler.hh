/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   InputHandler.hh
 * Author: esenov
 *
 * Created on September 17, 2013, 4:49 PM
 */

#ifndef KARABO_IO_INPUTHANDLER_HH
#define KARABO_IO_INPUTHANDLER_HH

#include <karabo/util/Configurator.hh>


namespace karabo {
    namespace io {

        /**
         * @class InputHandler
         * @brief The InputHandler class is the base class for registering and accessing I/O and end of stream input
         * handlers
         */
        class InputHandler {
           public:
            KARABO_CLASSINFO(InputHandler, "InputHandler", "1.0")

            /**
             * Register an input handler for I/O events. The handler must be of a callable type.
             * @param eventHandler
             */
            virtual void registerIOEventHandler(const boost::any& eventHandler) = 0;

            /**
             * Register an input handler for end of stream events. The handler must be of a callable type.
             * @param endOfStreamEventHandler
             */
            virtual void registerEndOfStreamEventHandler(const boost::any& endOfStreamEventHandler) = 0;

            /**
             * Trigger an I/O event on the registered handler
             */
            virtual void triggerIOEvent() = 0;

            /**
             * Trigger an end of stream event on the registered handler
             */
            virtual void triggerEndOfStreamEvent() = 0;
        };
    } // namespace io
} // namespace karabo

#endif /* KARABO_IO_INPUTHANDLER_HH */
