from numbers import Integral, Real
import json

from numpy import ndarray

from .basetypes import KaraboValue
from .hash import Hash


class KaraboJSONEncoder(json.JSONEncoder):
    """Encode Karabo values for JSON

    The encoding is pretty greedy: Hashes lose their attributes, Karabo
    values their timestamp, and no bit sizes are preserved.
    """
    def default(self, v):
        if isinstance(v, KaraboValue):
            v = v.value
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
