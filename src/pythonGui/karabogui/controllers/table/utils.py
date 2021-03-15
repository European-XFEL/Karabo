#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 6, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo.common.api import (
    KARABO_SCHEMA_DISPLAY_TYPE, KARABO_SCHEMA_DISPLAY_TYPE_STATE)


def is_state_display_type(binding):
    """Return if the display type belongs to a state element"""
    dtype = binding.attributes.get(KARABO_SCHEMA_DISPLAY_TYPE, '')

    return dtype == KARABO_SCHEMA_DISPLAY_TYPE_STATE
