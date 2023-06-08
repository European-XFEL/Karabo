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
#include "CppInputHandler.hh"

#include <karabo/io/Input.hh>


KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler,
                             karabo::io::CppInputHandler<karabo::io::Input<karabo::util::Hash> >,
                             karabo::io::AbstractInput::Pointer);
KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler,
                             karabo::io::CppInputHandler<karabo::io::Input<std::vector<char> > >,
                             karabo::io::AbstractInput::Pointer);
