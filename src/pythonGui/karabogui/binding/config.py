# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from traits.api import TraitError, Undefined

from karabo.common import const
from karabo.native import (
    AccessMode, Assignment, Hash, Timestamp, has_changes, is_equal)

from .binding_types import (
    BindingNamespace, BindingRoot, NodeBinding, SlotBinding, VectorHashBinding)
from .proxy import PropertyProxy
from .validate import sanitize_table_value


def iterate_binding(node, base=""):
    """Flat iterate over the binding namespace to yield `key` and `binding`

    Note: Remove recursive binding iteration in future
    """
    namespace = node.value
    base = base + "." if base else ""
    for name in namespace:
        subname = base + name
        subnode = getattr(namespace, name)
        if isinstance(subnode, NodeBinding):
            yield from iterate_binding(subnode, base=subname)
        elif not isinstance(subnode, SlotBinding):
            yield subname, subnode


def apply_fast_data(config, binding, timestamp):
    """Recursively set values from a fast data Hash object to a binding
    object.
    """
    namespace = binding.value
    assert isinstance(namespace, BindingNamespace)

    for key, value, _ in config.iterall():
        if key not in namespace:
            continue

        node = getattr(namespace, key)
        if isinstance(value, Hash) and isinstance(node, NodeBinding):
            apply_fast_data(value, node, timestamp)
        else:
            traits = {"value": value, "timestamp": timestamp}
            # Set everything at once and notify via the config_update event
            try:
                node.trait_set(trait_change_notify=False, **traits)
            except TraitError:
                # value in the configuration is not compatible to schema
                continue
            node.config_update = True

    binding.config_update = True


def apply_configuration(config, binding):
    """Recursively set values from a configuration Hash object to a binding
    object.

    This method sets an `online` (network) configuration Hash to a proxy
    binding and thus always notifies and does not include attributes!
    """
    namespace = binding.value
    assert isinstance(namespace, BindingNamespace)

    for key, value, attrs in config.iterall():
        if key not in namespace:
            continue

        node = getattr(namespace, key)
        if isinstance(value, Hash) and isinstance(node, NodeBinding):
            apply_configuration(value, node)
        else:
            # Set the timestamp no matter what
            ts = Timestamp.fromHashAttributes(attrs) or Timestamp()
            traits = {"value": value, "timestamp": ts}
            # Set everything at once and notify via the config_update event
            try:
                node.trait_set(trait_change_notify=False, **traits)
            except TraitError:
                # value in the configuration is not compatible to schema
                continue
            node.config_update = True

    # Notify listeners
    binding.config_update = True


def apply_default_configuration(binding):
    """Recursively set default values for a binding object.
    """
    assert isinstance(binding, BindingRoot)

    def _iter_binding(node):
        namespace = node.value
        for name in namespace:
            subnode = getattr(namespace, name)
            if isinstance(subnode, NodeBinding):
                yield from _iter_binding(subnode)
            else:
                yield subnode

    for node in _iter_binding(binding):
        default_value = node.attributes.get(const.KARABO_SCHEMA_DEFAULT_VALUE)
        if default_value is not None:
            node.value = default_value
        else:
            node.value = Undefined


def apply_project_configuration(config, binding, base=""):
    """Recursively set values from a configuration Hash object to a binding
    object of a project device.

    A project configuration consists of `values` and `attributes` and is
    in certain cases sanitized if necessary (table element).

    Setting a project configuration sets the value directly and does not
    notify!

    :Returns: failure `dict` of keys and error messages
    """
    namespace = binding.value
    assert isinstance(namespace, BindingNamespace)

    fails = Hash()
    base = base + "." if base else ""

    for key, value, attrs in config.iterall():
        if key not in namespace:
            continue

        # For storage in fail dict!
        subkey = base + key
        node = getattr(namespace, key)
        if isinstance(value, Hash) and isinstance(node, NodeBinding):
            fails.update(apply_project_configuration(value, node, base=subkey))
        else:
            if isinstance(node, VectorHashBinding):
                success, value = sanitize_table_value(node, value)
                if not success:
                    fails.update(
                        {subkey: "The table configuration was "
                                 "sanitized - <b>The table is corrupted</b>"})
            traits = {"value": value}
            # Set the timestamp no matter what
            ts = Timestamp.fromHashAttributes(attrs)
            traits["timestamp"] = ts or Timestamp()
            try:
                node.trait_set(trait_change_notify=False, **traits)
            except TraitError:
                # value in the configuration is not compatible to schema
                fails.update({subkey: f"Setting value of {value} failed"})

    return fails


def extract_configuration(binding):
    """Extract all the values set on a binding into a Hash object.
    """
    assert isinstance(binding, BindingRoot)

    def _get_binding_value(binding):
        return binding.value

    retval = Hash()
    for key, node in iterate_binding(binding):
        value = _get_binding_value(node)
        if value is Undefined:
            continue
        retval[key] = _get_binding_value(node)

    return retval


def extract_edits(schema, binding):
    """Extract all user edited (non default) values on a binding into a
    Hash object.
    """
    assert isinstance(binding, BindingRoot)

    def _get_binding_default(binding):
        val = binding.attributes.get(const.KARABO_SCHEMA_DEFAULT_VALUE)
        return val

    def _is_readonly(key):
        return schema.hash[key, "accessMode"] == AccessMode.READONLY.value

    retval = Hash()
    for key, node in iterate_binding(binding):
        default_val = _get_binding_default(node)
        value = None if node.value is Undefined else node.value

        has_val_changes = not is_equal(default_val, value)
        is_readonly = _is_readonly(key)
        if not has_val_changes or is_readonly:
            continue
        retval[key] = value

    return retval


