/*
 * File:   JmsBroker_Test.cc
 * Author: heisenb
 *
 * Created on Mar 20, 2013, 3:31:07 PM
 */

#include "JmsBroker_Test.hh"


CPPUNIT_TEST_SUITE_REGISTRATION(JmsBroker_Test);

using namespace std;
using namespace karabo::util;
using namespace karabo::net;


JmsBroker_Test::JmsBroker_Test() : m_messagesRead(0) {
    Hash tmp("a.b.c", 1, "a.b.d", vector<int>(5, 1), "a.b.e", vector<Hash > (2, Hash("a", 1)), "a.d", std::complex<double>(1.2, 4.2));
    tmp.setAttribute("a", "a1", true);
    tmp.setAttribute("a", "a2", 3.4);
    tmp.setAttribute("a.b", "b1", "3");
    tmp.setAttribute("a.b.c", "c1", 2);
    tmp.setAttribute("a.b.c", "c2", vector<string > (3, "bla"));
    m_hash = tmp;
}


JmsBroker_Test::~JmsBroker_Test() {
}


void JmsBroker_Test::readHandler1(karabo::net::BrokerChannel::Pointer channel, const karabo::util::Hash::Pointer& header, const std::string& body) {

    if ((body == "Random message body") &&
            (header->has("randomHeaderGarbage") == true) &&
            (header->get<string > ("randomHeaderGarbage") == "indeed")) {
        m_messagesRead++;
    }
    
    channel->readAsyncHashHash(boost::bind(&JmsBroker_Test::readHandler2, this, _1, _2, _3));

    channel->write(Hash(), m_hash);
}


void JmsBroker_Test::readHandler2(karabo::net::BrokerChannel::Pointer channel, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) {
    if (karabo::util::similar(*body, m_hash)) {
        m_messagesRead++;
    }
}


void JmsBroker_Test::testMethod() {


    BrokerConnection::Pointer connection;

    try {

        connection = BrokerConnection::create("Jms");

    } catch (const Exception& e) {
        return;
    }

    BrokerIOService::Pointer ioService = connection->getIOService();

    connection->start();
    
    BrokerChannel::Pointer channel = connection->createChannel();

    channel->readAsyncHashString(boost::bind(&JmsBroker_Test::readHandler1, this, _1, _2, _3));

    //channel->setErrorHandler(&onError);
    
    boost::this_thread::sleep(boost::posix_time::millisec(100));

    channel->write(Hash("randomHeaderGarbage", "indeed"), "Random message body");

    ioService->run();

    CPPUNIT_ASSERT(m_messagesRead == 2);


}

void JmsBroker_Test::testBinaryTransport() {
    
    BrokerConnection::Pointer connection;

    try {

        connection = BrokerConnection::create("Jms", Hash("serializationType", "binary"));

    } catch (const Exception& e) {
        return;
    }

    BrokerIOService::Pointer ioService = connection->getIOService();

    connection->start();
    
    BrokerChannel::Pointer channel = connection->createChannel();

    channel->readAsyncHashString(boost::bind(&JmsBroker_Test::readHandler1, this, _1, _2, _3));

    //channel->setErrorHandler(&onError);

    channel->write(Hash("randomHeaderGarbage", "indeed"), "Random message body");

    ioService->run();

    CPPUNIT_ASSERT(m_messagesRead == 2);
}


