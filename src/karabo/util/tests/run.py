#!/usr/bin/python

from libvehiclepybind import *
from config_ext import *
c=Config()
c.setFromPath("BobbyCar.name", "AAAAA" )
a= Vehicle.create(c)
a.start()
d=Config()
d.setFromPath("Car.name", 12)
b= Vehicle.create(d)
b.start()
