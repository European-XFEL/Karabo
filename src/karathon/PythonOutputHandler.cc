/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#include "PythonOutputHandler.hh"

namespace karathon {
    KARABO_REGISTER_IN_FACTORY(karabo::io::OutputHandler, PythonOutputHandler);

    KARABO_REGISTER_IN_FACTORY_1(karabo::io::OutputHandler, PythonOutputHandler, karabo::io::AbstractOutput::Pointer);
} // namespace karathon
