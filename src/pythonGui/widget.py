#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a factory class to create
   display widgets.
"""

__all__ = ["DisplayWidget"]


from PyQt4.QtCore import QObject, pyqtSignal

class MetaWidget(QObject.__class__):
    def __new__(self, name, bases, dict):
        ret = QObject.__class__.__new__(self, name, bases, dict)
        if "category" not in dict:
            ret.categoryToAliases = { } # <category, [alias1,alias2,..]>
            ret.aliasToCategory = { } # <alias, category>
            ret.aliasConcreteClass = { } # dict of actual classes
            return ret
        if ret.category in ret.categoryToAliases:
            ret.categoryToAliases[ret.category].append(ret.alias)
        else:
            ret.categoryToAliases[ret.category] = [ret.alias]
        ret.aliasToCategory[ret.alias] = ret.category
        ret.aliasConcreteClass[ret.alias] = ret
        return ret

class Widget(QObject):
    __metaclass__ = MetaWidget

    def __init__(self, **kwargs):
        # This method should not be necessary. It is, because we
        # get kwargs which are not empty.
        QObject.__init__(self)

    @classmethod
    def get_class(cls, alias):
        # Get module and class name as tuple (moduleName,className)
        return cls.aliasConcreteClass[alias]
    
    @classmethod
    def getAliasesViaCategory(cls, category):
        return cls.categoryToAliases[category]


class DisplayWidget(Widget):
    def __init__(self, **kwargs):
        Widget.__init__(self, **kwargs)
        self.valueType = None

class VacuumWidget(Widget):
    pass

class EditableWidget(Widget):
    signalEditingFinished = pyqtSignal(str, object)

    def valueEditingFinished(self, key, value):
        self.signalEditingFinished.emit(key, value)
