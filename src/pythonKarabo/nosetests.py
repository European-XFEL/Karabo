#! /usr/bin/python

__author__="Sergey Esenov <serguei.essenov@xfel.eu>"
__date__ ="$Apr 8, 2014 11:32:06 AM$"

import os
from nose import main

if __name__ == "__main__":
    os.chdir("tests")
    main()
    os.chdir("..")
