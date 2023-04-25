# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.native import Configurable, Float, Int8, Int32


class Other(Configurable):
    prop = Int32(minExc=0, maxExc=5)


class Object(Configurable):
    prop = Float(minInc=-2.0, maxInc=4.0)


class LargeRange(Configurable):
    prop = Float(minInc=-2e7, maxInc=4e7)


class InRangeInt(Configurable):
    prop = Int8(minInc=-128, maxInc=127)
