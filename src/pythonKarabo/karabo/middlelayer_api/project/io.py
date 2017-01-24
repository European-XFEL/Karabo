import base64
from io import StringIO

from lxml import etree

from karabo.common.project.api import (
    PROJECT_DB_TYPE_DEVICE_INSTANCE, PROJECT_DB_TYPE_DEVICE_CONFIG,
    PROJECT_DB_TYPE_DEVICE_SERVER, PROJECT_DB_TYPE_MACRO,
    PROJECT_DB_TYPE_PROJECT, PROJECT_DB_TYPE_SCENE, PROJECT_OBJECT_CATEGORIES,
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    MacroModel, ProjectModel, read_device, read_device_server, write_device,
    write_device_server
)
from karabo.common.scenemodel.api import SceneModel, read_scene, write_scene
from karabo.middlelayer_api.hash import Hash

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
        'revision': int(metadata['revision']),
        'alias': metadata['alias'],
        'simple_name': metadata['simple_name'],
        'description': metadata['description'],
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

    if (existing.uuid != traits['uuid'] or
            existing.revision != traits['revision']):
        raise AssertionError('Object does not match one read from database!')

    existing.trait_set(**traits)
    return existing


def _device_reader(io_obj, existing, metadata):
    """ A reader for device configurations
    """
    traits = _db_metadata_reader(metadata)
    existing = _check_preexisting(existing, DeviceInstanceModel, traits)

    device = read_device(io_obj)
    device.trait_set(**traits)

    # Now copy into the existing object
    existing.class_id = device.class_id
    existing.instance_id = device.instance_id
    existing.if_exists = device.if_exists
    existing.configs[:] = device.configs[:]
    existing.active_config_ref = device.active_config_ref
    existing.initialized = True  # Do this last to avoid triggering `modified`

    return existing


def _device_config_reader(io_obj, existing, metadata):
    """ A reader for device configurations
    """
    traits = _db_metadata_reader(metadata)
    dev = _check_preexisting(existing, DeviceConfigurationModel, traits)

    hsh = Hash.decode(io_obj.read(), 'XML')
    for class_id, configuration in hsh.items():
        break

    dev.trait_set(class_id=class_id, configuration=configuration)
    dev.initialized = True  # Do this last to avoid triggering `modified`

    return dev


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
    macro = _check_preexisting(existing, MacroModel, traits)

    root = etree.parse(io_obj).getroot()
    code = root.text
    if code is not None:
        macro.trait_set(code=base64.b64decode(code).decode('utf-8'))
    macro.initialized = True  # Do this last to avoid triggering `modified`

    return macro


def _project_reader(io_obj, existing, metadata):
    """ A reader for projects
    """
    def _get_items(hsh, type_name):
        klass = _PROJECT_ITEM_TYPES[type_name]
        entries = hsh[type_name]
        return [klass(uuid=h['uuid'], revision=h['revision'],
                      initialized=False)
                for h in entries]

    traits = _db_metadata_reader(metadata)
    project = _check_preexisting(existing, ProjectModel, traits)

    hsh = Hash.decode(io_obj.read(), 'XML')
    project_hash = hsh[PROJECT_DB_TYPE_PROJECT]
    traits.update({k: _get_items(project_hash, k)
                   for k in PROJECT_OBJECT_CATEGORIES})
    project.trait_set(**traits)
    project.initialized = True  # Do this last to avoid triggering `modified`
    return project


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
    attrs['revision'] = str(model.revision)
    attrs['alias'] = model.alias
    attrs['simple_name'] = model.simple_name
    attrs['description'] = model.description
    return attrs


def _device_config_writer(model):
    """ A writer for device configurations
    """
    hsh = Hash(model.class_id, model.configuration)
    return hsh.encode('XML').decode()


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
        project[item_type] = [Hash('uuid', obj.uuid, 'revision', obj.revision)
                              for obj in objects]

    hsh = Hash(PROJECT_DB_TYPE_PROJECT, project)
    return hsh.encode('XML').decode()
