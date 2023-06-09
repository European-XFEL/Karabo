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
from abc import abstractmethod

import pkg_resources
from lxml import etree

from karabo.native import (
    AccessLevel, AccessMode, Bool, Configurable, Node, String)

DEFAULT_PROTOCOL = "exist_db"


class ProjectDBError(Exception):
    pass


class DbConnectionNodeBase(Configurable):
    @abstractmethod
    def get_db(self, test_mode=False, init_db=False):
        """creates an object subclass of ``DatabaseBase``

        To Be implemented in the subclass

        :param test_mode: Defines if the DB Connection is used during a test.
                          Behaviour is implementation-specific.
        :param init_db: True if database should be initialized
        :return: an object subclass of ``DatabaseBase``
        """
        raise NotImplementedError


def get_node():
    entrypoints = list(pkg_resources.iter_entry_points("karabo.project_db"))
    protocols = [ep.name for ep in entrypoints]
    assert DEFAULT_PROTOCOL in protocols, ",".join(protocols)

    class ProjectNode(Configurable):
        _plugins = {}

        protocol = String(
            defaultValue=DEFAULT_PROTOCOL,
            options=protocols,
            accessMode=AccessMode.INITONLY)

        testMode = Bool(
            displayedName="Test Mode",
            defaultValue=False,
            requiredAccessLevel=AccessLevel.ADMIN)

        for ep in entrypoints:
            node_key = ep.name
            node_cls = ep.load()
            assert issubclass(node_cls, DbConnectionNodeBase)
            _plugins[node_key] = node_cls
            locals()[node_key] = Node(node_cls)

    return Node(ProjectNode)


def get_project(node, init_db=False):
    """creates an object subclass of ``DatabaseBase``

    :param node: the instance of ``Node`` object implementing the
                 ``ProjectNode`` class.
    :param init_db: True if database should be initialized.
    :return: an object subclass of ``DatabaseBase``
    """
    protocol = node.protocol.value
    ep_node = getattr(node, protocol)
    return ep_node.get_db(test_mode=node.testMode.value, init_db=init_db)


def to_string(xml_rep):
    """returns a string serialization of an xml element

    the output is listed on multiple lines and unicode encoded"""
    return etree.tostring(
        xml_rep,
        pretty_print=True,
        encoding="unicode",
        xml_declaration=False)
