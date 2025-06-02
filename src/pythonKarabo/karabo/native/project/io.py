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
import base64
from io import StringIO
from xml.sax.saxutils import escape

from lxml import etree

from karabo.common.project.api import (
    PROJECT_DB_TYPE_DEVICE_CONFIG, PROJECT_DB_TYPE_DEVICE_INSTANCE,
    PROJECT_DB_TYPE_DEVICE_SERVER, PROJECT_DB_TYPE_MACRO,
    PROJECT_DB_TYPE_PROJECT, PROJECT_DB_TYPE_SCENE, PROJECT_OBJECT_CATEGORIES,
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    MacroModel, ProjectModel, read_device, read_device_server, write_device,
    write_device_server)
from karabo.common.scenemodel.api import SceneModel, read_scene, write_scene
from karabo.native.data import Hash, decodeXML, encodeXML

_ITEM_TYPES = {
    DeviceConfigurationModel: PROJECT_DB_TYPE_DEVICE_CONFIG,
    DeviceInstanceModel: PROJECT_DB_TYPE_DEVICE_INSTANCE,
    DeviceServerModel: PROJECT_DB_TYPE_DEVICE_SERVER,
    MacroModel: PROJECT_DB_TYPE_MACRO,
    ProjectModel: PROJECT_DB_TYPE_PROJECT,
    SceneModel: PROJECT_DB_TYPE_SCENE
}
_PROJECT_ITEM_TYPES = {
    'servers': DeviceServerModel,
    'macros': MacroModel,
    'subprojects': ProjectModel,
    'scenes': SceneModel,
}
# Check it
assert len(_PROJECT_ITEM_TYPES) == len(PROJECT_OBJECT_CATEGORIES)


def get_item_type(obj):
    """Return the item type for a project model"""
    return _ITEM_TYPES.get(type(obj), 'unknown')


def read_project_model(io_obj, existing=None):
    """ Deserialize a project model object.

    :param io_obj: A file-like object that can be read from
    :param existing: Optional preexisting object which should be filled by
                     the reader.
    :return: A project data model object (device config, scene, macro, etc.)
    """
    factories = {
        PROJECT_DB_TYPE_DEVICE_INSTANCE: _device_reader,
        PROJECT_DB_TYPE_DEVICE_CONFIG: _device_config_reader,
        PROJECT_DB_TYPE_DEVICE_SERVER: _device_server_reader,
        PROJECT_DB_TYPE_MACRO: _macro_reader,
        PROJECT_DB_TYPE_PROJECT: _project_reader,
        PROJECT_DB_TYPE_SCENE: _scene_reader,
    }
    # Unwrap the model's XML into parent and child elements
    parent, child = _unwrap_child_element_xml(io_obj.read())

    # Grab the metadata from the parent
    parent = etree.parse(StringIO(parent)).getroot()
    metadata = dict(parent.items())
    item_type = metadata.get('item_type')
    factory = factories.get(item_type)

    # Construct from the child data + metadata
    return factory(StringIO(child), existing, metadata)


def write_project_model(model):
    """ Serialize a project model object.

    :param model: A project data model object (project, scene, macro, etc.)
    :return: An XML bytestring representation of the object
    """
    writers = {
        PROJECT_DB_TYPE_DEVICE_INSTANCE: write_device,
        PROJECT_DB_TYPE_DEVICE_CONFIG: _device_config_writer,
        PROJECT_DB_TYPE_DEVICE_SERVER: write_device_server,
        PROJECT_DB_TYPE_MACRO: _macro_writer,
        PROJECT_DB_TYPE_PROJECT: _project_writer,
        PROJECT_DB_TYPE_SCENE: write_scene,
    }

    if not model.initialized:
        msg = "Attempted to write a '{}' object which is uninitialized!"
        raise AssertionError(msg.format(type(model).__name__))

    item_type = _ITEM_TYPES.get(type(model))
    writer = writers.get(item_type)
    child_xml = writer(model)

    root_metadata = _model_db_metadata(model)
    root_metadata['item_type'] = item_type
    return _wrap_child_element_xml(child_xml, root_metadata)

# -----------------------------------------------------------------------------


def _unwrap_child_element_xml(xml):
    """ Unwrap a blob of XML into two independent documents
    """
    start_index = xml.find('>') + 1
    end_index = xml.rfind('</xml>')
    parent = xml[:start_index] + xml[end_index:]
    child = xml[start_index:end_index]
    return parent, child


def _wrap_child_element_xml(child_xml, root_metadata):
    """ Insert a complex XML document into a much simpler document consisting
    of a single root element with included attributes.
    """
    element = etree.Element('xml', **root_metadata)
    element.text = ''  # This guarantees a closing tag (</xml>)
    root_xml = etree.tostring(element, encoding='unicode')

    index = root_xml.rfind('</xml>')
    return root_xml[:index] + child_xml + root_xml[index:]

# -----------------------------------------------------------------------------


def _db_metadata_reader(metadata):
    """ Read the traits which are common to all BaseProjectObjectModel objects
    """
    attrs = {
        'uuid': metadata['uuid'],
        'simple_name': metadata.get('simple_name', ''),
        'date': metadata.get('date', ''),
    }
    return attrs


