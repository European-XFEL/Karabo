/*
 * $Id$
 *
 * Author: <gero.flucke@xfel.eu>
 *
 * Created on November 6, 2015
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <iosfwd>

#include <cstdlib>
#include <iomanip>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/Epochstamp.hh>
#include <karabo/util/MetaTools.hh>
#include <karabo/net/JmsConnection.hh>
#include <karabo/net/JmsConsumer.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/log/Logger.hh>

using namespace karabo;


class BrokerStatistics : public boost::enable_shared_from_this<BrokerStatistics> {



public:

    /// Stats as pair of number of calls and accumulated size in bytes.
    typedef std::pair<unsigned int, size_t> Stats;

    /// SignalId as pair of instanceId and signalId of sender.
    typedef std::pair<const std::string, const std::string> SignalId;

    typedef std::map<SignalId, Stats> SignalStatsMap;

    /// SlotId is single string: a ':' separates slotInstanceId and slotFunction.
    typedef std::string SlotId;

    typedef std::map<SlotId, Stats> SlotStatsMap;

    /// Constructor with length of interval to use for averaging.


    BrokerStatistics(util::TimeValue intervalSec)
        : m_interval(intervalSec, 0ll), m_start(0ull, 0ull) {
    }

    // virtual ~BrokerStatistics() {} // no need for virtual...

    /// Register a message, i.e. increase statistics and possibly print.
    void registerMessage(const util::Hash::Pointer& header,
                         const util::Hash::Pointer& body);

private:

    void registerPerSignal(const util::Hash::Pointer& header, size_t bodySize);

    void registerPerSlot(const util::Hash::Pointer& header, size_t bodySize);

    void registerLogMessage(size_t bodySize);

    void printStatistics(const util::Epochstamp& timeStamp,
                         float elapsedSeconds) const;

    /// The keys of StatsMap must either support
    /// ostream& operator<<(ostream&, const KeyType&),
    /// preferably with a width of 59 characters or there must be a specialisation
    /// of BrokerStatistics::printId for IdType=KeyType.
    template <typename StatsMap>
    Stats printStatistics(const StatsMap& statsMap,
                          const util::Epochstamp& timeStamp,
                          float elapsedSeconds) const;

    /// Helper of printStatistics.
    template <class IdType>
    void printLine(const IdType& id, const Stats& stats,
                   float elapsedSeconds) const;

    /// Helper of printLine (see also printStatistics).
    template <class IdType>
    void printId(std::ostream& out, const IdType& id) const;

    const util::TimeDuration m_interval;
    util::Epochstamp m_start;

    /// mapping SignalId to Stats
    SignalStatsMap m_signalStats;

    /// mapping SlotId to Stats
    SlotStatsMap m_slotStats;

    static const std::string m_delimLine;
};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

const std::string BrokerStatistics::m_delimLine
("===============================================================================\n");

////////////////////////////////////////////////////////////////////////////


template <class IdType>
void BrokerStatistics::printId(std::ostream& out, const IdType& id) const {
    out << std::left << std::setw(59) << id;
}


template <>
void BrokerStatistics::printId(std::ostream& out, const SignalId& id) const {
    out << std::left
            << std::setw(39) << id.first
            << std::setw(20) << id.second;
}


template <>
void BrokerStatistics::printId(std::ostream& out, const SlotId& id) const {
    // SlotId is a string with a colon
    std::vector<std::string> deviceSlot;
    boost::split(deviceSlot, id, boost::is_any_of(":"));

    const std::string& slotName = (deviceSlot.size() >= 2 ? deviceSlot[1] : "");
    out << std::left
            << std::setw(39) << deviceSlot[0]
            << std::setw(20) << slotName;

    // Create a warning if deviceSlot.size() > 2, at least once?
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::registerMessage(const util::Hash::Pointer& header,
                                       const util::Hash::Pointer& body) {
    try {
        // In the very first call we reset the start time.
        // Otherwise (if the constructor initialises m_start with 'now') starting this
        // tool and then starting the first device in the topic leads to wrongly low
        // rates.
        if (!m_start.getSeconds()) m_start.now();

        // Since we told the consumer to skip serialisation, there is just the raw data:
        const size_t bodySize = body->get<std::vector<char> >("raw").size();
        boost::optional<util::Hash::Node&> targetNode = header->find("target");
        if (targetNode && targetNode->is<std::string>() && targetNode->getValue<std::string>() == "log") {
            this->registerLogMessage(bodySize);
        } else {
            // Register for per sender and per (intended) receiver:
            this->registerPerSignal(header, bodySize);
            this->registerPerSlot(header, bodySize);
        }

        // Now it might be time to print and reset.
        // Since it is done inside registerMessage, one does not get any printout if
        // the watched topic is silent :-(. But then we do not need monitoring anyway.
        const util::Epochstamp now;
        const util::TimeDuration diff = now.elapsed(m_start);
        if (diff >= m_interval) {
            // Calculating in single float precision should be enough...
            const float elapsedSeconds = static_cast<float> (diff.getTotalSeconds())
                    + diff.getFractions(util::MICROSEC) / 1.e6f;
            this->printStatistics(now, elapsedSeconds);

            // Reset:
            m_start = now;
            m_signalStats.clear();
            m_slotStats.clear();
        }
    } catch (const std::exception& e) {
        std::cerr << "Problem registering message: " << e.what() << "\nheader:\n" << *header << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::registerPerSignal(const util::Hash::Pointer& header, size_t bodySize) {
    // Get who sent the message:
    const std::string& signalId = header->get<std::string>("signalInstanceId");
    const std::string& signalFunc = header->get<std::string>("signalFunction");

    // Find signal ID in map and increase statistics.
    const SignalId key(signalId, signalFunc);
    Stats& stats = m_signalStats[key];
    ++stats.first;
    stats.second += bodySize;
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::registerPerSlot(const util::Hash::Pointer& header, size_t bodySize) {
    // Get who sent the message, e.g.:
    // "slotFunctions": |DataLogger-Cam7_Proc:slotChanged||Karabo_GuiServer_0:_slotChanged|
    // Asynchronous replies do not have that key, so we use instead:
    // "slotInstanceIds": |DataLogger-Cam7_Proc||Karabo_GuiServer_0|
    boost::optional<util::Hash::Node&> funcNode = header->find("slotFunctions");
    const std::string& slots = (funcNode ? funcNode->getValue<std::string>()
                                : header->get<std::string>("slotInstanceIds"));

    std::vector<std::string> slotsVec;
    // token_compress_on: treat "||" as "|"
    boost::split(slotsVec, slots, boost::is_any_of("|"), boost::token_compress_on);

    for (std::vector<std::string>::const_iterator iSlot = slotsVec.begin(),
         iEnd = slotsVec.end(); iSlot != iEnd; ++iSlot) {
        if (iSlot->empty()) continue; // before first or after last '|'

        // Find slot ID in map and increase statistics.
        const SlotId& key = *iSlot;
        Stats& stats = m_slotStats[key];
        ++stats.first;
        stats.second += bodySize;
    }
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::registerLogMessage(size_t bodySize) {

    // We have no clue who sends or receives log messages.
    // Treat them as send and received once:
    const SignalId signalKey("?", "log");
    Stats& senderStats = m_signalStats[signalKey];
    ++senderStats.first;
    senderStats.second += bodySize;

    const SlotId senderKey("?:log");
    Stats& receiverStats = m_slotStats[senderKey];
    ++receiverStats.first;
    receiverStats.second += bodySize;
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::printStatistics(const util::Epochstamp& timeStamp,
                                       float elapsedSeconds) const {
    // Print kind of header
    std::cout << "\n" << m_delimLine << m_delimLine
            << std::setprecision(2) << std::fixed // 2 digits, keep 0s
            << timeStamp.toFormattedString() << " (UTC) - average over "
            << elapsedSeconds << " s:\n";


    std::cout << "Senders:\n" << m_delimLine.substr(0, 8) << "\n";
    Stats total = this->printStatistics(m_signalStats, timeStamp, elapsedSeconds);
    this->printLine(SignalId("Total sent", ""), total, elapsedSeconds);
    std::cout << m_delimLine;

    std::cout << "Receivers:\n" << m_delimLine.substr(0, 10) << "\n";
    total = this->printStatistics(m_slotStats, timeStamp, elapsedSeconds);
    this->printLine(SlotId("Total to be received"), total, elapsedSeconds);
    std::cout /*<< m_delimLine*/ << std::flush;
}

