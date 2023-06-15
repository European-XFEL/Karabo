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

#include "ConfigurationTestClasses.hh"

using namespace configurationTest;

KARABO_REGISTER_FOR_CONFIGURATION(Shape, Circle)
KARABO_REGISTER_FOR_CONFIGURATION(Shape, Circle, EditableCircle)
KARABO_REGISTER_FOR_CONFIGURATION(Shape, Rectangle)
KARABO_REGISTER_FOR_CONFIGURATION(GraphicsRenderer)
KARABO_REGISTER_FOR_CONFIGURATION(TestStruct1, TestStruct2)
KARABO_REGISTER_FOR_CONFIGURATION(SchemaNodeElements)
KARABO_REGISTER_FOR_CONFIGURATION(SomeClass)
