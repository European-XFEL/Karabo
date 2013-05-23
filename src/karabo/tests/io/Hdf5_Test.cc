/* 
 * File:   Hdf5_Test.cc
 * Author: wrona
 * 
 * Created on March 1, 2013, 4:02 PM
 */

#include "Hdf5_Test.hh"
#include <hdf5/hdf5.h>
#include <karabo/util/Profiler.hh>
#include "TestPathSetup.hh"
#include <karabo/log/Tracer.hh>
#include <karabo/util/Dims.hh>

#include <karabo/io/h5/Table.hh>

using namespace karabo::util;
using namespace karabo::io::h5;
using namespace std;


CPPUNIT_TEST_SUITE_REGISTRATION(Hdf5_Test);


Hdf5_Test::Hdf5_Test() {

    karabo::log::Tracer tr;
    tr.disableAll();
    tr.reconfigure();

    m_numImages = 100; // number of images to be written
    m_extentMultiplier = 1; //image size multiplier: 1 means 1Mpx, 2 - 4Mpx, 3 - 9 Mpx, etc
    m_report = false;

}


Hdf5_Test::~Hdf5_Test() {
}


void Hdf5_Test::setUp() {

}


void Hdf5_Test::tearDown() {
}


void Hdf5_Test::testPureHdf5() {



    #define DET_NX 1024
    #define DET_NY 1024

    string filename = "/dev/shm/pure.h5"; // in memory file
    filename = resourcePath("pure.h5"); // file on disk ($KARABO/src/karabo/tests/io/resources/pure.h5)

    // end of configure




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


    unsigned int nx = m_extentMultiplier*DET_NX; //number of pixles along x-dimension
    unsigned int ny = m_extentMultiplier*DET_NY; //number of pixles along y-dimension
    const char *fname = filename.c_str();

    //compute the number of points in a single frame
    unsigned long long imageSize = nx*ny;
    //compute total data size in MBytes
    unsigned long long totalSize = imageSize * m_numImages * sizeof (unsigned short) / 1024 / 1024;

    Profiler p("write");
    p.start("allocate");
    //allocate memory for one image and set the data value
    data = (unsigned short*) malloc(sizeof (unsigned short) *imageSize);
    for (unsigned int i = 0; i < imageSize; ++i) data[i] = static_cast<unsigned short> (i % 10);

    p.stop("allocate");
    p.start("create");

    //create data file
    fid = H5Fcreate(fname, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    //create dtype
    tid = H5Tcopy(H5T_NATIVE_USHORT);

    //create data-space on disk
    // initially for zero images (dims[0] = 0). Needs to be extended before writing
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

    // used for navigation in file
    offset[0] = 0;
    offset[1] = 0;
    offset[2] = 0;
    counts[0] = 1;
    counts[1] = nx;
    counts[2] = ny;


    p.stop("create");
    p.start("write");


    for (unsigned int i = 0; i < m_numImages; i++) {
        H5Dset_extent(did, dims);
        sid = H5Dget_space(did);

        //set the selection
        offset[0] = i;
        H5Sselect_hyperslab(sid, H5S_SELECT_SET, offset, NULL, counts, NULL);
        //write data
        H5Dwrite(did, tid, msid, sid, H5P_DEFAULT, data);

        //flush data to disk
        H5Fflush(fid, H5F_SCOPE_LOCAL);


        dims[0]++;

    }

    p.stop("write");


    p.start("close");
    //close everything down
    H5Tclose(tid);
    H5Sclose(sid);
    H5Sclose(msid);
    H5Dclose(did);
    H5Pclose(pid);

    H5Fclose(fid);

    p.stop("close");

    free(data);

    double allocateTime = HighResolutionTimer::time2double(p.getTime("allocate"));
    double createTime = HighResolutionTimer::time2double(p.getTime("create"));
    double writeTime = HighResolutionTimer::time2double(p.getTime("write"));
    double closeTime = HighResolutionTimer::time2double(p.getTime("close"));

    if (m_report) {
        clog << endl;
        clog << "file : " << filename << endl;
        clog << "allocate memory                  : " << allocateTime << " [s]" << endl;
        clog << "open/prepare file                : " << createTime << " [s]" << endl;
        clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
        clog << "written data size                : " << totalSize << " [MB]" << endl;
        clog << "writing speed                    : " << totalSize / writeTime << " [MB/s]" << endl;
        clog << "close                            : " << closeTime << " [s]" << endl;
        clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
        clog << "write+close(flush to disk) speed : " << totalSize / (writeTime + closeTime) << " [MB/s]" << endl;

    }


}


void Hdf5_Test::testKaraboHdf5() {


    #define DET_NX 1024
    #define DET_NY 1024


    string filename = "/dev/shm/karabo.h5"; // in memory file
    filename = resourcePath("karabo.h5"); // file on disk ($KARABO/src/karabo/tests/io/resources/pure.h5)
    // end of configure


    unsigned int nx = m_extentMultiplier*DET_NX; //number of pixles along x-dimension
    unsigned int ny = m_extentMultiplier*DET_NY; //number of pixles along y-dimension


    //compute the number of points in a single frame
    unsigned long long imageSize = nx*ny;
    //compute total data size in MBytes
    unsigned long long totalSize = imageSize * m_numImages * sizeof (unsigned short) / 1024 / 1024;


    Profiler p("writeKarabo");

    p.start("allocate");
    //allocate memory for one image and set the data value   

    vector<unsigned short> data(imageSize);
    for (size_t i = 0; i < imageSize; ++i) data[i] = static_cast<unsigned short> (i % 10);

    Hash h;

    h.set("detector", data).setAttribute("dims", Dims(nx, ny).toVector());

    p.stop("allocate");

    p.start("create");
    
    Format::Pointer dataFormat = Format::discover(h);

    File file(filename);
    file.open(File::TRUNCATE);
    Table::Pointer t = file.createTable("/karabo", dataFormat, 1);
    p.stop("create");

    p.start("write");
    for (unsigned int i = 0; i < m_numImages; ++i)
        t->write(h, i);
    p.stop("write");


    p.start("close");
    t->close();

    // check if all objects are closed (apart from file) - requires making  m_h5file to be made temporary public in File.hh
    //    clog << "files    : " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_FILE) << endl;
    //    clog << "datasets : " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_DATASET)<< endl;
    //    clog << "datatypes: " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_DATATYPE)<< endl;
    //    clog << "attribute: " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_ATTR)<< endl;
    //    clog << "groups   : " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_GROUP)<< endl;
    //    clog << "all      : " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_ALL)<< endl;


    file.close();

    p.stop("close");

    double allocateTime = HighResolutionTimer::time2double(p.getTime("allocate"));
    double createTime = HighResolutionTimer::time2double(p.getTime("create"));
    double writeTime = HighResolutionTimer::time2double(p.getTime("write"));
    double closeTime = HighResolutionTimer::time2double(p.getTime("close"));



    if (m_report) {
        clog << endl;
        clog << "file: " << filename << endl;
        clog << "allocate memory                  : " << allocateTime << " [s]" << endl;
        clog << "open/prepare file                : " << createTime << " [s]" << endl;
        clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
        clog << "written data size                : " << totalSize << " [MB]" << endl;
        clog << "writing speed                    : " << totalSize / writeTime << " [MB/s]" << endl;
        clog << "close                            : " << closeTime << " [s]" << endl;
        clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
        clog << "write+close(flush to disk) speed : " << totalSize / (writeTime + closeTime) << " [MB/s]" << endl;
    }

}


void Hdf5_Test::testManyDatasets() {




    string filename = "/dev/shm/pureManyDs.h5"; // in memory file
    filename = resourcePath("pureManyDs.h5"); // file on disk ($KARABO/src/karabo/tests/io/resources/pure.h5)

    // end of configure




    hsize_t dims[3], maxdims[3];
    hsize_t mdims[2];
    //hsize_t offset[3], counts[3];
    hid_t fid; //file ID
    hid_t tid; //type ID
    hid_t did; //dataset ID
    hid_t sid; //dataspace ID
    hid_t pid; //dataset creation property list
    hid_t msid; //memory data space ID
    unsigned short *data;


    unsigned int nx = m_extentMultiplier*DET_NX; //number of pixles along x-dimension
    unsigned int ny = m_extentMultiplier*DET_NY; //number of pixles along y-dimension
    const char *fname = filename.c_str();

    //compute the number of points in a single frame
    unsigned long long imageSize = nx*ny;
    //compute total data size in MBytes
    unsigned long long totalSize = imageSize * m_numImages * sizeof (unsigned short) / 1024 / 1024;

    Profiler p("write");
    p.start("allocate");
    //allocate memory for one image and set the data value
    data = (unsigned short*) malloc(sizeof (unsigned short) *imageSize);
    for (unsigned int i = 0; i < imageSize; ++i) data[i] = static_cast<unsigned short> (i % 10);

    p.stop("allocate");
    p.start("create");

    //create data file
    fid = H5Fcreate(fname, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    //create dtype
    tid = H5Tcopy(H5T_NATIVE_UINT);

    //create data-space on disk
    // initially for zero images (dims[0] = 0). Needs to be extended before writing
    dims[0] = 0;
    dims[1] = 1;
    maxdims[0] = H5S_UNLIMITED;
    maxdims[1] = 1;

    sid = H5Screate_simple(2, dims, maxdims);
    //create data-space in memory
    mdims[0] = 1;
    mdims[1] = 1;
    msid = H5Screate_simple(2, mdims, mdims);


    //create the datasets

    for (size_t i = 0; i < 100000L; ++i) {
        //create the property list for the dataset
        pid = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_layout(pid, H5D_CHUNKED);
        dims[0] = 1;
        H5Pset_chunk(pid, 2, dims);

        hid_t lid = H5Pcreate(H5P_LINK_CREATE);
        
        H5Pset_create_intermediate_group( lid, 1 );
        
        string ds = "/base/c1/de/tec/tor" + toString(i);
        did = H5Dcreate2(fid, ds.c_str(), tid, sid, lid, pid, H5P_DEFAULT);
        H5Pclose(lid);
    }
    p.stop("create");

    // used for navigation in file
    //    offset[0] = 0;
    //    offset[1] = 0;
    //    offset[2] = 0;
    //    counts[0] = 1;
    //    counts[1] = nx;
    //    counts[2] = ny;
    //
    //
    //    p.stop("create");
    //    p.start("write");
    //
    //
    //    for (unsigned int i = 0; i < m_numImages; i++) {
    //        H5Dset_extent(did, dims);
    //        sid = H5Dget_space(did);
    //
    //        //set the selection
    //        offset[0] = i;
    //        H5Sselect_hyperslab(sid, H5S_SELECT_SET, offset, NULL, counts, NULL);
    //        //write data
    //        H5Dwrite(did, tid, msid, sid, H5P_DEFAULT, data);
    //
    //        //flush data to disk
    //        H5Fflush(fid, H5F_SCOPE_LOCAL);
    //
    //
    //        dims[0]++;
    //
    //    }

    p.stop("write");


    p.start("close");
    //close everything down
    H5Tclose(tid);
    H5Sclose(sid);
    H5Sclose(msid);
    H5Dclose(did);
    H5Pclose(pid);

    H5Fclose(fid);

    p.stop("close");

    free(data);

    double allocateTime = HighResolutionTimer::time2double(p.getTime("allocate"));
    double createTime = HighResolutionTimer::time2double(p.getTime("create"));
    double writeTime = HighResolutionTimer::time2double(p.getTime("write"));
    double closeTime = HighResolutionTimer::time2double(p.getTime("close"));

    if (m_report) {
        clog << endl;
        clog << "file : " << filename << endl;
        clog << "allocate memory                  : " << allocateTime << " [s]" << endl;
        clog << "open/prepare file                : " << createTime << " [s]" << endl;
        clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
        clog << "written data size                : " << totalSize << " [MB]" << endl;
        clog << "writing speed                    : " << totalSize / writeTime << " [MB/s]" << endl;
        clog << "close                            : " << closeTime << " [s]" << endl;
        clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
        clog << "write+close(flush to disk) speed : " << totalSize / (writeTime + closeTime) << " [MB/s]" << endl;

    }



}