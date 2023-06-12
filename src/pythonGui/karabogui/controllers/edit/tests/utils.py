# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from karabo.native import Configurable, Float, Int8, Int32


class Other(Configurable):
    prop = Int32(minExc=0, maxExc=5)


class Object(Configurable):
    prop = Float(minInc=-2.0, maxInc=4.0)


class LargeRange(Configurable):
    prop = Float(minInc=-2e7, maxInc=4e7)


class InRangeInt(Configurable):
    prop = Int8(minInc=-128, maxInc=127)
