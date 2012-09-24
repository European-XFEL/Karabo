#!/usr/bin/python

import sys

import cStringIO

sys.stderr = cStringIO.StringIO()

sys.path.append('Module')
sys.path.append('WriterConfig')
#print sys.path

from libpyexfel import *

