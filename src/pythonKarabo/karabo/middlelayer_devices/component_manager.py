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
    LineEditModel, IntLineEditModel, LineModel, SceneModel, StickerModel,
    TableElementModel, write_scene)
from karabo.common.services import KARABO_CONFIG_MANAGER
from karabo.config_db.utils import ConfigurationDBError
from karabo.native.configuration import sanitize_write_configuration
from karabo.middlelayer import (
    AccessLevel, AccessMode, allCompleted, Assignment, background, Bool,
    connectDevice, Configurable, DaqPolicy, Device, Hash,
    Overwrite, sleep, slot, Slot, State, String, UInt32, VectorHash,
    VectorString, waitUntilNew)

DEVICE_TIMEOUT = 4


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
        minSize=1,
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
            reply = await wait_for(self.call(KARABO_CONFIG_MANAGER,
                                             "slotListConfigurationSets", h),
                                   timeout=DEVICE_TIMEOUT)
            items = reply["items"]
        except (ConfigurationDBError, TimeoutError):
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

        def chunks(sequence, n):
            """Yield successive n-sized chunks from sequence"""
            for i in range(0, len(sequence), n):
                yield sequence[i:i + n]

        try:
            names = self.deviceNames.value
            priority = int(self.priority.value)
            description = self.description.value
            name = self.configurationName.value

            status = (f"Saving configuration {name} for devices {names}"
                      f" with priority {priority}.")
            self.status = status
            chunk_size = int(self.db.confBulkLimit)
            for chunk in chunks(names, chunk_size):
                h = Hash("deviceIds", chunk,
                         "name", name,
                         "priority", priority,
                         "description", description)

                timeout = DEVICE_TIMEOUT * len(chunk)
                await wait_for(self.call(KARABO_CONFIG_MANAGER,
                                         "slotSaveConfigurationFromName", h),
                               timeout=timeout)
        except TimeoutError:
            self.lastSuccess = False
            self.status = "Saving configurations failed due to timeout!"
            self.state = State.ON
        except (ConfigurationDBError, Exception):
            self.lastSuccess = False
            self.status = ("Saving configurations failed. The configuration "
                           " name is already taken.")
            self.state = State.ON
        else:
            await self._list_configurations()

    @Slot(displayedName="Apply configurations",
          requiredAccessLevel=AccessLevel.OPERATOR,
          allowedStates=[State.ON])
    async def applyConfigurations(self):
        """Apply a configuration for the component"""
        self.state = State.CHANGING
        background(self._apply_configurations())

    async def _apply_configurations(self):
        ping = await self._ping_beats_action()
        # Before saving we make sure all devices are there again!
        if not ping:
            self.status = "Not all devices are online responding..."
            self.lastSuccess = False
            self.state = State.ON
            return

        conf_name = self.configurationName.value
        status = (f"Trying to apply selected configuration {conf_name} for "
                  f"the devices {self.deviceNames}.")
        self.status = status
        futures = {deviceId: self.call(
            KARABO_CONFIG_MANAGER, "slotGetConfigurationFromName",
            Hash("deviceId", deviceId, "name", conf_name))
            for deviceId in self.deviceNames}

        config_items, *fails = await allCompleted(**futures,
                                                  timeout=DEVICE_TIMEOUT)

        chain = ChainMap(*fails)
        if chain.items():
            devs = ", ".join(chain.keys())
            status = (f"Retrieving configurations from the ConfigManager "
                      f"failed for device(s) {devs}")
            self.status = status
            self.lastSuccess = False
        else:
            # We got the configuration. Now we need to validate it against a
            # schema and state of the device!
            extracted = {deviceId: config["item"]["config"]
                         for deviceId, config in config_items.items()}

            async def _get_sanitized_conf(deviceId, configuration):
                # Note: We request a state dependent schema here!
                schema, *_ = await self.call(deviceId, "slotGetSchema", True)
                sanitized = sanitize_write_configuration(schema, configuration)
                return sanitized

            futures = {deviceId: _get_sanitized_conf(deviceId, config)
                       for deviceId, config in extracted.items()}
            sani_conf, *fails = await allCompleted(
                **futures, timeout=DEVICE_TIMEOUT)
            chain = ChainMap(*fails)
            if chain.items():
                devs = ", ".join(chain.keys())
                status = (f"Could not retrieve state dependent schema(s) to "
                          f"sanitize configuration(s) for: {devs}")
                self.status = status
                self.lastSuccess = False
            else:
                futures = {deviceId: self.call(
                    deviceId, "slotReconfigure", conf)
                    for deviceId, conf in sani_conf.items()}

                done, *fails = await allCompleted(**futures,
                                                  timeout=DEVICE_TIMEOUT)
                chain = ChainMap(*fails)
                if chain.items():
                    devs = ", ".join(chain.keys())
                    status = (f"The following device(s) could not be "
                              f"reconfigured: {devs}")
                    self.status = status
                    self.lastSuccess = False
                    # Note: Give a trace for developers only print log
                    msg = f"{status}\n\n{chain.values()}"
                    print(msg)
                else:
                    devs = ", ".join(done.keys())
                    status = (
                        f"Reconfiguration of following device(s) was "
                        f"successful: {devs}")
                    self.status = status
                    self.lastSuccess = True

        self.state = State.ON

    def __init__(self, configuration):
        super(ComponentManager, self).__init__(configuration)
        self.db = None
        self._beat_task = None
        self._db_monitor = None

    async def onInitialization(self):
        """Initialize the component manager device"""
        self.db = await connectDevice(KARABO_CONFIG_MANAGER)

        # If we have a device name already we can retrieve a list!
        if self.deviceNames:
            background(self._list_configurations())

        self._db_monitor = background(self._monitor_db_state())
        self._beat_task = background(self._ping_beats())

        # We are ready to take action!
        self.state = State.ON

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
        height=531.0, keys=[f'{deviceId}.view'],
        parent_component='DisplayComponent', width=751.0, x=340.0, y=270.0)
    scene1 = DisplayTextLogModel(
        height=171.0, keys=[f'{deviceId}.status'],
        parent_component='DisplayComponent', width=631.0, x=460.0, y=10.0)
    scene2 = LabelModel(
        font='Source Sans Pro,14,-1,5,50,0,1,0,0,0', height=31.0,
        parent_component='DisplayComponent', text='Component Manager',
        width=391.0, x=20.0, y=10.0)
    scene30 = DisplayLabelModel(
        font_size=14, font_weight='bold', height=37.0,
        keys=[f'{deviceId}.deviceId'], parent_component='DisplayComponent',
        width=391.0, x=10.0, y=40.0)
    scene31 = DisplayStateColorModel(
        height=37.0, keys=[f'{deviceId}.state'],
        parent_component='DisplayComponent', show_string=True, width=391.0,
        x=10.0, y=77.0)
    scene320 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=37.0, parent_component='DisplayComponent', text='Last Success',
        width=73.0, x=10.0, y=114.0)
    scene321 = ErrorBoolModel(
        height=37.0, keys=[f'{deviceId}.lastSuccess'],
        parent_component='DisplayComponent', width=318.0, x=83.0, y=114.0)
    scene32 = BoxLayoutModel(
        height=37.0, width=391.0, x=10.0, y=114.0,
        children=[scene320, scene321])
    scene3 = BoxLayoutModel(
        direction=2, height=111.0, width=391.0, x=10.0, y=40.0,
        children=[scene30, scene31, scene32])
    scene4 = LineModel(
        stroke='#000000', x1=30.0, x2=1080.0, y1=250.0, y2=250.0)
    scene5 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0,Normal', height=31.0,
        parent_component='DisplayComponent', text='Save options', width=161.0,
        x=10.0, y=610.0)
    scene6 = LineModel(
        stroke='#000000', x1=10.0, x2=330.0, y1=450.0, y2=450.0)
    scene70 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=31.0, parent_component='DisplayComponent', text='deviceNames',
        width=83.0, x=10.0, y=210.0)
    scene71 = DisplayListModel(
        height=31.0, keys=[f'{deviceId}.deviceNames'],
        parent_component='DisplayComponent', width=998.0, x=93.0, y=210.0)
    scene7 = BoxLayoutModel(
        height=31.0, width=1081.0, x=10.0, y=210.0,
        children=[scene70, scene71])
    scene8 = LineModel(
        stroke='#000000', x1=30.0, x2=1080.0, y1=200.0, y2=200.0)
    scene9 = DisplayCommandModel(
        height=41.0, keys=[f'{deviceId}.applyConfigurations'],
        parent_component='DisplayComponent', width=321.0, x=10.0, y=540.0)
    scene10 = DisplayCommandModel(
        height=40.0, keys=[f'{deviceId}.listConfigurations'],
        parent_component='DisplayComponent', width=321.0, x=10.0, y=280.0)
    scene11 = DisplayCommandModel(
        height=40.0, keys=[f'{deviceId}.saveConfigurations'],
        parent_component='DisplayComponent', width=321.0, x=10.0, y=320.0)
    scene12 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=21.0, parent_component='DisplayComponent',
        text='deviceHeartBeat', width=100.0, x=10.0, y=160.0)
    scene13 = DisplayStateColorModel(
        height=21.0, keys=[f'{deviceId}.deviceHeartBeat'],
        parent_component='DisplayComponent', show_string=True, width=131.0,
        x=180.0, y=160.0)
    scene14 = LabelModel(
        font='Source Sans Pro,8,-1,5,50,0,0,0,0,0,Normal', height=20.0,
        parent_component='DisplayComponent', text='Are the devices online ...',
        width=141.0, x=320.0, y=160.0)
    scene15 = StickerModel(
        background='transparent', font='Source Sans Pro,12,-1,5,50,0,0,0,0,0',
        height=61.0, parent_component='DisplayComponent',
        text='Note: Only properties will be updated which are allowed in the '
             'device states!',
        width=321.0, x=10.0, y=470.0)
    scene160 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=20.0, parent_component='DisplayComponent',
        text='configurationName', width=321.0, x=10.0, y=370.0)
    scene1610 = DisplayLabelModel(
        font_size=10, height=37.0, keys=[f'{deviceId}.configurationName'],
        parent_component='DisplayComponent', width=161.0, x=10.0, y=390.0)
    scene1611 = LineEditModel(
        height=37.0, keys=[f'{deviceId}.configurationName'],
        klass='EditableLineEdit',
        parent_component='EditableApplyLaterComponent', width=160.0, x=171.0,
        y=390.0)
    scene161 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=390.0,
        children=[scene1610, scene1611])
    scene16 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0, y=370.0,
        children=[scene160, scene161])
    scene170 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=20.0, parent_component='DisplayComponent', text='priority',
        width=321.0, x=10.0, y=657.0)
    scene1710 = DisplayLabelModel(
        font_size=10, height=37.0, keys=[f'{deviceId}.priority'],
        parent_component='DisplayComponent', width=161.0, x=10.0, y=677.0)
    scene1711 = IntLineEditModel(
        height=37.0, keys=[f'{deviceId}.priority'],
        parent_component='EditableApplyLaterComponent', width=160.0, x=171.0,
        y=677.0)
    scene171 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=677.0,
        children=[scene1710, scene1711])
    scene17 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0, y=657.0,
        children=[scene170, scene171])
    scene180 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=20.0, parent_component='DisplayComponent', text='description',
        width=321.0, x=10.0, y=714.0)
    scene1810 = DisplayLabelModel(
        font_size=10, height=37.0, keys=[f'{deviceId}.description'],
        parent_component='DisplayComponent', width=161.0, x=10.0, y=734.0)
    scene1811 = LineEditModel(
        height=37.0, keys=[f'{deviceId}.description'],
        klass='EditableLineEdit',
        parent_component='EditableApplyLaterComponent', width=160.0, x=171.0,
        y=734.0)
    scene181 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=734.0,
        children=[scene1810, scene1811])
    scene18 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0, y=714.0,
        children=[scene180, scene181])
    scene19 = LineModel(
        stroke='#000000', x1=10.0, x2=330.0, y1=640.0, y2=640.0)
    scene = SceneModel(
        height=821.0, width=1102.0,
        children=[scene0, scene1, scene2, scene3, scene4, scene5, scene6,
                  scene7, scene8, scene9, scene10, scene11, scene12, scene13,
                  scene14, scene15, scene16, scene17, scene18, scene19])
    return write_scene(scene)
