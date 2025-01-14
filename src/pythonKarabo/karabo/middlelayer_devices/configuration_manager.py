#############################################################################
# Author: degon & costar
# Created on August 21, 2020, 11:42 AM
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
#############################################################################
import os
import os.path as op
import re
from asyncio import CancelledError, Future, TimeoutError, gather, wait_for
from collections import defaultdict

from karabo.common.scenemodel.api import (
    BoxLayoutModel, DisplayCommandModel, DisplayLabelModel,
    DisplayStateColorModel, DisplayTextLogModel, EditableRegexModel,
    ErrorBoolModel, IntLineEditModel, LabelModel, LineEditModel, LineModel,
    SceneModel, TableElementModel, write_scene)
from karabo.config_db import (
    ConfigurationDatabase, DbHandle, hashFromBase64Bin, hashToBase64Bin,
    schemaFromBase64Bin, schemaToBase64Bin)
from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Bool, Configurable, DeviceClientBase,
    Hash, HashList, KaraboError, Overwrite, RegexString, Slot, State, String,
    Timestamp, UInt32, VectorHash, VectorString, background, dictToHash,
    sanitize_init_configuration, slot)

HIDDEN_KARABO_FOLDER = op.join(os.environ['HOME'], '.karabo')

DEVICE_TIMEOUT = 3
INSTANCE_NEW_TIMEOUT = 15
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


