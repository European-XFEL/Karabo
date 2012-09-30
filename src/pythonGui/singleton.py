#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which can be used as a base class for all
   classes that need to implement the Singleton design pattern.
"""

__all__ = ["Singleton"]


class Singleton(object):
    """This class represents a singleton object"""
    _instance = None
    
    def __new__(cls, *a, **k):
        
        if cls != type(cls._instance):
            cls._instance = object.__new__(cls)
            
            if 'init_single' in cls.__dict__: 
                cls._instance.init_single(*a,**k)
            else:
                cls._instance.init(*a,**k)
        return cls._instance

    def init(self, *p, **k):
        pass
