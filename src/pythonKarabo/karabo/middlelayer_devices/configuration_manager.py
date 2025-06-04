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
import re
from asyncio import CancelledError, Future, TimeoutError, gather, wait_for
from collections import defaultdict
from pathlib import Path

from karabo.common.scenemodel.api import (
    BoxLayoutModel, DisplayCommandModel, DisplayLabelModel,
    DisplayStateColorModel, DisplayTextLogModel, EditableRegexModel,
    ErrorBoolModel, LabelModel, LineEditModel, LineModel, SceneModel,
    StickerModel, TableElementModel, write_scene)
from karabo.config_db import ConfigurationDatabase
from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Bool, Configurable, DeviceClientBase,
    Hash, HashList, KaraboError, Overwrite, RegexString, Slot, State, String,
    Timestamp, UInt32, VectorHash, VectorString, background, decodeXML,
    dictToHash, encodeXML, extract_init_configuration, getClassSchema,
    getConfiguration, isStringSet, sanitize_init_configuration, slot)

DEVICE_TIMEOUT = 3
FILTER_KEYS = ["name", "timepoint"]
NAME_REGEX = r"^(?!default$)[A-Za-z0-9_-]{1,30}$"


def hashToHash(h: Hash) -> Hash:
    """Convert a Hash to a Hash without attrs"""
    return Hash(h.items())


def view_item(c: dict) -> dict:
    """Create a view dictionary of the configuration"""
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


