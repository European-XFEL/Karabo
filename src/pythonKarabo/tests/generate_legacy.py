""" This script generates the legacy.xml file used in
legacy_test.py """
from karabo.karathon import Schema, AssemblyRules, AccessType, Hash, \
    TextSerializerHash
from karabo.configurator import Configurator

import configuration_test_classes as ctc

if __name__ == "__main__":
    h = Hash()

    for c in ctc.__dict__.values():
        if isinstance(c, type) and hasattr(c, "expectedParameters"):
            if hasattr(c, "__base_classid__"):
                s = Configurator(c.__base_classid__).getSchema(c)
            else:
                s = Schema(c.__classid__, AssemblyRules(AccessType(7)))
                c.expectedParameters(s)
            h[c.__name__] = s

    s = TextSerializerHash.create("Xml", Hash())
    r = s.save(h)
    print(r)