////////////////////////////////////////////////////////////////////////////


template <typename StatsMap>
BrokerStatistics::Stats
BrokerStatistics::printStatistics(const StatsMap& statsMap,
                                  const util::Epochstamp& timeStamp,
                                  float elapsedSeconds) const {
    // Iterators for looping through map of stats:
    typename StatsMap::const_iterator iter = statsMap.begin();
    const typename StatsMap::const_iterator end = statsMap.end();

    // Keep track of the one with highest rate:
    typename StatsMap::const_iterator iterQuickest = iter;

    // Sum messages and kBytes:
    unsigned int numTotal = 0;
    size_t kBytesTotal = 0;

    // Now loop and print for each signal:
    for (; iter != end; ++iter) {
        const Stats& stats = iter->second; // i.e. count & sum_of_bytes
        if (stats.first) { // i.e. if some counts
            this->printLine(iter->first, stats, elapsedSeconds);
        }
        numTotal += stats.first;
        kBytesTotal += stats.second;
        // Check whether iter is the 'Plappermaul' so far:
        if (stats.first > iterQuickest->second.first) {
            iterQuickest = iter;
        }
    }

    // Finally print the one with the highest rate again...
    if (iterQuickest != end) { // even if 0 Hz... (else '&& maxCount > 0')
        std::cout << "\nHighest rate was:\n";
        this->printLine(iterQuickest->first, iterQuickest->second, elapsedSeconds);
    }

    return Stats(numTotal, kBytesTotal);
}

