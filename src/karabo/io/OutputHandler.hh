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
 * File:   OutputHandler.hh
 * Author: esenov
 *
 * Created on September 17, 2013, 6:08 PM
 */

#ifndef KARABO_IO_OUTPUTHANDLER_HH
#define KARABO_IO_OUTPUTHANDLER_HH

#include <karabo/util/Configurator.hh>

namespace karabo {
    namespace io {

        class OutputHandler {
           public:
            KARABO_CLASSINFO(OutputHandler, "OutputHandler", "1.0")

            virtual void registerIOEventHandler(const std::any& eventHandler) = 0;
            virtual void triggerIOEvent() = 0;
        };
    } // namespace io
} // namespace karabo

#endif /* KARABO_IO_OUTPUTHANDLER_HH */