def extract_online_edits(schema, binding):
    """Extract all user edited (non default) online values on a binding into a
    Hash object.

    This function takes a class schema as reference to extract the online
    edits and compares them to the default of the schema.
    Only values are considered which are different from the default.

    :param schema: The class schema
    :param binding: The binding of the online device

    :returns success: Boolean to indicate if we have a successful online to
                      offline transition, e.g. no DeviceNodes, ChoiceOfNodes,
                      ListOfNodes.

    """
    assert isinstance(binding, BindingRoot)

    # Backward compatibility protection for difficult keys. Reconsider
    # in Karabo 2.13 FIXME
    IGNORE_KEYS = ["_deviceId_", "_serverId_", "hostName"]
    IGNORE_NODES = ["Logger."]

    def _get_binding_default(binding):
        value = binding.attributes.get(const.KARABO_SCHEMA_DEFAULT_VALUE)
        return value

    def _iter_binding(node, base=""):
        namespace = node.value
        base = base + "." if base else ""
        for name in namespace:
            if base in IGNORE_NODES:
                continue

            subname = base + name
            subnode = getattr(namespace, name)
            if isinstance(subnode, NodeBinding):
                yield from _iter_binding(subnode, base=subname)
            elif not isinstance(subnode, SlotBinding):
                # All rest binding types
                yield subname, subnode

    def not_writable(key):
        """Check if a property is writable from external"""
        attributes = schema.hash[key, ...]
        read_only = attributes["accessMode"] == AccessMode.READONLY.value
        internal = attributes["assignment"] == Assignment.INTERNAL.value
        # Note: One can define tags such as `plc` here to filter out.
        return read_only or internal

    config = Hash()
    success = True
    for key, node in _iter_binding(binding):
        # Injected properties are not considered! Blacklisted keys are
        # filtered out as well!
        if key not in schema.hash or key in IGNORE_KEYS:
            continue

        value = node.value
        if node.displayType == "deviceNode":
            # DeviceNode may not be successful, until ~2.15 or forever ...
            # Future versions of the device node always have a value
            if not value or value is Undefined:
                success = False
                continue

        if value is Undefined:
            # Note: This did happen for output channels if they don't have
            # clients! (C++ / Bound). Fixed now ...
            # Other examples?
            continue

        # We take the online defaults to account schema injection. If a default
        # value from the online device is_equal, it should not go into the
        # configuration
        default_val = _get_binding_default(node)
        if not has_changes(default_val, value) or not_writable(key):
            continue
        config[key] = value

    return success, config


def extract_sparse_configurations(proxies, devices=None):
    """Extract values set on the bindings of a list of `PropertyProxy`
    instances into a dictionary of `Hash` objects (one per device).

    If desired, an existing dictionary object can be provided so that a
    configuration can be iteratively accumulated.
    """
    assert all(isinstance(p, PropertyProxy) for p in proxies)

    devices = {} if devices is None else devices
    for proxy in proxies:
        key, binding = proxy.path, proxy.binding
        if binding is None or proxy.edit_value is None:
            continue

        device_id = proxy.root_proxy.device_id
        hsh = devices.setdefault(device_id, Hash())
        hsh[key] = proxy.edit_value

    return devices


def extract_init_configuration(binding, config):
    """Extract an init configuration from `config` with `binding`

    This function will filter out read only properties without runtime
    attributes from the configurations
    """

    ret = Hash()

    for key, node in iterate_binding(binding):
        # key must be in configuration for processing!
        if key in config:
            value, attrs = config.getElement(key)
            # We only want reconfigurable properties, but in Karabo 2.11
            # read only parameter can still have attributes.
            read_only = node.accessMode is AccessMode.READONLY
            is_internal = node.assignment is Assignment.INTERNAL
            if read_only or is_internal:
                continue
            default = node.attributes.get(const.KARABO_SCHEMA_DEFAULT_VALUE)
            if is_equal(default, value):
                continue
            # We make use of and also filter the runtime attributes
            ret[key] = value

    return ret


def get_config_changes(old, new, project):
    """Extract the changes of Hash `new` with respect to Hash `old`

    :param project: Additional parameter denoting if this comparison is
                    happening for a project device proxy
    """
    changes_old, changes_new = Hash(), Hash()

    old_paths = old.paths(intermediate=False)
    new_paths = new.paths(intermediate=False)

    keys = sorted(set(old_paths).union(set(new_paths)))
    for key in keys:
        old_value = old.get(key, None)
        new_value = new.get(key, None)
        if isinstance(old_value, (bytes, bytearray)):
            continue
        if old_value is None and new_value is not None:
            if not project:
                changes_old[key] = "Missing from configuration"
            changes_new[key] = new_value
        elif old_value is not None and new_value is None:
            changes_old[key] = old_value
            changes_new[key] = "Removed from configuration"
        elif has_changes(old_value, new_value):
            changes_old[key] = old_value
            changes_new[key] = new_value

    return changes_old, changes_new
