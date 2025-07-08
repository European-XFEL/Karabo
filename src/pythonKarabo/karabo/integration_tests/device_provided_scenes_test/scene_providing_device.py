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
from karabo.bound import (
    KARABO_CLASSINFO, VECTOR_STRING_ELEMENT, Hash, PythonDevice)
from karabo.common.scenemodel.api import LabelModel, SceneModel, write_scene
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
        self.KARABO_SLOT(self.requestScene)

    def initialize(self):
        self.updateState(State.NORMAL)

    def requestScene(self, params):
        """Fulfill a scene request from another device.

         NOTE: Required by Scene Supply Protocol, which is defined in KEP 21.
               The format of the reply is also specified there.

        :param params: A `Hash` containing the method parameters
        """
        payload = Hash()

        name = params.get('name', default='')
        if name == 'scene':
            payload.set('name', name)
            payload.set('data', get_scene(self.getInstanceId()))
        else:
            raise RuntimeError(f"No scene with name {name}")
        self.reply(Hash('type', 'deviceScene', 'origin', self.getInstanceId(),
                        'payload', payload))


def get_scene(deviceId):
    scene0 = LabelModel(
        font='Source Sans Pro,11,-1,5,75,0,0,0,0,0',
        foreground='#000000', height=31.0,
        parent_component='DisplayComponent', text=f'{deviceId}',
        width=68.0, x=10.0, y=10.0)

    scene = SceneModel(
        height=100.0, width=100.0, children=[scene0])
    return write_scene(scene)
