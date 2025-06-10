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
import os.path as op
from contextlib import contextmanager
from functools import partial
from io import StringIO
from tempfile import TemporaryDirectory
from unittest import TestCase
from uuid import uuid4

import pytest
from traits.api import Bool, Enum, Float, HasTraits, Int, Range, String

from karabo.common.api import set_modified_flag, walk_traits_object
from karabo.common.project.api import (
    PROJECT_DB_TYPE_PROJECT, BaseProjectObjectModel, DeviceConfigurationModel,
    DeviceInstanceModel, DeviceServerModel, MacroModel, MemCacheWrapper,
    ProjectDBCache, ProjectModel, read_lazy_object, recursive_save_object)
from karabo.common.scenemodel.api import (
    SceneModel, SceneWriterException, set_scene_reader)
from karabo.native import Hash, encodeXML

from ..api import (
    OldProject, convert_old_project, read_project_model, write_project_model)

TEST_DOMAIN = 'TESTES'


class _StorageWrapper:
    """A thin wrapper around storage objects which handles serialization of
    project objects.
    """

    def __init__(self, storage):
        self._storage = storage

    def store(self, domain, uuid, obj):
        data = write_project_model(obj)
        self._storage.store(domain, uuid, data)


def _compare_projects(proj0, proj1):
    COMPARABLE_TRAIT_TYPES = (Bool, Enum, Float, Int, Range, String)

    class _ObjectFlattener:
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


@contextmanager
def _lazy_reading():
    try:
        set_scene_reader(lazy=True)
        yield
    finally:
        set_scene_reader(lazy=False)


def _write_project(project, storage):
    set_modified_flag(project, value=True)  # Ensure everything gets saved
    wrapped = _StorageWrapper(storage)
    recursive_save_object(project, wrapped, TEST_DOMAIN)


# -----------------------------------------------------------------------------


