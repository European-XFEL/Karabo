from contextlib import contextmanager
from io import BytesIO
import os.path as op
from tempfile import TemporaryDirectory

from traits.api import Dict, Enum, Int, String

from karabo.common.project.api import ProjectModel, PROJECT_OBJECT_CATEGORIES
from karabo.middlelayer import Project
import karabo.testing as testing_mod
from ..convert import convert_old_project
from ..io import read_project_model, write_project_model

TEST_PROJECT_PATH = op.join(op.dirname(testing_mod.__file__), 'resources',
                            'reference.krb')


class _ProjectStorage(object):
    """ An object to stand in for the project database
    """
    def __init__(self, dirpath):
        self.dirpath = dirpath

    def store(self, name, data):
        with open(op.join(self.dirpath, name), mode='wb') as fp:
            fp.write(data)

    def retrieve(self, name):
        with open(op.join(self.dirpath, name), mode='rb') as fp:
            return fp.read()


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
    assert proj0.version == proj1.version
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
    old_project = Project(TEST_PROJECT_PATH)
    old_project.unzip()
    return old_project


@contextmanager
def _project_storage():
    with TemporaryDirectory() as dirpath:
        yield _ProjectStorage(dirpath)


def _read_project(storage, name):
    data = storage.retrieve(name)
    lazy_project = read_project_model(BytesIO(data))
    project = ProjectModel(uuid=lazy_project.uuid,
                           version=lazy_project.version)

    for childname in PROJECT_OBJECT_CATEGORIES:
        lazy_children = getattr(lazy_project, childname)
        industrious_children = []
        for child in lazy_children:
            name = child.uuid
            data = storage.retrieve(name)
            industrious_children.append(read_project_model(BytesIO(data)))
        setattr(project, childname, industrious_children)

    return project


def _write_project(project, storage):
    for childname in PROJECT_OBJECT_CATEGORIES:
        children = getattr(project, childname)
        for child in children:
            name = child.uuid
            storage.store(name, write_project_model(child))

    name = project.uuid
    storage.store(name, write_project_model(project))

# -----------------------------------------------------------------------------


def test_save_project():
    old_project = _get_old_project()
    project = convert_old_project(old_project)

    with _project_storage() as storage:
        _write_project(project, storage)


def test_project_round_trip():
    old_project = _get_old_project()
    project = convert_old_project(old_project)

    with _project_storage() as storage:
        _write_project(project, storage)
        name = project.uuid
        rt_project = _read_project(storage, name)

    _compare_projects(project, rt_project)
