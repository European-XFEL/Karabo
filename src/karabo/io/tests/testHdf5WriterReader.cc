/*
 * $Id: testHdf5WriterReader.cc 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <boost/shared_array.hpp>
#include <exfel/util/Test.hh>
#include <exfel/util/Time.hh>
#include <exfel/util/Hash.hh>
#include <exfel/util/CArray.hh>
#include "../hdf5/Table.hh"
#include "../hdf5/File.hh"
#include "../hdf5/FLArrayFilter.hh"
#include "../hdf5/DataTypes.hh"
#include "../ArrayView.hh"
#include "../Writer.hh"
#include "../Reader.hh"


using namespace std;
using namespace exfel::util;
using namespace exfel::io;
using namespace exfel::io::hdf5;

int testHdf5WriterReader(int argc, char** argv) {

    try {

	Test t;
	TEST_INIT(t, argc, argv);

	cout << t << endl;

	std::cout << "InputDataType registry: \n";
	std::cout << GenericFactory< Factory<DataTypes> >::getInstance().getKeysAsString();

	std::cout << "InputData<float> registry: \n";
	std::cout << GenericFactory< Factory<FLArrayFilter<float> > >::getInstance().getKeysAsString();


	int pid = getpid();
	int repeat = 10;
	string filename = "writerReader.h5";
	
	File file(t.file(filename));
	file.open(File::TRUNCATE);

	// define vector of 1 048 576 elements and fill it with consecutive numbers 
	size_t viSize = 1024 * 1024;
	vector<int> vi(viSize);
	for (size_t i = 0; i < viSize; ++i) {
	    vi[i] = i + pid;
	}


	// define array dimensions ( 1dim of size 1048576 )
	// ArrayDimensions dims(vi.size());

	// define array dimensions ( 2dim of sizes [1024,1024] )
	ArrayDimensions dims(1024, 1024);
	// define array corresponding view
	Hash data("ArrayViewInt", ArrayView<int>(vi, dims));
	data.set("pid", pid);
	data.setFromPath("a.b.c", "blabla");


	DataFormat::Pointer dataFormat;
	try {
	    dataFormat = DataFormat::discoverFromData(data);
	} catch (...) {
	    RETHROW;
	}


	{
	    // write data format configuration to a file: writerReader.xml
	    Hash dataFormatConfig = dataFormat->getConfig();
	    cout << "dataFormatConfig: " << endl << dataFormatConfig << endl;
	    Writer<Hash>::Pointer writerConfig = Writer<Hash>::create(Hash("TextFile.filename", "writerReader.xml"));
	    writerConfig->write(dataFormatConfig);
	}


	// create a table called /"test" using the dataFormat discovered
	Table::Pointer table = file.createTable("/test", dataFormat);

	cout << "table created " << endl;
	
	cout << "Start appending... " << endl;
	long long t1 = exfel::util::Time::getMsSinceEpoch();
	for (int i = 0; i < repeat; ++i) {
	    table->append(data);
	}
	long long t2 = exfel::util::Time::getMsSinceEpoch();
	cout << 1 /*vec and arrayView*/ * repeat * viSize * sizeof (int) / 1024 / 1024 << " [MB] written in: " << (t2 - t1) / 1000.0 << " [s]" << endl;

	file.close();

	cout << "Reading data... " << endl;
	long long t3 = exfel::util::Time::getMsSinceEpoch();

	File fileRead(t.file(filename));
	fileRead.open(File::READONLY);

	Table::Pointer tableRead = fileRead.getTable("/test");

	Hash dataRead;
	tableRead->allocate(dataRead);

	size_t nRecords = tableRead->getNumberOfRecords();
	for (size_t i = 0; i < nRecords; ++i) {
	    tableRead->read(dataRead, i);
	    ArrayView<int> arrayRead = dataRead.get<ArrayView<int> >("ArrayViewInt");
	    assert(arrayRead.getNumDims() == 2);
	    int* ptr = &arrayRead[0];
	    assert(ptr[0] == (0 + pid));
	    assert(ptr[10 * 1024 + 8] == (10248 + pid));

	}
	tableRead->close();
	fileRead.close();
	long long t4 = exfel::util::Time::getMsSinceEpoch();
	cout << 1 /*vec and arrayView*/ * nRecords * viSize * sizeof (int) / 1024 / 1024 << " [MB] read in: " << (t4 - t3) / 1000.0 << " [s]" << endl;

    } catch (Exception e) {
	cout << e;
	RETHROW
    }

    return 0;


}
