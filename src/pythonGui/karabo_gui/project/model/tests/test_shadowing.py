from karabo.common.project.api import (
    DeviceServerModel, DeviceInstanceModel, MacroModel, ProjectModel,
    PROJECT_OBJECT_CATEGORIES
)
from karabo.common.scenemodel.api import SceneModel
from ..macro import MacroModelItem
from ..scene import SceneModelItem
from ..shadow import (
    create_device_server_model_shadow, create_project_model_shadow,
    destroy_project_model_shadow
)


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


def test_project_model_shadowing():
    macros = [MacroModel()]
    scenes = [SceneModel()]
    dev0 = DeviceInstanceModel(instance_id='dev0')
    dev1 = DeviceInstanceModel(instance_id='dev1')
    servers = [DeviceServerModel(devices=[dev0, dev1])]
    subsubproject = ProjectModel(macros=macros)
    subprojects = [ProjectModel(macros=macros, subprojects=[subsubproject])]
    proj = ProjectModel(macros=macros, scenes=scenes, servers=servers,
                        subprojects=subprojects)

    creators = (MacroModelItem, SceneModelItem,
                create_device_server_model_shadow, create_project_model_shadow)
    item_model = create_project_model_shadow(proj)
    for subgroup, creator in zip(item_model.children, creators):
        assert subgroup.child_create is creator
        assert len(subgroup.children) == 1

    proj_servers = proj.servers
    assert len(proj_servers) == len(servers) == 1
    serv = proj_servers[0]
    assert len(serv.devices) == len(servers[0].devices) == 2
    assert serv.devices[0].instance_id == 'dev0'
    assert serv.devices[1].instance_id == 'dev1'

    subproj = proj.subprojects.pop()
    assert len(item_model.children[-1].children) == 0
    assert_no_notification_handlers(subproj)

    destroy_project_model_shadow(item_model)
    assert_no_notification_handlers(proj)
