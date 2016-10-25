import base64
from io import BytesIO

from lxml import etree

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceGroupModel, DeviceServerModel,
    LazyDeviceGroupModel, LazyDeviceServerModel, LazyProjectModel,
    MacroModel, MonitorModel, ProjectModel, ProjectObjectReference,
    PROJECT_DB_TYPE_DEVICE, PROJECT_DB_TYPE_DEVICE_GROUP,
    PROJECT_DB_TYPE_DEVICE_SERVER, PROJECT_DB_TYPE_MACRO,
    PROJECT_DB_TYPE_MONITOR, PROJECT_DB_TYPE_PROJECT, PROJECT_DB_TYPE_SCENE,
    PROJECT_OBJECT_CATEGORIES)
from karabo.common.scenemodel.api import SceneModel, read_scene, write_scene
from karabo.middlelayer_api.hash import Hash


def read_project_model(io_obj):
    """ Given a file-like object ``io_obj`` containing XML data, read a project
    model object.
    """
    factories = {
        PROJECT_DB_TYPE_DEVICE: _device_reader,
        PROJECT_DB_TYPE_DEVICE_GROUP: _device_group_reader,
        PROJECT_DB_TYPE_DEVICE_SERVER: _device_server_reader,
        PROJECT_DB_TYPE_MACRO: _macro_reader,
        PROJECT_DB_TYPE_MONITOR: _monitor_reader,
        PROJECT_DB_TYPE_PROJECT: _project_reader,
        PROJECT_DB_TYPE_SCENE: _scene_reader,
    }
    # Unwrap the model's XML into parent and child elements
    parent, child = _unwrap_child_element_xml(io_obj.read())

    # Grab the metadata from the parent
    parent = etree.parse(BytesIO(parent)).getroot()
    metadata = dict(parent.items())
    typename = metadata.get('type')
    factory = factories.get(typename)

    # Construct from the child data + metadata
    return factory(BytesIO(child), metadata)


def write_project_model(model):
    """ Given an object based on BaseProjectObjectModel, return an XML
    bytestring containing the serialized object.
    """
    typemap = {
        DeviceConfigurationModel: PROJECT_DB_TYPE_DEVICE,
        DeviceGroupModel: PROJECT_DB_TYPE_DEVICE_GROUP,
        DeviceServerModel: PROJECT_DB_TYPE_DEVICE_SERVER,
        MacroModel: PROJECT_DB_TYPE_MACRO,
        MonitorModel: PROJECT_DB_TYPE_MONITOR,
        ProjectModel: PROJECT_DB_TYPE_PROJECT,
        SceneModel: PROJECT_DB_TYPE_SCENE
    }
    writers = {
        PROJECT_DB_TYPE_DEVICE: _device_writer,
        PROJECT_DB_TYPE_DEVICE_GROUP: _device_group_writer,
        PROJECT_DB_TYPE_DEVICE_SERVER: _device_server_writer,
        PROJECT_DB_TYPE_MACRO: _macro_writer,
        PROJECT_DB_TYPE_MONITOR: _monitor_writer,
        PROJECT_DB_TYPE_PROJECT: _project_writer,
        PROJECT_DB_TYPE_SCENE: write_scene,
    }
    typename = typemap.get(model.__class__)
    writer = writers.get(typename)
    child_xml = writer(model)

    root_metadata = _model_db_metadata(model)
    root_metadata['type'] = typename
    return _wrap_child_element_xml(child_xml, root_metadata)

# -----------------------------------------------------------------------------


def _unwrap_child_element_xml(xml):
    """ Unwrap a blob of XML bytes into two independent documents
    """
    start_index = xml.find(b'>') + 1
    end_index = xml.rfind(b'</xml>')
    parent = xml[:start_index] + xml[end_index:]
    child = xml[start_index:end_index]
    return parent, child


def _wrap_child_element_xml(child_xml, root_metadata):
    """ Insert a complex XML document into a much simpler document consisting
    of a single root element with included attributes.
    """
    element = etree.Element('xml', **root_metadata)
    element.text = ''  # This guarantees a closing tag (</xml>)
    root_xml = etree.tostring(element)

    index = root_xml.rfind(b'</xml>')
    return root_xml[:index] + child_xml + root_xml[index:]

# -----------------------------------------------------------------------------


def _db_metadata_reader(metadata):
    """ Read the traits which are common to all BaseProjectObjectModel objects
    """
    attrs = {
        'revision': int(metadata['revision']),
        'uuid': metadata['uuid'],
        'db_attrs': {k: metadata.get(k, '') for k in ('key', 'path')},
    }
    return attrs


