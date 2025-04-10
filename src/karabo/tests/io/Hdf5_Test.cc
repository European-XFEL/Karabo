/* 
 * File:   Hdf5_Test.cc
 * Author: wrona
 * 
 * Created on March 1, 2013, 4:02 PM
 */

#include "Hdf5_Test.hh"
#include <hdf5/hdf5.h>
#include <karabo/util/TimeProfiler.hh>
#include <karabo/util/TimeDuration.hh>
#include "TestPathSetup.hh"
#include "karabo/io/Input.hh"
#include <karabo/log/Tracer.hh>
#include <karabo/util/Dims.hh>
#include <karabo/io/Output.hh>
#include <karabo/io/h5/Table.hh>

using namespace karabo::util;
using namespace karabo::io;
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

    TimeProfiler p("write");
    p.open();
    p.startPeriod("allocate");
    //allocate memory for one image and set the data value
    data = (unsigned short*) malloc(sizeof (unsigned short) *imageSize);
    for (unsigned int i = 0; i < imageSize; ++i) data[i] = static_cast<unsigned short> (i % 10);

    p.stopPeriod("allocate");
    p.startPeriod("create");

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


    p.stopPeriod("create");
    p.startPeriod("write");


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

    p.stopPeriod("write");


    p.startPeriod("close");
    //close everything down
    H5Tclose(tid);
    H5Sclose(sid);
    H5Sclose(msid);
    H5Dclose(did);
    H5Pclose(pid);

    H5Fclose(fid);

    p.stopPeriod("close");
    p.close();
    free(data);

    TimeDuration allocateTime = p.getPeriod("allocate").getDuration();
    TimeDuration createTime = p.getPeriod("create").getDuration();
    TimeDuration writeTime = p.getPeriod("write").getDuration();
    TimeDuration closeTime = p.getPeriod("close").getDuration();

    if (false) {
        clog << endl;
        clog << "file : " << filename << endl;
        clog << "allocate memory                  : " << allocateTime << " [s]" << endl;
        clog << "open/prepare file                : " << createTime << " [s]" << endl;
        clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
        clog << "written data size                : " << totalSize << " [MB]" << endl;
        //clog << "writing speed                    : " << totalSize / writeTime << " [MB/s]" << endl; //TODO
        clog << "close                            : " << closeTime << " [s]" << endl;
        clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
        // clog << "write+close(flush to disk) speed : " << totalSize / (writeTime + closeTime) << " [MB/s]" << endl; //TODO

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


    TimeProfiler p("writeKarabo");
    p.open();
    p.startPeriod("allocate");
    //allocate memory for one image and set the data value   

    vector<unsigned short> data(imageSize);
    for (size_t i = 0; i < imageSize; ++i) data[i] = static_cast<unsigned short> (i % 10);

    Hash h;

    h.set("detector", data).setAttribute("dims", Dims(nx, ny).toVector());

    p.stopPeriod("allocate");

    p.startPeriod("create");

    Format::Pointer dataFormat = Format::discover(h);

    File file(filename);
    file.open(File::TRUNCATE);
    Table::Pointer t = file.createTable("/karabo", dataFormat);
    p.stopPeriod("create");

    p.startPeriod("write");
    for (unsigned int i = 0; i < m_numImages; ++i)
        t->write(h, i);
    p.stopPeriod("write");


    p.startPeriod("close");
    file.closeTable(t);

    // check if all objects are closed (apart from file) - requires making  m_h5file to be made temporary public in File.hh
    //    clog << "files    : " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_FILE) << endl;
    //    clog << "datasets : " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_DATASET)<< endl;
    //    clog << "datatypes: " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_DATATYPE)<< endl;
    //    clog << "attribute: " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_ATTR)<< endl;
    //    clog << "groups   : " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_GROUP)<< endl;
    //    clog << "all      : " << H5Fget_obj_count(file.m_h5file, H5F_OBJ_ALL)<< endl;


    file.close();

    p.stopPeriod("close");
    p.close(); 
    TimeDuration allocateTime = p.getPeriod("allocate").getDuration();
    TimeDuration createTime = p.getPeriod("create").getDuration();
    TimeDuration writeTime = p.getPeriod("write").getDuration();
    TimeDuration closeTime = p.getPeriod("close").getDuration();



    if (m_report) {
        clog << endl;
        clog << "file: " << filename << endl;
        clog << "allocate memory                  : " << allocateTime << " [s]" << endl;
        clog << "open/prepare file                : " << createTime << " [s]" << endl;
        clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
        clog << "written data size                : " << totalSize << " [MB]" << endl;
        clog << "writing speed                    : " << totalSize / double(writeTime) << " [MB/s]" << endl;
        clog << "close                            : " << closeTime << " [s]" << endl;
        clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
        clog << "write+close(flush to disk) speed : " << totalSize / double(writeTime + closeTime) << " [MB/s]" << endl;
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
//    unsigned long long totalSize = imageSize * m_numImages * sizeof (unsigned short) / 1024 / 1024;

    TimeProfiler p("write");
    p.open();
    p.startPeriod("allocate");
    //allocate memory for one image and set the data value
    data = (unsigned short*) malloc(sizeof (unsigned short) *imageSize);
    for (unsigned int i = 0; i < imageSize; ++i) data[i] = static_cast<unsigned short> (i % 10);

    p.stopPeriod("allocate");
    p.startPeriod("create");

    //create data file
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
    fid = H5Fcreate(fname, H5F_ACC_TRUNC, H5P_DEFAULT, fapl);

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
    size_t max = 10000L;
    vector<hid_t> vdid(max, 0);
    for (size_t i = 0; i < max; ++i) {
        //create the property list for the dataset
        pid = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_layout(pid, H5D_CHUNKED);
        dims[0] = 1;
        H5Pset_chunk(pid, 2, dims);

        hid_t lid = H5Pcreate(H5P_LINK_CREATE);

        H5Pset_create_intermediate_group(lid, 1);

        string ds = "/base/c1/de/tec/tor" + toString(i);
        vdid[i] = H5Dcreate2(fid, ds.c_str(), tid, sid, lid, pid, H5P_DEFAULT);

        H5Dclose(vdid[i]);
        H5Pclose(lid);
        H5Pclose(pid);
    }
    hid_t scalar_id = H5Screate(H5S_SCALAR);
    for (size_t i = 0; i < max; ++i) {
        string ds = "/base/c1/de/tec/tor" + toString(i);
        vdid[i] = H5Dopen(fid, ds.c_str(), H5P_DEFAULT);
        if (vdid[i] < 0) {
            clog << "Error opening dataset " << i << endl;
        }
        for (int j = 0; j < 2; j++) {
            ostringstream oss;
            oss << "Attr" << j;
            hid_t attr_id = H5Acreate2(vdid[i], oss.str().c_str(), H5T_NATIVE_INT, scalar_id, H5P_DEFAULT, H5P_DEFAULT);
            H5Awrite(attr_id, H5T_NATIVE_INT, &j);
            H5Aclose(attr_id);
        }
        //    }
        //    for (size_t i = 0; i < max; ++i) {
        H5Dclose(vdid[i]);
    }


    p.stopPeriod("create");

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

    //p.stopPeriod("write");


    p.startPeriod("close");
    //close everything down
    H5Tclose(tid);
    H5Sclose(sid);
    H5Sclose(msid);
    //    H5Dclose(did);
    //    H5Pclose(pid);

    //    H5Fclose(fid);

    // H5Fopen("pure.h5");
    p.stopPeriod("close");

    free(data);

    p.startPeriod("open");

    for (size_t i = 0; i < 100L; ++i) {

        string ds = "/base/c1/de/tec/tor" + toString(i);
        did = H5Dopen(fid, ds.c_str(), H5P_DEFAULT);

        hid_t scalar_id = H5Screate(H5S_SCALAR);

        for (int j = 0; j < 2; j++) {
            ostringstream oss;
            oss << "Attr" << j;
            hid_t attr_id = H5Aopen(did, oss.str().c_str(), H5P_DEFAULT);
            H5Aclose(attr_id);
        }
        H5Dclose(did);
        H5Sclose(scalar_id);
    }
    p.stopPeriod("open");
    p.close();

    H5Fclose(fid);

    TimeDuration allocateTime = p.getPeriod("allocate").getDuration();
    TimeDuration createTime = p.getPeriod("create").getDuration();
    double writeTime = 0; //p.getPeriod("write").getDuration();
    TimeDuration closeTime = p.getPeriod("close").getDuration();
    TimeDuration openTime = p.getPeriod("open").getDuration();
    if (false) {
        clog << endl;
        clog << "file : " << filename << endl;
        clog << "allocate memory                  : " << allocateTime << " [s]" << endl;
        clog << "open/prepare file                : " << createTime << " [s]" << endl;
        clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
        //        clog << "written data size                : " << totalSize << " [MB]" << endl;
        //        clog << "writing speed                    : " << totalSize / double(writeTime) << " [MB/s]" << endl;
        clog << "close                            : " << closeTime << " [s]" << endl;
        clog << "open                             : " << openTime << " [s]" << endl;
        //        clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
        //        clog << "write+close(flush to disk) speed : " << totalSize / double(writeTime + closeTime) << " [MB/s]" << endl;

    }



}


void Hdf5_Test::testManyDatasets1() {




    string filename = "/dev/shm/pureManyDs.h5"; // in memory file
    filename = resourcePath("pureManyDs.h5"); // file on disk ($KARABO/src/karabo/tests/io/resources/pure.h5)

    // end of configure



    TimeProfiler p("write");
    p.open();
    p.startPeriod("all");

    //create data file
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    //H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
    hid_t fid = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);

    //create dtype
    unsigned int data = 123;
    hid_t tid = H5Tcopy(H5T_NATIVE_UINT);

    //create data-space on disk    
    hsize_t dims[] = {1};
    hid_t fsid = H5Screate_simple(1, dims, NULL);
    //    //create data-space in memory
    //    mdims[0] = 1;
    //    mdims[1] = 1;
    //    msid = H5Screate_simple(2, mdims, NULL);
    hid_t lid = H5Pcreate(H5P_LINK_CREATE);
    H5Pset_create_intermediate_group(lid, 1);


    //create the datasets
    size_t max = 100000;
    //vector<hid_t> vdid(max, 0);
    hid_t pid = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_layout(pid, H5D_CHUNKED);
    //        dims[0] = 1;
    H5Pset_chunk(pid, 1, dims);

    for (size_t i = 0; i < max; ++i) {
        //create the property list for the dataset
        //

        //string ds = "/base/c1/de/tec/tor" + toString(i);
        string ds = toString(i);
        hid_t did = H5Dcreate2(fid, ds.c_str(), tid, fsid, lid, pid, H5P_DEFAULT);
        H5Dwrite(did, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data);
        H5Dclose(did);
    }
    //flush data to disk        
    H5Fflush(fid, H5F_SCOPE_LOCAL);
    H5Pclose(lid);
    H5Tclose(tid);
    H5Sclose(fsid);
    H5Fclose(fid);
    p.stopPeriod("all");
    p.close();
    TimeDuration allTime = p.getPeriod("all").getDuration();
//    clog << "Time: " << allTime << endl;
    //
    //    hid_t scalar_id = H5Screate(H5S_SCALAR);
    //    for (size_t i = 0; i < max; ++i) {
    //        string ds = "/base/c1/de/tec/tor" + toString(i);
    //        vdid[i] = H5Dopen(fid, ds.c_str(), H5P_DEFAULT);
    //        if (vdid[i] < 0) {
    //            clog << "Error opening dataset " << i << endl;
    //        }
    //        for (int j = 0; j < 2; j++) {
    //            ostringstream oss;
    //            oss << "Attr" << j;
    //            hid_t attr_id = H5Acreate2(vdid[i], oss.str().c_str(), H5T_NATIVE_INT, scalar_id, H5P_DEFAULT, H5P_DEFAULT);
    //            H5Awrite(attr_id, H5T_NATIVE_INT, &j);
    //            H5Aclose(attr_id);
    //        }
    //        //    }
    //        //    for (size_t i = 0; i < max; ++i) {
    //        H5Dclose(vdid[i]);
    //    }
    //
    //
    //    p.stop("create");

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

    //    p.stop("write");
    //
    //
    //    p.start("close");
    //    //close everything down
    //    H5Tclose(tid);
    //    H5Sclose(sid);
    //    H5Sclose(msid);
    //    //    H5Dclose(did);
    //    //    H5Pclose(pid);
    //
    //    //    H5Fclose(fid);
    //
    //    // H5Fopen("pure.h5");
    //    p.stop("close");
    //
    //    free(data);
    //
    //    p.start("open");
    //
    //    for (size_t i = 0; i < 100L; ++i) {
    //
    //        string ds = "/base/c1/de/tec/tor" + toString(i);
    //        did = H5Dopen(fid, ds.c_str(), H5P_DEFAULT);
    //
    //        hid_t scalar_id = H5Screate(H5S_SCALAR);
    //
    //        for (int j = 0; j < 2; j++) {
    //            ostringstream oss;
    //            oss << "Attr" << j;
    //            hid_t attr_id = H5Aopen(did, oss.str().c_str(), H5P_DEFAULT);
    //            H5Aclose(attr_id);
    //        }
    //        H5Dclose(did);
    //        H5Sclose(scalar_id);
    //    }
    //    p.stop("open");
    //
    //
    //    H5Fclose(fid);
    //
    //


    //    double allocateTime = HighResolutionTimer::time2double(p.getTime("allocate"));
    //    double createTime = HighResolutionTimer::time2double(p.getTime("create"));
    //    double writeTime = 0; //HighResolutionTimer::time2double(p.getTime("write"));
    //    double closeTime = HighResolutionTimer::time2double(p.getTime("close"));
    //    double openTime = HighResolutionTimer::time2double(p.getTime("open"));
    //
    //    if (true) {
    //        clog << endl;
    //        clog << "file : " << filename << endl;
    //        clog << "allocate memory                  : " << allocateTime << " [s]" << endl;
    //        clog << "open/prepare file                : " << createTime << " [s]" << endl;
    //        clog << "write data (may use memory cache): " << writeTime << " [s]" << endl;
    //        //        clog << "written data size                : " << totalSize << " [MB]" << endl;
    //        //        clog << "writing speed                    : " << totalSize / writeTime << " [MB/s]" << endl;
    //        clog << "close                            : " << closeTime << " [s]" << endl;
    //        clog << "open                             : " << openTime << " [s]" << endl;
    //        //        clog << "write+close(flush to disk)       : " << writeTime + closeTime << " [s]" << endl;
    //        //        clog << "write+close(flush to disk) speed : " << totalSize / (writeTime + closeTime) << " [MB/s]" << endl;
    //
    //    }
    //


}


