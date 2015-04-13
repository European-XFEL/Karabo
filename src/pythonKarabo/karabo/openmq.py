import numbers
from ctypes import (CDLL, CFUNCTYPE, POINTER, byref, c_void_p, c_char,
                    c_char_p, c_bool, c_byte, c_short, c_int, c_longlong,
                    c_float, c_double, c_uint, string_at, Structure)
import collections.abc
import site

dll = CDLL("libopenmqc.so")


class Error(Exception):
    def __init__(self, status):
        s = dll.dll.MQGetStatusString(status)
        self.status = status
        try:
            super().__init__(self, s.value.decode("utf8"))
        finally:
            dll.MQFreeString(s)


class MQError(Structure):
    _fields_ = [("error", c_uint)]


class String(c_char_p):
    pass


class Wrapper(object):
    def __init__(self, dll):
        self.dll = dll
        dll.MQGetStatusString.restype = String

    def __getattr__(self, name):
        inner = getattr(self.dll, name)
        inner.restype = MQError

        def wrapper(*args):
            status = inner(*args)
            if self.dll.MQStatusIsError(status.error):
                raise Error(status.error)
            return status.error
        setattr(self, name, wrapper)
        return wrapper

    def MQFreeString(self, string):
        self.dll.MQFreeString(string)

    def MQPropertiesKeyIterationHasNext(self, handle):
        return bool(self.dll.MQPropertiesKeyIterationHasNext(handle))


dll = Wrapper(dll)

types = [c_bool, c_byte, c_short, c_int, c_longlong,
         c_float, c_double, c_char_p]
typenames = ["Bool", "Int8", "Int16", "Int32", "Int64",
             "Float32", "Float64", "String"]


class Properties(collections.abc.MutableMapping):
    def __new__(cls, handle=None):
        if handle is None:
            handle = c_int()
            dll.MQCreateProperties(byref(handle))
        self = super().__new__(cls)
        self.handle = handle
        return self

    def invalidate(self):
        """ mark the properties as invalid

        this is done after using OpenMQ functions which eat the properties"""
        self.handle = c_uint(0xFEEEFEEE)

    def valid(self):
        return self.handle.value != 0xFEEEFEEE

    def __del__(self):
        if self.valid():
            dll.MQFreeProperties(self.handle)

    def __iter__(self):
        dll.MQPropertiesKeyIterationStart(self.handle)
        while dll.MQPropertiesKeyIterationHasNext(self.handle):
            key = c_char_p()
            dll.MQPropertiesKeyIterationGetNext(self.handle, byref(key))
            yield key.value.decode('utf8')

    def __getitem__(self, key):
        key = c_char_p(key.encode('utf8'))
        type = c_int()
        try:
            dll.MQGetPropertyType(self.handle, key, byref(type))
        except Error as e:
            if e.status == 1104:  # not found
                raise KeyError(key)
            raise
        ret = types[type.value]()
        getattr(dll, "MQGet{}Property".format(typenames[type.value]))(
            self.handle, key, byref(ret))
        return ret.value

    def __setitem__(self, key, value):
        key = c_char_p(key.encode('utf8'))
        if isinstance(value, bool):
            dll.MQSetBoolProperty(self.handle, key, c_bool(value))
        elif isinstance(value, numbers.Integral):
            if -(1 << 7) <= value < 2 << 7:
                dll.MQSetInt8Property(self.handle, key, c_byte(value))
            elif -(1 << 15) <= value < 1 << 15:
                dll.MQSetInt16Property(self.handle, key, c_short(value))
            elif -(1 << 31) <= value < 1 << 31:
                dll.MQSetInt32Property(self.handle, key, c_int(value))
            else:
                dll.MQSetInt64Property(self.handle, key, c_longlong(value))
        elif isinstance(value, numbers.Real):
            dll.MQSetFloat64Property(self.handle, key, c_double(value))
        elif isinstance(value, str):
            dll.MQSetStringProperty(self.handle, key,
                                    c_char_p(value.encode("utf8")))
        elif isinstance(value, bytes):
            dll.MQSetStringProperty(self.handle, key, c_char_p(value))
        else:
            raise TypeError("cannot convert '{}' to openmq type.".
                            format(type(value)))

    def __len__(self):
        raise NotImplementedError

    def __delitem__(self, key):
        raise NotImplementedError


