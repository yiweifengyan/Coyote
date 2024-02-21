#include <iostream>
#include <string>
#include <malloc.h>
#include <time.h> 
#include <sys/time.h>  
#include <chrono>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <iomanip>
#ifdef EN_AVX
#include <x86intrin.h>
#endif
#include <boost/program_options.hpp>
#include <numeric>
#include <stdlib.h>

#include "./lib/util/util.hpp"

using namespace std;
using namespace fpga;

#define BOOL_STR(b) ((b)?"t":"f")

int main(int argc, char *argv[]) {

  boost::program_options::options_description programDescription("Options:");
  programDescription.add_options()("txnlen,l", boost::program_options::value<uint32_t>(), "txnlen")
                                  ("txncnt,c", boost::program_options::value<uint32_t>(), "txncnt")
                                  ("tuplelen,e", boost::program_options::value<uint32_t>(), "tuplelen")
                                  ("wrratio,w", boost::program_options::value<double>(), "wrRatio")
                                  ("iszipfian,z", boost::program_options::value<bool>(), "iszipfian")
                                  ("isnaive,a", boost::program_options::value<bool>(), "isnaive")
                                  ("ztheta,t", boost::program_options::value<double>(), "zipFianTheta")
                                  ("widx,i", boost::program_options::value<uint32_t>(), "workload idx (file name)")
                                  ("nodeid,n", boost::program_options::value<uint32_t>(), "nodeid");
  boost::program_options::variables_map commandLineArgs;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
  boost::program_options::notify(commandLineArgs);

  uint32_t txnlen = commandLineArgs["txnlen"].as<uint32_t>();
  uint32_t txncnt = commandLineArgs["txncnt"].as<uint32_t>();
  uint32_t tuplelen = commandLineArgs["tuplelen"].as<uint32_t>();
  double wrratio = commandLineArgs["wrratio"].as<double>();
  bool iszipfian = commandLineArgs["iszipfian"].as<bool>();
  bool isnaive = commandLineArgs["isnaive"].as<bool>();
  double ztheta = commandLineArgs["ztheta"].as<double>();
  uint32_t widx = commandLineArgs["widx"].as<uint32_t>();

  uint32_t nodeid = 0;
  if(commandLineArgs.count("nodeid") > 0) {
      nodeid = commandLineArgs["nodeid"].as<uint32_t>();
  }

  // string fname = "txn_" + to_string(txnlen) + "_" + to_string(txncnt) + "_" + to_string(wrratio) + "_" + BOOL_STR(iszipfian) + "_" + BOOL_STR(isnaive) + "_" + to_string(ztheta);

  string fname = "txn_" + to_string(widx) + ".bin";

  // one hbm channel, each tuple with 64 B
  uint64_t gtsize = (1<<22);

  // txnTask
  txnTask txn_task(iszipfian, ztheta, isnaive, gtsize);
  txn_task.keyInit(gtsize, wrratio, txnlen, txncnt);

  std::cout << "[INFO] Txn task generated." << std::endl;

  uint64_t* instMem = (uint64_t*) malloc(txncnt*64*sizeof(uint64_t));

  uint64_t wTupleLen = uint64_t(log2(tuplelen/64));

  for (int ii=0; ii<txncnt; ii++){
    *(instMem + ii * 64) = txnlen;
    for (int jj=0; jj<txnlen; jj++){
      uint64_t key = txn_task.getKey();
      // nid, cid, tid, rd/wr, wLen
      uint64_t nid = nodeid;
      uint64_t cid = 0;
      uint64_t content = nid + (cid<<1) + (key << 1) + ((uint64_t)(txn_task.getRW() ? 1 : 0) << (1+22+3)) + (wTupleLen << (1+22+3+2));
      // uint64_t content = nid + (cid<<1) + (key << (1+3)) + ((uint64_t)(txn_task.getRW() ? 1 : 0) << (1+3+22)) + (wTupleLen << (1+3+22+2));
      // rd upgrade to wr
      // uint64_t content = 0 + (0<<1) + (key << 1) + (0 << 23) + ((uint64_t)0 << 25);
      // *(instMem + ii * 64 + jj * 2 + 1) = content;
      // content = 0 + (0<<1) + (key << 1) + (3 << 23) + ((uint64_t)0 << 25);
      *(instMem + ii * 64 + jj + 1) = content;
    }
  }


  // write to file
  ofstream wf(fname, ios::out | ios::binary);
  if(!wf) {
    cout << "Cannot open file!" << endl;
    return 1;
  }

  wf.write((char*) instMem, txncnt*64*sizeof(uint64_t));
  wf.close();

  return EXIT_SUCCESS;
}






























