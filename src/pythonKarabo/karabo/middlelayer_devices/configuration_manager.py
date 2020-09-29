#############################################################################
# Author: degon & costar
# Created on August 21, 2020, 11:42 AM
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from asyncio import CancelledError, gather, TimeoutError, wait_for
import os
import os.path as op

from karabo.common.scenemodel.api import (
    BoxLayoutModel, DisplayCommandModel, DisplayLabelModel,
    DisplayStateColorModel, DisplayTextLogModel, ErrorBoolModel, LabelModel,
    LineEditModel, IntLineEditModel, LineModel, SceneModel, TableElementModel,
    write_scene)

from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, background, Bool, Configurable,
    coslot, DaqPolicy, Device, dictToHash, KaraboError, Hash, HashList, 
    Overwrite, slot, Slot, State, String, Timestamp, UInt32, VectorHash,
    VectorString)

from karabo.config_db.configuration_database import (
    ConfigurationDatabase, DbHandle)
from karabo.config_db.utils import (
    hashFromBase64Bin, hashToBase64Bin, schemaToBase64Bin, schemaFromBase64Bin)

HIDDEN_KARABO_FOLDER = op.join(os.environ['HOME'], '.karabo')
KARABO_CONFIG_DB_FOLDER = op.join(HIDDEN_KARABO_FOLDER, 'config_db')

DEVICE_TIMEOUT = 2
FILTER_KEYS = ["name", "timepoint", "description", "priority"]


def scratch_conf(c: dict) -> dict:
    """Create a scoped down dictionary of the configuration"""
    ret = {k: v for k, v in c.items() if k in FILTER_KEYS}
    return ret


class RowSchema(Configurable):
    name = String(
        defaultValue="",
        description="The name of the configuration",
        accessMode=AccessMode.READONLY)

    timepoint = String(
        defaultValue="",
        description="The timepoint when the configuration was saved",
        accessMode=AccessMode.READONLY)

    description = String(
        defaultValue="",
        description="The description of the configuration",
        accessMode=AccessMode.READONLY)

    priority = UInt32(
        defaultValue=1,
        description="The priority of the configuration",
        accessMode=AccessMode.READONLY)


