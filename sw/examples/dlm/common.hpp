#pragma once

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

#include "cBench.hpp"
#include "cProcess.hpp"
#include "ibvQpConn.hpp"
#include "ibvQpMap.hpp"

#include "./lib/util/util.hpp"

using namespace std;
using namespace fpga;

// cRegs
#define CTRLOFS_HBMHOST 0
#define CTRLOFS_TXNENG 7

enum class HBMRegs : uint32_t {
  MODE 	   = CTRLOFS_HBMHOST + 0,
  HOSTADDR = CTRLOFS_HBMHOST + 1,
  CMEMADDR = CTRLOFS_HBMHOST + 2,
  LEN      = CTRLOFS_HBMHOST + 3,
  CNT      = CTRLOFS_HBMHOST + 4,
  PID      = CTRLOFS_HBMHOST + 5,
  CNTDONE  = CTRLOFS_HBMHOST + 6
};

/* types.scala - CMemHostIO
  def regMap(r: AxiLite4SlaveFactory, baseR: Int): Int = {
    implicit val baseReg = baseR
    val rMode = r.rwInPort(mode,     r.getAddr(0), 0, "CMemHost: mode")
    when(rMode.orR) (rMode.clearAll()) // auto clear
    r.rwInPort(hostAddr, r.getAddr(1), 0, "CMemHost: hostAddr")
    r.rwInPort(cmemAddr, r.getAddr(2), 0, "CMemHost: cmemAddr")
    r.rwInPort(len,      r.getAddr(3), 0, "CMemHost: len")
    r.rwInPort(cnt,      r.getAddr(4), 0, "CMemHost: cnt")
    r.rwInPort(pid,      r.getAddr(5), 0, "CMemHost: pid")
    r.read(cntDone,      r.getAddr(6), 0, "CMemHost: cntDone")
    val assignOffs = 7
    assignOffs
  }
*/

enum class TXNENGRegs : uint32_t {
  FlowMstr  = CTRLOFS_TXNENG + 0,
  FlowSlve  = CTRLOFS_TXNENG + 1,
  Start     = CTRLOFS_TXNENG + 2,
  NodeId    = CTRLOFS_TXNENG + 3,
  TxnNum    = CTRLOFS_TXNENG + 4,
  CMemAddr  = CTRLOFS_TXNENG + 5,
  CntTxnCmt = CTRLOFS_TXNENG + 6,
  CntTxnAbt = CTRLOFS_TXNENG + 7,
  CntTxnLd  = CTRLOFS_TXNENG + 8,
  CntClk    = CTRLOFS_TXNENG + 9,
  Done      = CTRLOFS_TXNENG + 10
};

/* Types.scala - NodeNetIO
  def regMap(r: AxiLite4SlaveFactory, baseR: Int): Int = {
    implicit val baseReg = baseR
    // in
    // rdmaCtrl MSB <-> LSB: flowId(4b), qpn(24b), len(32b), en(1b)
    r.rwInPort(rdmaCtrl(0).en, r.getAddr(0), 0, "TxnEng: RDMA Mstr en")
    r.rwInPort(rdmaCtrl(0).len, r.getAddr(0), 1, "TxnEng: RDMA Mstr len")
    r.rwInPort(rdmaCtrl(0).qpn, r.getAddr(0), 33, "TxnEng: RDMA Mstr qpn")
    r.rwInPort(rdmaCtrl(0).flowId, r.getAddr(0), 57, "TxnEng: RDMA Mstr flowId")

    r.rwInPort(rdmaCtrl(1).en, r.getAddr(1), 0, "TxnEng: RDMA Slve en")
    r.rwInPort(rdmaCtrl(1).len, r.getAddr(1), 1, "TxnEng: RDMA Slve len")
    r.rwInPort(rdmaCtrl(1).qpn, r.getAddr(1), 33, "TxnEng: RDMA Slve qpn")
    r.rwInPort(rdmaCtrl(1).flowId, r.getAddr(1), 57, "TxnEng: RDMA Slve flowId")

    val rStart = r.rwInPort(node.start,     r.getAddr(2), 0, "TxnEng: start")
    rStart.clearWhen(rStart) // auto clear start sig
    r.rwInPort(node.nodeId,     r.getAddr(3), 0, "TxnEng: nodeId")
    r.rwInPort(node.txnNumTotal, r.getAddr(4), 0, "TxnEng: txnNumTotal")

    var assignOffs = 5
    node.cmdAddrOffs.foreach { e =>
      r.rwInPort(e, r.getAddr(assignOffs), 0, "TxnEng: cmemAddr")
      assignOffs += 1
    }

    // out
    node.cntTxnCmt.foreach { e =>
      r.read(e, r.getAddr(assignOffs), 0, "TxnEng: cntTxnCmt")
      assignOffs += 1
    }
    node.cntTxnAbt.foreach { e =>
      r.read(e, r.getAddr(assignOffs), 0, "TxnEng: cntTxnAbt")
      assignOffs += 1
    }
    node.cntTxnLd.foreach { e =>
      r.read(e, r.getAddr(assignOffs), 0, "TxnEng: cntTxnLd")
      assignOffs += 1
    }
    node.cntClk.foreach { e =>
      r.read(e, r.getAddr(assignOffs), 0, "TxnEng: cntClk")
      assignOffs += 1
    }
    r.read(node.done.asBits, r.getAddr(assignOffs), 0, "TxnEng: done bitVector")
    assignOffs += 1

    assignOffs
  }
*/



// help functions

