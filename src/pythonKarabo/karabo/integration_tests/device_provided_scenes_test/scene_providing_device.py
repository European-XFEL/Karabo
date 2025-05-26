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
from karabo.bound import KARABO_CLASSINFO, VECTOR_STRING_ELEMENT, PythonDevice
from karabo.common.states import State


@KARABO_CLASSINFO("SceneProvidingDevice", "2.0")
class SceneProvidingDevice(PythonDevice):

    def expectedParameters(expected):

        (
            VECTOR_STRING_ELEMENT(expected).key("availableScenes")
            .readOnly().initialValue([])
            .commit(),
        )

    def __init__(self, config):
        super().__init__(config)
        self.registerInitialFunction(self.initialize)

    def initialize(self):
        self.updateState(State.NORMAL)