def _check_preexisting(existing, klass, traits):
    """ Make sure a preexisting object is in order.
    """
    if existing is None:
        return klass(**traits)

    if not isinstance(existing, klass):
        msg = ('Incorrect object passed to reader!\n'
               'Expected: {}, Got: {}').format(klass, type(existing))
        raise AssertionError(msg)

    if existing.uuid != traits['uuid']:
        raise AssertionError('Object does not match one read from database!')

    existing.trait_set(**traits)
    return existing


def _device_reader(io_obj, existing, metadata):
    """ A reader for device instances
    """
    traits = _db_metadata_reader(metadata)
    existing = _check_preexisting(existing, DeviceInstanceModel, traits)

    device = read_device(io_obj)
    device.trait_set(**traits)

    # Now copy into the existing object
    existing.class_id = device.class_id
    existing.instance_id = device.instance_id
    existing.configs[:] = device.configs[:]
    existing.active_config_ref = device.active_config_ref

    # NOTE: ``inititalized`` flag is explicitly NOT set to ``True`` here
    # because a ``DeviceInstanceModel`` is ONLY ``initialized`` whenever ALL
    # ``DeviceConfigurationModel``s are ``initialized``

    return existing


def _device_config_reader(io_obj, existing, metadata):
    """ A reader for device configurations
    """
    traits = _db_metadata_reader(metadata)
    existing = _check_preexisting(existing, DeviceConfigurationModel, traits)

    hsh = decodeXML(io_obj.read())
    for class_id, configuration in hsh.items():
        break

    existing.trait_set(class_id=class_id, configuration=configuration)
    existing.initialized = True  # Do this last to avoid triggering `modified`

    return existing


def _device_server_reader(io_obj, existing, metadata):
    """ A reader for device server models
    """
    traits = _db_metadata_reader(metadata)
    existing = _check_preexisting(existing, DeviceServerModel, traits)

    server = read_device_server(io_obj)
    server.trait_set(**traits)

    # Now copy into the existing object
    existing.server_id = server.server_id
    existing.host = server.host
    existing.devices[:] = server.devices[:]
    existing.initialized = True  # Do this last to avoid triggering `modified`

    return existing


def _macro_reader(io_obj, existing, metadata):
    """ A reader for macros
    """
    traits = _db_metadata_reader(metadata)
    existing = _check_preexisting(existing, MacroModel, traits)

    root = etree.parse(io_obj).getroot()
    code = root.text
    if code is not None:
        existing.trait_set(code=base64.b64decode(code).decode('utf-8'))
    existing.initialized = True  # Do this last to avoid triggering `modified`

    return existing


def _project_reader(io_obj, existing, metadata):
    """ A reader for projects
    """
    def _get_items(hsh, type_name):
        klass = _PROJECT_ITEM_TYPES[type_name]
        entries = hsh.get(type_name, [])
        return [klass(uuid=h['uuid'], initialized=False) for h in entries]

    traits = _db_metadata_reader(metadata)
    traits['is_trashed'] = (metadata.get('is_trashed') == 'true')
    existing = _check_preexisting(existing, ProjectModel, traits)

    hsh = decodeXML(io_obj.read())
    project_hash = hsh[PROJECT_DB_TYPE_PROJECT]
    traits.update({k: _get_items(project_hash, k)
                   for k in PROJECT_OBJECT_CATEGORIES})
    existing.trait_set(**traits)
    existing.initialized = True  # Do this last to avoid triggering `modified`
    return existing


def _scene_reader(io_obj, existing, metadata):
    """ A reader for scenes
    """
    traits = _db_metadata_reader(metadata)
    existing = _check_preexisting(existing, SceneModel, traits)

    scene = read_scene(io_obj)
    scene.trait_set(**traits)

    # Then copy into the existing
    existing.file_format_version = scene.file_format_version
    existing.extra_attributes = scene.extra_attributes.copy()
    existing.width = scene.width
    existing.height = scene.height
    existing.children[:] = scene.children[:]
    existing.initialized = True  # Do this last to avoid triggering `modified`

    return existing

# -----------------------------------------------------------------------------


def _model_db_metadata(model):
    """ Extract attributes which are stored in the root of a project DB object
    """
    attrs = {}
    attrs['uuid'] = model.uuid
    attrs['simple_name'] = escape(model.simple_name)
    attrs['date'] = model.date

    if isinstance(model, ProjectModel):
        # This attribute currently only applies to the project model
        attrs['is_trashed'] = str(model.is_trashed).lower()
    return attrs


def _device_config_writer(model):
    """ A writer for device configurations
    """
    hsh = Hash(model.class_id, model.configuration)
    return encodeXML(hsh)


def _macro_writer(model):
    """ A writer for macros
    """
    element = etree.Element(PROJECT_DB_TYPE_MACRO)
    code = model.code.encode('utf-8')
    element.text = base64.b64encode(code)
    return etree.tostring(element, encoding='unicode')


def _project_writer(model):
    """ A writer for projects
    """
    project = Hash()
    for item_type in PROJECT_OBJECT_CATEGORIES:
        objects = getattr(model, item_type)
        # XXX: Keep adding 'revision' to protect code that people don't upgrade
        project[item_type] = [Hash('uuid', obj.uuid, 'revision', 0)
                              for obj in objects]

    hsh = Hash(PROJECT_DB_TYPE_PROJECT, project)
    return encodeXML(hsh)
