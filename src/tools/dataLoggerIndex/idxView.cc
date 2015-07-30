/* 
 * File:   main.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on July 28, 2015, 12:11 PM
 */

#include <cstdlib>
#include <iostream>
#include <boost/filesystem.hpp>
#include <karabo/util/Epochstamp.hh>
#include <karabo/util/Exception.hh>
#include <karabo/core/DataLoggerStructs.hh>


namespace bf = boost::filesystem;
namespace bs = boost::system;
using namespace std;
using namespace karabo::core;
using namespace karabo::util;

/*
 * 
 */
int main(int argc, char** argv) {
    
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <binary index file>" << endl;
        return 1;
    }
    
    bs::error_code ec;
    size_t fsize = bf::file_size(argv[1], ec);
    if (ec) throw KARABO_PARAMETER_EXCEPTION("File \"" + string(argv[1]) + "\" -- " + ec.message());
    cout << "File \"" << argv[1] << "\" has size = " << fsize << " in hex: 0x" << hex << fsize << endl; 

    ifstream mf(argv[1], ios::in | ios::binary);
    if (!mf.is_open())
        throw KARABO_PARAMETER_EXCEPTION("File \"" + string(argv[1]) + "\" -- failed to open");
    
    size_t nrecs = fsize / sizeof (MetaData::Record);
    cout << "#record\t" << "timestamp\t" << "\ttrainId\t" << "rawpos\t" << "extent1\t" << "extent2\t" << endl;
    for (size_t i = 0; i < nrecs; ++i) {
        MetaData::Record record;
        mf.read((char*)&record, sizeof (MetaData::Record));
        size_t seconds = std::floor(record.epochstamp);
        size_t attosec = std::floor((record.epochstamp - seconds) * 1000000000000000000);
        Epochstamp epoch(seconds, attosec);
        cout << dec << i << "\t"
                << epoch.toIso8601Ext() << "\t"
                << record.trainId << "\t"
                << record.positionInRaw << "\t"
                << hex << "0x" << record.extent1 << "\t"
                << hex << "0x" << record.extent2 << endl;
    }
    
    mf.close();
    
    return 0;
}
