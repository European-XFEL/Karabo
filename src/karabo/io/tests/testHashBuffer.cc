/*
 * $Id: testHashBuffer.cc 6608 2012-06-25 19:41:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <exfel/util/Test.hh>
#include <exfel/util/Time.hh>
#include <exfel/util/Hash.hh>
#include <exfel/util/Profiler.hh>


using namespace std;
using namespace exfel::util;

#define report(p,name) std::cout << name << ": " << exfel::util::HighResolutionTimer::time2double(p.getTime(name)) << std::endl

int testHashBuffer(int argc, char** argv) {

    try {

	Test t;
	TEST_INIT(t, argc, argv);

	cout << t << endl;
	// use t.file("filename"); to access file

	{
	    size_t nRecords = 1024 * 1024 * 25; //25mln records of int (4 bytes) =>  100MB


            Profiler p("test");
	    vector<int> vec(nRecords, 0);

	    
            p.start("vector initialization");
	    for (size_t i = 0; i < nRecords; ++i) {
		vec[i] = i;
	    }
            p.stop();

            p.start("copying vector");
	    Hash h;
	    h.set("abc", vec);
            p.stop();
	    
            p.start("accessing by reference");
	    Hash::iterator it = h.find("abc");
            vector<int>& vecRef = h.get<vector<int> >(it);
            int value = 0;
            for (size_t i = 0; i < nRecords; ++i) {		
		value = vecRef[i];
                assert(value == static_cast<int>(i));                
	    }           
            p.stop();
            
            report(p,"vector initialization");
            report(p,"copying vector");
            report(p,"accessing by reference");            

	}

    } catch (Exception e) {
	cout << e;
	RETHROW
    }

    return 0;
}

