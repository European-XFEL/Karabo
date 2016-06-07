#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from contextlib import contextmanager


@contextmanager
def save_painter_state(painter):
    painter.save()
    yield
    painter.restore()
