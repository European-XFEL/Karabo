/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   main.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on July 28, 2015, 12:11 PM
 */

#include <boost/system/error_code.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <karabo/util/DataLogUtils.hh>
#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/types/Exception.hh"


using namespace std;
namespace bf = std::filesystem;
namespace bs = boost::system;
using namespace karabo::data;
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
    if (!mf.is_open()) throw KARABO_PARAMETER_EXCEPTION("File \"" + string(argv[1]) + "\" -- failed to open");

    size_t nrecs = fsize / sizeof(MetaData::Record);
    cout << "#record\t"
         << "timestamp\t"
         << "\ttrainId\t"
         << "rawpos\t"
         << "extent1\t"
         << "extent2\t" << endl;
    for (size_t i = 0; i < nrecs; ++i) {
        MetaData::Record record;
        mf.read((char*)&record, sizeof(MetaData::Record));
        size_t seconds = std::floor(record.epochstamp);
        size_t attosec = std::floor((record.epochstamp - seconds) * 1000000000000000000);
        Epochstamp epoch(seconds, attosec);
        cout << dec << i << "\t" << epoch.toIso8601Ext() << "\t" << record.trainId << "\t" << record.positionInRaw
             << "\t" << hex << "0x" << record.extent1 << "\t" << hex << "0x" << record.extent2 << endl;
    }

    mf.close();

    return 0;
}
