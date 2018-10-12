from asyncio import coroutine, get_event_loop, new_event_loop, set_event_loop
from datetime import datetime
from unittest.mock import Mock, patch

from nose.tools import assert_raises

from karabo.middlelayer import (
    getConfigurationFromPast, getSchemaFromPast, Hash, KaraboError, Schema,
    SignalSlotable
)


def setup():
    set_event_loop(new_event_loop())


@coroutine
def _get_log_reader_id(device_id):
    if device_id == "aDeviceNotInHistory":
        raise KaraboError
    return 'data_logger_device_id'


@patch('karabo.middlelayer_api.device_client.get_instance')
@patch('karabo.middlelayer_api.device_client._getLogReaderId',
       side_effect=_get_log_reader_id)
def test_getConfigurationSchemaFromPast(_getLogReaderId, get_instance):
    # REMOVE the @synchronize wrapping
    global getConfigurationFromPast, getSchemaFromPast
    getConfigurationFromPast = getConfigurationFromPast.__wrapped__
    getSchemaFromPast = getSchemaFromPast.__wrapped__

    call_args = []

    @coroutine
    def call(*args):
        call_args.append(args)
        return Hash('value', 42), Schema()

    instance = Mock(SignalSlotable)
    instance.call = call
    get_instance.return_value = instance
    time = datetime.now().isoformat()

    @coroutine
    def sub_test_1():
        with assert_raises(KaraboError):
            yield from getConfigurationFromPast("aDeviceNotInHistory", time)

    get_event_loop().run_until_complete(sub_test_1())

    @coroutine
    def sub_test_2():
        h = yield from getConfigurationFromPast("aDeviceInHistory", time)
        assert isinstance(h, Hash)
        assert h['value'] == 42

    get_event_loop().run_until_complete(sub_test_2())

    @coroutine
    def sub_test_3():
        s = yield from getSchemaFromPast("aDeviceInHistory", time)
        assert isinstance(s, Schema)

    get_event_loop().run_until_complete(sub_test_3())
