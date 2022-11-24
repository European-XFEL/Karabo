from abc import ABC, abstractmethod


class Broker(ABC):
    def __init__(self, need_subscribe):
        self.needSubscribe = need_subscribe

    @abstractmethod
    def send(self, p, args):
        pass

    @abstractmethod
    def heartbeat(self, interval):
        pass

    @abstractmethod
    def notify_network(self, info):
        pass

    @abstractmethod
    def call(self, signal, targets, reply, args):
        pass

    @abstractmethod
    def request(self, device, target, *args):
        pass

    @abstractmethod
    def emit(self, signal, targets, *args):
        pass

    @abstractmethod
    def reply(self, message, reply, error=False):
        pass

    @abstractmethod
    def replyException(self, message, exception):
        pass

    @abstractmethod
    def connect(self, deviceId, signal, slot):
        pass

    @abstractmethod
    def async_connect(self, deviceId, signal, slot):
        pass

    @abstractmethod
    def disconnect(self, deviceId, signal, slot):
        pass

    @abstractmethod
    def async_disconnect(self, deviceId, signal, slot):
        pass

    @abstractmethod
    def consume(self, device):
        pass

    @abstractmethod
    def handleMessage(self, message, device):
        pass

    @abstractmethod
    def register_slot(self, name, slot):
        pass

    @abstractmethod
    def main(self, device):
        pass

    @abstractmethod
    def stop_tasks(self):
        pass

    @abstractmethod
    def enter_context(self, context):
        pass

    @abstractmethod
    def enter_async_context(self, context):
        pass

    @abstractmethod
    def updateInstanceInfo(self, info):
        pass

    @abstractmethod
    def decodeMessage(self, message):
        pass

    @abstractmethod
    def get_property(self, message, prop):
        return None

    async def ensure_connection(self):
        pass

    async def async_unsubscribe_all(self):
        pass

    async def ensure_disconnect(self, device):
        pass

    @staticmethod
    def create_connection(hosts, connection):
        # Get scheme (protocol) of first URI...
        scheme, _ = hosts[0].split('://')
        if scheme == 'tcp':
            from .jms_broker import JmsBroker
            return (JmsBroker.create_connection(hosts, connection),
                    JmsBroker)
        elif scheme == 'mqtt':
            from .mqtt_broker import MqttBroker
            return (MqttBroker.create_connection(hosts, connection),
                    MqttBroker)
        elif scheme == 'redis':
            from .redis_broker import RedisBroker
            return (RedisBroker.create_connection(hosts, connection),
                    RedisBroker)
        elif scheme == 'amqp':
            from .amqp_broker import AmqpBroker
            return (AmqpBroker.create_connection(hosts, connection),
                    AmqpBroker)
        else:
            raise RuntimeError(f"Unsupported protocol {scheme}")