void Hdf5_Test::testSerializer() {

    try {

        TimeProfiler p("Hdf5Serializer");
        p.open();
        {
            Hash data;


            data.set("a.01.s.gh.j.kklklklklk", static_cast<char> ('A'));
            data.set("a.02", static_cast<signed char> (50)).setAttribute("mama", static_cast<unsigned long long> (120));
            data.set("a.03", static_cast<unsigned char> (8));
            data.set("a.04", static_cast<short> (-87));
            data.set("a.05", static_cast<unsigned short> (87));
            data.set("a.06", static_cast<int> (8));
            data.set("a.07", static_cast<unsigned int> (8));
            data.set("a.08", static_cast<long long> (8));
            data.set("a.09", static_cast<unsigned long long> (8));
            data.set("a.10", static_cast<float> (8));
            data.set("a.11", static_cast<double> (8));
            data.set("a.12", static_cast<bool> (8));
            data.set("a.13", "Hello");
            data.set("a.14", std::complex<float>(0.4, 0.5));
            data.set("a.15", std::complex<double>(0.4, 0.5));
            data.set("b.01", std::vector<char> (8, 'A'));
            data.set("b.02", std::vector<signed char> (8, 2));
            data.set("b.03", std::vector<unsigned char> (8, 2));
            data.set("b.04", std::vector<short> (8, 2));
            data.set("b.05", std::vector<unsigned short> (8, 2));
            data.set("b.06", std::vector<int> (8, 2));
            data.set("b.07", std::vector<unsigned int> (8, 2));
            data.set("b.08", std::vector<long long> (8, 2));
            data.set("b.09", std::vector<unsigned long long> (8, 2));
            data.set("b.10", std::vector<float> (8, 2));
            data.set("b.11", std::vector<double> (8, 2));
            data.set("b.12", std::vector<bool> (8, true));
            data.set("b.13", std::vector<string> (8, "Hi"));
            data.set("b.14", std::vector< std::complex<float> > (5, std::complex<float>(0.11, 0.21)));
            data.set("b.15", std::vector< std::complex<double> > (5, std::complex<double>(0.12, 0.22)));
            data.set("m", 1);
            Hash::Attributes attr;
            attr.set("0001", static_cast<char> ('N'));
            attr.set("0002", static_cast<signed char> (5));
            attr.set("0003", static_cast<short> (5));
            attr.set("0004", static_cast<int> (5));
            attr.set("0005", static_cast<long long> (5));
            attr.set("0006", static_cast<unsigned char> (5));
            attr.set("0007", static_cast<unsigned short> (5));
            attr.set("0008", static_cast<unsigned int> (5));
            attr.set("0009", static_cast<unsigned long long> (5));
            attr.set("0010", static_cast<float> (5));
            attr.set("0011", static_cast<double> (5));
            attr.set("0012", static_cast<string> ("Hello attribute"));
            attr.set("0013", static_cast<bool> (true));
            attr.set("0014", complex<float>(34.11, 34.12));
            attr.set("0015", complex<double>(98.21, 98.22));
            attr.set("0201", vector<char> (6,'N'));
            attr.set("0202", vector<signed char> (6,58));
            attr.set("0203", vector<short> (6,5));
            attr.set("0204", vector<int> (6,5));
            attr.set("0205", vector<long long> (6,5));
            attr.set("0206", vector<unsigned char> (6,5));
            attr.set("0207", vector<unsigned short> (6,5));
            attr.set("0208", vector<unsigned int> (6,5));
            attr.set("0209", vector<unsigned long long> (6,5));
            attr.set("0210", vector<float> (6,5));
            attr.set("0211", vector<double> (6,5));
            attr.set("0212", vector<string> (6,"Hello attribute"));
            attr.set("0213", vector<bool> (6,true));
            attr.set("0214", vector< std::complex<float> >(3, std::complex<float>(34.11, 34.12)) );
            attr.set("0215", vector< std::complex<double> >(2, std::complex<double>(98.21, 98.22)) );


            data.setAttributes("m", attr);
            Hash a("aa", 1, "bb", 12.89, "cc", -4);
            vector<Hash> v(4, a);
            data.set("c.d", v);
//            clog << "Original Hash:\n" << data << endl;           
            p.startPeriod("all");
            Output<Hash>::Pointer out = Output<Hash>::create("Hdf5File", Hash("filename", resourcePath("test1.h5")));
            out->write(data);
            p.stopPeriod("all");
            p.close();
            TimeDuration allTime = p.getPeriod("all").getDuration();
//            clog << "Write time: " << allTime << endl;



            Input<Hash>::Pointer in = Input<Hash>::create("Hdf5File", Hash("filename", resourcePath("test1.h5")));
            p.open();
            p.startPeriod("read");
            Hash rdata;
            in->read(rdata);
            p.stopPeriod("read");
            p.close();
            TimeDuration readTime = p.getPeriod("read").getDuration();
//            clog << "Read time: " << readTime << endl;
//            clog << "Read Hash\n" << rdata << endl;
        }

        // return;



        {
            Hash data;
            for (size_t i = 0; i < 3/*700*/; i += 6) {
                string path = "a.b." + toString(i) + ".d.e" + toString(i);
                data.set(path, static_cast<int> (i));
                path = "a.b." + toString(i + 1) + ".d.e" + toString(i + 1);
                data.set(path, static_cast<float> (i + 1));
                path = "a.b." + toString(i + 2) + ".d.e" + toString(i + 2);
                data.set(path, static_cast<unsigned char> ((i + 2) % 64));
                path = "a.b." + toString(i + 3) + ".d.e" + toString(i + 3);
                data.set(path, std::complex<float>(i + 0.4, i + 0.5));
                path = "a.b." + toString(i + 4) + ".d.e" + toString(i + 4);
                data.set(path, toString(i + 4));
                //        data.set(path, static_cast<double> ((i + 4) ));
                path = "a.b." + toString(i + 5) + ".d.e" + toString(i + 5);
                data.set(path, std::vector<long long> (8, 2));
            }
            p.open();
            p.startPeriod("all");
            Output<Hash>::Pointer out = Output<Hash>::create("Hdf5File", Hash("filename", resourcePath("test2.h5")));
            out->write(data);
            p.stopPeriod("all");
            p.close();
            TimeDuration allTime = p.getPeriod("all").getDuration();
//            clog << "Write time: " << allTime << endl;

            Input<Hash>::Pointer in = Input<Hash>::create("Hdf5File", Hash("filename", resourcePath("test2.h5")));
            p.open();
            p.startPeriod("read");
            Hash rdata;
            in->read(rdata);
            p.stopPeriod("read");
            p.close();
            TimeDuration readTime = p.getPeriod("read").getDuration();
//            clog << "Read time: " << readTime << endl;
//            clog << "Read Hash\n" << rdata << endl;

        }

        {
//            clog << "+++++++++++++++++++++++++++" << endl;
            Hash data;

            data.set("a.01.Hello.World", static_cast<char> ('B'));
            data.set("a.02", static_cast<signed char> (50)).setAttribute("mama", static_cast<unsigned long long> (120));
            data.set("a.03", static_cast<unsigned char> (8));
            data.set("a.04", static_cast<short> (-87));
            data.set("a.05", static_cast<unsigned short> (87));
            data.set("a.06", static_cast<int> (8));
            data.set("a.07", static_cast<unsigned int> (8));
            p.open();
            p.startPeriod("write");
            Output<Hash>::Pointer out = Output<Hash>::create("Hdf5File", Hash("filename", resourcePath("test4.h5"), "enableAppendMode", true));
            for (size_t i = 0; i < 10; ++i) {
                data.set("i", static_cast<int> (i));
                out->write(data);
            }
            out->update();
            p.stopPeriod("write");
            p.close();
            TimeDuration allTime = p.getPeriod("write").getDuration();
//            clog << "Write time: " << allTime << endl;

            Input<Hash>::Pointer in = Input<Hash>::create("Hdf5File", Hash("filename", resourcePath("test4.h5")));
            size_t size = in->size();
//            clog << "number of Hashes in the file: " << size << endl;

            for (size_t i = 0; i < size; ++i) {
                Hash rdata;
                p.open();
                p.startPeriod("read");
                in->read(rdata, i);
                p.stopPeriod("read");
                p.close();
                TimeDuration readTime = p.getPeriod("read").getDuration();
//                clog << "Read time: " << readTime << endl;
             //   clog << "Read Hash\n" << rdata << endl;
            }
//            clog << "+++++++++++++++++++++++++++" << endl;
        }


        {

            Hash rooted("a.b.c", 1, "a.b.d", vector<int>(5, 1), "a.b.e", vector<Hash > (2, Hash("a", 1)), "a.d", 2.4); //std::complex<double>(1.2, 4.2));
            rooted.setAttribute("a", "a1", 1); //true);
            rooted.setAttribute("a", "a2", 3.4);
            rooted.setAttribute("a.b", "b1", 3); //"3"); 
            rooted.setAttribute("a.b.c", "c1", 2);
            rooted.setAttribute("a.b.c", "c2", vector<int> (3, 1222)); // vector<string > (3, "bla"));
            Hash m_rootedHash = rooted;

            TimeProfiler p("a");
            p.open();
            p.startPeriod("vec");
            //    Hash big("a.b", std::vector<double>(20*1024*1024, 1.0));
            Hash big("a.b", std::vector<double>(1000, 1.0));
            p.stopPeriod("vec");
            try {
                //        double time = HighResolutionTimer::time2double(p.getTime("vec"));
                //        clog << "creating big Hash took " << time << " [s]" << endl;

                vector<Hash>& tmp = big.bindReference<vector<Hash> >("a.c");
                tmp.resize(10000);
                for (size_t i = 0; i < tmp.size(); ++i) {
                    tmp[i] = m_rootedHash;
                }
                Hash m_bigHash = big;
//                clog << "start m_bigHash" << endl;
                p.startPeriod("all");
                Output<Hash>::Pointer out = Output<Hash>::create("Hdf5File", Hash("filename", resourcePath("test3.h5")));
                out->write(m_bigHash);
                p.stopPeriod("all");  
                p.close();
                TimeDuration allTime = p.getPeriod("all").getDuration();
//                clog << "Write time: " << allTime << endl;
     

                Input<Hash>::Pointer in = Input<Hash>::create("Hdf5File", Hash("filename", resourcePath("test3.h5")));
                p.open();
                p.startPeriod("read");
                Hash rdata;
                in->read(rdata);
                p.stopPeriod("read");
                p.close();
                TimeDuration readTime = p.getPeriod("read").getDuration();
                
//                clog << "Read time: " << readTime << endl;
                //                clog << "Read Hash\n" << rdata << endl;


            } catch (Exception& ex) {
                clog << ex << endl;
            }
        }
    } catch (Exception& ex) {
        clog << ex << endl;
    }

}