class ProjectTest(TestCase):

    def test_simple_read(self):
        model = MacroModel(code='print(42)', initialized=True)
        xml = write_project_model(model)
        rmodel = read_project_model(StringIO(xml), existing=None)
        assert model.code == rmodel.code

    def test_invalid_read(self):
        with _project_storage() as storage:
            # Try to read something which is NOT THERE
            project = ProjectModel()
            obj = read_lazy_object(TEST_DOMAIN, project.uuid, storage,
                                   read_project_model, existing=project)
            assert obj is project
            assert not obj.initialized

    def test_reading_incomplete(self):
        from ..io import _wrap_child_element_xml

        # Create XML which looks like a project but lacks certain data
        hsh = Hash(PROJECT_DB_TYPE_PROJECT, Hash())
        xml = encodeXML(hsh)
        meta = {'item_type': PROJECT_DB_TYPE_PROJECT, 'uuid': str(uuid4())}
        xml = _wrap_child_element_xml(xml, meta)
        # Then make sure it can still be read
        model = read_project_model(StringIO(xml), existing=None)
        assert isinstance(model, ProjectModel)

    def test_save_project(self):
        old_project = _get_old_project()
        project = convert_old_project(old_project)

        with _project_storage() as storage:
            _write_project(project, storage)

    def test_project_convert(self):
        old_project = _get_old_project()
        project = convert_old_project(old_project)

        for server in project.servers:
            for dev_inst in server.devices:
                assert not dev_inst.instance_id.endswith('.xml')

    def test_project_round_trip(self):
        old_project = _get_old_project()
        project = convert_old_project(old_project)

        with _project_storage() as storage:
            _write_project(project, storage)
            rt_project = ProjectModel(uuid=project.uuid, is_trashed=True)
            rt_project = read_lazy_object(
                TEST_DOMAIN, project.uuid, storage,
                read_project_model, existing=rt_project)

        _compare_projects(project, rt_project)

    def test_project_cache(self):
        old_project = _get_old_project()
        project = convert_old_project(old_project)

        with _project_storage() as storage:
            _write_project(project, storage)
            proj_data = storage.get_available_project_data(
                TEST_DOMAIN, 'project')
            assert proj_data[0]['uuid'] == project.uuid
            assert proj_data[0]['simple_name'] == project.simple_name

    def test_inmemory_cache(self):
        cache_data = {}
        old_project = _get_old_project()
        project = convert_old_project(old_project)
        with _project_storage() as storage:
            memcache = MemCacheWrapper(cache_data, storage)
            _write_project(project, storage)
            _write_project(project, memcache)

            # remove one item (exercises more branches in MemCacheWrapper)
            del cache_data[TEST_DOMAIN][project.scenes[0].uuid]

            rt_project = ProjectModel(uuid=project.uuid)
            read_lazy_object(TEST_DOMAIN, project.uuid, memcache,
                             read_project_model, existing=rt_project)

        _compare_projects(project, rt_project)

    def test_empty_inmemory_cache(self):
        old_project = _get_old_project()
        project = convert_old_project(old_project)
        with _project_storage() as storage:
            _write_project(project, storage)
            memcache = MemCacheWrapper({}, storage)
            rt_project = ProjectModel(uuid=project.uuid)
            read_lazy_object(TEST_DOMAIN, project.uuid, memcache,
                             read_project_model, existing=rt_project)

        _compare_projects(project, rt_project)

    def test_uninitialized_save(self):
        model = ProjectModel()
        with self.assertRaises(AssertionError):
            write_project_model(model)

    def test_wrong_existing_type(self):
        model = MacroModel(code='print(42)', initialized=True)
        xml = write_project_model(model)
        with self.assertRaises(AssertionError):
            read_project_model(StringIO(xml), existing=ProjectModel())

    def test_existing_obj_mismatch(self):
        # `model` and `existing` have different UUIDs
        model = MacroModel(code='print(42)', initialized=True)
        existing = MacroModel()

        xml = write_project_model(model)
        with self.assertRaises(AssertionError):
            read_project_model(StringIO(xml), existing=existing)

    def test_existing_returns_existing(self):
        # DeviceConfigurationModel blows up without a class_id value...
        dcm_klass = partial(DeviceConfigurationModel, class_id='Tester')
        dcm_klass.__name__ = 'DeviceConfigurationModel'

        model_classes = (dcm_klass, DeviceInstanceModel, DeviceServerModel,
                         MacroModel, ProjectModel, SceneModel)
        for klass in model_classes:
            model = klass(initialized=True)
            xml = write_project_model(model)

            # If an object is passed with the `existing` keyword argument,
            # the same object must be returned to the caller
            existing = klass(uuid=model.uuid)
            read_model = read_project_model(StringIO(xml), existing=existing)
            msg = f'{klass.__name__} reader is broken!'
            assert read_model is existing, msg

    def test_modified_after_read(self):
        def _loaded_checker(model):
            if isinstance(model, BaseProjectObjectModel):
                assert model.initialized
                assert not model.modified

        old_project = _get_old_project()
        project = convert_old_project(old_project)

        with _project_storage() as storage:
            _write_project(project, storage)
            rt_project = ProjectModel(uuid=project.uuid)
            rt_project = read_lazy_object(
                TEST_DOMAIN, project.uuid, storage,
                read_project_model, existing=rt_project)

        # Make sure the following is true of freshly loaded projects:
        # - The `modified` flag is False on all objects
        # - The `initialized` flag is True
        walk_traits_object(rt_project, _loaded_checker)

    def test_project_lazy_scene(self):
        old_project = _get_old_project()
        project = convert_old_project(old_project)

        def visitor(obj):
            if isinstance(obj, SceneModel):
                obj.assure_svg_data()

        def _loaded_checker(model):
            if isinstance(model, BaseProjectObjectModel):
                assert model.initialized
                assert not model.modified

        with _lazy_reading():
            with _project_storage() as storage:
                _write_project(project, storage)
                rt_project = ProjectModel(uuid=project.uuid, is_trashed=True)
                rt_project = read_lazy_object(
                    TEST_DOMAIN, project.uuid, storage,
                    read_project_model, existing=rt_project)

            # all are initialized but not modified
            walk_traits_object(rt_project, _loaded_checker)

            # Check, we are not equal
            with pytest.raises(AssertionError):
                _compare_projects(project, rt_project)

            with _project_storage() as storage:
                with pytest.raises(SceneWriterException):
                    _write_project(rt_project, storage)

            # Write project will set all modified to `True`, here we set back
            set_modified_flag(rt_project, value=False)

            # build all scene children
            walk_traits_object(rt_project, visitor)
            walk_traits_object(rt_project, _loaded_checker)
            # Now we can compare and write
            _compare_projects(project, rt_project)

            with _project_storage() as storage:
                _write_project(rt_project, storage)
