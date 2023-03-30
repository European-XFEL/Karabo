import os
from contextlib import suppress

broker = os.environ.get("KARABO_BROKER")
if broker:
    amqp = broker.startswith("amqp://")
    mqtt = broker.startswith("mqtt://")
    jms = broker.startswith("tcp://")
else:
    # For now, keep JMS as the default
    amqp = mqtt = False
    jms = True


def _get_exception():
    if jms:
        from .openmq import Error as JMSError
        return JMSError
    elif amqp:
        from aiormq.exceptions import AMQPException
        return AMQPException
    elif mqtt:
        from .pahomqtt import MqttError
        return MqttError


exception = _get_exception()


def suppressBrokerException():
    return suppress(exception)