class ConfigurationManager(DeviceClientBase):
    """This configuration manager service is to control device configurations

    - Tag device configurations by ``name``
    - Retrieve device configurations by ``name``
    - List device configurations by ``deviceId``
    """

    state = Overwrite(
        defaultValue=State.INIT,
        options=[State.INIT, State.CHANGING, State.UNKNOWN, State.ON])

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
        regex=NAME_REGEX,
        defaultValue="name",
        description="The configuration name",
        requiredAccessLevel=AccessLevel.OPERATOR)

    availableScenes = VectorString(
        displayedName="Available Scenes",
        displayType="Scenes",
        description="Provides a scene for the Configuration Manager.",
        accessMode=AccessMode.READONLY,
        defaultValue=["scene"])

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
        defaultValue="karaboDB3",
        displayedName="DB Name",
        description="The database name",
        requiredAccessLevel=AccessLevel.OPERATOR,
        assignment=Assignment.MANDATORY,
        accessMode=AccessMode.INITONLY)

    isConfigMode = Bool(
        defaultValue=True,
        description=("Set to `True` for real configuration mode. "
                     "Only value changes from default and class "
                     "schema are stored"),
        accessMode=AccessMode.INITONLY,
        requiredAccessLevel=AccessLevel.OPERATOR)

    @slot
    def requestScene(self, params: Hash):
        """Fulfill a scene request from another device."""
        payload = Hash("success", False)
        name = params.get("name", default="scene")
        if name == "scene":
            payload.set("success", True)
            payload.set("name", name)
            payload.set("data", get_scene(self.deviceId))

        return Hash("type", "deviceScene",
                    "origin", self.deviceId,
                    "payload", payload)

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
            items = await self.db.list_configurations(deviceId, name_part)
            # Adjust for the table element!
            items = [view_item(c) for c in items]
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
        config_name = self.configurationName.value
        try:
            serverId, classId = self._get_server_attributes(deviceId)
            conf = await wait_for(
                getConfiguration(deviceId), timeout=DEVICE_TIMEOUT)
            if self.isConfigMode:
                class_schema = await self.get_schema(serverId, classId)
                conf = extract_init_configuration(class_schema, conf)
            else:
                conf = hashToHash(conf)
        except (CancelledError, TimeoutError):
            self.status = (f"Failure: Saving configuration for {deviceId} "
                           f"failed. The device is not online.")
            self.lastSuccess = False
        except Exception as e:
            # Handle unexpected error
            self.status = str(e)
            self.lastSuccess = False
        else:
            configs = {"deviceId": deviceId, "config": encodeXML(conf),
                       "serverId": serverId, "classId": classId}
            items = [configs]
            # Now we save and list again, and we should not expect any errors
            try:
                await self.db.save_configuration(config_name, items)
                self.status = (f"Success: Saved configuration {config_name} "
                               f"for device {deviceId}!")
                current_items = await self.db.list_configurations(deviceId)
                current_items = [view_item(c) for c in current_items]
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
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.READONLY)

    def _get_server_attributes(self, deviceId: str) -> tuple[str, str]:
        """Return the attributes `classId` and `serverId` of a device"""
        try:
            attrs = self.systemTopology[f"device.{deviceId}", ...]
            classId = attrs["classId"]
            serverId = attrs["serverId"]
            return serverId, classId
        except KeyError:
            raise RuntimeError(f"Device {deviceId} is not online.")

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
        self._schema_futures = {}

        # Dictionary of serverId: {classId: schema}
        self._class_schemas = defaultdict(dict)

    async def onInitialization(self):
        """Initialize the configuration database device and create the `DB`"""
        if not isStringSet(self.dbName):
            raise RuntimeError("DbName needs to be configured.")
        folder = Path(os.environ["KARABO"]).joinpath(
            "var", "data", "config_db")
        path = folder / self.dbName.value
        path.parent.mkdir(parents=True, exist_ok=True)
        self.db = ConfigurationDatabase(path)
        await self.db.assure_existing()
        self.state = State.ON

        # If we have a device name already we can retrieve a list!
        if self.deviceName:
            background(self._list_configurations())

        self.logger.info(f"Starting configuration manager {self.deviceId}")

    # Karabo Slot Interface
    # -----------------------------------------------------------------------

    @slot
    async def slotListInitConfigurations(self, info):
        """Slot to list configurations from name

        This slot requires an info Hash with `deviceId`
        """
        deviceId = info["deviceId"]
        name_part = info.get("name", "")
        items = await self.db.list_configurations(deviceId, name_part)
        items = HashList([dictToHash(config) for config in items])

        return Hash("success", True, "items", items)

    @slot
    async def slotGetInitConfiguration(self, info):
        """Slot to get an init configuration

        The info `Hash` must contain `deviceId` and `name`.
        """
        deviceId = info["deviceId"]
        name = info["name"]

        item = await self.db.get_configuration(deviceId, name)
        if not item:
            reason = (f"Failure: No configuration for device {deviceId} and "
                      f"name {name} found!")
            raise KaraboError(reason)

        item = dictToHash(item)
        config = item["config"]
        item["config"] = decodeXML(config)
        return Hash("success", True, "item", item)

    @slot
    async def slotListDevices(self, info: Hash):
        """List deviceIds that are stored in the database
        """
        devices = await self.db.list_devices()
        return Hash("success", True, "item", devices)

    @slot
    async def slotDeleteConfiguration(self, info: Hash):
        """Delete a device configuration"""
        await self.db.delete_configuration(info["deviceId"], info["name"])
        return Hash("success", True)

    @slot
    async def slotSaveInitConfiguration(self, info):
        """Slot to save configuration(s) from name

          The info Hash should contain:

           - client: client information
           - name: the non-empty (and unique for the device) name to be
                   associated with the configuration(s)

           - deviceIds: a vector of strings with deviceIds
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

        async def fetch_config(device_id):
            """Poll a single device for server and classId"""
            config = await getConfiguration(device_id)
            serverId, classId = self._get_server_attributes(device_id)
            if self.isConfigMode:
                schema = await self.get_schema(serverId, classId)
                config = extract_init_configuration(schema, config)
            else:
                config = hashToHash(config)
            config = encodeXML(config)
            config_dict = {"deviceId": device_id,
                           "config": config,
                           "classId": classId,
                           "serverId": serverId}
            return config_dict

        futures = [fetch_config(device_id) for device_id in deviceIds]
        timeout = len(deviceIds) * DEVICE_TIMEOUT
        items = await wait_for(gather(*futures), timeout=timeout)

        timestamp = Timestamp().toLocal()
        # Let it throw here if needed!
        await self.db.save_configuration(
            config_name, items, timestamp=timestamp)

        return Hash("success", True)

    async def get_schema(self, serverId: str, classId: str):
        # Get the class schema for this device!
        schema = self._class_schemas[serverId].get(classId, None)
        if schema is not None:
            return schema
        # No schema there, check if we are already looking for it!
        future = self._schema_futures.get((serverId, classId), None)
        if future is not None:
            # Let it throw here in case of failure
            await future
            schema = future.result()
            return schema
        else:
            future = Future()
            self._schema_futures[(serverId, classId)] = future
            try:
                schema = await wait_for(
                    getClassSchema(serverId, classId),
                    timeout=DEVICE_TIMEOUT)
            except (CancelledError, TimeoutError) as e:
                future.set_exception(e)
            except Exception as e:
                # In case we experience an unknown error, notify!
                future.set_exception(e)
            else:
                future.set_result(schema)
                # Got a new schema, cache it!
                self._class_schemas[serverId][classId] = schema
            finally:
                self._schema_futures.pop((serverId, classId), None)
                return future.result()

    @slot
    async def slotInstantiateDevice(self, info):
        """Slot to instantiate a device via the configuration manager

        The required info `Hash` must have at least the params:

        - deviceId: The mandatory parameter
        - name: Mandatory parameter
        - serverId: Optional parameter
        """
        deviceId = info["deviceId"]
        name = info["name"]
        serverId = info.get("serverId", None)

        item = await self.db.get_configuration(deviceId, name)
        if not item:
            reason = (f"No configuration for device {deviceId} and name "
                      f"{name} found!")
            raise KaraboError(reason)

        config = decodeXML(item["config"])
        classId = item["classId"]
        # If we did not provide a serverId, we take it from the item
        if serverId is None:
            serverId = item["serverId"]

        schema = await self.get_schema(serverId, classId)

        # XXX: In principle not required, safety measure to
        # remove further obsolete and readonly keys after storage
        # in case device changed over time
        cleaned_config = sanitize_init_configuration(schema, config)
        if not cleaned_config.fullyEqual(config):
            self.logger.info(
                f"Configuration {name} for device {deviceId} "
                f"with {classId} not fully equal")

        h = Hash()
        h["deviceId"] = deviceId
        h["classId"] = classId
        h["serverId"] = serverId
        h["configuration"] = cleaned_config

        success, msg = await self.call(serverId, "slotStartDevice", h)
        if not success:
            raise KaraboError(msg)

        return Hash("success", True)


def get_scene(deviceId):
    scene00 = TableElementModel(
        height=391,
        keys=[f"{deviceId}.view"],
        parent_component="DisplayComponent",
        width=481,
        x=370,
        y=270,
    )
    scene01 = DisplayTextLogModel(
        height=121,
        keys=[f"{deviceId}.status"],
        parent_component="DisplayComponent",
        width=671,
        x=190,
        y=80,
    )
    scene02 = LabelModel(
        font="Source Sans Pro,14,-1,5,75,0,1,0,0,0",
        height=31,
        parent_component="DisplayComponent",
        text="Karabo Configuration Manager",
        width=391,
        x=10,
        y=10,
    )
    scene03 = LineModel(
        stroke="#000000",
        stroke_width=2.0,
        x=20,
        x1=20,
        x2=840,
        y=200,
        y1=200,
        y2=200,
    )
    scene04 = DisplayLabelModel(
        font_size=10,
        font_weight="normal",
        height=27,
        keys=[f"{deviceId}.deviceId"],
        parent_component="DisplayComponent",
        width=391,
        x=10,
        y=50,
    )
    scene05 = DisplayStateColorModel(
        font_size=10,
        font_weight="normal",
        height=27,
        keys=[f"{deviceId}.state"],
        parent_component="DisplayComponent",
        show_string=True,
        width=441,
        x=410,
        y=50,
    )
    scene0600 = LabelModel(
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0",
        foreground="#000000",
        height=37,
        parent_component="DisplayComponent",
        text="Last Success",
        width=78,
        x=10,
        y=90,
    )
    scene0601 = ErrorBoolModel(
        height=37,
        keys=[f"{deviceId}.lastSuccess"],
        parent_component="DisplayComponent",
        width=103,
        x=88,
        y=90,
    )
    scene06 = BoxLayoutModel(
        height=37, width=181, x=10, y=90, children=[scene0600, scene0601]
    )
    scene07 = LabelModel(
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0",
        foreground="#000000",
        height=30,
        parent_component="DisplayComponent",
        text="Device",
        width=81,
        x=10,
        y=220,
    )
    scene08 = DisplayLabelModel(
        font_size=10,
        font_weight="normal",
        height=31,
        keys=[f"{deviceId}.deviceName"],
        parent_component="DisplayComponent",
        width=291,
        x=70,
        y=220,
    )
    scene09 = LineEditModel(
        height=31,
        keys=[f"{deviceId}.deviceName"],
        klass="EditableLineEdit",
        parent_component="EditableApplyLaterComponent",
        width=321,
        x=370,
        y=220,
    )
    scene010 = DisplayCommandModel(
        font_size=10,
        height=31,
        keys=[f"{deviceId}.listConfigurations"],
        parent_component="DisplayComponent",
        width=151,
        x=700,
        y=220,
    )
    scene011 = DisplayCommandModel(
        font_size=10,
        height=31,
        keys=[f"{deviceId}.saveConfigurations"],
        parent_component="DisplayComponent",
        width=171,
        x=190,
        y=350,
    )
    scene012 = LabelModel(
        font="Source Sans Pro,12,-1,5,50,0,1,0,0,0",
        foreground="#000000",
        height=37,
        parent_component="DisplayComponent",
        text="Save Configuration Name",
        width=191,
        x=10,
        y=270,
    )
    scene01300 = DisplayLabelModel(
        font_size=10,
        font_weight="normal",
        height=31,
        keys=[f"{deviceId}.configurationName"],
        parent_component="DisplayComponent",
        width=176,
        x=10,
        y=310,
    )
    scene01301 = EditableRegexModel(
        height=31,
        keys=[f"{deviceId}.configurationName"],
        parent_component="EditableApplyLaterComponent",
        width=175,
        x=186,
        y=310,
    )
    scene013 = BoxLayoutModel(
        height=31, width=351, x=10, y=310, children=[scene01300, scene01301]
    )
    scene014 = StickerModel(
        background="#d9d9d9",
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0",
        height=111,
        parent_component="DisplayComponent",
        text="- Configuration names are unique.\n- If there is an existing "
        "name, the configuration for that name is overwritten.\n- "
        "Configurations can only be deleted with an EXPERT access.\n",
        width=351,
        x=10,
        y=420,
    )
    scene = SceneModel(
        height=673,
        width=862,
        children=[
            scene00,
            scene01,
            scene02,
            scene03,
            scene04,
            scene05,
            scene06,
            scene07,
            scene08,
            scene09,
            scene010,
            scene011,
            scene012,
            scene013,
            scene014,
        ],
    )
    return write_scene(scene)
