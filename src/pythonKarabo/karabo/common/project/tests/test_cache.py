# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os
import os.path as op
import sys

from karabo.common.project.api import get_user_cache


def test_get_user_cache():
    cache = get_user_cache()

    if sys.platform.startswith("win"):
        expected = op.join(os.environ["APPDATA"], "karabo", "project_db_cache")
    else:
        expected = op.expanduser(op.join("~", ".karabo", "project_db_cache"))

    assert expected == cache.dirpath
