#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 7, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which defines a certain number of constants for
   the application.
"""


class _const:
    class ConstError(TypeError): pass
    def __setattr__(self,name,value):
        if self.__dict__.has_key(name):
            raise self.ConstError, "Can't rebind const(%s)"%name
        self.__dict__[name]=value
        
import sys
sys.modules[__name__]=_const()


import const
from PyQt4.QtCore import *

# Parameter item properties
const.INTERNAL_KEY           = Qt.UserRole
const.VALUE_TYPE             = Qt.UserRole + 1
const.DEFAULT_VALUE          = Qt.UserRole + 2
const.CURRENT_INSTANCE_VALUE = Qt.UserRole + 3
const.CURRENT_EDITABLE_VALUE = Qt.UserRole + 4
const.ALIAS                  = Qt.UserRole + 5
const.TAGS                   = Qt.UserRole + 6
const.DESCRIPTION            = Qt.UserRole + 7
const.TIMESTAMP              = Qt.UserRole + 8
const.REQUIRED_ACCESS_LEVEL  = Qt.UserRole + 9
const.IS_CHOICE_ELEMENT      = Qt.UserRole + 10
const.IS_LIST_ELEMENT        = Qt.UserRole + 11
const.UPDATE_NEEDED          = Qt.UserRole + 12
const.ACCESS_TYPE            = Qt.UserRole + 13
const.CLASS_ALIAS            = Qt.UserRole + 14
const.ALLOWED_STATE          = Qt.UserRole + 15
const.UNIT_SYMBOL            = Qt.UserRole + 16
const.METRIC_PREFIX_SYMBOL   = Qt.UserRole + 17
const.ENUMERATION            = Qt.UserRole + 18

