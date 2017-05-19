from collections.abc import MutableMapping
from ctypes import (CDLL, CFUNCTYPE, POINTER, byref, c_void_p, c_char,
                    c_char_p, c_bool, c_byte, c_short, c_int, c_longlong,
                    c_float, c_double, c_uint, string_at, Structure)
import logging
import numbers

_dll = None
logger = logging.getLogger(__name__)

TYPES = [c_bool, c_byte, c_short, c_int, c_longlong,
         c_float, c_double, c_char_p]
TYPENAMES = ["Bool", "Int8", "Int16", "Int32", "Int64",
             "Float32", "Float64", "String"]


@CFUNCTYPE(c_int, c_int, c_int, c_char_p, c_longlong, c_longlong, c_char_p,
           c_int, c_char_p)
def _logging_func(severity, logCode, logMessage, timeOfMessage, connectionId,
                  filename, fileLineNumber, callbackData):
    logger.log([logging.ERROR, logging.WARNING, logging.INFO, logging.DEBUG,
                logging.DEBUG, logging.DEBUG, logging.DEBUG][severity],
               logMessage.decode("ascii"))
    return 0


def _get_openmqc():
    """ Make sure the openmqc library is loaded and return a reference to it.
    """
    global _dll
    if _dll is None:
        _dll = CDLL("libopenmqc.so")
        _dll = Wrapper(_dll)
        _dll.MQSetLoggingFunc(_logging_func, 0)
        _dll.MQSetStdErrLogLevel(-1)

    return _dll


class Error(Exception):
    def __init__(self, wrapped_dll, status):
        s = wrapped_dll.dll.MQGetStatusString(status)
        self.status = status
        try:
            super(Error, self).__init__(s.value.decode("utf8"))
        finally:
            wrapped_dll.MQFreeString(s)


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
                raise Error(self, status.error)
            return status.error
        setattr(self, name, wrapper)
        return wrapper

    def MQFreeString(self, string):
        self.dll.MQFreeString(string)

    def MQPropertiesKeyIterationHasNext(self, handle):
        return bool(self.dll.MQPropertiesKeyIterationHasNext(handle))


class Properties(MutableMapping):
    def __new__(cls, handle=None):
        dll = _get_openmqc()
        if handle is None:
            handle = c_int()
            dll.MQCreateProperties(byref(handle))
        self = super(Properties, cls).__new__(cls)
        self.dll = dll
        self.handle = handle
        return self

    def invalidate(self):
        """ mark the properties as invalid

        this is done after using OpenMQ functions which eat the properties"""
        self.handle = c_uint(0xFEEEFEEE)

    def valid(self):
        return self.handle.value != 0xFEEEFEEE

    def __repr__(self):
        if not self.valid():
            return "MQProperty{INVALID}"
        else:
            props = ("{}:{!r}".format(k, v) for k, v in self.items())
            return "MQProperty{" + ", ".join(props) + "}"

    def __del__(self):
        if self.valid():
            self.dll.MQFreeProperties(self.handle)

    def __iter__(self):
        self.dll.MQPropertiesKeyIterationStart(self.handle)
        while self.dll.MQPropertiesKeyIterationHasNext(self.handle):
            key = c_char_p()
            self.dll.MQPropertiesKeyIterationGetNext(self.handle, byref(key))
            yield key.value.decode('utf8')

    def __getitem__(self, key):
        key = c_char_p(key.encode('utf8'))
        type = c_int()
        try:
            self.dll.MQGetPropertyType(self.handle, key, byref(type))
        except Error as e:
            if e.status == 1104:  # not found
                raise KeyError(key)
            raise
        ret = TYPES[type.value]()
        getattr(self.dll, "MQGet{}Property".format(TYPENAMES[type.value]))(
            self.handle, key, byref(ret))
        return ret.value

    def __setitem__(self, key, value):
        key = c_char_p(key.encode('utf8'))
        if isinstance(value, bool):
            self.dll.MQSetBoolProperty(self.handle, key, c_bool(value))
        elif isinstance(value, numbers.Integral):
            if -(1 << 7) <= value < 2 << 7:
                self.dll.MQSetInt8Property(self.handle, key, c_byte(value))
            elif -(1 << 15) <= value < 1 << 15:
                self.dll.MQSetInt16Property(self.handle, key, c_short(value))
            elif -(1 << 31) <= value < 1 << 31:
                self.dll.MQSetInt32Property(self.handle, key, c_int(value))
            else:
                self.dll.MQSetInt64Property(self.handle, key,
                                            c_longlong(value))
        elif isinstance(value, numbers.Real):
            self.dll.MQSetFloat64Property(self.handle, key, c_double(value))
        elif isinstance(value, str):
            self.dll.MQSetStringProperty(self.handle, key,
                                         c_char_p(value.encode("utf8")))
        elif isinstance(value, bytes):
            self.dll.MQSetStringProperty(self.handle, key, c_char_p(value))
        else:
            raise TypeError("cannot convert '{}' to openmq type.".
                            format(type(value)))

    def __len__(self):
        raise NotImplementedError

    def __delitem__(self, key):
        raise NotImplementedError