class Connection(object):
    def __init__(self, properties, username, password, clientID=None):
        self.handle = c_int()
        dll.MQCreateConnection(
            properties.handle, c_char_p(username.encode("utf8")),
            c_char_p(password.encode("utf8")),
            c_char_p(clientID), c_char_p(0), c_char_p(0), byref(self.handle))
        properties.invalidate()

    @property
    def properties(self):
        handle = c_int()
        dll.MQGetConnectionProperties(self.handle, byref(handle))
        return Properties(handle)

    @property
    def metadata(self):
        handle = c_int()
        dll.MQGetMetaData(self.handle, byref(handle))
        return Properties(handle)

    def start(self):
        dll.MQStartConnection(self.handle)

    def stop(self):
        dll.MQStopConnection(self.handle)

    def close(self):
        dll.MQCloseConnection(self.handle)


class Session(object):
    def __init__(self, connection, isTransacted, acknowledgeMode, receiveMode):
        self.handle = c_int()
        dll.MQCreateSession(connection.handle, c_bool(isTransacted),
                            c_int(acknowledgeMode), c_int(receiveMode),
                            byref(self.handle))

    def commit(self):
        dll.MQCommitSession(self.handle)

    def rollback(self):
        dll.MQRollbackSession(self.handle)

    def recover(self):
        dll.MQRecoverSession(self.handle)

    def close(self):
        dll.MQCloseSession(self.handle)

    def getAcknowledgeMode(self):
        ret = c_int()
        dll.MQGetAcknowledgeMode(self.handle, byref(ret))
        return ret.value

    def acknowledge(self, message):
        dll.MQAcknowledgeMessage(self.handle, message.handle)


class _Destination(object):
    def __new__(cls, handle):
        self = super().__new__(cls)
        self.handle = handle
        return self

    def __del__(self):
        dll.MQFreeDestination(self.handle)

    @property
    def type(self):
        ret = c_int()
        dll.MQDestinationGetType(self.handle, byref(ret))
        return ret.value

    @property
    def name(self):
        name = c_char_p()
        dll.MQGetDestinationName(self.handle, byref(name))
        ret = name.value.decode('utf8')
        dll.MQFreeString(name)
        return ret


class Destination(_Destination):
    def __new__(cls, session, name, type):
        handle = c_int()
        dll.MQCreateDestination(session.handle, c_char_p(name.encode("utf8")),
                                c_char_p(type), byref(handle))
        return super().__new__(cls, handle)


class Producer(object):
    def __init__(self, session, destination=None):
        self.handle = c_int()
        if destination is None:
            dll.MQCreateMessageProducer(session.handle, byref(self.handle))
        else:
            dll.MQCreateMessageProducerForDestination(
                session.handle, destination.handle, byref(self.handle))

    def close(self):
        dll.MQCloseMessageProducer(self.handle)

    def send(self, message, mode, priority, timeToLive):
        dll.MQSendMessageExt(self.handle, message.handle, c_int(mode),
                             c_byte(priority), c_longlong(timeToLive))


class _Consumer(object):
    def __init__(self):
        self.handle = c_int()

    def close(self):
        dll.MQCloseMessageConsumer(self.handle)


class Consumer(_Consumer):
    def __init__(self, session, destination, selector, noLocal):
        super().__init__()
        dll.MQCreateMessageConsumer(
            session.handle, destination.handle,
            c_char_p(selector.encode("utf8")),
            c_bool(noLocal), byref(self.handle))

    def receiveMessage(self, timeout=None):
        ret = c_int()
        if timeout is None:
            dll.MQReceiveMessageWait(self.handle, byref(ret))
        elif timeout == 0:
            dll.MQReceiveMessageNoWait(self.handle, byref(ret))
        else:
            dll.MQReceiveMessageWithTimeout(self.handle, c_int(timeout),
                                            byref(ret))
        return Message._create(ret)


class SharedConsumer(_Consumer):
    def __init__(self, session, destination, subscription, selector):
        super().__init__(self)
        dll.MQCreateSharedMessageConsumer(
            session.handle, destination.handle,
            c_char_p(subscription.encode("utf8")),
            c_char_p(selector.encode("utf8")), byref(self.handle))


class _DurableConsumer(_Consumer):
    def __init__(self, durableName, **kwargs):
        super().__init__(**kwargs)
        self.durableName = c_char_p(durableName.encode("utf8"))

    def unsubscribe(self):
        dll.MQUnsubscribeDurableMessageConsumer(self.handle, self.durableName)


class DurableConsumer(_DurableConsumer):
    def __init__(self, session, destination, durableName, selector, noLocal):
        super().__init__(self, durableName=durableName)
        dll.MQCreateDurableMessageConsumer(
            session.handle, destination.handle, self.durableName,
            c_char_p(selector.encode("utf8")), c_bool(noLocal),
            byref(self.handle))


class SharedDurableConsumer(_DurableConsumer):
    def __init__(self, session, destination, durableName, selector):
        super().__init__(self, durableName=durableName)
        dll.MQCreateSharedDurableMessageConsumer(
            session.handle, destination.handle, self.durableName,
            c_char_p(selector.encode("utf8")),
            c_bool(noLocal), byref(self.handle))


