import json

from karabo.bound import State
from karabo.bound.testing import ServerContext, eventLoop, sleepUntil


def test_server_context(eventLoop: eventLoop):
    config = {"PropTestA": {"classId": "PropertyTest"}}
    initA = json.dumps(config)
    serverA = ServerContext("testServerA",
                            ["log.level=DEBUG", f"init={initA}"])
    with serverA:
        remote = serverA.remote()
        sleepUntil(lambda: "PropTestA" in remote.getDevices(), timeout=10)
        assert len(remote.getDevices()) > 0
        assert remote.get("PropTestA", "state") == State.NORMAL
        remote.execute("PropTestA", "startWritingOutput")
        sleepUntil(lambda: remote.get("PropTestA", "state") == State.STARTED,
                   timeout=5)
        assert remote.get("PropTestA", "state") == State.STARTED

        config = {"PropTestB": {"classId": "PropertyTest"}}
        initB = json.dumps(config)

        serverB = ServerContext("testServerB",
                                ["log.level=DEBUG", f"init={initB}"],
                                client=remote)
        with serverB:
            sleepUntil(lambda: "PropTestB" in remote.getDevices(), timeout=10)
