/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#include "PythonInputHandler.hh"

#include <karabo/io/Input.hh>

namespace karathon {
    // KARABO_REGISTER_IN_FACTORY(karabo::io::InputEventHandler, PythonInputHandler);

    // KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputEventHandler, PythonInputHandler,
    // karabo::io::AbstractInput::Pointer);

    KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler, PythonInputHandler<karabo::io::Input<karabo::util::Hash> >,
                                 karabo::io::AbstractInput::Pointer);
    KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler, PythonInputHandler<karabo::io::Input<std::vector<char> > >,
                                 karabo::io::AbstractInput::Pointer);
} // namespace karathon