////////////////////////////////////////////////////////////////////////////


template <class IdType>
void BrokerStatistics::printLine(const IdType& id, const Stats& stats,
                                 float elapsedSeconds) const {
    const float kBytes = stats.first ? stats.second / (1.e3f * stats.first) : 0.f;

    this->printId(std::cout, id);
    std::cout << ":"
            << std::right << std::setw(6) << stats.first / elapsedSeconds << " Hz,"
            << std::right << std::setw(6) << kBytes << " kB\n";
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


void printHelp(const char* name) {
    // Get name without leading directories
    std::string nameStr(name ? name : "'command'");
    const std::string::size_type lastSlashPos = nameStr.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        nameStr.replace(0, lastSlashPos + 1, "");
    }
    std::cout << "\n  " << nameStr << " [-h|--help] [interValSec]\n\n"
            << "Prints the rate and average size of all signals sent to the "
            << "broker and of\n"
            << "the intended calls of the slots that receive the signals.\n"
            << "Broker host and topic are read from the usual environment "
            << "variables\nKARABO_BROKER and KARABO_BROKER_TOPIC or, if "
            << "these are not defined, use\nthe usual defaults.\n"
            << "Optional argument is the time (in seconds) for averaging "
            << "(default: 20).\n"
            << std::endl;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


int main(int argc, char** argv) {

    util::TimeValue interval = 20ull; // in seconds
    if (argc > 1) {
        const std::string arg1(argv[1]);
        if (arg1 == "-h" || arg1 == "--help") {
            printHelp(argv[0]);
            return EXIT_SUCCESS;
        } else {
            const long int argument = strtol(argv[1], 0, 0);
            if (argument > 0) {
                interval = static_cast<util::TimeValue> (argument);
            } else { // Maybe not an unsigned integer or 0 passed.
                std::cerr << "Refusing time interval '" << argv[1] << "'." << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    std::string topic;
    char* env = getenv("KARABO_BROKER_TOPIC");
    if (env != 0) {
        topic = env;
    } else {
        env = getenv("USER");
        if (env != 0) topic = env;
    }
    if (topic.empty()) {
        std::cerr << "No topic specified since neither KARABO_BROKER_TOPIC nor USER set." << std::endl;
        return EXIT_FAILURE;
    }
    // Start Logger, but suppress INFO and DEBUG
    log::Logger::configure(util::Hash("priority", "WARN"));
    log::Logger::useOstream();

    try {
        // Create connection object
        net::JmsConnection::Pointer connection = boost::make_shared<net::JmsConnection>();
        connection->connect();

        std::string selector; // Could be made configurable as in broker MessageLogger.
        // 3rd argument true: skip serialisation (but get access to raw message size)!
        net::JmsConsumer::Pointer consumer = connection->createConsumer(topic, selector, true);

        std::cout << "\nStart monitoring signal and slot rates of \n   topic     '"
                << topic << "'\n   on broker '"
                << connection->getBrokerUrl() << "',\n"
                << "intervall is " << interval << " s."
                << std::endl;

        // Register our registration message as async reader:
        boost::shared_ptr<BrokerStatistics> stats(new BrokerStatistics(interval));
        consumer->startReading(boost::bind(&BrokerStatistics::registerMessage, stats, _1, _2));

        // Block forever
        net::EventLoop::work();

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
