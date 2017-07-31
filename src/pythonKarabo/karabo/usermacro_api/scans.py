""""Scantool macros"""
from .usermacro import UserMacro


class AScan(UserMacro):
    """Absolute scan"""
    pass


class AMesh(AScan):
    """Absolute Mesh scan"""
    pass


class APathScan(AScan):
    """Absolute Mesh scan"""
    pass


class DScan(AScan):
    """Delta scan"""
    pass


class TScan(UserMacro):
    """Time scan"""
    pass


class DMesh(AScan):
    """Delta Mesh scan"""
    pass


class AMove(UserMacro):
    """Absolute move"""
    pass


class DMove(UserMacro):
    """Delta move"""
    pass