class ConfigurationManager(Device):
    """This configuration manager service is to control device configurations

    Requests from various clients (GUI's and devices) will be managed.

    - Tag device configurations by ``name``
    - Retrieve device configurations by ``name``
    - List device configurations by ``deviceId``
    """
    state = Overwrite(
        defaultValue=State.INIT,
        options=[State.INIT, State.CHANGING, State.UNKNOWN, State.ON])

    visibility = Overwrite(
        defaultValue=AccessLevel.ADMIN,
        options=[AccessLevel.ADMIN])

    @String(
        defaultValue="",
        description="The deviceId to look for a configuration",
        requiredAccessLevel=AccessLevel.OPERATOR)
    async def deviceName(self, value):
        if value is not None:
            self.deviceName = value
            if self.db is not None:
                background(self._list_configurations())

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

    lastError = Bool(
        defaultValue=False,
        displayedName="Last Error",
        description="Indicates if the last action was providing an error",
        accessMode=AccessMode.READONLY)

    view = VectorHash(
        rows=RowSchema,
        defaultValue=[],
        displayedName="View",
        accessMode=AccessMode.READONLY,
        daqPolicy=DaqPolicy.OMIT)

    dbName = String(
        defaultValue="karaboDB",
        displayedName="DB Name",
        description="The database name",
        requiredAccessLevel=AccessLevel.OPERATOR,
        assignment=Assignment.MANDATORY,
        accessMode=AccessMode.INITONLY)

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
        """List all configuration for a device in the table element!"""
        self.lastError = False
        self.state = State.CHANGING
        background(self._list_configurations())

    async def _list_configurations(self):
        deviceId = self.deviceName.value
        name_part = ""
        items = []
        try:
            items = self.db.list_configurations(deviceId, name_part)
            # Adjust for the table element!
            items = [scratch_conf(c) for c in items]
            # Convert to a list of Hashes!
            items = [dictToHash(c) for c in items]
        except Exception as e:
            self.status = str(e)
            self.lastError = True
        else:
            self.status = f"Listing configurations for {deviceId}"
        self.view = items
        self.state = State.ON

    @Slot(displayedName="Save configurations",
          requiredAccessLevel=AccessLevel.OPERATOR,
          allowedStates=[State.ON])
    async def saveConfigurations(self):
        """Save a configuration for a given `deviceId`"""
        self.lastError = False
        self.state = State.CHANGING
        background(self._save_configuration())

    async def _save_configuration(self):
        deviceId = self.deviceName.value
        priority = int(self.priority.value)
        description = self.description.value
        config_name = self.configurationName.value
        try:
            schema, _ = await wait_for(self.call(
                deviceId, "slotGetSchema", False), timeout=DEVICE_TIMEOUT)
            conf, _ = await wait_for(self.call(
                deviceId, "slotGetConfiguration"), timeout=DEVICE_TIMEOUT)

            configs = {}
            configs.update({"deviceId": deviceId})
            configs.update({"config": hashToBase64Bin(conf)})
            configs.update({"schema": schemaToBase64Bin(schema)})

            items = [configs]
            self.db.save_configuration(
                config_name, items, description=description, user=".",
                priority=priority, timestamp="")
        except (CancelledError, TimeoutError):
            self.status = (f"Saving configuration for {deviceId} failed. The "
                           f"device is not online")
            self.lastError = True
        except Exception as e:
            # Config DB Error
            self.status = str(e)
            self.lastError = True
        else:
            self.status = (f"Saved configuration {config_name} for "
                           f"device {deviceId}!")
            try:
                current_items = self.db.list_configurations(
                    deviceId, name_part="")
                current_items = [scratch_conf(c) for c in current_items]
                current_items = [dictToHash(c) for c in current_items]
                self.view = current_items
            except Exception as e:
                self.status = str(e)
                self.lastError = True
        finally:
            self.state = State.ON

    def __init__(self, configuration):
        super(ConfigurationManager, self).__init__(configuration)
        self.db = None

    async def onInitialization(self):
        """Initialize the configuration database device and create the `DB`"""
        path = op.join(KARABO_CONFIG_DB_FOLDER, self.dbName.value)
        dir_name = op.dirname(path)
        if not op.exists(dir_name):
            os.makedirs(dir_name, exist_ok=True)

        connection = DbHandle(path)
        if not connection.authenticate():
            # XXX: We bring the device down if the db is not available!
            raise KaraboError(f"Database with address {self.dbName.value} is"
                              f"not available.")
        self.db = ConfigurationDatabase(connection)
        self.db.assureExisting()
        self.state = State.ON

        # If we have a device name already we can retrieve a list!
        if self.deviceName:
            background(self._list_configurations())

    # Karabo Slot Interface
    # -----------------------------------------------------------------------

    @coslot
    async def slotListConfigurationFromName(self, info):
        """Slot to list configurations from name

        This slot requires an info Hash with `deviceId`
        """
        deviceId = info["deviceId"]
        name_part = info.get("name", "")

        items = self.db.list_configurations(deviceId, name_part)
        items = HashList([dictToHash(config) for config in items])

        return Hash("items", items,
                    "input", info)

    @coslot
    async def slotGetConfigurationFromName(self, info):
        """Slot to get a configuration from name

        The info Hash must contain `deviceId` and `name`.
        """
        deviceId = info["deviceId"]
        name = info["name"]

        item = self.db.get_configuration(deviceId, name)
        if not item:
            reason = (f"No configuration for device {deviceId} and name "
                      f"{name} found!")
            raise KaraboError(reason)

        item = dictToHash(item)
        config64 = item["config"]
        schema64 = item["schema"]
        item["config"] = hashFromBase64Bin(config64)
        item["schema"] = schemaFromBase64Bin(schema64)

        return Hash("item", item,
                    "input", info)

    @coslot
    async def slotGetLastConfiguration(self, info):
        """Slot to get a the last configuration

        The info Hash must contain `deviceId` and can obtain `priority`.
        """
        deviceId = info["deviceId"]
        priority = info.get("priority", 3)

        item = self.db.get_last_configuration(deviceId, priority=priority)
        if not item:
            reason = (f"No configuration for device {deviceId} and "
                      f"priority {priority} found!")
            raise KaraboError(reason)

        item = dictToHash(item)
        config64 = item["config"]
        schema64 = item["schema"]
        item["config"] = hashFromBase64Bin(config64)
        item["schema"] = schemaFromBase64Bin(schema64)

        return Hash("item", item,
                    "input", info)

    @coslot
    async def slotSaveConfigurationFromName(self, info):
        """Slot to save configuration(s) from name

          The info Hash should contain:

           - client: client information
           - name: the non-empty (and unique for the device) name to be
                   associated with the configuration(s)

           - priority: priority of the configuration(s) as integer ranging
                       from 1-3 with 3 being the highest priority

           - description: an optional description for the named configs

           - user: the user name of the currently logged in user in the client

           - deviceIds: a vector of strings with deviceIds
        """
        # Optional information that can be taken as defaults!
        user = info.get("user", ".")
        description = info.get("description", "")
        priority = info.get("priority", 1)
        timestamp = info.get("timestamp", Timestamp().toLocal())

        config_name = info["name"]  # Note: Must be there!
        deviceIds = info["deviceIds"]

        try:

            async def poll_(device_id):
                schema, _ = await self.call(device_id, "slotGetSchema", False)
                config, _ = await self.call(device_id, "slotGetConfiguration")
                config_dict = {}
                config64 = hashToBase64Bin(config)
                schema64 = schemaToBase64Bin(schema)
                config_dict.update({"deviceId": device_id})
                config_dict.update({"config": config64})
                config_dict.update({"schema": schema64})
                return config_dict


            futures = [poll_(device_id) for device_id in deviceIds]
            timeout = len(deviceIds) * DEVICE_TIMEOUT
            items = await wait_for(gather(*futures), timeout=timeout)

            self.db.save_configuration(
                config_name, items, description=description, user=user,
                priority=priority, timestamp=timestamp)
        except (CancelledError, TimeoutError):
            raise

        return Hash("input", info)


