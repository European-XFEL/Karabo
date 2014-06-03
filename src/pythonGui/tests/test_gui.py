from unittest import TestCase, main
import util # assure sip api is set first
from PyQt4.QtCore import QObject, pyqtSignal
import icons
from manager import Manager
import network

from karabo.hash import Hash, XMLParser
from karabo.hashtypes import Schema_
from itertools import count

from os import path

class Network(QObject):
    signalServerConnectionChanged = pyqtSignal(bool)
    signalUserChanged = pyqtSignal()

    def onServerConnection(self, connect):
        pass


    def onQuitApplication(self):
        pass

network.network = Network()
from gui import init


class Tests(TestCase):
    directory = path.dirname(__file__)
    def setUp(self):
        self.app = init([])


    def systemTopology(self):
        s = Hash("testserver", None)
        s["testserver", "deviceClasses"] = ["testclass"]
        s["testserver", "visibilities"] = [5]
        d = Hash("testdevice", None, "incompatible", None)
        d["testdevice", "serverId"] = "testserver"
        d["testdevice", "classId"] = "testclass"
        d["incompatible", "serverId"] = "testserver"
        d["incompatible", "classId"] = "testclass"
        h = Hash("device", d, "server", s)
        Manager().handleSystemTopology(h)
        self.assertTrue(Manager().systemTopology.has("testserver"))
        self.assertTrue(Manager().systemTopology.has("testdevice"))
        self.assertTrue(Manager().systemTopology.has("incompatible"))
        self.assertFalse(Manager().systemTopology.has("something"))


    def schema(self):
        r = XMLParser()
        with open("tests/schema.xml", "r") as fin:
            s = r.read(fin.read())
        h = Hash("serverId", "testserver", "classId", "testclass")
        h["schema"] = Schema_("testschema", s)
        cls = Manager().getClass("testserver", "testclass")
        Manager().handleClassSchema(h)

        self.assertEqual(cls.type, "class")
        self.assertEqual(cls.value.int32.value, 1234)
        root = cls.parameterEditor.invisibleRootItem()
        for i in count():
            node = root.child(i)
            if node.box.path[0] == "int32":
                break
        self.assertEqual(node.displayComponent.widget.text(), "0x4D2")
        self.assertEqual(node.text(0), "32 bit integer")


    def findIcon(self, a):
        for k in dir(icons):
            try:
                if a.cacheKey() == getattr(icons, k).cacheKey():
                    return k
            except AttributeError:
                pass
            except TypeError:
                pass
        return a


    def assertIcon(self, a, b):
        assert a.cacheKey() == b.cacheKey(), "{} is not {}".format(
                                        self.findIcon(a), self.findIcon(b))


    def project(self):
        Manager().projectTopology.projectOpen(path.join(self.directory,
                                              "project.krb"))

        root = Manager().projectTopology.invisibleRootItem()
        devices = root.child(0).child(0)
        self.assertEqual(devices.text(), "Devices")
        self.assertEqual(devices.child(0).text(), "testdevice")
        self.assertIcon(devices.child(0).icon(), icons.deviceInstance)
        self.assertEqual(devices.child(1).text(), "noserver")
        self.assertIcon(devices.child(1).icon(), icons.deviceOfflineNoServer)
        self.assertEqual(devices.child(2).text(), "noplugin")
        self.assertIcon(devices.child(2).icon(), icons.deviceOfflineNoPlugin)
        self.assertEqual(devices.child(3).text(), "incompatible")
        self.assertIcon(devices.child(3).icon(), icons.deviceOffline)
        self.assertEqual(devices.child(4).text(), "offline")
        self.assertIcon(devices.child(4).icon(), icons.deviceOffline)


    def test_gui(self):
        self.systemTopology()
        self.schema()
        self.project()


if __name__ == "__main__":
    main()
