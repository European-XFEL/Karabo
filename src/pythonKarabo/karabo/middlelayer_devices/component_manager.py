#############################################################################
# Author: degon
# Created on October 06, 2020, 14:12 AM
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from asyncio import TimeoutError, wait_for
from collections import ChainMap

from karabo.common.scenemodel.api import (
    BoxLayoutModel, DisplayCommandModel, DisplayLabelModel, DisplayListModel,
    DisplayStateColorModel, DisplayTextLogModel, ErrorBoolModel, LabelModel,
    LineEditModel, IntLineEditModel, LineModel, SceneModel, TableElementModel,
    write_scene)

from karabo.middlelayer import (
    AccessLevel, AccessMode, allCompleted, Assignment, background, Bool,
    connectDevice, Configurable, DaqPolicy, Device, Hash,
    Overwrite, sleep, slot, Slot, State, String, UInt32, VectorHash,
    VectorString, waitUntilNew)

KARABO_CONFIGURATION_MANAGER = "KaraboConfigurationManager"
DEVICE_TIMEOUT = 5


class RowSchema(Configurable):
    name = String(
        defaultValue="",
        description="The name of the configuration",
        accessMode=AccessMode.READONLY)

    description = String(
        defaultValue="",
        description="The description of the configuration",
        accessMode=AccessMode.READONLY)

    priority = UInt32(
        defaultValue=1,
        description="The priority of the configuration",
        accessMode=AccessMode.READONLY)

    user = String(
        defaultValue="",
        description="The user of the configuration",
        accessMode=AccessMode.READONLY)

    min_timepoint = String(
        defaultValue="",
        description="The minimum timepoint when a configuration was saved",
        accessMode=AccessMode.READONLY)

    max_timepoint = String(
        defaultValue="",
        description="The maximum timepoint when a configuration was saved",
        accessMode=AccessMode.READONLY)

    diff_timepoint = String(
        defaultValue="",
        description="The differences between max and min timepoint",
        accessMode=AccessMode.READONLY)


