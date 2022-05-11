import os
from contextlib import suppress

from aio_pika import AMQPException
from aioredis import RedisError

from karabo.middlelayer_api.openmq import Error as JMSError
from karabo.middlelayer_api.pahomqtt import MqttError

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


def suppressBrokerException():
    return suppress(AMQPException, JMSError, MqttError, RedisError)