def get_scene(deviceId):
    """Return a device scene"""
    scene0 = TableElementModel(
        height=431.0, keys=['{}.view'.format(deviceId)],
        parent_component='DisplayComponent',
        width=601.0, x=340.0, y=230.0)
    scene1 = DisplayTextLogModel(
        height=199.0, keys=['{}.status'.format(deviceId)],
        parent_component='DisplayComponent',
        width=531.0, x=410.0, y=10.0)
    scene20 = DisplayCommandModel(
        height=35.0, keys=['{}.listConfigurations'.format(deviceId)],
        parent_component='DisplayComponent', width=321.0, x=80.0, y=311.0)
    scene21 = DisplayCommandModel(
        height=35.0, keys=['{}.saveConfigurations'.format(deviceId)],
        parent_component='DisplayComponent', width=321.0, x=80.0, y=346.0)
    scene2 = BoxLayoutModel(
        direction=2, height=80.0, width=311.0, x=10.0, y=310.0,
        children=[scene20, scene21])
    scene3 = LabelModel(
        font='Sans Serif,14,-1,5,50,0,1,0,0,0', height=31.0,
        parent_component='DisplayComponent', text='Configuration Manager',
        width=391.0, x=20.0, y=10.0)
    scene40 = DisplayLabelModel(
        font_size=10, height=31.0, keys=['{}.deviceId'.format(deviceId)],
        parent_component='DisplayComponent', width=311.0, x=70.0, y=50.0)
    scene41 = DisplayStateColorModel(
        height=31.0, keys=['{}.state'.format(deviceId)],
        parent_component='DisplayComponent',
        show_string=True, width=311.0, x=70.0, y=90.0)
    scene420 = LabelModel(
        font='Sans Serif,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=31.0, parent_component='DisplayComponent', text='Last Error',
        width=73.0, x=70.0, y=130.0)
    scene421 = ErrorBoolModel(
        height=31.0, invert=True, keys=['{}.lastError'.format(deviceId)],
        parent_component='DisplayComponent', width=68.0, x=143.0, y=130.0)
    scene42 = BoxLayoutModel(
        height=37.0, width=391.0, x=10.0, y=114.0,
        children=[scene420, scene421])
    scene4 = BoxLayoutModel(
        direction=2, height=111.0, width=391.0, x=10.0, y=40.0,
        children=[scene40, scene41, scene42])
    scene5 = LineModel(
        stroke='#000000', x1=30.0, x2=900.0, y1=210.0, y2=210.0)
    scene60 = LabelModel(
        font='Sans Serif,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=27.0, parent_component='DisplayComponent', text='Device',
        width=71.0, x=10.0, y=250.0)
    scene610 = DisplayLabelModel(
        font_size=10, height=27.0, keys=['{}.deviceName'.format(deviceId)],
        parent_component='DisplayComponent', width=156.0, x=10.0, y=280.0)
    scene611 = LineEditModel(
        height=27.0, keys=['{}.deviceName'.format(deviceId)],
        klass='EditableLineEdit',
        parent_component='EditableApplyLaterComponent', width=155.0, x=166.0,
        y=280.0)
    scene61 = BoxLayoutModel(
        height=27.0, width=311.0, x=10.0, y=270.0,
        children=[scene610, scene611])
    scene6 = BoxLayoutModel(
        direction=2, height=47.0, width=311.0, x=10.0, y=250.0,
        children=[scene60, scene61])
    scene7 = LabelModel(
        font='Sans Serif,10,-1,5,50,0,0,0,0,0,Normal', height=31.0,
        parent_component='DisplayComponent', text='Save options', width=161.0,
        x=10.0, y=410.0)
    scene8 = LineModel(
        stroke='#000000', x1=10.0, x2=330.0, y1=450.0, y2=450.0)
    scene900 = LabelModel(
        font='Sans Serif,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=20.0, parent_component='DisplayComponent',
        text='configurationName', width=321.0, x=10.0, y=470.0)
    scene9010 = DisplayLabelModel(
        font_size=10, height=27.0,
        keys=['{}.configurationName'.format(deviceId)],
        parent_component='DisplayComponent', width=161.0, x=10.0, y=490.0)
    scene9011 = LineEditModel(
        height=27.0, keys=['{}.configurationName'.format(deviceId)],
        klass='EditableLineEdit',
        parent_component='EditableApplyLaterComponent', width=160.0, x=171.0,
        y=490.0)
    scene901 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=490.0,
        children=[scene9010, scene9011])
    scene90 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0, y=470.0,
        children=[scene900, scene901])
    scene910 = LabelModel(
        font='Sans Serif,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=20.0, parent_component='DisplayComponent', text='priority',
        width=321.0, x=10.0, y=530.0)
    scene9110 = DisplayLabelModel(
        font_size=10, height=34.0, keys=['{}.priority'.format(deviceId)],
        parent_component='DisplayComponent', width=161.0, x=10.0, y=550.0)
    scene9111 = IntLineEditModel(
        height=34.0, keys=['{}.priority'.format(deviceId)],
        parent_component='EditableApplyLaterComponent', width=160.0, x=171.0,
        y=550.0)
    scene911 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=547.0,
        children=[scene9110, scene9111])
    scene91 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0, y=527.0,
        children=[scene910, scene911])
    scene920 = LabelModel(
        font='Sans Serif,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=20.0, parent_component='DisplayComponent', text='description',
        width=321.0, x=10.0, y=584.0)
    scene9210 = DisplayLabelModel(
        font_size=10, height=34.0, keys=['{}.description'.format(deviceId)],
        parent_component='DisplayComponent', width=161.0, x=10.0, y=604.0)
    scene9211 = LineEditModel(
        height=34.0, keys=['{}.description'.format(deviceId)],
        klass='EditableLineEdit',
        parent_component='EditableApplyLaterComponent', width=160.0, x=171.0,
        y=604.0)
    scene921 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=604.0,
        children=[scene9210, scene9211])
    scene92 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0, y=584.0,
        children=[scene920, scene921])
    scene9 = BoxLayoutModel(
        direction=2, height=171.0, width=321.0, x=10.0, y=470.0,
        children=[scene90, scene91, scene92])
    scene = SceneModel(
        height=673.0, width=948.0,
        children=[scene0, scene1, scene2, scene3, scene4, scene5, scene6,
                  scene7, scene8, scene9])
    return write_scene(scene)
