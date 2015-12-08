try:
    from karabo._version import full_version as __version__
except ImportError:
    # Don't cause a failure when running setup.py
    __version__ = ''

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Jul 11, 2013 5:20:23 PM$"
