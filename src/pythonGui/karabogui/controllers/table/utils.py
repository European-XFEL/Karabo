#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 6, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo.common.api import KARABO_SCHEMA_DISPLAY_TYPE_STATE


def is_state_display_type(binding):
    """Return if the display type belongs to a state element"""
    return binding.display_type == KARABO_SCHEMA_DISPLAY_TYPE_STATE


def string2list(value: str) -> list:
    """Convert a list of chars to a list of strings with delimiter ','.

    This function strips white spaces and returns an empty list if an
    empty string is passed.
    """
    return [v.strip() for v in value.split(",")] if value else []


def list2string(value: list) -> str:
    """Convert a list to a string representation with delimiter ','"""
    value = [] if value is None else value
    return ",".join(str(v) for v in value)
