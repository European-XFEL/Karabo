from collections import OrderedDict

from karabo.native import AccessLevel

ACCESS_LEVELS = OrderedDict()
ACCESS_LEVELS['Admin'] = AccessLevel.ADMIN
ACCESS_LEVELS['Expert'] = AccessLevel.EXPERT
ACCESS_LEVELS['Operator'] = AccessLevel.OPERATOR
ACCESS_LEVELS['User'] = AccessLevel.USER
ACCESS_LEVELS['Observer'] = AccessLevel.OBSERVER

USERNAMES = [level.lower() for level in ACCESS_LEVELS.keys()]
