#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 21, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

PROJECT_DB_TYPE_DEVICE = 'device_config'
PROJECT_DB_TYPE_DEVICE_SERVER = 'device_server'
PROJECT_DB_TYPE_MACRO = 'macro'
PROJECT_DB_TYPE_PROJECT = 'project'
PROJECT_DB_TYPE_SCENE = 'scene'
PROJECT_OBJECT_CATEGORIES = (
    'macros', 'scenes', 'servers', 'subprojects'
)

NS_EXISTDB_VERSIONING = "{http://exist-db.org/versioning}"
EXISTDB_INITIAL_VERSION = 0