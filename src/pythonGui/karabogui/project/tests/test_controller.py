from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.project.api import (
    PROJECT_OBJECT_CATEGORIES, DeviceConfigurationModel, DeviceInstanceModel,
    DeviceServerModel, MacroModel, ProjectModel)
from karabo.common.scenemodel.api import SceneModel
from karabogui.singletons.project_model import ProjectViewItemModel
from karabogui.testing import GuiTestCase

from ..controller.build import (
    create_device_server_controller, create_macro_controller,
    create_project_controller, destroy_project_controller)
from ..controller.scene import SceneController


def assert_no_notification_handlers(proj_model):
    # BaseProjectObjectModel sets a handler for all traits. Ignore it.
    ACCEPTABLE_HANDLERS = ('_anytrait_changed',)

    for name in PROJECT_OBJECT_CATEGORIES:
        t = proj_model.trait(name)
        notifiers = t._notifiers(1)

        acceptable = True
        for n in notifiers:
            if n.handler.__name__ not in ACCEPTABLE_HANDLERS:
                acceptable = False

        if not acceptable:
            names = '\n\t'.join([repr(n.handler) for n in notifiers])
            msg = 'Trait {} still has notifiers!\n\t{}'.format(name, names)
            raise AssertionError(msg)

    # Recurse!
    for sub in proj_model.subprojects:
        assert_no_notification_handlers(sub)


class ControllerTestCase(GuiTestCase):

    def setUp(self):
        super(ControllerTestCase, self).setUp()
        push_exception_handler(lambda *args: None, reraise_exceptions=True)

    def tearDown(self):
        super(ControllerTestCase, self).tearDown()
        pop_exception_handler()

    def test_project_controller(self):
        macros = [MacroModel()]
        scenes = [SceneModel()]
        dev0 = DeviceInstanceModel(instance_id='dev0')
        dev1 = DeviceInstanceModel(instance_id='dev1')
        servers = [DeviceServerModel(devices=[dev0, dev1])]
        subsubproject = ProjectModel(macros=macros)
        subprojects = [ProjectModel(macros=macros,
                                    subprojects=[subsubproject])]
        proj = ProjectModel(macros=macros, scenes=scenes, servers=servers,
                            subprojects=subprojects)

        creators = (create_macro_controller, SceneController,
                    create_device_server_controller, create_project_controller)

        insertions, removals = [], []

        def rows_inserted(parent, start, end):
            insertions.append((start, end))

        def rows_removed(parent, start, end):
            removals.append((start, end))

        qt_model = ProjectViewItemModel(parent=None)
        qt_model.rowsInserted.connect(rows_inserted)
        qt_model.rowsRemoved.connect(rows_removed)

        # Cause the controllers to be created and get a ref to the root
        qt_model.root_model = proj
        controller = qt_model.root_controller

        for subgroup, creator in zip(controller.children, creators):
            assert subgroup.child_create is creator
            assert len(subgroup.children) == 1

        proj_servers = proj.servers
        assert len(proj_servers) == len(servers) == 1
        serv = proj_servers[0]
        assert len(serv.devices) == len(servers[0].devices) == 2
        assert serv.devices[0].instance_id == 'dev0'
        assert serv.devices[1].instance_id == 'dev1'

        assert len(insertions) == 0
        assert len(removals) == 0

        proj.scenes.append(SceneModel())
        assert len(controller.children[1].children) == 2
        assert len(insertions) == 1
        assert insertions[0] == (1, 1)

        subproj = proj.subprojects.pop()
        assert len(controller.children[-1].children) == 0
        assert len(removals) == 1
        assert removals[0] == (0, 0)
        assert_no_notification_handlers(subproj)

        destroy_project_controller(controller)
        assert_no_notification_handlers(proj)

    def test_device_server_controller(self):
        sc0 = SceneModel()
        conf = DeviceConfigurationModel()
        dev0 = DeviceInstanceModel(instance_id='dev0', configs=[conf])
        dev1 = DeviceInstanceModel(instance_id='dev1', configs=[conf])
        devServ0 = DeviceServerModel(devices=[dev0, dev1])
        subdev0 = DeviceInstanceModel(instance_id='subdev0')
        subdev1 = DeviceInstanceModel(instance_id='subdev1')
        subDevServ0 = DeviceServerModel(devices=[subdev0, subdev1])
        subprojects = [ProjectModel(servers=[subDevServ0])]
        proj = ProjectModel(scenes=[sc0], servers=[devServ0],
                            subprojects=subprojects)

        qt_model = ProjectViewItemModel(parent=None)
        # Cause the controllers to be created and get a ref to the root
        qt_model.root_model = proj
        controller = qt_model.root_controller

        proj_groups = ['macros', 'scenes', 'servers', 'subprojects']
        assert len(proj_groups) == len(controller.children)

        for child in controller.children:
            if child.trait_name == proj_groups[1]:
                assert len(child.children) == 1
                proj.scenes.pop()
                assert len(child.children) == 0
                proj.scenes.append(SceneModel())
            elif child.trait_name == proj_groups[2]:
                assert len(child.children) == 1
                server_items = child.children
                assert server_items[0].model is devServ0
                device_instances = server_items[0].children
                assert len(device_instances) == 2
                # Add a small patch for the project_devices
                for device_instance in device_instances:
                    device_instance.__dict__["project_device"] = None
                devServ0.devices.pop()
                assert len(server_items[0].children) == 1
                dev2 = DeviceInstanceModel(instance_id='dev2')
                devServ0.devices.append(dev2)
                assert len(server_items[0].children) == 2