class ConfigurationManager(DeviceClientBase):
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

    configurationName = RegexString(
        regex="^[A-Za-z0-9_-]{1,30}$",
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
        accessMode=AccessMode.READONLY)

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
            self.lastSuccess = False
        else:
            self.status = f"Listing configurations for {deviceId}"
            self.lastSuccess = True

        self.view = items
        self.state = State.ON

    @Slot(displayedName="Save configurations",
          requiredAccessLevel=AccessLevel.OPERATOR,
          allowedStates=[State.ON])
    async def saveConfigurations(self):
        """Save a configuration for a given `deviceId`"""
        self.state = State.CHANGING
        background(self._save_configuration())

    async def _save_configuration(self):
        deviceId = self.deviceName.value
        priority = int(self.priority.value)
        description = self.description.value
        config_name = self.configurationName.value

        taken = self.db.is_config_name_taken(config_name, [deviceId])
        if taken:
            self.status = (f"Failure: Configuration name {config_name} "
                           f"is already taken for device {deviceId}")
            self.lastSuccess = False
            self.state = State.ON
            return
        # Get the schema and the configuration first!
        try:
            schema, _ = await wait_for(self.call(
                deviceId, "slotGetSchema", False), timeout=DEVICE_TIMEOUT)
            conf, _ = await wait_for(self.call(
                deviceId, "slotGetConfiguration"), timeout=DEVICE_TIMEOUT)
        except (CancelledError, TimeoutError):
            self.status = (f"Failure: Saving configuration for {deviceId} "
                           f"failed. The device is not online.")
            self.lastSuccess = False
        except Exception as e:
            # Handle unexpected error
            self.status = str(e)
            self.lastSuccess = False
        else:
            configs = {}
            configs.update({"deviceId": deviceId})
            configs.update({"config": hashToBase64Bin(conf)})
            configs.update({"schema": schemaToBase64Bin(schema)})
            items = [configs]
            # Now we save and list again, and we should not expect any errors
            try:
                self.db.save_configuration(
                    config_name, items, description=description, user=".",
                    priority=priority, timestamp="")

                self.status = (f"Success: Saved configuration {config_name} "
                               f"for device {deviceId}!")
                current_items = self.db.list_configurations(
                    deviceId, name_part="")
                current_items = [scratch_conf(c) for c in current_items]
                current_items = [dictToHash(c) for c in current_items]
                self.view = current_items
            except Exception as e:
                # Handle unexpected errors as bulk
                self.status = str(e)
                self.lastSuccess = False
            else:
                self.lastSuccess = True

        # At the very end, go to `ON` state again
        self.state = State.ON

    confBulkLimit = UInt32(
        defaultValue=30,
        description="The limit of configurations allowed in a single save "
                    "configuration call",
        requiredAccessLevel=AccessLevel.ADMIN,
        accessMode=AccessMode.READONLY)

    @slot
    async def slotInstanceNew(self, instanceId, info):
        if info["type"] == "server":
            self._class_schemas.pop(instanceId, None)
        await super().slotInstanceNew(instanceId, info)

    @slot
    def slotInstanceGone(self, instanceId, info):
        if info["type"] == "server":
            self._class_schemas.pop(instanceId, None)
        return super().slotInstanceGone(instanceId, info)

    def __init__(self, configuration):
        super().__init__(configuration)
        self.db = None
        regex = self.configurationName.descriptor.regex
        self.name_pattern = re.compile(regex)

        # Dictionary of serverId: {classId: schema}
        self._class_schemas = defaultdict(dict)

    async def onInitialization(self):
        """Initialize the configuration database device and create the `DB`"""
        folder = op.join(os.environ["KARABO"], 'var', 'data', 'config_db')
        path = op.join(folder, self.dbName.value)
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

        self._schema_futures = {}
        # If we have a device name already we can retrieve a list!
        if self.deviceName:
            background(self._list_configurations())

        self.logger.info(f"Starting configuration manager {self.deviceId}")

    # Karabo Slot Interface
    # -----------------------------------------------------------------------

    @slot
    async def slotListConfigurationFromName(self, info):
        """Slot to list configurations from name

        This slot requires an info Hash with `deviceId`
        """
        deviceId = info["deviceId"]
        name_part = info.get("name", "")

        items = self.db.list_configurations(deviceId, name_part)
        items = HashList([dictToHash(config) for config in items])

        return Hash("success", True, "items", items)

    @slot
    async def slotListConfigurationSets(self, info):
        """Slot to list configurations from name

        This slot requires an info Hash with:
            - `deviceIds`: list of deviceIds whose configurations should be
                           listed.

        :returns: list of configuration items (can be empty) with meta
                  information.
                  A configuration item has keys `name`, `priority`,
                  `description`, `user`, `min_point`, `max_timepoint`
                  and `diff_timepoint`.
        """
        deviceIds = info["deviceIds"]

        items = self.db.list_configuration_sets(deviceIds)
        items = HashList([dictToHash(config) for config in items])

        return Hash("success", True, "items", items)

    @slot
    async def slotGetConfigurationFromName(self, info):
        """Slot to get a configuration from name

        The info `Hash` must contain `deviceId` and `name`.

        Note: If the info `Hash` contains `schema`, a schema is returned
        as well.
        """
        deviceId = info["deviceId"]
        name = info["name"]

        item = self.db.get_configuration(deviceId, name)
        if not item:
            reason = (f"Failure: No configuration for device {deviceId} and "
                      f"name {name} found!")
            raise KaraboError(reason)

        item = dictToHash(item)
        config64 = item["config"]
        schema64 = item["schema"]
        item["config"] = hashFromBase64Bin(config64)
        item["schema"] = schemaFromBase64Bin(schema64)

        # A configuration is always connected to a schema, but typically
        # the majority is not interested in that. Hence, we remove it
        # under conditions here and not in the config db logic.
        if not info.has("schema") or info["schema"] is False:
            item.pop("schema")

        return Hash("success", True, "item", item)

    @slot
    async def slotGetLastConfiguration(self, info):
        """Slot to get a the last configuration

        The info `Hash` must contain `deviceId` and can obtain `priority`.

        Note: If the info `Hash` contains `schema`, a schema is returned
        as well.
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

        # A configuration is always connected to a schema, but typically
        # the majority is not interested in that. Hence, we remove it
        # under conditions here and not in the config db logic.
        if not info.has("schema") or info["schema"] is False:
            item.pop("schema")

        return Hash("success", True, "item", item)

    @slot
    async def slotCheckConfigurationFromName(self, info):
        """Slot to check configuration(s) from name
           - name: the non-empty (and unique for the device) name to be
                   associated with the configuration(s)

           - deviceIds: a vector of strings with deviceIds
        """
        config_name = info["name"]  # Note: Must be there!
        deviceIds = info["deviceIds"]
        taken = self.db.is_config_name_taken(config_name, deviceIds)

        return Hash("success", True, "taken", taken)

    @slot
    async def slotListDevices(self, info):
        """List deviceIds that are stored in the database for a priority

        The info hash must contain the priority!
        """
        priority = info.get("priority", 3)
        devices = self.db.list_devices(priority=int(priority))
        return Hash("success", True, "item", devices)

    @slot
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

           - overwritable: Will the configuration be overwritable?
                           Ignored if the named configuration already exists

        """
        config_name = info["name"]  # Note: Must be there!
        deviceIds = info["deviceIds"]
        if not self.name_pattern.match(config_name):
            raise KaraboError(f"The config name {config_name} does not "
                              f"comply with the allowed settings. Don't use "
                              f"special character or spaces and the maximum "
                              f"size is 30 characters.")

        if len(deviceIds) > int(self.confBulkLimit):
            raise KaraboError(
                f"The number of configurations {len(deviceIds)}"
                f" exceeds the allowed limit {self.confBulkLimit}")

        is_taken = self.db.is_config_name_taken(config_name, deviceIds)
        if is_taken:
            raise KaraboError(f"The config name {config_name} is already "
                              "used by a read-only configuration saved "
                              "for at least one of the devices in the list: "
                              f"{deviceIds}. Or by one writable configuration "
                              "that doesn't match the same list of devices.")

        # Optional information that can be taken as defaults!
        user = info.get("user", ".")
        description = info.get("description", "")
        priority = info.get("priority", 1)
        timestamp = info.get("timestamp", Timestamp().toLocal())
        overwritable = info.get("overwritable", False)

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
        except (CancelledError, TimeoutError):
            raise

        # Let it throw here if needed!
        self.db.save_configuration(
            config_name, items, description=description, user=user,
            priority=priority, overwritable=overwritable, timestamp=timestamp)

        return Hash("success", True)

    @slot
    async def slotInstantiateDevice(self, info):
        """Slot to instantiate a device via the configuration manager

        The required info `Hash` must have at least the params:

        - deviceId: The mandatory parameter
        - name: Optional parameter. If no `name` is provided, the latest
                configuration is retrieved with priority 3 (INIT).
        - classId: Optional parameter for validation
        - serverId: Optional parameter
        """
        deviceId = info["deviceId"]
        name = info.get("name", None)
        # Note: classId and serverId can be optional
        # classId is taken for validation!
        classId = info.get("classId", None)
        serverId = info.get("serverId", None)

        # If no `name` is provided, we try to get the latest config
        if name is None:
            item = self.db.get_last_configuration(deviceId, priority=3)
            if not item:
                reason = (f"No configuration for device {deviceId} and "
                          f"priority 3 (INIT) found!")
                raise KaraboError(reason)
        else:
            item = self.db.get_configuration(deviceId, name)
            if not item:
                reason = (f"No configuration for device {deviceId} and name "
                          f"{name} found!")
                raise KaraboError(reason)
        config = hashFromBase64Bin(item["config"])
        # If the classId was provided we can validate!
        if classId is not None and classId != config["classId"]:
            raise KaraboError(f"The configuration for {deviceId} was "
                              f"recorded for classId {classId}. The "
                              f"input classId {classId} does not match!")
        # If we did not provide the classId, we take it from the configuration!
        if classId is None:
            classId = config["classId"]
        # If we did not provide a serverId, we take it from the config
        if serverId is None:
            serverId = config["serverId"]
        # Get the class schema for this device!
        schema = self._class_schemas[serverId].get(classId, None)
        if schema is None:
            # No schema there, check if we are already looking for it!
            future = self._schema_futures.get((serverId, classId), None)
            if future is not None:
                # Let it throw here in case of failure
                await future
                schema = future.result()
            else:
                future = Future()
                self._schema_futures[(serverId, classId)] = future
                try:
                    schema, *_ = await wait_for(
                        self.call(serverId, "slotGetClassSchema", classId),
                        timeout=DEVICE_TIMEOUT)
                except (CancelledError, TimeoutError) as e:
                    future.set_exception(e)
                    raise KaraboError(
                        f"server {serverId} is not available to start "
                        f"device with deviceId {deviceId} ... Could not "
                        f"retrieve the schema!")
                except Exception as e:
                    # In case we experience an unknown error, notify!
                    future.set_exception(e)
                    raise KaraboError(
                        f"Exception occured when starting device with "
                        f"deviceId {deviceId} on {serverId}: {e}")
                else:
                    future.set_result(schema)
                    # Got a new schema, cache it!
                    self._class_schemas[serverId][classId] = schema
                finally:
                    self._schema_futures.pop((serverId, classId), None)

        config = sanitize_init_configuration(schema, config)
        h = Hash()
        h["deviceId"] = deviceId
        h["classId"] = classId
        h["serverId"] = serverId
        h["configuration"] = config

        success, msg = await self.call(serverId, "slotStartDevice", h)
        if not success:
            raise KaraboError(msg)

        return Hash("success", True)