class Connection(object):
    def __init__(self, properties, username, password, clientID=None):
        self.dll = _get_openmqc()
        self.handle = c_int()
        self.dll.MQCreateConnection(
            properties.handle, c_char_p(username.encode("utf8")),
            c_char_p(password.encode("utf8")),
            c_char_p(clientID), c_char_p(0), c_char_p(0), byref(self.handle))
        properties.invalidate()

    @property
    def properties(self):
        handle = c_int()
        self.dll.MQGetConnectionProperties(self.handle, byref(handle))
        return Properties(handle)

    @property
    def metadata(self):
        handle = c_int()
        self.dll.MQGetMetaData(self.handle, byref(handle))
        return Properties(handle)

    def start(self):
        self.dll.MQStartConnection(self.handle)

    def stop(self):
        self.dll.MQStopConnection(self.handle)

    def close(self):
        self.dll.MQCloseConnection(self.handle)


class Session(object):
    def __init__(self, connection, isTransacted, acknowledgeMode, receiveMode):
        self.dll = _get_openmqc()
        self.handle = c_int()
        self.dll.MQCreateSession(connection.handle, c_bool(isTransacted),
                                 c_int(acknowledgeMode), c_int(receiveMode),
                                 byref(self.handle))

    def commit(self):
        self.dll.MQCommitSession(self.handle)

    def rollback(self):
        self.dll.MQRollbackSession(self.handle)

    def recover(self):
        self.dll.MQRecoverSession(self.handle)

    def close(self):
        self.dll.MQCloseSession(self.handle)

    def getAcknowledgeMode(self):
        ret = c_int()
        self.dll.MQGetAcknowledgeMode(self.handle, byref(ret))
        return ret.value

    def acknowledge(self, message):
        self.dll.MQAcknowledgeMessage(self.handle, message.handle)


class _Destination(object):
    def __new__(cls, handle):
        self = super(_Destination, cls).__new__(cls)
        self.dll = _get_openmqc()
        self.handle = handle
        return self

    def __del__(self):
        self.dll.MQFreeDestination(self.handle)

    @property
    def type(self):
        ret = c_int()
        self.dll.MQDestinationGetType(self.handle, byref(ret))
        return ret.value

    @property
    def name(self):
        name = c_char_p()
        self.dll.MQGetDestinationName(self.handle, byref(name))
        ret = name.value.decode('utf8')
        self.dll.MQFreeString(name)
        return ret


class Destination(_Destination):
    def __new__(cls, session, name, type):
        dll = _get_openmqc()
        handle = c_int()
        dll.MQCreateDestination(session.handle, c_char_p(name.encode("utf8")),
                                c_char_p(type), byref(handle))
        return super(Destination, cls).__new__(cls, handle)