uint32_t convert( const std::string& ipv4Str ) {
    std::istringstream iss( ipv4Str );
    
    uint32_t ipv4 = 0;
    
    for( uint32_t i = 0; i < 4; ++i ) {
        uint32_t part;
        iss >> part;
        if ( iss.fail() || part > 255 )
            throw std::runtime_error( "Invalid IP address - Expected [0, 255]" );
        
        // LSHIFT and OR all parts together with the first part as the MSB
        ipv4 |= part << ( 8 * ( 3 - i ) );

        // Check for delimiter except on last iteration
        if ( i != 3 ) {
            char delimiter;
            iss >> delimiter;
            if ( iss.fail() || delimiter != '.' ) 
                throw std::runtime_error( "Invalid IP address - Expected '.' delimiter" );
        }
    }
    
    return ipv4;
}


int hbmRW(cProcess& cproc, const void* hMem, const bool isWr, const uint32_t len, const uint64_t hbmAddr) {

  cproc.setCSR(hbmAddr, static_cast<uint32_t>(HBMRegs::CMEMADDR)); // set card memory addr
  // std::cout << "Write CSR"<< checkCSR_ADDR() << "+" << static_cast<uint32_t>(HBMRegs::CMEMADDR) << ": " << hbmAddr << std::endl;
  // std::cout << "Write CSR Offset " << static_cast<uint32_t>(HBMRegs::CMEMADDR) << ": CMEMADDR " << hbmAddr << std::endl;
 
  cproc.setCSR(reinterpret_cast<uint64_t>(hMem), static_cast<uint32_t>(HBMRegs::HOSTADDR)); // set hostAddr
  // std::cout << "Write CSR Offset " << static_cast<uint32_t>(HBMRegs::HOSTADDR) << ": HOSTADDR " << reinterpret_cast<uint64_t>(hMem) << std::endl;
 
  cproc.setCSR(64, static_cast<uint32_t>(HBMRegs::LEN)); // set burst length
  // std::cout << "Write CSR Offset " << static_cast<uint32_t>(HBMRegs::LEN) << ": LEN " << 64 << std::endl;
 
  cproc.setCSR(len/64, static_cast<uint32_t>(HBMRegs::CNT)); // set beat cnt
  // std::cout << "Write CSR Offset " << static_cast<uint32_t>(HBMRegs::CNT) << ": CNT " << len/64 << std::endl;

  cproc.setCSR(cproc.getCpid(), static_cast<uint32_t>(HBMRegs::PID)); // set pid
  // std::cout << "Write CSR Offset " << static_cast<uint32_t>(HBMRegs::PID) << ": PID " << cproc.getCpid() << std::endl;

  DBG("[INFO] HBM CSR set done.");

  if(isWr){
    cproc.setCSR(0x01, static_cast<uint32_t>(HBMRegs::MODE)); // start wr to hbm
    int wtime=0;
    while(cproc.getCSR(static_cast<uint32_t>(HBMRegs::CNTDONE)) < (len/64)){
      sleep(1);
      wtime+=1;
      if(wtime>10)
        break;
      std::cout << "[INFO] HBM Writing Byte "<< cproc.getCSR(static_cast<uint32_t>(HBMRegs::CNTDONE)) << std::endl;
    }
  } else {
    cproc.setCSR(0x02, static_cast<uint32_t>(HBMRegs::MODE)); // start rd from hbm
    while(cproc.getCSR(static_cast<uint32_t>(HBMRegs::CNTDONE)) < (len/64)){
      sleep(1);
      std::cout << "[INFO] HBM Reading Byte "<< cproc.getCSR(static_cast<uint32_t>(HBMRegs::CNTDONE)) << std::endl;
    }
  }
  return 0;
}

int initTxnCmd(cProcess& cproc, uint32_t txncnt, string bin_fname, uint64_t insoffs) {
  // Handles and alloc
  uint32_t n_pages = (txncnt*64*8 + hugePageSize - 1) / hugePageSize;
  void* hMem = cproc.getMem({CoyoteAlloc::HOST_2M, n_pages});

  ifstream rf(bin_fname, ios::in | ios::binary);
  rf.read((char*) hMem, txncnt*64*sizeof(uint64_t));
  rf.close();

  hbmRW(cproc, hMem, true, (sizeof(uint64_t)*64*txncnt), insoffs);
  DBG("[INFO] Txn loaded to card.");

  return 0;
}

int txnManPrtStatus(cProcess& cproc) {
  std::cout << "CntTxnCmt: " << cproc.getCSR(static_cast<uint32_t>(TXNENGRegs::CntTxnCmt)) << std::endl;
  std::cout << "CntTxnAbt: " << cproc.getCSR(static_cast<uint32_t>(TXNENGRegs::CntTxnAbt)) << std::endl;
  std::cout << "CntTxnLd: " << cproc.getCSR(static_cast<uint32_t>(TXNENGRegs::CntTxnLd)) << std::endl;
  std::cout << "CntClk: " << cproc.getCSR(static_cast<uint32_t>(TXNENGRegs::CntClk)) << std::endl;
  std::cout << "Done: " << cproc.getCSR(static_cast<uint32_t>(TXNENGRegs::Done)) << std::endl;
  return 0;
}

int txnManCnfg(cProcess& cproc, uint32_t nodeid, uint32_t txncnt, uint64_t insoffs) {
  cproc.setCSR(nodeid, static_cast<uint32_t>(TXNENGRegs::NodeId));
  cproc.setCSR(txncnt, static_cast<uint32_t>(TXNENGRegs::TxnNum));
  cproc.setCSR(insoffs>>6, static_cast<uint32_t>(TXNENGRegs::CMemAddr)); // cmdOffs unit: 64B
  return 0;
}

int txnManStart(cProcess& cproc) {
  cproc.setCSR(1, static_cast<uint32_t>(TXNENGRegs::Start));
  return 0;
}