def get_scene(deviceId):
    scene0 = TableElementModel(
        height=431.0, keys=[f'{deviceId}.view'],
        parent_component='DisplayComponent',
        width=601.0, x=340.0, y=230.0)
    scene1 = DisplayTextLogModel(
        height=199.0, keys=[f'{deviceId}.status'],
        parent_component='DisplayComponent',
        width=531.0, x=410.0, y=10.0)
    scene20 = DisplayCommandModel(
        height=35.0, keys=[f'{deviceId}.listConfigurations'],
        parent_component='DisplayComponent', width=321.0, x=80.0, y=311.0)
    scene21 = DisplayCommandModel(
        height=35.0, keys=[f'{deviceId}.saveConfigurations'],
        parent_component='DisplayComponent', width=321.0, x=80.0, y=346.0)
    scene2 = BoxLayoutModel(
        direction=2, height=80.0, width=321.0, x=10.0, y=320.0,
        children=[scene20, scene21])
    scene3 = LabelModel(
        font='Source Sans Pro,14,-1,5,50,0,1,0,0,0', height=31.0,
        parent_component='DisplayComponent', text='Configuration Manager',
        width=391.0, x=20.0, y=10.0)
    scene40 = DisplayLabelModel(
        font_size=10, height=31.0, keys=[f'{deviceId}.deviceId'],
        parent_component='DisplayComponent', width=311.0, x=70.0, y=50.0)
    scene41 = DisplayStateColorModel(
        height=31.0, keys=[f'{deviceId}.state'],
        parent_component='DisplayComponent',
        show_string=True, width=311.0, x=70.0, y=90.0)
    scene420 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=31.0, parent_component='DisplayComponent', text='Last Success',
        width=73.0, x=70.0, y=130.0)
    scene421 = ErrorBoolModel(
        height=31.0, keys=[f'{deviceId}.lastSuccess'],
        parent_component='DisplayComponent', width=68.0, x=143.0, y=130.0)
    scene42 = BoxLayoutModel(
        height=37.0, width=391.0, x=10.0, y=114.0,
        children=[scene420, scene421])
    scene4 = BoxLayoutModel(
        direction=2, height=111.0, width=391.0, x=10.0, y=40.0,
        children=[scene40, scene41, scene42])
    scene5 = LineModel(
        stroke='#000000', x1=30.0, x2=900.0, y1=210.0, y2=210.0)
    scene6 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0,Normal', height=31.0,
        parent_component='DisplayComponent', text='Save options', width=161.0,
        x=10.0, y=410.0)
    scene7 = LineModel(
        stroke='#000000', x1=10.0, x2=330.0, y1=450.0, y2=450.0)
    scene800 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=20.0, parent_component='DisplayComponent',
        text='configurationName', width=321.0, x=10.0, y=470.0)
    scene8010 = DisplayLabelModel(
        font_size=10, height=27.0,
        keys=[f'{deviceId}.configurationName'],
        parent_component='DisplayComponent', width=161.0, x=10.0, y=490.0)
    scene8011 = EditableRegexModel(
        height=27.0, keys=[f'{deviceId}.configurationName'],
        parent_component='EditableApplyLaterComponent', width=160.0, x=171.0,
        y=490.0)
    scene801 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=490.0,
        children=[scene8010, scene8011])
    scene80 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0, y=470.0,
        children=[scene800, scene801])
    scene810 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=20.0, parent_component='DisplayComponent', text='priority',
        width=321.0, x=10.0, y=530.0)
    scene8110 = DisplayLabelModel(
        font_size=10, height=34.0, keys=[f'{deviceId}.priority'],
        parent_component='DisplayComponent', width=161.0, x=10.0, y=550.0)
    scene8111 = IntLineEditModel(
        height=34.0, keys=[f'{deviceId}.priority'],
        parent_component='EditableApplyLaterComponent', width=160.0, x=171.0,
        y=550.0)
    scene811 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=547.0,
        children=[scene8110, scene8111])
    scene81 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0, y=527.0,
        children=[scene810, scene811])
    scene820 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=20.0, parent_component='DisplayComponent', text='description',
        width=321.0, x=10.0, y=584.0)
    scene8210 = DisplayLabelModel(
        font_size=10, height=34.0, keys=[f'{deviceId}.description'],
        parent_component='DisplayComponent', width=161.0, x=10.0, y=604.0)
    scene8211 = LineEditModel(
        height=34.0, keys=[f'{deviceId}.description'],
        klass='EditableLineEdit',
        parent_component='EditableApplyLaterComponent', width=160.0, x=171.0,
        y=604.0)
    scene821 = BoxLayoutModel(
        height=37.0, width=321.0, x=10.0, y=604.0,
        children=[scene8210, scene8211])
    scene82 = BoxLayoutModel(
        direction=2, height=57.0, width=321.0, x=10.0, y=584.0,
        children=[scene820, scene821])
    scene8 = BoxLayoutModel(
        direction=2, height=171.0, width=321.0, x=10.0, y=470.0,
        children=[scene80, scene81, scene82])
    scene90 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=20.0, parent_component='DisplayComponent', text='Device',
        width=311.0, x=10.0, y=220.0)
    scene91 = DisplayLabelModel(
        font_size=10, height=27.0, keys=[f'{deviceId}.deviceName'],
        parent_component='DisplayComponent', width=311.0, x=10.0, y=240.0)
    scene92 = LineEditModel(
        height=27.0, keys=[f'{deviceId}.deviceName'],
        klass='EditableLineEdit',
        parent_component='EditableApplyLaterComponent', width=311.0, x=10.0,
        y=270.0)
    scene9 = BoxLayoutModel(
        direction=2, height=91.0, width=321.0, x=10.0, y=220.0,
        children=[scene90, scene91, scene92])
    scene = SceneModel(
        height=673.0, width=948.0,
        children=[scene0, scene1, scene2, scene3, scene4, scene5, scene6,
                  scene7, scene8, scene9])
    return write_scene(scene)