class Producer(object):
    def __init__(self, session, destination=None):
        self.dll = _get_openmqc()
        self.handle = c_int()
        if destination is None:
            self.dll.MQCreateMessageProducer(session.handle,
                                             byref(self.handle))
        else:
            self.dll.MQCreateMessageProducerForDestination(
                session.handle, destination.handle, byref(self.handle))

    def close(self):
        self.dll.MQCloseMessageProducer(self.handle)

    def send(self, message, mode, priority, timeToLive):
        self.dll.MQSendMessageExt(self.handle, message.handle, c_int(mode),
                                  c_byte(priority), c_longlong(timeToLive))


class _Consumer(object):
    def __init__(self):
        self.handle = c_int()
        self.dll = _get_openmqc()

    def close(self):
        self.dll.MQCloseMessageConsumer(self.handle)


class Consumer(_Consumer):
    def __init__(self, session, destination, selector, noLocal):
        super(Consumer, self).__init__()
        self.dll.MQCreateMessageConsumer(
            session.handle, destination.handle,
            c_char_p(selector.encode("utf8")),
            c_bool(noLocal), byref(self.handle))

    def receiveMessage(self, timeout=None):
        ret = c_int()
        try:
            if timeout is None:
                self.dll.MQReceiveMessageWait(self.handle, byref(ret))
            elif timeout == 0:
                self.dll.MQReceiveMessageNoWait(self.handle, byref(ret))
            else:
                self.dll.MQReceiveMessageWithTimeout(
                    self.handle, c_int(timeout), byref(ret))
        except Error as e:
            if e.status == 3120:  # broker dropped messages
                e.message = Message._create(ret)
            raise
        return Message._create(ret)


class SharedConsumer(_Consumer):
    def __init__(self, session, destination, subscription, selector):
        super(SharedConsumer, self).__init__()
        self.dll.MQCreateSharedMessageConsumer(
            session.handle, destination.handle,
            c_char_p(subscription.encode("utf8")),
            c_char_p(selector.encode("utf8")), byref(self.handle))


class _DurableConsumer(_Consumer):
    def __init__(self, durableName=None):
        super(_DurableConsumer, self).__init__()
        self.durableName = c_char_p(durableName.encode("utf8"))

    def unsubscribe(self):
        self.dll.MQUnsubscribeDurableMessageConsumer(self.handle,
                                                     self.durableName)


class DurableConsumer(_DurableConsumer):
    def __init__(self, session, destination, durableName, selector, noLocal):
        super(DurableConsumer, self).__init__(durableName=durableName)
        self.dll.MQCreateDurableMessageConsumer(
            session.handle, destination.handle, self.durableName,
            c_char_p(selector.encode("utf8")), c_bool(noLocal),
            byref(self.handle))


class SharedDurableConsumer(_DurableConsumer):
    def __init__(self, session, destination, durableName, selector):
        super(SharedDurableConsumer, self).__init__(durableName=durableName)
        self.dll.MQCreateSharedDurableMessageConsumer(
            session.handle, destination.handle, self.durableName,
            c_char_p(selector.encode("utf8")),
            byref(self.handle))


class _AsyncConsumer(_Consumer):
    Callback = CFUNCTYPE(c_int, c_int, c_int, c_int, c_void_p)

    def __init__(self, **kwargs):
        super(_AsyncConsumer, self).__init__()
        self.callback = self.Callback(self.callback)

    def callback(self, session, consumer, message, data):
        assert self.handle.value == consumer.value
        message = Message._create(message)
        self.listen(message)
        return 0


class AsyncConsumer(_AsyncConsumer):
    def __init__(self, session, destination, selector, noLocal):
        super(AsyncConsumer, self).__init__()
        self.dll.MQCreateAsyncMessageConsumer(
            session.handle, destination.handle,
            c_char_p(selector.encode("utf8")),
            c_bool(noLocal), self.callback, c_void_p(), byref(self.handle))


class AsyncSharedConsumer(_AsyncConsumer):
    def __init__(self, session, destination, subscription, selector):
        super(AsyncSharedConsumer, self).__init__()
        self.dll.MQCreateAsyncSharedMessageConsumer(
            session.handle, destination.handle,
            c_char_p(subscription.encode("utf8")),
            c_char_p(selector.encode("utf8")), self.callback, c_void_p(),
            byref(self.handle))


