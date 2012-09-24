/*
 * $Id$
 *
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include "../Test.hh"

#include "../Profiler.hh"
#include "../Timer.hh"

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

using namespace std;
using namespace exfel::util;

int testProfiler(int argc, char** argv) {

    try {

        Test t;
        TEST_INIT(t, argc, argv);

        cout << t << endl;
        // use t.file("filename"); to access file


        Profiler p("Test profiler");

        const size_t MEM_SIZE = 1024 * 1024 * 16;

        p.start("allocate memory");

        char* buffer = new char [MEM_SIZE];

        memset(buffer, 0, MEM_SIZE);
        p.stop();

        //usleep(50000);

        char alphabet[] = "abcdefghijklmnopqrstuvwxyz";

        p.start("init memory");

        for (size_t i = 0; i < MEM_SIZE; ++i) {
            buffer[i] = alphabet[rand() % 26];
        }
        p.stop();

        //usleep(25000);
        
        p.start("search memory");
        size_t count = 0L;
        for (size_t i = 0; i < 5; ++i) {

            char str[4];

            for (size_t j = 0; j < 4; ++j) {
                str[j] = alphabet[rand() % 26];
            }

            p.start();
            //volatile char* res = (char*) memmem(buffer, MEM_SIZE, str, 4);
            p.stop();
            
            //if(res != 0)
            //    count++;           
        }
        p.stop();
       
        cout << "# of successful searches: " << 5-count << endl;

        cout << "Global time: " << HighResolutionTimer::format(p.getGlobalTime(), "%s.%n") << endl;
        cout << "Effective time: " << HighResolutionTimer::format(p.getEffectiveTime(), "%s.%n") << endl;
        cout << p.report() << endl;
        
        cout << "Total search time: " << HighResolutionTimer::format(p.getTime("search memory"), "%s.%n") << endl;

        assert(HighResolutionTimer::time2int(HighResolutionTimer::now()) != HighResolutionTimer::time2int(HighResolutionTimer::now()));

        assert(HighResolutionTimer::time2int(p.getGlobalTime()) > HighResolutionTimer::time2int(p.getEffectiveTime()));

        // Test invalid period name
        assert(HighResolutionTimer::time2int(p.getTime("delete memory")) == 0L);

        delete [] buffer;

    } catch (Exception e) {
        cout << e;
        RETHROW
    }

    return 0;
}

