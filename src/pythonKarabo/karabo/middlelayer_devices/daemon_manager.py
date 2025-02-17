#############################################################################
# Author: degon
# Created on November, 2019, 01:06 PM
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
import json
import socket
from asyncio import CancelledError

from tornado.httpclient import AsyncHTTPClient
from tornado.platform.asyncio import AsyncIOMainLoop, to_asyncio_future

from karabo.common.scenemodel.api import (
    BoxLayoutModel, DisplayLabelModel, DisplayStateColorModel,
    FilterTableElementModel, LabelModel, RectangleModel, SceneModel,
    write_scene)
from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Bool, Configurable, Device, Double,
    Hash, KaraboError, Overwrite, State, String, UInt32, Unit, VectorHash,
    VectorString, background, dictToHash, has_changes, sleep, slot)

STATUS_PAGE = "{}/status.json"
NETWORK_PAGE = "{}/network.json"

STATUS_UP = "UP"
STATUS_DOWN = "DOWN"
STATUS_CHANGING = "CHANGING"
WAIT_STATUS_POLLTIME = 1  # seconds


def get_hostname_from_alias(host_alias):
    """Get the hostname from the alias."""
    ip_address = socket.gethostbyname(host_alias)
    host_name = socket.getfqdn(ip_address).split(".")[0]
    return host_name


def get_table_row(entry):
    """Compute the table row information for a service entry

    returns: tuple of karabo name and boolean state machine information
    """
    status = entry["status"]
    is_stop = status.startswith("up")
    is_start = status.startswith("down")
    status_orphanage = "orphanage" in status
    status_want = "want" in status

    low_duration = entry["duration"] <= 1
    simple_status = STATUS_UP
    if is_start:
        simple_status = STATUS_DOWN
    elif status_want or low_duration or status_orphanage:
        simple_status = STATUS_CHANGING
        is_stop = low_duration or status_orphanage

    is_restart = is_stop
    return (entry["karabo_name"], simple_status, is_start, is_restart, is_stop)


COMMAND_MAP = {
    # maps ServiceInteractiveRow keys
    # to daemontools commands
    "start": "up",
    "stop": "down",
    "restart": "down"
}


class ServiceInteractiveRow(Configurable):
    name = String(
        defaultValue="",
        displayedName="Service",
        description="The name of the service",
        accessMode=AccessMode.READONLY)

    status = String(
        defaultValue="",
        displayedName="Status",
        displayType="TableColor|DOWN=red&UP=lightgreen&default=lightblue",
        description="The status of the service, either running or down, etc.",
        accessMode=AccessMode.READONLY)

    start = Bool(
        defaultValue=True,
        displayedName="Start",
        displayType="TableBoolButton",
        description="Start the service",
        accessMode=AccessMode.READONLY)

    restart = Bool(
        defaultValue=True,
        displayedName="Restart",
        displayType="TableBoolButton|confirmation=1",
        description="Restart the service",
        accessMode=AccessMode.READONLY)

    stop = Bool(
        defaultValue=True,
        displayedName="Stop",
        displayType="TableBoolButton|confirmation=1",
        description="Stop the service",
        accessMode=AccessMode.READONLY)