class ComponentManager(Device):
    """This component manager service is to control device configurations
    """
    state = Overwrite(
        defaultValue=State.INIT,
        options=[State.INIT, State.CHANGING, State.UNKNOWN, State.ON])

    visibility = Overwrite(
        defaultValue=AccessLevel.ADMIN,
        options=[AccessLevel.ADMIN])

    deviceNames = VectorString(
        defaultValue=[],
        description="The devices to control",
        requiredAccessLevel=AccessLevel.OPERATOR,
        assignment=Assignment.MANDATORY,
        accessMode=AccessMode.INITONLY)

    deviceHeartBeat = String(
        displayType="State",
        description="Are the devices of the component online?",
        defaultValue=State.INIT,
        enum=State,
        options=[State.ON, State.ERROR, State.INIT],
        accessMode=AccessMode.READONLY)

    pingBeat = UInt32(
        displayedName="Beat interval",
        description="The interval to check for slotPing of the devices",
        defaultValue=10,
        minInc=2,
        maxInc=30,
        requiredAccessLevel=AccessLevel.EXPERT)

    configurationName = String(
        defaultValue="default",
        description="The configuration name",
        requiredAccessLevel=AccessLevel.OPERATOR)

    priority = UInt32(
        defaultValue=1,
        minInc=1,
        maxInc=3,
        description="The priority of the configuration",
        requiredAccessLevel=AccessLevel.OPERATOR)

    description = String(
        defaultValue="",
        description="The configuration description",
        requiredAccessLevel=AccessLevel.OPERATOR)

    availableScenes = VectorString(
        displayedName="Available Scenes",
        displayType="Scenes",
        description="Provides a scene for the Configuration Manager.",
        accessMode=AccessMode.READONLY,
        daqPolicy=DaqPolicy.OMIT,
        defaultValue=['scene'])

    lastSuccess = Bool(
        defaultValue=True,
        displayedName="Last Success",
        description="Indicates if the last action was successful",
        accessMode=AccessMode.READONLY)

    view = VectorHash(
        rows=RowSchema,
        defaultValue=[],
        displayedName="View",
        accessMode=AccessMode.READONLY,
        daqPolicy=DaqPolicy.OMIT)

    @slot
    def requestScene(self, params):
        """Fulfill a scene request from another device.

        :param params: A `Hash` containing the method parameters
        """
        payload = Hash('success', False)
        name = params.get('name', default='scene')
        if name == 'scene':
            payload.set('success', True)
            payload.set('name', name)
            payload.set('data', get_scene(self.deviceId))

        return Hash('type', 'deviceScene',
                    'origin', self.deviceId,
                    'payload', payload)

    @Slot(displayedName="List configurations",
          requiredAccessLevel=AccessLevel.OPERATOR,
          allowedStates=[State.ON])
    async def listConfigurations(self):
        """List all configurations for the component"""
        self.state = State.CHANGING
        background(self._list_configurations())

    async def _list_configurations(self):
        """Internal method to list the configuration of the component"""
        h = Hash("deviceIds", self.deviceNames.value)
        try:
            reply = await wait_for(self.call(KARABO_CONFIGURATION_MANAGER,
                                             "slotListConfigurationSets", h),
                                   timeout=DEVICE_TIMEOUT)
            items = reply["items"]
        except (Exception, TimeoutError):
            self.lastSuccess = False
            items = []
        else:
            self.lastSuccess = True
        finally:
            self.view = items
            self.state = State.ON

    @Slot(displayedName="Save configurations",
          requiredAccessLevel=AccessLevel.OPERATOR,
          allowedStates=[State.ON])
    async def saveConfigurations(self):
        """Save a configuration for the component"""
        self.state = State.CHANGING
        background(self._save_configurations())

    async def _save_configurations(self):
        ping = await self._ping_beats_action()
        # Before saving we make sure all devices are there again!
        if not ping:
            self.status = "Not all devices are online responding..."
            self.lastSuccess = False
            self.state = State.ON
            return

        h = Hash("deviceIds", self.deviceNames.value,
                 "name", self.configurationName.value,
                 "priority", int(self.priority.value),
                 "description", self.description.value)
        try:
            timeout = DEVICE_TIMEOUT * len(self.deviceNames.value)
            await wait_for(self.call(KARABO_CONFIGURATION_MANAGER,
                                     "slotSaveConfigurationFromName", h),
                           timeout=timeout)
        except TimeoutError:
            self.lastSuccess = False
            self.status = "Saving configurations failed due to timeout!"
        except Exception:
            self.lastSuccess = False
            self.status = ("Saving configurations failed. The configuration "
                           " name is already taken.")
        else:
            await self._list_configurations()

    def __init__(self, configuration):
        super(ComponentManager, self).__init__(configuration)
        self.db = None
        self._beat_task = None
        self._db_monitor = None

    async def onInitialization(self):
        """Initialize the component manager device"""
        self.db = await connectDevice(KARABO_CONFIGURATION_MANAGER)
        self.state = State.ON

        # If we have a device name already we can retrieve a list!
        if self.deviceNames:
            background(self._list_configurations())

        self._db_monitor = background(self._monitor_db_state())
        self._beat_task = background(self._ping_beats())

    async def _ping_beats(self):
        while True:
            await self._ping_beats_action()
            await sleep(self.pingBeat.value)

    async def _ping_beats_action(self):
        fut = {deviceId: self.call(deviceId, "slotPing", deviceId, 1, True)
               for deviceId in self.deviceNames}

        _, *fails = await allCompleted(**fut, timeout=DEVICE_TIMEOUT)
        chain = ChainMap(*fails)
        if chain.items():
            status = ("The following device(s) could not be reached: "
                      "{}".format(", ".join(chain.keys())))
            if self.status != status:
                self.status = status
            state = State.ERROR
        else:
            state = State.ON
        if state != self.deviceHeartBeat:
            self.deviceHeartBeat = state

        return state == State.ON

    async def _monitor_db_state(self):
        while True:
            await waitUntilNew(self.db.state)
            state = self.db.state
            if state != self.state:
                self.state = state

    async def onDestruction(self):
        if self._beat_task is not None:
            self._beat_task.cancel()
        if self._db_monitor is not None:
            self._db_monitor.cancel()


