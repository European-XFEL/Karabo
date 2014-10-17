#!/usr/bin/env python3

__author__="Sergey Esenov <serguei.essenov@xfel.eu>"
__date__ ="$Apr 8, 2014 11:32:06 AM$"

import os
import nose
import sys

if __name__ == "__main__":
    os.chdir("tests")
    nose.run()
    os.chdir("..")
