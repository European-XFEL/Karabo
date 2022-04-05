#############################################################################
# Author: degon
# Created on November, 2019, 01:06 PM
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import json
from asyncio import CancelledError

from tornado.httpclient import AsyncHTTPClient
from tornado.platform.asyncio import AsyncIOMainLoop, to_asyncio_future

from karabo.common.scenemodel.api import (
    BoxLayoutModel, DisplayLabelModel, DisplayStateColorModel,
    FilterTableElementModel, LabelModel, RectangleModel, SceneModel,
    write_scene)
from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Bool, Configurable, DaqPolicy, Device,
    Double, Hash, Overwrite, State, String, UInt32, Unit, VectorHash,
    VectorString, background, coslot, has_changes, sleep, slot)

STATUS_PAGE = "{}/status.json"


def get_tablenew_row(entry):
    status = entry["status"]
    is_stop = status.startswith("up")
    is_start = status.startswith("down")
    simple_status = "UP"
    if is_start:
        simple_status = "DOWN"
    elif "want" in status or entry["duration"] < 3:
        simple_status = "CHANGING"
        is_stop = entry["duration"] < 3
    return (entry["karabo_name"], simple_status, is_start, is_stop)


COMMAND_MAP = {
    # maps ServiceInteractiveRow keys
    # to daemontools commands
    "start": "up",
    "stop": "down",
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
        accessMode=AccessMode.READONLY
    )

    stop = Bool(
        defaultValue=True,
        displayedName="Stop",
        displayType="TableBoolButton",
        description="Stop the service",
        accessMode=AccessMode.READONLY
    )


class DaemonManager(Device):
    """This daemon manager is provides control to the web services

    An http connection is established to the `WebAggregator` service
    controlling the various web servers.

    This device exposes the control offered by the web server to the
    karabo control system for restarting device servers
    """
    state = Overwrite(
        defaultValue=State.INIT,
        options=[State.INIT, State.UNKNOWN, State.ON])

    visibility = Overwrite(
        defaultValue=AccessLevel.ADMIN,
        options=[AccessLevel.ADMIN])

    host = String(
        displayedName="Host",
        defaultValue="localhost",
        description="The host name of the web aggregator service",
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.MANDATORY,
        daqPolicy=DaqPolicy.OMIT)

    port = UInt32(
        displayedName="Port",
        defaultValue=8585,
        description="The port of the daemon service aggregator",
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.MANDATORY,
        daqPolicy=DaqPolicy.OMIT)

    numHosts = UInt32(
        displayedName="Number of Hosts",
        defaultValue=0,
        description="This value provides the number of hosts observed",
        accessMode=AccessMode.READONLY,
        daqPolicy=DaqPolicy.OMIT)

    numServices = UInt32(
        displayedName="Number of Services",
        defaultValue=0,
        description="This value provides the number of services observed",
        accessMode=AccessMode.READONLY,
        daqPolicy=DaqPolicy.OMIT)

    services = VectorHash(
        defaultValue=[],
        displayedName="Server View",
        description="Servers Table with interactive interface",
        rows=ServiceInteractiveRow,
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
        daqPolicy=DaqPolicy.OMIT)

    updateTime = Double(
        defaultValue=5.0,
        minInc=5.0,
        maxInc=20.0,
        description="Update time",
        unitSymbol=Unit.SECOND,
        daqPolicy=DaqPolicy.OMIT)

    availableScenes = VectorString(
        displayedName="Available Scenes",
        displayType="Scenes",
        description="Provides a scene for the Daemon Manager.",
        accessMode=AccessMode.READONLY,
        daqPolicy=DaqPolicy.OMIT,
        defaultValue=["scene"])

    def __init__(self, configuration):
        super(DaemonManager, self).__init__(configuration)
        self.client = None
        self.post_action_tasks = dict()
        self.aggregator_uri = ""
        self.services_info = dict()

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

    async def fetch(self):
        while True:
            await self._fetch()
            await sleep(self.updateTime)

    async def _fetch(self):
        try:
            fut = self.client.fetch(
                STATUS_PAGE.format(self.aggregator_uri), method="GET")
            response = await to_asyncio_future(fut)
        except CancelledError:
            # NOTE: Do nothing, we only got cancelled by the server
            pass
        except Exception as e:
            if self.state != State.UNKNOWN:
                self.state = State.UNKNOWN
                self.status = "Error: {}".format(e)
        else:
            if self.state != State.ON:
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
            self.services_info = dict()
            for host_name in servers:
                info = servers[host_name]
                link = info["link"]
                for service in info["services"]:
                    entry = {
                        data_key: service[data_key]
                        for data_key in data_keys
                    }
                    entry["link"] = link
                    entry["host_name"] = host_name
                    table_value.append(get_tablenew_row(entry))
                    # Store the web link in a separate dict
                    key = f"{entry['host_name']}.{entry['karabo_name']}"
                    self.services_info[key] = entry
            # Set the number of seen services in the ecosystem!
            table_value.sort()
            table_value = self.services.descriptor.toKaraboValue(table_value)
            if has_changes(self.services.to_hashlist(),
                           table_value.to_hashlist()):
                self.numServices = len(table_value)
                self.services = table_value

    @coslot
    async def requestAction(self, params):
        payload = Hash("success", False, "reason", "Unknown")
        try:
            action = params.get("action", "missing")
            # only TableButton Action is implemented
            assert action == "TableButton", f"unexpected action '{action}'"
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
            success, text = await self._action_service(
                server_id, host_id, command, post_action=True)
            payload.set("success", success)
            payload.set("reason", text)
            hashlist = self.services.to_hashlist()
            for _row in hashlist:
                if _row["name"] == server_id:
                    _row["status"] = "CHANGING"
                    _row["start"] = False
                    _row["restart"] = False
                    _row["stop"] = False
                    break
            self.services = hashlist
        except (KeyError, AssertionError) as e:
            payload.set("success", False)
            payload.set("reason", str(e))

        return Hash("type", "requestAction",
                    "origin", self.deviceId,
                    "payload", payload)

    async def _action_service(self, service_name, host, command,
                              post_action=False):
        self.logger.info(
            f"Action executed by daemon device: Action: {service_name}, "
            f"service_name: {host}, host: {command}")

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
            if post_action:
                self._post_action(service_name, host, command)
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

    def _post_action(self, service_name, host, command):
        task = self.post_action_tasks.get((service_name, host))
        if command == "down" and task is None:
            self.post_action_tasks[(service_name, host)] = background(
                self._kill_service_if_late(service_name, host))

    async def _kill_service_if_late(self, service_name, host):
        try:
            # fetch for 7 seconds
            calls = 7
            while calls > 0:
                await sleep(1)
                await self._fetch()
                hashlist = self.services.to_hashlist()
                for _row in hashlist:
                    # if the server is down, we do not need further action
                    if (_row["name"] == service_name and
                            _row["status"] == "DOWN"):
                        return
            # kill if the server is not down yet
            await self._action_service(service_name, host, "kill")
        except CancelledError:
            pass
        finally:
            self.post_action_tasks.pop((service_name, host))


def get_scene(deviceId):
    scene0 = FilterTableElementModel(
        height=533.0, keys=[f"{deviceId}.services"],
        parent_component="DisplayComponent", resizeToContents=True,
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