class _AsyncConsumer(_Consumer):
    Callback = CFUNCTYPE(c_int, c_int, c_int, c_int, c_void_p)

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.callback = self.Callback(self.callback)

    def callback(self, session, consumer, message, data):
        assert self.handle.value == consumer.value
        message = Message._create(message)
        self.listen(message)
        return 0


class AsyncConsumer(_AsyncConsumer):
    def __init__(self, session, destination, selector, noLocal):
        super().__init__()
        dll.MQCreateAsyncMessageConsumer(
            session.handle, destination.handle,
            c_char_p(selector.encode("utf8")),
            c_bool(noLocal), self.callback, c_void_p(), byref(self.handle))


class AsyncSharedConsumer(_AsyncConsumer):
    def __init__(self, session, destination, subscription, selector):
        super().__init__()
        dll.MQCreateAsyncSharedMessageConsumer(
            session.handle, destination.handle,
            c_char_p(subscription.encode("utf8")),
            c_char_p(selector.encode("utf8")), self.callback, c_void_p(),
            byref(self.handle))


class AsyncDurableConsumer(_AsyncConsumer, _DurableConsumer):
    def __init__(self, session, destination, durableName, selector, noLocal):
        super().__init__(durableName=durableName)
        dll.MQCreateAsyncDurableMessageConsumer(
            session.handle, destination.handle, self.durableName,
            c_char_p(selector.encode("utf8")), c_bool(noLocal), self.callback,
            c_void_p(), byref(self.handle))


class AsyncSharedDurableConsumer(_AsyncConsumer, _DurableConsumer):
    def __init__(self, session, destination, durableName, selector, noLocal):
        super().__init__(durableName=durableName)
        dll.MQCreateAsyncSharedDurableMessageConsumer(
            session.handle, destination.handle, self.durableName,
            c_char_p(selector.encode("utf8")), self.callback, c_void_p(),
            byref(self.handle))


class Message(object):
    def __new__(cls, handle=None):
        if handle is None:
            handle = c_int()
            dll.MQCreateMessage(byref(handle))
        self = super().__new__(cls)
        self.handle = handle
        return self

    @staticmethod
    def _create(handle):
        type = c_int()
        dll.MQGetMessageType(handle, byref(type))
        if type.value > 1:
            raise RuntimeError
        return (TextMessage, BytesMessage,
                Message, Message)[type.value](handle)

    def __del__(self):
        dll.MQFreeMessage(self.handle)

    @property
    def type(self):
        ret = c_int()
        dll.MQGetMessageType(self.handle, byref(ret))
        return ret.value

    @property
    def properties(self):
        handle = c_int()
        dll.MQGetMessageProperties(self.handle, byref(handle))
        return Properties(handle)

    @properties.setter
    def properties(self, properties):
        dll.MQSetMessageProperties(self.handle, properties.handle)
        properties.invalidate()

    @property
    def headers(self):
        handle = c_int()
        dll.MQGetMessageHeaders(self.handle, byref(handle))
        return Properties(handle)

    @headers.setter
    def headers(self, properties):
        dll.MQSetMessageHeaders(self.handle, properties.handle)
        properties.invalidate()

    @property
    def replyTo(self):
        handle = c_int()
        dll.MQGetMessageReplyTo(self.handle, byref(handle))
        return _Destination(handle)

    @replyTo.setter
    def replyTo(self, destination):
        dll.MQSetMessageReplyTo(self.handle, destination.handle)


class TextMessage(Message):
    def __new__(cls, handle=None):
        if handle is None:
            handle = c_int()
            dll.MQCreateTextMessage(byref(handle))
        return super().__new__(cls, handle)

    @property
    def data(self):
        ret = c_char_p()
        dll.MQGetTextMessageText(self.handle, byref(ret))
        return ret.value.decode('utf8')

    @data.setter
    def data(self, data):
        dll.MQSetTextMessageText(self.handle, c_char_p(
            str(data).encode('utf8')))


class BytesMessage(Message):
    def __new__(cls, handle=None):
        if handle is None:
            handle = c_int()
            dll.MQCreateBytesMessage(byref(handle))
        return super().__new__(cls, handle)

    @property
    def data(self):
        m = POINTER(c_char)()
        l = c_int()
        dll.MQGetBytesMessageBytes(self.handle, byref(m), byref(l))
        return string_at(m, l)

    @data.setter
    def data(self, data):
        data = bytes(data)
        dll.MQSetBytesMessageBytes(self.handle, c_char_p(data),
                                   c_int(len(data)))
