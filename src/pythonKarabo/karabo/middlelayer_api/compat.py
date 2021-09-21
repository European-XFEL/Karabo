import os

broker = os.environ.get("KARABO_BROKER")
if broker:
    amqp = broker.startswith("amqp://")
    mqtt = broker.startswith("mqtt://")
    redis = broker.startswith("redis://")
    jms = broker.startswith("tcp://")
else:
    # For now, keep JMS as the default
    amqp = mqtt = redis = False
    jms = True
