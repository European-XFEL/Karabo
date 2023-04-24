/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#include "CppInputHandler.hh"

#include <karabo/io/Input.hh>


KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler,
                             karabo::io::CppInputHandler<karabo::io::Input<karabo::util::Hash> >,
                             karabo::io::AbstractInput::Pointer);
KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler,
                             karabo::io::CppInputHandler<karabo::io::Input<std::vector<char> > >,
                             karabo::io::AbstractInput::Pointer);
