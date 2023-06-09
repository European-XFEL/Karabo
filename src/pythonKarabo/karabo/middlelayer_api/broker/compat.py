# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
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
