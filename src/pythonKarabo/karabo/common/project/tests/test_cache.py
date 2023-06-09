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
