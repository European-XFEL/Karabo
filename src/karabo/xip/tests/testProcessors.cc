/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 31, 2011, 2:01 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <stdlib.h>
#include <iostream>

#include <exfel/util/Test.hh>

#include "../SingleProcessor.hh"
#include "../CpuImage.hh"

using namespace exfel::util;
using namespace exfel::xip;
using namespace std;


int testProcessors(int argc, char** argv) {

    try {

        Test t;
        TEST_INIT(t, argc, argv);
        cout << t << endl;

        /***************************************
         *          Noise - Poisson            *
         ***************************************/

        {
            CpuImgD img1(8, 8, 1, 1.0);

            Hash h("Noise.type.Poisson");
            SingleProcessor<CpuImgD>::Pointer p = SingleProcessor<CpuImgD>::create(h);
            img1.print("Before Poisson");
            p->processInPlace(img1);
            img1.print("After Poisson");
        }

        /***************************************
         *          Noise - Gaussian           *
         ***************************************/

        {
            CpuImgD img1(8, 8, 1, 1.0);
            img1.setHeader(Hash("testParameter", "testValue"));

            Hash h("Noise.type.Gaussian.sigma", 1.0);
            SingleProcessor<CpuImgD>::Pointer p = SingleProcessor<CpuImgD>::create(h);
            img1.print("Before Gaussian");
            p->processInPlace(img1);
            img1.print("After Gaussian");
        }



    } catch (...) {
        RETHROW
    }


    return (EXIT_SUCCESS);
}

