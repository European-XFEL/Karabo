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
from asyncio import ensure_future, gather, sleep

from karabo.native import (
    AccessMode, Bool, Configurable, Hash, Timestamp, UInt32)

from .signalslot import slot

SLOT_PING_SLEEP = 2


class HeartBeatMixin(Configurable):

    track = Bool(
        defaultValue=False,
        displayedName="Track Heartbeats",
        description="This boolean will enable the heart beat tracking",
        accessMode=AccessMode.INITONLY)

    tickInterval = UInt32(
        defaultValue=20,
        minInc=20,
        description="Tick interval to check for heartbeats in seconds",
        accessMode=AccessMode.INITONLY)

    numBeats = UInt32(
        defaultValue=3,
        minInc=2,
        maxInc=10,
        description=("The number of potential beats that are left out before "
                     "an instanceGone is channeled"),
        accessMode=AccessMode.INITONLY)

    def __init__(self, configuration={}):
        super().__init__(configuration)
        self.deviceInstanceMap = {}
        self.systemTopology = Hash("device", Hash(), "server", Hash(),
                                   "macro", Hash(), "client", Hash())

    async def _run(self, **kwargs):
        await super()._run(**kwargs)
        if self.track:
            self._sigslot.emit("call", {"*": ["slotPing"]}, self.serverId, 0)
            # Before we actually start ticking, we wait a few seconds for all
            # slotInstanceInfos to arrive
            await sleep(SLOT_PING_SLEEP)
            ensure_future(self._sigslot.consume_beats(self))
            ensure_future(self._track_heartbeats())

    @slot
    def slotPingAnswer(self, instanceId, info):
        self._update_instance_info(instanceId, info)

    @slot
    async def slotInstanceNew(self, instanceId, info):
        await super().slotInstanceNew(instanceId, info)
        if self.track:
            await self._add_instance(instanceId, info)

    @slot
    def slotInstanceGone(self, instanceId, info):
        super().slotInstanceGone(instanceId, info)
        if self.track:
            self._remove_instance(instanceId, info)

    @slot
    def slotInstanceUpdated(self, instanceId, info):
        super().slotInstanceUpdated(instanceId, info)
        if self.track:
            self._update_instance_info(instanceId, info)

    async def updateHeartBeat(self, instanceId, info):
        """This method stamps the arrival of a heartbeat notification

        If the ``type`` of the instance is a server it is checked for
        underlying devices and eventually channel a topology event.
        """
        # This method is only called when we are tracking!
        instance_type = info["type"]
        if instanceId not in self.systemTopology[instance_type]:
            # Resurrection, refresh instanceInfo directly
            info = await self._sigslot.request(
                instanceId, "slotPing", instanceId, 1)
            await self._topology_changed(new=[(instanceId, info)], gone=[])
            await self._add_instance(instanceId, info)
        else:
            self._update_instance_info(instanceId, info)

    # Private interface
    # ----------------------------------------------------------------------

    async def _track_heartbeats(self):
        """Track the heartbeats of all instances"""
        while True:
            await sleep(self.tickInterval)
            await self._track_action()

    async def _track_action(self):
        actual_time = Timestamp()
        instances = []
        for instanceId, _, info in self.systemTopology["device"].iterall():
            if (actual_time - Timestamp.fromHashAttributes(info) >
                    self.numBeats * info["heartbeatInterval"]):
                instances.append((instanceId, info))
        for instanceId, _, info in self.systemTopology["server"].iterall():
            if (actual_time - Timestamp.fromHashAttributes(info) >
                    self.numBeats * info["heartbeatInterval"]):
                instances.append((instanceId, info))
        for instanceId, info in instances:
            self._remove_instance(instanceId, info)
        await self._topology_changed(new=[], gone=instances)

    def _update_instance_info(self, instanceId, info):
        """Update the system topology with the instanceInfo from `instance`

        This method adds the timestamp information to the `info`
        """
        instance_type = info["type"]
        info.update(Timestamp().toDict())
        ret = Hash(instance_type, Hash())
        ret[instance_type][instanceId] = Hash()
        ret[instance_type][instanceId, ...] = dict(info.items())
        self.systemTopology.merge(ret)

    def _remove_server_children(self, instanceId, info):
        """Cleanup the device children from the server

        Returns: devices if they were still in the topology
        """
        if info["type"] == "server":
            devices = [(k, a) for k, v, a in
                       self.systemTopology["device"].iterall()
                       if a["serverId"] == instanceId]
            for instanceId, _ in devices:
                self.systemTopology["device"].pop(instanceId, None)

            return devices if devices else None

    async def _add_instance(self, instanceId, info):
        """An instance is freshly added to the topology

        If a server is added to the topology, we check if devices
        are still tracked. Since the server is added, we must
        announce a `slotInstanceGone` for all devices.
        """
        self._update_instance_info(instanceId, info)
        # Check if this was a server coming back online!
        devices = self._remove_server_children(instanceId, info)
        if devices:
            await self._topology_changed(new=[], gone=devices)

    def _remove_instance(self, instanceId, info):
        """An instance is removed from the topology"""
        self._remove_server_children(instanceId, info)
        self.systemTopology[info["type"]].pop(instanceId, None)

    async def _topology_changed(self, new, gone):
        """Channel the topology tracking information

        This method will send out `slotInstanceGone and `slotInstanceNew`
        to the device children.
        """
        if new:
            await gather(*[dev.slotInstanceNew(instanceId, info)
                           for instanceId, info in new
                           for dev in self.deviceInstanceMap.values()])
        if gone:
            for device in self.deviceInstanceMap.values():
                for instanceId, info in gone:
                    device.slotInstanceGone(instanceId, info)
