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
import json
from numbers import Integral, Real

from numpy import ndarray

from karabo.native.data import Hash


class KaraboJSONEncoder(json.JSONEncoder):
    """Encode Karabo values for JSON

    The encoding is pretty greedy: Hashes lose their attributes, Karabo
    values their timestamp, and no bit sizes are preserved.
    """

    def default(self, v):
        try:
            v = v.value
        except AttributeError:
            # No KaraboValue
            pass
        if isinstance(v, Hash):
            return {k: self.default(v) for k, v in v.items()}
        elif isinstance(v, (ndarray, list)):
            return list(self.default(x) for x in v)
        elif isinstance(v, dict):
            return {self.default(k): self.default(v) for k, v in v.items()}
        elif isinstance(v, bool):
            return v
        elif isinstance(v, Integral):
            return int(v)
        elif isinstance(v, Real):
            return float(v)
        elif v is None:
            return v
        else:
            return super().default(v)
