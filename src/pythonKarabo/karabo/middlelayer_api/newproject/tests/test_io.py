from contextlib import contextmanager
import os.path as op
from tempfile import TemporaryDirectory

from traits.api import Dict, Enum, Int, String

from karabo.common.project.api import (ProjectDBCache, read_lazy_object,
                                       PROJECT_OBJECT_CATEGORIES)
from karabo.middlelayer import Project
from ..convert import convert_old_project
from ..io import read_project_model, write_project_model


def _compare_projects(proj0, proj1):
    COMPARABLE_TRAIT_TYPES = (Dict, Enum, Int, String)

    def _get_comparable_trait_names(obj):
        comparables = []
        names = obj.visible_traits()
        for n in names:
            trait = obj.trait(n)
            if trait.trait_type in COMPARABLE_TRAIT_TYPES:
                comparables.append(n)
        return comparables

    assert proj0.uuid == proj1.uuid
    assert proj0.revision == proj1.revision
    assert proj0.simple_name == proj1.simple_name

    for part in PROJECT_OBJECT_CATEGORIES:
        proj0_models = getattr(proj0, part)
        proj1_models = getattr(proj1, part)

        assert len(proj0_models) == len(proj1_models)
        if len(proj0_models) == 0:
            continue

        trait_names = _get_comparable_trait_names(proj0_models[0])
        for model0, model1 in zip(proj0_models, proj1_models):
            for attrname in trait_names:
                assert getattr(model0, attrname) == getattr(model1, attrname)


def _get_old_project():
    import karabo.testing as mod

    path = op.join(op.dirname(mod.__file__), 'resources', 'reference.krb')
    old_project = Project(path)
    old_project.unzip()
    return old_project


@contextmanager
def _project_storage():
    with TemporaryDirectory() as dirpath:
        yield ProjectDBCache(dirpath)


def _write_project(project, devices, storage):
    for childname in PROJECT_OBJECT_CATEGORIES:
        children = getattr(project, childname)
        for child in children:
            data = write_project_model(child)
            storage.store(child.uuid, child.revision, data)
    for dev in devices:
        data = write_project_model(dev)
        storage.store(dev.uuid, dev.revision, data)

    data = write_project_model(project)
    storage.store(project.uuid, project.revision, data)

# -----------------------------------------------------------------------------


def test_save_project():
    old_project = _get_old_project()
    project, devices = convert_old_project(old_project)

    with _project_storage() as storage:
        _write_project(project, devices, storage)


def test_project_round_trip():
    old_project = _get_old_project()
    project, devices = convert_old_project(old_project)

    with _project_storage() as storage:
        _write_project(project, devices, storage)
        rt_project = read_lazy_object(project.uuid, project.revision, storage,
                                      read_project_model)

    _compare_projects(project, rt_project)


def test_project_cache():
    old_project = _get_old_project()
    project, devices = convert_old_project(old_project)

    with _project_storage() as storage:
        _write_project(project, devices, storage)
        project_uuids = storage.get_uuids_of_type('project')
        assert len(project_uuids) == 1
        assert project_uuids[0] == project.uuid
