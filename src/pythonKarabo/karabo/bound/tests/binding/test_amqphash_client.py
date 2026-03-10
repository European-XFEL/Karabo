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
import socket
import threading

from karabo.bound import (
    AmqpConnection, AmqpHashClient, Broker, EventLoop, Hash)


def test_amqp_hash_client():
    urls = Broker.brokersFromEnv()
    domain = Broker.brokerDomainFromEnv()
    hostname = socket.gethostname()
    pid = os.getpid()
    cid = f'{domain}.messageLogger/{hostname}/{pid}'  # instanceId
    qargs = Hash("x-max-length", 10000,      # queue limit
                 "x-overflow", "drop-head",  # drop oldest if limit reached
                 "x-message-ttl", 30000)     # 30 seconds

    def read_handler(header, body, exchange, routing_key):
        print(f'Message to exchange "{exchange}" with routingKey '
              f'"{routing_key}"\n\n{header}\n\n{body}\n{71 * "-"}\n')

    def error_handler(errmsg):
        print(f'Error reading message: {errmsg}\n{71 * "-"}')

    # Create connection and client object
    connection = AmqpConnection(urls)
    client = AmqpHashClient.create(
        connection, cid, qargs, read_handler, error_handler)

    # Establsh connection between client and broker
    connectedCond = threading.Condition()
    connectedFlag = False

    def onConnect(ec):
        nonlocal connectedFlag
        if ec.value() != 0:
            print(f"Error code {ec.value()} -- {ec.message()}")
            raise RuntimeError(f"Connection failed: {ec}")
        connectedFlag = True
        with connectedCond:
            connectedCond.notify()

    try:
        connection.asyncConnect(onConnect)
    except BaseException as e:
        print(f'\nException caught while connecting: {e}')
        exit(1)

    with connectedCond:
        connectedCond.wait(timeout=0.5)

    # client is connected ...
    assert connectedFlag

    print("\n# Starting to consume messages...")
    print(f"# Broker (AMQP): {connection.getCurrentUrl()}")
    print(f"# Domain: {domain}")

    # Subscribe client to the topics of interest ...
    def subscribe(cl, exch, rkey):
        print(f"# Exchange: '{exch}' and binding key: '{rkey}'")
        cv = threading.Condition()

        def on_subscribe(ec):
            if ec.value() != 0:
                raise RuntimeError(f"Subscription failed: {ec.message()}")
            with cv:
                cv.notify()

        client.asyncSubscribe(exch, rkey, on_subscribe)
        return cv

    cvs = []
    table = [(f"{domain}.Signals", "#"),
             (f"{domain}.Slots", "#"),
             (f"{domain}.Global_Slots", "#")]
    for e, r in table:
        cvs.append(subscribe(client, e, r))

    # wait for all subscriptions completed...
    for cv in cvs:
        with cv:
            cv.wait(timeout=1)

    # client subscribed to messages circulated in given domain
    assert connection.isConnected()

    # Use EventLoop.run() here if you want just to test API
    # Use EventLoop.work() if you want to be like 'karabo-brokermessagelogger'
    EventLoop.run()

    # Let broker to unsubscribe from all topics we used
    unsubscribeCond = threading.Condition()

    def on_unsubscribe(ec):
        if ec.value() != 0:
            raise RuntimeError("Failed asyncUnsubscribeAll: {ec.message()}")
        with unsubscribeCond:
            unsubscribeCond.notify()

    client.asyncUnsubscribeAll(on_unsubscribe)
    with unsubscribeCond:
        unsubscribeCond.wait(timeout=0.1)

    # Run eventloop once more to process unsubscription
    EventLoop.run()

    print("\n\nEnd of job")
