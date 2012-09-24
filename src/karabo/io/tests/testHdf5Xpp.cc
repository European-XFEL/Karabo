/*
 * $Id: testHdf5Xpp.cc 5260 2012-02-26 22:13:16Z wrona $
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
#include <exfel/util/Hash.hh>
#include <exfel/util/Profiler.hh>

#include "../hdf5/Column.hh"
#include "../hdf5/Table.hh"
#include "../hdf5/File.hh"
#include "../hdf5/DataTypes.hh"
#include "../ArrayView.hh"
#include "../Reader.hh"


//#define XPP_PRINT 1

using namespace std;
using namespace exfel::util;
using namespace exfel::io;
using namespace exfel::io::hdf5;

int testHdf5Xpp(int argc, char** argv) {

    try {

	Test t;
	TEST_INIT(t, argc, argv);

	cout << t << endl;

	exit(0);


	string filename = "/diskmnt/a/wrona/xpp34511-r0260.h5";
	
	string group, dataset;	
	int accessCase = 1;
	
	if( accessCase == 1 ){
	 group = "/Configure:0000/Run:0000/CalibCycle:0000/CsPad::ElementV2/XppGon.0:Cspad.0";
	 dataset = "data";
	}else if (accessCase == 2 ){
	 group = "/Configure:0000/Run:0000/CalibCycle:0000/CsPad::ElementV2";
	 dataset = "XppGon.0:Cspad.0/data";
	} else {
	 group = "/Configure:0000/Run:0000/CalibCycle:0000"; 
	 dataset = "CsPad::ElementV2/XppGon.0:Cspad.0/data";
	}

	Hash readerConfig;
	readerConfig.setFromPath("TextFile.filename", t.file("XppData.xml"));
	Reader<Hash>::Pointer formatReader = Reader<Hash>::create(readerConfig);

	Hash dataFormatConfig;
	formatReader->read(dataFormatConfig);


	DataFormat::Pointer dataFormat = DataFormat::create(dataFormatConfig);

	cout << "Reading data... " << endl;

	{
	    Profiler p("xpp");

	    p.start("open");
	    
	    // open file in read only mode
	    File fileRead(filename);
	    fileRead.open(File::READONLY);
	    cout << "File " << filename << " opened" << endl;

	    // open table
	    Table::Pointer tableRead = fileRead.getTable(group);
	    
	    // dataset name needs to be given as relative to the table (group). 
	    // see above 3 different access cases
	    Column<ArrayView<short> > data(dataset, tableRead);
	    
	    // calculate number of all records in the file
	    size_t nRecords = tableRead->getNumberOfRecords();	    
	    p.stop();
	    
	    
	    p.start("read");
	    for (size_t i = 0; i < nRecords; ++i) {		
		if (!(i%100)) cout << "record id: " << i << endl;
		// get data from record number i
		// this access pattern uses buffer (cache) to read entire chunk of data
		ArrayView<short> av = data[i];
				
		vector<ArrayView<short> > vec;
		// convert 3-dim array to vector of 2-dim arrays
		av.getVectorOfArrayViews(vec);
		
		for (size_t j = 0; j < vec.size(); ++j) {
		    ArrayView<short> av1 = vec[j];
		    ArrayDimensions av1Dims = av1.getDims();
#ifdef XPP_PRINT
		    cout << "vec[" << j << "]: " << av1Dims[0] << ", " << av1Dims[1] << endl;
		    short* data = av1.getData();
		    for (size_t m = 0; m < 2 /*av1Dims[0]*/; ++m) {
			for (size_t n = 0; n < av1Dims[1]/*av1Dims[1]*/; ++n) {
			    cout << data[ m * av1Dims[1] + n ] << " "; //
			}
			cout << endl;
		    }
		    exit(0);
#endif		    
		}
	    }
	    p.stop();
	    p.start("close");
	    tableRead->close();
	    fileRead.close();
	    p.stop();

	    std::cout << "open  : " << HighResolutionTimer::time2string(p.getTime("open")) << std::endl;
	    std::cout << "read  : " << HighResolutionTimer::time2string(p.getTime("read")) << std::endl;
	    std::cout << "close : " << HighResolutionTimer::time2string(p.getTime("close")) << std::endl;

	    long long sizeDataRead = nRecords * 32 * 185 * 388 * sizeof (short) / 1024 / 1024;
	    cout << "read rate : " << sizeDataRead / (HighResolutionTimer::time2double(p.getTime("read"))) << " [MB/s]" << endl;

	}

    } catch (Exception e) {
	cout << e;
	RETHROW
    }

    return 0;


}
