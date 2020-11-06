#############################################################################
# Author: degon
# Created on November, 2019, 01:06 PM
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from asyncio import CancelledError
import json
from tornado.httpclient import AsyncHTTPClient
from tornado.platform.asyncio import AsyncIOMainLoop, to_asyncio_future

from karabo.common.scenemodel.api import (
    BoxLayoutModel, DaemonManagerModel, DisplayLabelModel,
    DisplayStateColorModel, LabelModel, RectangleModel, SceneModel,
    write_scene)
from karabo.middlelayer import (
    AccessMode, AccessLevel, Assignment, background, Configurable, coslot,
    DaqPolicy, Device, Double, Hash, Overwrite, sleep, slot, State,
    String, Unit, UInt32, VectorHash, VectorString)

STATUS_PAGE = "{}/status.json"


class ServiceSchema(Configurable):
    name = String(
        defaultValue="",
        description="The name of the service",
        accessMode=AccessMode.READONLY)

    status = String(
        defaultValue="",
        description="The status of the service, either running or down, etc.",
        accessMode=AccessMode.READONLY)

    since = String(
        defaultValue="",
        description="Since when did the service status not change",
        accessMode=AccessMode.READONLY)

    duration = String(
        defaultValue="0.0",
        description="The duration in seconds",
        unitSymbol=Unit.SECOND,
        accessMode=AccessMode.READONLY)

    host = String(
        defaultValue="",
        description="The host of the service",
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
        displayType="DaemonManager",
        defaultValue=[],
        displayedName="Servers",
        rows=ServiceSchema,
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
        defaultValue=['scene'])

    def __init__(self, configuration):
        super(DaemonManager, self).__init__(configuration)
        self.client = None
        self.aggregator_uri = ""
        self.service_links = Hash()

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
        payload = Hash('success', False)
        name = params.get('name', default='scene')
        if name == 'scene':
            payload.set('success', True)
            payload.set('name', name)
            payload.set('data', get_scene(self.deviceId))

        return Hash('type', 'deviceScene',
                    'origin', self.deviceId,
                    'payload', payload)

    @coslot
    async def requestDaemonAction(self, params):
        payload = Hash()
        server_id = params.get('serverId', default='')
        host_id = params.get('hostId', default='')
        action = params.get('action', default='')
        success, text = await self._action_service(server_id, host_id, action)
        # We further fill the payload for future use cases
        payload.set('success', success)
        payload.set('reason', text)
        return Hash('type', 'daemonAction',
                    'origin', self.deviceId,
                    'payload', payload)

    async def fetch(self):
        while True:
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
                    self.status = "Fetched server information"

                reply = json.loads(response.body)
                servers = reply['servers']
                self.numHosts = len(servers)

                # XXX: the webservers exposes the services using the the
                # service directory name. slashes and dots are mangled
                num_services = 0
                table_value = []
                self.service_links = Hash()
                for host_name in servers:
                    info = servers[host_name]
                    num_services += len(info['services'])
                    link = info['link']
                    for service in info['services']:
                        table_value.append(
                            (service['name'], service['status'],
                             service['since'], service['duration'],
                             host_name))
                        # Store the web link in a separate hash
                        key = "{}.{}".format(host_name, service['name'])
                        self.service_links[key] = link
                # Set the number of seen services in the ecosystem!
                self.numServices = num_services
                self.services = table_value

            await sleep(self.updateTime)

    async def _action_service(self, service_name, host, command):
        self.logger.info(
            "Action executed by daemon device: Action: {}, service_name: {}, "
            "host: {}".format(service_name, host, command))

        try:
            link = self.service_links["{}.{}".format(host, service_name)]
            cmd = "{}/api/servers/{}.json".format(link, service_name)
            my_json = {
                "server":
                    {
                        "name": service_name,
                        "command": command,
                    },
            }
            fut = self.client.fetch(
                cmd, body=json.dumps(my_json), method="PUT")
            await to_asyncio_future(fut)
            return True, ("Command {} executed succesfully for "
                          "service {}".format(command, service_name))
        except CancelledError:
            # NOTE: Do nothing, we only got cancelled by the server
            return False, ("Command {} was CANCELLED for service {}".format(
                command, service_name))
        except Exception as e:
            self.status = "Error: {}".format(e)
            return False, ("Command {} was NOT executed succesfully for "
                           "service {}".format(command, service_name))


def get_scene(deviceId):
    scene0 = DaemonManagerModel(
        height=533.0, keys=[f'{deviceId}.services'],
        parent_component='DisplayComponent', width=960.0, x=11.0, y=158.0)
    scene1 = LabelModel(
        font='Source Sans Pro,12,-1,5,75,0,0,0,0,0,Bold', height=42.0,
        parent_component='DisplayComponent', text='Karabo Daemon Manager',
        width=363.0, x=12.0, y=10.0)
    scene2 = RectangleModel(
        fill='#b8b8b8', height=4.0, stroke='#000000', width=958.0, x=9.0,
        y=141.0)
    scene30 = DisplayLabelModel(
        font_size=10, height=36.0, keys=[f'{deviceId}.numHosts'],
        parent_component='DisplayComponent', width=109.0, x=173.0, y=52.0)
    scene31 = DisplayLabelModel(
        font_size=10, height=35.0, keys=[f'{deviceId}.numServices'],
        parent_component='DisplayComponent', width=109.0, x=173.0, y=88.0)
    scene3 = BoxLayoutModel(
        direction=2, height=71.0, width=109.0, x=173.0, y=52.0,
        children=[scene30, scene31])
    scene40 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=35.0, parent_component='DisplayComponent',
        text='Number of Hosts', width=148.0, x=29.0, y=52.0)
    scene41 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=35.0, parent_component='DisplayComponent',
        text='Number of Services', width=148.0, x=29.0, y=87.0)
    scene4 = BoxLayoutModel(
        direction=2, height=70.0, width=148.0, x=29.0, y=52.0,
        children=[scene40, scene41])
    scene50 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=36.0, parent_component='DisplayComponent', text='DeviceID',
        width=59.0, x=268.0, y=18.0)
    scene51 = DisplayLabelModel(
        font_size=10, height=36.0, keys=[f'{deviceId}.deviceId'],
        parent_component='DisplayComponent', width=398.0, x=327.0, y=18.0)
    scene5 = BoxLayoutModel(
        height=36.0, width=457.0, x=268.0, y=18.0, children=[scene50, scene51])
    scene60 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=54.0, parent_component='DisplayComponent', text='Status',
        width=42.0, x=285.0, y=60.0)
    scene61 = DisplayLabelModel(
        font_size=10, height=54.0, keys=[f'{deviceId}.status'],
        parent_component='DisplayComponent', width=543.0, x=327.0, y=60.0)
    scene6 = BoxLayoutModel(
        height=54.0, width=585.0, x=285.0, y=60.0, children=[scene60, scene61])
    scene7 = DisplayStateColorModel(
        height=26.0, keys=[f'{deviceId}.state'],
        parent_component='DisplayComponent', show_string=True, width=237.0,
        x=734.0, y=23.0)
    scene = SceneModel(
        height=712.0, width=983.0,
        children=[scene0, scene1, scene2, scene3, scene4, scene5, scene6,
                  scene7])
    return write_scene(scene)
