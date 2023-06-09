# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import pytest

from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Configurable, DeviceNode, Double,
    Float, Int32, MetricPrefix, Node, Overwrite, Slot, State, Unit, unit)
from karabo.middlelayer_api.injectable import InjectMixin
from karabo.middlelayer_api.tests import eventloop


def test_deviceNode():
    class A(Configurable):
        node = DeviceNode()

    a = A({"node": "remote"})
    schema = a.getClassSchema()
    assert schema.hash['node', 'accessMode'] == AccessMode.INITONLY.value
    assert schema.hash['node', 'assignment'] == Assignment.MANDATORY.value


def test_deviceNode_default():
    class A(Configurable):
        node = DeviceNode(defaultValue="remote")

    a = A()
    schema = a.getClassSchema()
    assert schema.hash['node', 'accessMode'] == AccessMode.INITONLY.value
    assert schema.hash['node', 'assignment'] == Assignment.MANDATORY.value
    assert schema.hash['node', 'defaultValue'] == "remote"


@pytest.mark.asyncio
async def test_overwrite_inject(event_loop: eventloop):
    class Mandy(InjectMixin):
        number = Int32(displayedName="whatever", minExc=7,
                       accessMode=AccessMode.READONLY,
                       allowedStates={State.ON}, tags=set(),
                       unitSymbol=Unit.METER,
                       metricPrefixSymbol=MetricPrefix.MILLI,
                       options=[8, 9, 10])

        numberEnum = Int32(displayedName="EnumAccess",
                           defaultValue=AccessLevel.OPERATOR,
                           enum=AccessLevel)

        @Slot(displayedName="MandyRandy", allowedStates=[State.INIT])
        def randyMandy(self):
            pass

        deviceId = None

        def _register_slots(self):
            pass

        def _notifyNewSchema(self):
            pass

        def signalChanged(self, deviceId, hsh):
            pass

    mandy = Mandy()
    setter_before_inject = mandy.__class__.number.setter
    mandy.__class__.number = Overwrite(
        minExc=3, allowedStates={State.OFF},
        accessMode=AccessMode.INITONLY,
        unitSymbol=Unit.SECOND, metricPrefixSymbol=MetricPrefix.MEGA,
        tags={"naughty"}, options=[6, 4])

    mandy.__class__.numberEnum = Overwrite(defaultValue=AccessLevel.ADMIN,
                                           options=[AccessLevel.ADMIN])

    mandy.__class__.randyMandy = Overwrite(
        displayedName="NoMandy", allowedStates=[State.ON]
    )
    await mandy.publishInjectedParameters()
    setter_after_inject = mandy.__class__.number.setter
    assert mandy.number.descriptor.key == "number"
    assert Mandy.number.minExc == 7
    assert mandy.number.descriptor.minExc == 3
    assert Mandy.number.displayedName == "whatever"
    assert mandy.number.descriptor.displayedName == "whatever"
    assert Mandy.number.allowedStates == {State.ON}
    assert mandy.number.descriptor.allowedStates == {State.OFF}
    assert Mandy.number.tags == set()
    assert mandy.number.descriptor.tags == {"naughty"}
    assert Mandy.number.accessMode is AccessMode.READONLY
    assert mandy.number.descriptor.accessMode is AccessMode.INITONLY
    assert Mandy.number.unitSymbol is Unit.METER
    assert mandy.number.descriptor.unitSymbol is Unit.SECOND
    assert Mandy.number.units == unit.millimeter
    assert mandy.number.descriptor.units == unit.megasecond
    assert Mandy.number.options == [8, 9, 10]
    assert mandy.number.descriptor.options == [6, 4]
    assert mandy.numberEnum.descriptor.options == [AccessLevel.ADMIN]
    assert mandy.numberEnum.descriptor.defaultValue == AccessLevel.ADMIN
    assert mandy.randyMandy.descriptor.displayedName == "NoMandy"
    assert mandy.randyMandy.descriptor.allowedStates == {State.ON}
    assert setter_before_inject == setter_after_inject


@pytest.mark.asyncio
async def test_inject_parameter(event_loop: eventloop):
    class Mandy(InjectMixin):
        number = Int32(displayedName="whatever", minExc=7,
                       accessMode=AccessMode.READONLY,
                       allowedStates={State.ON}, tags=set(),
                       unitSymbol=Unit.METER,
                       metricPrefixSymbol=MetricPrefix.MILLI,
                       options=[8, 9, 10])

        numberEnum = Int32(displayedName="EnumAccess",
                           defaultValue=AccessLevel.OPERATOR,
                           enum=AccessLevel)

        deviceId = None

        def _register_slots(self):
            pass

        def _notifyNewSchema(self):
            pass

        def signalChanged(self, deviceId, hsh):
            pass

    mandy = Mandy()
    mandy.__class__.newInt = Int32(displayedName="New Int")
    await mandy.publishInjectedParameters(newInt=5)
    assert mandy.newInt == 5
    # Run injectParameters new but no new injected descriptor
    # and try to set a new value which will fail as expected
    await mandy.publishInjectedParameters(newInt=15)
    assert mandy.newInt != 15
    assert mandy.newInt == 5
    mandy.__class__.newDouble = Double(displayedName="New Double")
    await mandy.publishInjectedParameters("newDouble", 15.12)
    assert mandy.newDouble == 15.12

    # args will override kwargs!
    mandy.__class__.newFloat = Float(displayedName="New Float")
    await mandy.publishInjectedParameters("newFloat", 7.12, newFloat=3.43)
    assert mandy.newFloat == pytest.approx(7.12, 0.001)


@pytest.mark.asyncio
async def test_inject_node(event_loop: eventloop):
    class Mandy(InjectMixin):
        integer = Int32(defaultValue=0)

        deviceId = None

        def _register_slots(self):
            pass

        def _notifyNewSchema(self):
            pass

        def signalChanged(self, deviceId, hsh):
            pass

    mandy = Mandy()
    mandy.__class__.nested = Node(Mandy)
    mandy.__class__.extraInteger = Int32(defaultValue=10)
    await mandy.publishInjectedParameters("nested", {"integer": 5})
    assert mandy.integer == 0
    assert mandy.nested.integer == 5


@pytest.mark.asyncio
async def test_inject_raise_parameter(event_loop: eventloop):
    class Mandy(InjectMixin):
        integer = Int32(defaultValue=0)

        deviceId = None

        def _register_slots(self):
            pass

        def _notifyNewSchema(self):
            pass

        def signalChanged(self, deviceId, hsh):
            pass

    mandy = Mandy()
    mandy.__class__.extraInteger = Int32(defaultValue=10)
    mandy.__class__.newInteger = Int32(defaultValue=2)
    # Odd number of arguments must fail
    with pytest.raises(RuntimeError):
        await mandy.publishInjectedParameters("extraInteger", 12, 12)

    # Odd number of arguments must fail
    with pytest.raises(RuntimeError):
        await mandy.publishInjectedParameters(
            "extraInteger", 12, "newInteger")
