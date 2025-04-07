/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   InputHandler.hh
 * Author: esenov
 *
 * Created on September 17, 2013, 4:49 PM
 */

#ifndef KARABO_DATA_IO_INPUTHANDLER_HH
#define KARABO_DATA_IO_INPUTHANDLER_HH

#include "karabo/data/schema/Configurator.hh"


namespace karabo {
    namespace data {

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
            virtual void registerIOEventHandler(const std::any& eventHandler) = 0;

            /**
             * Register an input handler for end of stream events. The handler must be of a callable type.
             * @param endOfStreamEventHandler
             */
            virtual void registerEndOfStreamEventHandler(const std::any& endOfStreamEventHandler) = 0;

            /**
             * Trigger an I/O event on the registered handler
             */
            virtual void triggerIOEvent() = 0;

            /**
             * Trigger an end of stream event on the registered handler
             */
            virtual void triggerEndOfStreamEvent() = 0;
        };
    } // namespace data
} // namespace karabo

#endif /* KARABO_IO_INPUTHANDLER_HH */