class AsyncDurableConsumer(_AsyncConsumer, _DurableConsumer):
    def __init__(self, session, destination, durableName, selector, noLocal):
        super(AsyncDurableConsumer, self).__init__(durableName=durableName)
        self.dll.MQCreateAsyncDurableMessageConsumer(
            session.handle, destination.handle, self.durableName,
            c_char_p(selector.encode("utf8")), c_bool(noLocal), self.callback,
            c_void_p(), byref(self.handle))


class AsyncSharedDurableConsumer(_AsyncConsumer, _DurableConsumer):
    def __init__(self, session, destination, durableName, selector, noLocal):
        super(AsyncSharedDurableConsumer, self).__init__(
            durableName=durableName)
        self.dll.MQCreateAsyncSharedDurableMessageConsumer(
            session.handle, destination.handle, self.durableName,
            c_char_p(selector.encode("utf8")), self.callback, c_void_p(),
            byref(self.handle))


class Message(object):
    def __new__(cls, handle=None, dll=None):
        if dll is None:
            dll = _get_openmqc()
        if handle is None:
            handle = c_int()
            dll.MQCreateMessage(byref(handle))
        self = super(Message, cls).__new__(cls)
        self.dll = dll
        self.handle = handle
        return self

    @staticmethod
    def _create(handle):
        type = c_int()
        dll = _get_openmqc()
        dll.MQGetMessageType(handle, byref(type))
        if type.value > 1:
            raise RuntimeError
        return (TextMessage, BytesMessage,
                Message, Message)[type.value](handle)

    def __del__(self):
        self.dll.MQFreeMessage(self.handle)

    @property
    def type(self):
        ret = c_int()
        self.dll.MQGetMessageType(self.handle, byref(ret))
        return ret.value

    @property
    def properties(self):
        handle = c_int()
        self.dll.MQGetMessageProperties(self.handle, byref(handle))
        return Properties(handle)

    @properties.setter
    def properties(self, properties):
        self.dll.MQSetMessageProperties(self.handle, properties.handle)
        properties.invalidate()

    @property
    def headers(self):
        handle = c_int()
        self.dll.MQGetMessageHeaders(self.handle, byref(handle))
        return Properties(handle)

    @headers.setter
    def headers(self, properties):
        self.dll.MQSetMessageHeaders(self.handle, properties.handle)
        properties.invalidate()

    @property
    def replyTo(self):
        handle = c_int()
        self.dll.MQGetMessageReplyTo(self.handle, byref(handle))
        return _Destination(handle)

    @replyTo.setter
    def replyTo(self, destination):
        self.dll.MQSetMessageReplyTo(self.handle, destination.handle)


class TextMessage(Message):
    def __new__(cls, handle=None):
        dll = _get_openmqc()
        if handle is None:
            handle = c_int()
            dll.MQCreateTextMessage(byref(handle))
        return super(TextMessage, cls).__new__(cls, handle, dll=dll)

    @property
    def data(self):
        ret = c_char_p()
        self.dll.MQGetTextMessageText(self.handle, byref(ret))
        return ret.value.decode('utf8')

    @data.setter
    def data(self, data):
        self.dll.MQSetTextMessageText(self.handle, c_char_p(
            str(data).encode('utf8')))


class BytesMessage(Message):
    def __new__(cls, handle=None):
        dll = _get_openmqc()
        if handle is None:
            handle = c_int()
            dll.MQCreateBytesMessage(byref(handle))
        return super(BytesMessage, cls).__new__(cls, handle, dll=dll)

    @property
    def data(self):
        m = POINTER(c_char)()
        l = c_int()
        self.dll.MQGetBytesMessageBytes(self.handle, byref(m), byref(l))
        return string_at(m, l)

    @data.setter
    def data(self, data):
        data = bytes(data)
        self.dll.MQSetBytesMessageBytes(self.handle, c_char_p(data),
                                        c_int(len(data)))