def get_scene(deviceId):
    scene0 = TableElementModel(
        height=391.0, keys=[f'{deviceId}.view'],
        parent_component='DisplayComponent',
        width=751.0, x=340.0, y=270.0)
    scene1 = DisplayTextLogModel(
        height=171.0, keys=[f'{deviceId}.status'],
        parent_component='DisplayComponent',
        width=531.0, x=410.0, y=10.0)
    scene20 = DisplayCommandModel(
        height=35.0,
        keys=[f'{deviceId}.listConfigurations'],
        parent_component='DisplayComponent',
        width=321.0, x=80.0, y=311.0)
    scene21 = DisplayCommandModel(
        height=35.0,
        keys=[f'{deviceId}.saveConfigurations'],
        parent_component='DisplayComponent',
        width=321.0, x=80.0, y=346.0)
    scene2 = BoxLayoutModel(
        direction=2, height=80.0, width=321.0, x=10.0,
        y=280.0, children=[scene20, scene21])
    scene3 = LabelModel(
        font='Sans Serif,14,-1,5,50,0,1,0,0,0', height=31.0,
        parent_component='DisplayComponent',
        text='Component Manager', width=391.0, x=20.0, y=10.0)
    scene40 = DisplayLabelModel(
        font_size=14, font_weight='bold', height=31.0,
        keys=[f'{deviceId}.deviceId'],
        parent_component='DisplayComponent',
        width=311.0, x=70.0, y=50.0)
    scene41 = DisplayStateColorModel(
        height=31.0, keys=[f'{deviceId}.state'],
        parent_component='DisplayComponent',
        show_string=True, width=311.0, x=70.0,
        y=90.0)
    scene420 = LabelModel(
        font='Sans Serif,10,-1,5,50,0,0,0,0,0',
        foreground='#000000', height=31.0,
        parent_component='DisplayComponent',
        text='Last Success', width=73.0, x=70.0, y=130.0)
    scene421 = ErrorBoolModel(
        height=31.0, keys=[f'{deviceId}.lastSuccess'],
        parent_component='DisplayComponent', width=68.0,
        x=143.0, y=130.0)
    scene42 = BoxLayoutModel(
        height=37.0, width=391.0, x=10.0, y=114.0,
        children=[scene420, scene421])
    scene4 = BoxLayoutModel(
        direction=2, height=111.0, width=391.0, x=10.0,
        y=40.0, children=[scene40, scene41, scene42])
    scene5 = LineModel(stroke='#000000', x1=30.0, x2=1080.0, y1=250.0,
                       y2=250.0)
    scene6 = LabelModel(
        font='Sans Serif,10,-1,5,50,0,0,0,0,0,Normal',
        height=31.0, parent_component='DisplayComponent',
        text='Save options', width=161.0, x=10.0, y=410.0)
    scene7 = LineModel(
        stroke='#000000', x1=10.0, x2=330.0, y1=450.0, y2=450.0)
    scene800 = LabelModel(
        font='Sans Serif,10,-1,5,50,0,0,0,0,0',
        foreground='#000000', height=20.0,
        parent_component='DisplayComponent',
        text='configurationName', width=321.0, x=10.0,
        y=470.0)
    scene8010 = DisplayLabelModel(
        font_size=10, height=27.0,
        keys=[f'{deviceId}.configurationName'],
        parent_component='DisplayComponent',
        width=161.0, x=10.0, y=490.0)
    scene8011 = LineEditModel(
        height=27.0,
        keys=[f'{deviceId}.configurationName'],
        klass='EditableLineEdit',
        parent_component='EditableApplyLaterComponent',
        width=160.0, x=171.0, y=490.0)
    scene801 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=490.0,
        children=[scene8010, scene8011])
    scene80 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0,
        y=470.0, children=[scene800, scene801])
    scene810 = LabelModel(
        font='Sans Serif,10,-1,5,50,0,0,0,0,0',
        foreground='#000000', height=20.0,
        parent_component='DisplayComponent', text='priority',
        width=321.0, x=10.0, y=530.0)
    scene8110 = DisplayLabelModel(
        font_size=10, height=34.0,
        keys=[f'{deviceId}.priority'],
        parent_component='DisplayComponent',
        width=161.0, x=10.0, y=550.0)
    scene8111 = IntLineEditModel(
        height=34.0, keys=[f'{deviceId}.priority'],
        parent_component='EditableApplyLaterComponent',
        width=160.0, x=171.0, y=550.0)
    scene811 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=547.0,
        children=[scene8110, scene8111])
    scene81 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0,
        y=527.0, children=[scene810, scene811])
    scene820 = LabelModel(
        font='Sans Serif,10,-1,5,50,0,0,0,0,0',
        foreground='#000000', height=20.0,
        parent_component='DisplayComponent',
        text='description', width=321.0, x=10.0, y=584.0)
    scene8210 = DisplayLabelModel(
        font_size=10, height=34.0,
        keys=[f'{deviceId}.description'],
        parent_component='DisplayComponent',
        width=161.0, x=10.0, y=604.0)
    scene8211 = LineEditModel(
        height=34.0, keys=[f'{deviceId}.description'],
        klass='EditableLineEdit',
        parent_component='EditableApplyLaterComponent',
        width=160.0, x=171.0, y=604.0)
    scene821 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=604.0,
        children=[scene8210, scene8211])
    scene82 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0,
        y=584.0, children=[scene820, scene821])
    scene8 = BoxLayoutModel(
        direction=2, height=171.0, width=321.0, x=10.0,
        y=470.0, children=[scene80, scene81, scene82])
    scene90 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0',
        foreground='#000000', height=26.0,
        parent_component='DisplayComponent',
        text='deviceNames', width=83.0, x=170.0, y=240.0)
    scene91 = DisplayListModel(
        height=26.0, keys=[f'{deviceId}.deviceNames'],
        parent_component='DisplayComponent',
        width=342.0)
    scene9 = BoxLayoutModel(
        height=31.0, width=1081.0, x=10.0, y=210.0,
        children=[scene90, scene91])
    scene10 = LineModel(
        stroke='#000000', x1=30.0, x2=1080.0, y1=200.0,
        y2=200.0)
    scene = SceneModel(
        height=673.0, width=1102.0,
        children=[scene0, scene1, scene2, scene3, scene4,
                  scene5, scene6, scene7, scene8, scene9,
                  scene10])
    return write_scene(scene)
