import os

amqp = os.environ["KARABO_BROKER"].startswith("amqp://")
mqtt = os.environ["KARABO_BROKER"].startswith("mqtt://")
redis = os.environ["KARABO_BROKER"].startswith("redis://")
jms = os.environ["KARABO_BROKER"].startswith("tcp://")
