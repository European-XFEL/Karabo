/* 
 * File:   Hdf5_Test.cc
 * Author: wrona
 * 
 * Created on March 1, 2013, 4:02 PM
 */

#include "Hdf5_Test.hh"
#include <hdf5/hdf5.h>

#include<stdlib.h>
#include<stdio.h>
#include<time.h>

#include <karabo/util/Profiler.hh>

using namespace karabo::util;
using namespace std;


CPPUNIT_TEST_SUITE_REGISTRATION(Hdf5_Test);

Hdf5_Test::Hdf5_Test() {
}

Hdf5_Test::~Hdf5_Test() {
}

void Hdf5_Test::setUp() {

}

void Hdf5_Test::tearDown() {
}

void Hdf5_Test::testPureHdf5() {

    return;
    
    #define DET_NX 1024
    #define DET_NY 1024


    hsize_t dims[3], maxdims[3];
    hsize_t mdims[2];
    hsize_t offset[3], counts[3];
    hid_t fid; //file ID
    hid_t tid; //type ID
    hid_t did; //dataset ID
    hid_t sid; //dataspace ID
    hid_t pid; //dataset creation property list
    hid_t msid; //memory data space ID
    unsigned short *data;
    unsigned long i = 0;
    time_t start, stop;
    unsigned int nx, ny;
    double etime;
    double totsize;
    double fsize;
    int smult = 1;
    unsigned int npoints = 0;
    //unsigned short value = 0;
    const char *fname;

    //read command line arguments
    smult = 1; // 1Mpxl atoi(argv[1]); //read the frame size multiplier
    nx = smult*DET_NX; //number of pixles along x-dimension
    ny = smult*DET_NY; //number of pixles along y-dimension
    npoints = 100; //(unsigned int) (atoi(argv[2])); //read the number of points
    //value = 25; // (unsigned short) (atoi(argv[3])); //read the data value to store
    fname = "/dev/shm/pure.h5"; //argv[4];
    fname = "pure.h5"; //argv[4];
    //compute the number of points in a single frame
    fsize = nx*ny;
    //compute total data size in MBytes
    totsize = fsize * npoints * sizeof (unsigned short) / 1024 / 1024;

    //allocate memory and set the data value
    data = (unsigned short*) malloc(sizeof (unsigned short) *fsize);
    for (i = 0; i < fsize; i++) data[i] = static_cast<unsigned short> (i % 10);
    //create data file
    fid = H5Fcreate(fname, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    //create dtype
    tid = H5Tcopy(H5T_NATIVE_USHORT);

    //create data-space on disk
    dims[0] = 0;
    dims[1] = nx;
    dims[2] = ny;
    maxdims[0] = H5S_UNLIMITED;
    maxdims[1] = nx;
    maxdims[2] = ny;
    sid = H5Screate_simple(3, dims, maxdims);
    //create data-space in memory
    mdims[0] = nx;
    mdims[1] = ny;
    msid = H5Screate_simple(2, mdims, mdims);

    //create the property list for the dataset
    pid = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_layout(pid, H5D_CHUNKED);
    dims[0] = 1;
    H5Pset_chunk(pid, 3, dims);

    //create the dataset
    did = H5Dcreate2(fid, "detector", tid, sid, H5P_DEFAULT, pid, H5P_DEFAULT);

    //write data
    offset[0] = 0;
    offset[1] = 0;
    offset[2] = 0;
    counts[0] = 1;
    counts[1] = nx;
    counts[2] = ny;
    start = time(NULL);


    Profiler p("write");
    p.start("write");
    
    for (i = 0; i < npoints; i++) {
        if ((i % 100) == 0) fprintf(stdout, "point %lu ...\n", i);
        //resize the dataset
        dims[0]++;
        H5Dset_extent(did, dims);
        sid = H5Dget_space(did);

        //set the selection
        offset[0] = i;
        H5Sselect_hyperslab(sid, H5S_SELECT_SET, offset, NULL, counts, NULL);
        //write data
        H5Dwrite(did, tid, msid, sid, H5P_DEFAULT, data);

        //flush data to disk
        H5Fflush(fid, H5F_SCOPE_LOCAL);
    }
    p.stop();
    clog << "Total write: " << HighResolutionTimer::time2double( p.getTime("write") ) << endl;
    stop = time(NULL);
    etime = difftime(stop, start);
    fprintf(stdout, "Elapsed time: %e (sec.) %f (Mbyte/sec) %f (fps)\n", etime, ((double) totsize) / etime, npoints / etime);

    //close everything down
    H5Tclose(tid);
    H5Sclose(sid);
    H5Sclose(msid);
    H5Dclose(did);
    H5Pclose(pid);

    free(data);

    H5Fclose(fid);


}