def _device_reader(io_obj, metadata):
    """ A reader for device configurations
    """
    kwargs = _db_metadata_reader(metadata)
    hsh = Hash.decode(io_obj.read(), 'XML')
    for class_id, configuration, attributes in hsh.iterall():
        kwargs.update({k: attributes[k]
                       for k in ('server_id', 'instance_id', 'if_exists')})
        return DeviceConfigurationModel(
            class_id=class_id, configuration=configuration, **kwargs)


def _device_group_reader(io_obj, metadata):
    """ A reader for device group models
    """
    kwargs = _db_metadata_reader(metadata)
    group = Hash.decode(io_obj.read(), 'XML')
    devices = [_reference_reader(h) for h in group.values()]
    return LazyDeviceGroupModel(devices=devices, **kwargs)


def _device_server_reader(io_obj, metadata):
    """ A reader for device server models
    """
    # XXX: What is in a device server XML file?
    kwargs = _db_metadata_reader(metadata)
    server = Hash.decode(io_obj.read(), 'XML')
    devices = [_reference_reader(h) for h in server.values()]
    return LazyDeviceServerModel(devices=devices, **kwargs)


def _macro_reader(io_obj, metadata):
    """ A reader for macros
    """
    kwargs = _db_metadata_reader(metadata)
    root = etree.parse(io_obj).getroot()
    code = base64.b64decode(root.text).decode('utf-8')
    return MacroModel(code=code, **kwargs)


def _monitor_reader(io_obj, metadata):
    """ A reader for monitors
    """
    kwargs = _db_metadata_reader(metadata)
    configuration = Hash.decode(io_obj.read(), 'XML')
    return MonitorModel(configuration=configuration, **kwargs)


def _project_reader(io_obj, metadata):
    """ A reader for projects
    """
    def _get_references(hsh, key):
        return [_reference_reader(h) for h in hsh[key]]

    kwargs = _db_metadata_reader(metadata)
    hsh = Hash.decode(io_obj.read(), 'XML')
    project = hsh['project']
    kwargs.update({k: _get_references(project, k)
                   for k in PROJECT_OBJECT_CATEGORIES})
    return LazyProjectModel(**kwargs)


def _reference_reader(hsh):
    """ Given a Hash object containing an object reference, return
    a new ProjectObjectReference object.
    """
    return ProjectObjectReference(uuid=hsh['uuid'], revision=hsh['revision'])


def _scene_reader(io_obj, metadata):
    """ A reader for scenes
    """
    traits = _db_metadata_reader(metadata)
    scene = read_scene(io_obj)
    scene.trait_set(**traits)
    return scene

# -----------------------------------------------------------------------------


def _model_db_metadata(model):
    """ Extract attributes which are stored in the root of a project DB object
    """
    attrs = model.db_attrs.copy()
    attrs['uuid'] = model.uuid
    attrs['revision'] = str(model.revision)
    return attrs


def _device_writer(model):
    """ A writer for device configurations
    """
    hsh = Hash(model.class_id, model.configuration)
    for name in ('server_id', 'instance_id', 'if_exists'):
        hsh[model.class_id, name] = getattr(model, name)
    return hsh.encode('XML')


def _device_group_writer(model):
    """ A writer for device group models
    """
    devices = [Hash('uuid', dev.uuid, 'revision', dev.revision)
               for dev in model.devices]
    hsh = Hash('group', devices)
    return hsh.encode('XML')


def _device_server_writer(model):
    """ A writer for device server models
    """
    devices = [Hash('uuid', dev.uuid, 'revision', dev.revision)
               for dev in model.devices]
    hsh = Hash('server', devices)
    return hsh.encode('XML')


def _macro_writer(model):
    """ A writer for macros
    """
    element = etree.Element('macro')
    code = model.code.encode('utf-8')
    element.text = base64.b64encode(code)
    return etree.tostring(element)


def _monitor_writer(model):
    """ A writer for monitors
    """
    return model.configuration.encode('XML')


def _project_writer(model):
    """ A writer for projects
    """
    project = Hash()
    for typename in PROJECT_OBJECT_CATEGORIES:
        objects = getattr(model, typename)
        project[typename] = [Hash('uuid', obj.uuid, 'revision', obj.revision)
                             for obj in objects]

    hsh = Hash('project', project)
    return hsh.encode('XML')
