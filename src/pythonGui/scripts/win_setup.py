from distutils.core import setup
import json

# Work around mbcs bug in distutils.
# http://bugs.python.org/issue10945
import codecs
try:
    codecs.lookup('mbcs')
except LookupError:
    ascii = codecs.lookup('ascii')
    func = lambda name, enc=ascii: {True: enc}.get(name=='mbcs')
    codecs.register(func)

if __name__ == '__main__':
    # Read our metadata from the METADATA file (created by setup.py)
    with open('METADATA', 'r') as fp:
        metadata = json.load(fp)

    setup(scripts=['scripts/win_post_install.py'], **metadata)
