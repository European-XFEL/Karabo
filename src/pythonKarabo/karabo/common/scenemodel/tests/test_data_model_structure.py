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
import hashlib
from collections import OrderedDict

from traits.api import Enum, Instance

from karabo.common.api import BaseSavableModel

# Hey Code Reviewers! Be sure to ask questions when this value changes!
EXPECTED_HASH = (
    "33f6ac67bf1567cb11e5a5219da22136a1c91bc64e1add5f46861533017606ce"
)
FAILURE_MSG = """
##############################################################################
                            !!! WARNING !!!
Changes have been detected in the structure of the Scene data model. If you
haven't already, please read the 'Developing the Scene' section of the Karabo
documentation before proceeding!

Once the changes are complete, update the expected hash value here.
##############################################################################
"""


def _get_classes():
    """Get an alphabetized dictionary of scene model classes."""
    import karabo.common.scenemodel.api as model_api

    classes = []
    for name in dir(model_api):
        symbol = getattr(model_api, name)
        if isinstance(symbol, type) and issubclass(symbol, BaseSavableModel):
            classes.append((name, symbol))

    classes.sort(key=lambda x: x[0])
    return OrderedDict(classes)


def _hash_model_structure():
    """Compute a sha256 hash of the structure of the scene data model.

    This is done by getting a sorted list of scene model classes and adding
    their names and child trait names to the hashlib.sha256 hash function.

    The intention is to detect changes made to the file format of the scene and
    warn developers that they should proceed deliberately.
    """
    hsh = hashlib.sha256()
    classes = _get_classes()

    for name, klass in classes.items():
        instance = klass()
        trait_names = list(sorted(instance.copyable_trait_names()))
        traits = [_trait_sig(instance.trait(n)) for n in trait_names]
        klass_contents = [n + t for n, t in zip(trait_names, traits)]
        hsh.update(name.encode("utf-8"))
        hsh.update("".join(klass_contents).encode("utf-8"))

    return hsh.hexdigest()


def _trait_sig(trait):
    """Recursively stringify an object trait"""
    trait_type = trait.trait_type
    sig = type(trait_type).__name__
    if isinstance(trait_type, Instance):
        if isinstance(trait_type.klass, type):
            sig += trait_type.klass.__name__
        else:
            # klass is a string
            sig += trait_type.klass
    elif isinstance(trait_type, Enum):
        values = ",".join([str(v) for v in trait_type.values])
        sig += f"({values})"
    sig += "".join([_trait_sig(sub) for sub in trait_type.inner_traits()])
    return sig


def test_datamodel_structure():
    model_hash = _hash_model_structure()
    assert model_hash == EXPECTED_HASH, FAILURE_MSG
