/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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

            virtual void registerIOEventHandler(const boost::any& eventHandler) = 0;
            virtual void triggerIOEvent() = 0;
        };
    } // namespace io
} // namespace karabo

#endif /* KARABO_IO_OUTPUTHANDLER_HH */
