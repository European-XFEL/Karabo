from contextlib import contextmanager
from io import StringIO
import os.path as op
from tempfile import TemporaryDirectory

from nose.tools import assert_raises
from traits.api import HasTraits, Bool, Enum, Float, Int, Range, String

from karabo.common.project.api import (
    MacroModel, ProjectDBCache, ProjectModel,
    read_lazy_object, walk_traits_object,
    PROJECT_OBJECT_CATEGORIES)
from ..api import (
    convert_old_project, read_project_model, write_project_model, OldProject
)

TEST_DOMAIN = 'TESTES'


def _compare_projects(proj0, proj1):
    COMPARABLE_TRAIT_TYPES = (Bool, Enum, Float, Int, Range, String)

    class _ObjectFlattener(object):
        def __init__(self):
            self.instances = []

        def __call__(self, obj):
            if isinstance(obj, HasTraits):
                self.instances.append(obj)

    def _get_comparable_trait_names(obj):
        comparables = []
        for n in obj.copyable_trait_names():
            trait = obj.trait(n)
            if isinstance(trait.trait_type, COMPARABLE_TRAIT_TYPES):
                comparables.append(n)
        if 'uuid' in comparables:
            comparables.remove('uuid')
        return comparables

    flattener0 = _ObjectFlattener()
    flattener1 = _ObjectFlattener()
    walk_traits_object(proj0, flattener0)
    walk_traits_object(proj1, flattener1)
    flat_proj0 = flattener0.instances
    flat_proj1 = flattener1.instances

    assert len(flat_proj0) == len(flat_proj1)

    for obj0, obj1 in zip(flat_proj0, flat_proj1):
        assert type(obj0) is type(obj1)
        trait_names = _get_comparable_trait_names(obj0)
        for attrname in trait_names:
            attr0 = getattr(obj0, attrname)
            attr1 = getattr(obj1, attrname)
            assert attr0 == attr1


def _get_old_project():
    import karabo.testing as mod

    path = op.join(op.dirname(mod.__file__), 'resources', 'reference.krb')
    old_project = OldProject(path)
    old_project.unzip()
    return old_project


@contextmanager
def _project_storage():
    with TemporaryDirectory() as dirpath:
        yield ProjectDBCache(dirpath)


def _write_project(project, devices, configs, storage):
    for childname in PROJECT_OBJECT_CATEGORIES:
        children = getattr(project, childname)
        for child in children:
            data = write_project_model(child)
            storage.store(TEST_DOMAIN, child.uuid, child.revision, data)
    for dev in devices:
        data = write_project_model(dev)
        storage.store(TEST_DOMAIN, dev.uuid, dev.revision, data)
    for conf in configs:
        data = write_project_model(conf)
        storage.store(TEST_DOMAIN, conf.uuid, conf.revision, data)

    data = write_project_model(project)
    storage.store(TEST_DOMAIN, project.uuid, project.revision, data)

# -----------------------------------------------------------------------------


def test_simple_read():
    model = MacroModel(code='print(42)', initialized=True)
    xml = write_project_model(model)
    rmodel = read_project_model(StringIO(xml), existing=None)
    assert model.code == rmodel.code


def test_invalid_read():
    with _project_storage() as storage:
        # Try to read something which is NOT THERE
        project = ProjectModel(revision=5)
        obj = read_lazy_object(TEST_DOMAIN, project.uuid, project.revision,
                               storage, read_project_model, existing=project)
        assert obj is project
        assert not obj.initialized


def test_save_project():
    old_project = _get_old_project()
    project, devices, configs = convert_old_project(old_project)

    with _project_storage() as storage:
        _write_project(project, devices, configs, storage)


def test_project_convert():
    old_project = _get_old_project()
    project, devices, configs = convert_old_project(old_project)

    for server in project.servers:
        for dev_inst in server.devices:
            assert not dev_inst.instance_id.endswith('.xml')


def test_project_round_trip():
    old_project = _get_old_project()
    project, devices, configs = convert_old_project(old_project)

    with _project_storage() as storage:
        _write_project(project, devices, configs, storage)
        rt_project = ProjectModel(uuid=project.uuid, revision=project.revision)
        rt_project = read_lazy_object(TEST_DOMAIN, project.uuid,
                                      project.revision, storage,
                                      read_project_model, existing=rt_project)

    _compare_projects(project, rt_project)


def test_project_cache():
    old_project = _get_old_project()
    project, devices, configs = convert_old_project(old_project)

    with _project_storage() as storage:
        _write_project(project, devices, configs, storage)
        project_uuids = storage.get_uuids_of_type(TEST_DOMAIN, 'project')
        assert len(project_uuids) == 1
        assert project_uuids[0] == project.uuid
        proj_data = storage.get_available_project_data(TEST_DOMAIN, 'project')
        assert proj_data[0]['uuid'] == project.uuid
        assert len(proj_data[0]['revisions']) == 1
        assert proj_data[0]['simple_name'] == project.simple_name


def test_uninitialized_save():
    model = ProjectModel()
    with assert_raises(AssertionError):
        write_project_model(model)


def test_wrong_existing_type():
    model = MacroModel(code='print(42)', initialized=True)
    xml = write_project_model(model)
    with assert_raises(AssertionError):
        read_project_model(StringIO(xml), existing=ProjectModel())


def test_existing_obj_mismatch():
    # `model` and `existing` have different UUIDs
    model = MacroModel(code='print(42)', initialized=True)
    existing = MacroModel()

    xml = write_project_model(model)
    with assert_raises(AssertionError):
        read_project_model(StringIO(xml), existing=existing)