class DaemonManager(Device):
    """This daemon manager is provides control to the web services

    An http connection is established to the `WebAggregator` service
    controlling the various web servers.

    This device exposes the control offered by the web server to the
    karabo control system for restarting device servers
    """
    state = Overwrite(
        defaultValue=State.INIT,
        options=[State.INIT, State.UNKNOWN, State.CHANGING,
                 State.ON])

    host = String(
        displayedName="Host",
        defaultValue="localhost",
        description="The host name of the web aggregator service",
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.MANDATORY)

    port = UInt32(
        displayedName="Port",
        defaultValue=8585,
        description="The port of the daemon service aggregator",
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.MANDATORY)

    numHosts = UInt32(
        displayedName="Number of Hosts",
        defaultValue=0,
        description="This value provides the number of hosts observed",
        accessMode=AccessMode.READONLY)

    numServices = UInt32(
        displayedName="Number of Services",
        defaultValue=0,
        description="This value provides the number of services observed",
        accessMode=AccessMode.READONLY)

    services = VectorHash(
        defaultValue=[],
        displayedName="Server View",
        description="Servers Table with interactive interface",
        rows=ServiceInteractiveRow,
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.OPERATOR)

    updateTime = Double(
        defaultValue=5.0,
        minInc=1.0,
        maxInc=20.0,
        description="Update time",
        unitSymbol=Unit.SECOND)

    availableScenes = VectorString(
        displayedName="Available Scenes",
        displayType="Scenes",
        description="Provides a scene for the Daemon Manager.",
        accessMode=AccessMode.READONLY,
        defaultValue=["scene"])

    def __init__(self, configuration):
        super().__init__(configuration)
        self.client = None
        self.post_action_tasks = {}
        self.aggregator_uri = ""
        self.services_info = {}

    async def onInitialization(self):
        if hasattr(AsyncIOMainLoop, "initialized"):
            if not AsyncIOMainLoop.initialized():
                AsyncIOMainLoop().install()
        self.client = AsyncHTTPClient()
        self.aggregator_uri = "http://{}:{}".format(self.host.value,
                                                    self.port.value)
        self.task = background(self.fetch())

    @slot
    def requestScene(self, params):
        """Fulfill a scene request from another device.

        :param params: A `Hash` containing the method parameters
        """
        payload = Hash("success", False)
        name = params.get("name", default="scene")
        if name == "scene":
            payload.set("success", True)
            payload.set("name", name)
            payload.set("data", get_scene(self.deviceId))
        return Hash("type", "deviceScene",
                    "origin", self.deviceId,
                    "payload", payload)

    @slot
    async def requestNetwork(self, params):
        """ Fetch the network information of the given host.

        :param params: A `Hash` containing the hostname.
        """
        host = params.get("host")
        payload = Hash("host", host, "network", Hash())
        try:
            fut = self.client.fetch(
                NETWORK_PAGE.format(self.aggregator_uri), method="GET")
            response = await to_asyncio_future(fut)
        except CancelledError:
            raise
        except Exception:
            # Not supported, ...
            raise KaraboError(
                f"Network information not supported for {host}.")
        else:
            reply = json.loads(response.body)

            # Extract the network info of the host
            host_name = get_hostname_from_alias(host)
            network_info = reply["network"][host_name]
            host_info = Hash(host, dictToHash(network_info))
            payload["network"] = host_info

        return Hash("type", "requestNetwork",
                    "origin", self.deviceId,
                    "payload", payload)

    async def fetch(self):
        while True:
            await self._fetch()
            await sleep(self.updateTime)

    async def _fetch(self, update=True):
        """Fetch the online data from the webaggregator

        If set to `update` parameter is set to `True`, the service table
        will be updated.

        :returns: Success boolean and table
        """
        try:
            fut = self.client.fetch(
                STATUS_PAGE.format(self.aggregator_uri), method="GET")
            response = await to_asyncio_future(fut)
        except CancelledError:
            # NOTE: Do nothing, we only got cancelled by the server
            return False, None
        except Exception as e:
            if self.state != State.UNKNOWN:
                self.state = State.UNKNOWN
                self.status = f"Error in fetch: {e}"
                self.services = []
                self.numHosts = 0
                self.numServices = 0
            return False, None
        else:
            changing = len(self.post_action_tasks) > 0
            if self.state != State.ON and not changing:
                self.state = State.ON
            # Synchronize to error status outside of the normal polling.
            status = "Fetched server information"
            if self.status != status:
                self.status = status

            reply = json.loads(response.body)
            servers = reply["servers"]
            self.numHosts = len(servers)

            table_value = []
            data_keys = ("karabo_name", "name", "since", "status", "duration")
            self.services_info = {}
            for host_name in servers:
                info = servers[host_name]
                link = info["link"]
                if "services" not in info:
                    self.logger.error(f"No services for {host_name}")
                    continue
                for service in info["services"]:
                    entry = {
                        data_key: service[data_key]
                        for data_key in data_keys
                    }
                    entry["link"] = link
                    entry["host_name"] = host_name
                    table_value.append(get_table_row(entry))
                    # Store the web link in a separate dict
                    key = f"{entry['host_name']}.{entry['karabo_name']}"
                    self.services_info[key] = entry
            table_value.sort()
            table_value = self.services.descriptor.toKaraboValue(table_value)
            if update and has_changes(self.services.value, table_value.value):
                self.numServices = len(table_value)
                self.services = table_value
            return True, table_value

    @slot
    async def requestAction(self, params):
        action = params.get("action", "missing")
        # only TableButton Action is implemented
        assert action == "TableButton", f"unexpected action {action}"
        path = params["path"]
        assert path == "services", f"unexpected path '{path}'"
        data = params["table"]
        row = data["rowData"]
        server_id = row["name"]
        host_id = [_row["host_name"]
                   for _row in self.services_info.values()
                   if _row["karabo_name"] == server_id][0]
        header = data["header"]
        command = COMMAND_MAP[header]
        # We are still performing an action on device
        if self.post_action_tasks.get(
                (server_id, host_id)):
            raise KaraboError(
                "KaraboDaemonManager is still waiting for an action on "
                f"{server_id} on {host_id}. Please try again later.")

        success, text = await self._action_service(
            server_id, host_id, command, post_action=header)
        # The first entry is the found row, must be there
        index = self.services.where("name", server_id)[0]
        self.services[index] = Hash(
            "name", server_id,
            "status", "CHANGING",
            "start", False,
            "restart", False,
            "stop", False)

        payload = Hash("success", success, "reason", text)
        return Hash("type", "requestAction",
                    "origin", self.deviceId,
                    "payload", payload)

    async def _action_service(self, service_name, host, command,
                              post_action=None):
        self.logger.info(
            f"Action executed by daemon device: Action: {command}, "
            f"service_name: {service_name}, host: {host}, "
            f"post_action: {post_action}")

        try:
            info = self.services_info[f"{host}.{service_name}"]
            link = info["link"]
            daemon_name = info["name"]
            cmd = f"{link}/api/servers/{daemon_name}.json"
            request = {
                "server":
                    {
                        "name": daemon_name,
                        "command": command,
                    },
            }
            fut = self.client.fetch(
                cmd, body=json.dumps(request), method="PUT")
            await to_asyncio_future(fut)
            if post_action is not None:
                self._post_action(service_name, host, post_action)
            return True, (f"Command {command} executed succesfully for "
                          f"service {service_name}")
        except CancelledError:
            # NOTE: Do nothing, we only got cancelled by the server
            return False, (f"Command {command} was CANCELLED "
                           f"for service {service_name}")
        except Exception as e:
            self.status = f"Error: {e}"
            return False, (f"Command {command} was NOT executed "
                           f"succesfully for service {service_name}")

    def _post_action(self, service_name, host, request):
        """A post action for a service related to service request"""
        if self.state != State.CHANGING:
            self.state = State.CHANGING
        if request == "start":
            self.post_action_tasks[(service_name, host)] = background(
                self._ensure_service_up(service_name, host))
        elif request == "stop":
            self.post_action_tasks[(service_name, host)] = background(
                self._ensure_service_down(service_name, host))
        elif request == "restart":
            self.post_action_tasks[(service_name, host)] = background(
                self._ensure_service_restart(service_name, host))

    async def wait_status(self, service_name, status, num=7):
        """Wait for a status `status` for a `service_name` for
        a number of polls"""
        try:
            total_calls = num
            while total_calls > 0:
                await sleep(WAIT_STATUS_POLLTIME)
                success, table = await self._fetch(update=False)
                if success:
                    row = table.where_value(
                        "name", service_name).value[0]
                    if row["status"] == status:
                        return True
                total_calls -= 1
        except CancelledError:
            pass
        return False

    async def _ensure_service_restart(self, service_name, host):
        """Ensure a service can restart after being stopped"""
        try:
            if not await self.wait_status(service_name, STATUS_DOWN):
                self.logger.info(
                    "Server didn't go down as expected, now trying to kill.")
                await self._action_service(service_name, host, "kill")
            # Wait until the kill resolves
            if not await self.wait_status(service_name, STATUS_DOWN, num=15):
                self.logger.error(f"Service {service_name} does not go down")
                return
            await self._action_service(service_name, host, "up")
        finally:
            self.post_action_tasks.pop((service_name, host))

    async def _ensure_service_down(self, service_name, host):
        """Ensure a service is down by killing a service"""
        try:
            if not await self.wait_status(service_name, STATUS_DOWN):
                self.logger.info(
                    "Server didn't go down as expected, now trying to kill.")
                await self._action_service(service_name, host, "kill")
        finally:
            self.post_action_tasks.pop((service_name, host))

    async def _ensure_service_up(self, service_name, host):
        """If a service does not come online after a period, the process
        gets killed once to try again later"""
        try:
            if not await self.wait_status(service_name, STATUS_UP):
                self.logger.info(
                    "Server didn't come up as expected, now trying to kill.")
                await self._action_service(service_name, host, "kill")
        finally:
            self.post_action_tasks.pop((service_name, host))


