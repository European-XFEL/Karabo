# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import pytest

from ..utils import check_broker_scheme


def test_check_broker_scheme():
    # 1. different schemes
    hosts = ["amqp://xfel:karabo@exflc1:1111",
             "tcp://exfl-broker:7777"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)

    # 2. typo in schemes
    hosts = ["amqp://xfel:karabo@exflc1:1111",
             "amqp//xfel:karabo@exflc1:1111"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)

    # 3.1 correct scheme amqp, also with `-`
    hosts = ["amqp://xfel:karabo@exflc1:1111",
             "amqp://xfel:karabo@exfl-bkr-1:1111"]
    assert not check_broker_scheme(hosts)
    hosts = ["amqp://xfel:karabo@exflc1:1111"]
    assert not check_broker_scheme(hosts)

    # 3.2 correct scheme jms
    hosts = ["tcp://exfl-broker:7777",
             "tcp://exfl-broker:7777"]
    assert not check_broker_scheme(hosts)
    hosts = ["tcp://exfl-broker:7777"]
    assert not check_broker_scheme(hosts)

    # 4.1 incomplete scheme amqp, first missing port
    hosts = ["amqp://xfel:karabo@exflc1",
             "amqp://xfel:karabo@exflc1:1111"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)
    # 4.2 incomplete amqp, missing password, etc.
    hosts = ["amqp://xfel@exflc1:1111",
             "amqp://xfel:karabo@exflc1"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)

    # 4.3 No @ sign
    hosts = ["amqp://xfel:karaboexflc1:1111",
             "amqp://xfel:karabo@exflc1:1111"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)

    # 4.4 No : second
    hosts = ["amqp://xfel:karabo@exflc1:1111",
             "amqp://xfelkarabo@exflc1:1111"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)

    # 5. empty list
    hosts = ""
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)
