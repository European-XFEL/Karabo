/* 
 * File:   JmsConnection_Test.cc
 * Author: heisenb
 * 
 * Created on July 15, 2016, 5:05 PM
 */

#include "JmsConnection_Test.hh"
#include "karabo/net/JmsChannel.hh"
#include <karabo/net/JmsConnection.hh>

using namespace karabo::util;
using namespace karabo::net;

CPPUNIT_TEST_SUITE_REGISTRATION(JmsConnection_Test);


JmsConnection_Test::JmsConnection_Test() {
}


JmsConnection_Test::~JmsConnection_Test() {
}


void JmsConnection_Test::testConnect() {

    {
        JmsConnection con = JmsConnection();
        CPPUNIT_ASSERT(con.isConnected() == false);
        con.connect();
        CPPUNIT_ASSERT(con.isConnected() == true);
        con.disconnect();
        CPPUNIT_ASSERT(con.isConnected() == false);
        con.connect();
        CPPUNIT_ASSERT(con.isConnected() == true);
    }

    {
        JmsConnection con = JmsConnection("tcp://someBadHost:7777,tcp://exfl-broker.desy.de:7777");
        CPPUNIT_ASSERT(con.isConnected() == false);
        con.connect();
        CPPUNIT_ASSERT(con.isConnected() == true);
        CPPUNIT_ASSERT(con.getBrokerUrl() == "tcp://exfl-broker.desy.de:7777");
        con.disconnect();
        CPPUNIT_ASSERT(con.isConnected() == false);
    }

    {
        putenv("KARABO_BROKER=tcp://exfl-broker.desy.de:7777");
        JmsConnection con = JmsConnection("tcp://someBadHost:7777");
        CPPUNIT_ASSERT(con.isConnected() == false);
        con.connect();
        CPPUNIT_ASSERT(con.isConnected() == true);
        CPPUNIT_ASSERT(con.getBrokerUrl() == "tcp://exfl-broker.desy.de:7777");
        con.disconnect();
        CPPUNIT_ASSERT(con.isConnected() == false);
        unsetenv("KARABO_BROKER");
    }
}


void JmsConnection_Test::testChannel() {
    JmsConnection::Pointer con(new JmsConnection());
    con->connect();
    con->start();
    JmsChannel::Pointer channel = con->createChannel();
    Hash::Pointer header(new Hash("freakin", "header"));
    Hash::Pointer body(new Hash("freakin", "body"));
    channel->write("heisenb", header, body);
    con->disconnect();
    con->connect();
    con->start();
    channel->write("heisenb", header, body);





}

