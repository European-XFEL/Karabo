from karabo import api_version

if api_version == 1:
    from karabo.karathon_bin import *
else:
    from karabo.karathon_py import *