def get_scene(deviceId):
    scene0 = FilterTableElementModel(
        height=533.0, keys=[f"{deviceId}.services"],
        parent_component="DisplayComponent", resizeToContents=True,
        sortingEnabled=True,
        width=960.0, x=11.0, y=158.0)
    scene1 = LabelModel(
        font="Source Sans Pro,12,-1,5,75,0,0,0,0,0,Bold", height=42.0,
        parent_component="DisplayComponent", text="Karabo Daemon Manager",
        width=363.0, x=12.0, y=10.0)
    scene2 = RectangleModel(
        fill="#b8b8b8", height=4.0, stroke="#000000", width=958.0, x=9.0,
        y=141.0)
    scene30 = DisplayLabelModel(
        font_size=10, height=36.0, keys=[f"{deviceId}.numHosts"],
        parent_component="DisplayComponent", width=109.0, x=173.0, y=52.0)
    scene31 = DisplayLabelModel(
        font_size=10, height=35.0, keys=[f"{deviceId}.numServices"],
        parent_component="DisplayComponent", width=109.0, x=173.0, y=88.0)
    scene3 = BoxLayoutModel(
        direction=2, height=71.0, width=109.0, x=173.0, y=52.0,
        children=[scene30, scene31])
    scene40 = LabelModel(
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0", foreground="#000000",
        height=35.0, parent_component="DisplayComponent",
        text="Number of Hosts", width=148.0, x=29.0, y=52.0)
    scene41 = LabelModel(
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0", foreground="#000000",
        height=35.0, parent_component="DisplayComponent",
        text="Number of Services", width=148.0, x=29.0, y=87.0)
    scene4 = BoxLayoutModel(
        direction=2, height=70.0, width=148.0, x=29.0, y=52.0,
        children=[scene40, scene41])
    scene50 = LabelModel(
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0", foreground="#000000",
        height=36.0, parent_component="DisplayComponent", text="DeviceID",
        width=59.0, x=268.0, y=18.0)
    scene51 = DisplayLabelModel(
        font_size=10, height=36.0, keys=[f"{deviceId}.deviceId"],
        parent_component="DisplayComponent", width=398.0, x=327.0, y=18.0)
    scene5 = BoxLayoutModel(
        height=36.0, width=457.0, x=268.0, y=18.0, children=[scene50, scene51])
    scene60 = LabelModel(
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0", foreground="#000000",
        height=54.0, parent_component="DisplayComponent", text="Status",
        width=42.0, x=285.0, y=60.0)
    scene61 = DisplayLabelModel(
        font_size=10, height=54.0, keys=[f"{deviceId}.status"],
        parent_component="DisplayComponent", width=398.0, x=327.0, y=60.0)
    scene6 = BoxLayoutModel(
        height=54.0, width=440.0, x=285.0, y=60.0, children=[scene60, scene61])
    scene7 = DisplayStateColorModel(
        height=26.0, keys=[f"{deviceId}.state"],
        parent_component="DisplayComponent", show_string=True, width=237.0,
        x=734.0, y=23.0)
    scene = SceneModel(
        height=712.0, width=983.0,
        children=[scene0, scene1, scene2, scene3, scene4, scene5, scene6,
                  scene7])
    return write_scene(scene)
