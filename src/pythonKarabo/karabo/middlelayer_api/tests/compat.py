import os

mqtt = os.environ["KARABO_BROKER"].startswith("mqtt://")
jms = os.environ["KARABO_BROKER"].startswith("tcp://")