#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <thread>
#include <random>
#include <unistd.h>
#include <fstream>

#include "../foedus/uniform_random.hpp"
#include "../foedus/zipfian_random.hpp"

#define DBG(msg) do { std::cout << msg << std::endl; } while( false )


#define N_NODE 2
#define N_CH 8
#define N_TUPLE (1<<22)
#define DIST_PER_WARE 10

uint64_t g_num_wh = N_NODE * N_CH;
uint64_t g_cust_per_dist = 3000;
uint64_t g_max_items = 100000;


#define OFFS_DIST 2
#define OFFS_CUST 22
#define OFFS_ORDE 480022
#define OFFS_NORD 510022
#define OFFS_ORDL 519022
#define OFFS_STOK 1119022
#define OFFS_ITEM 1919022
#define OFFS_HIST 2119022

#define SIZE_WHSE 2
#define SIZE_DIST 2
#define SIZE_CUST 16
#define SIZE_ORDE 1
#define SIZE_NORD 1
#define SIZE_ORDL 2
#define SIZE_STOK 8
#define SIZE_ITEM 2
#define SIZE_HIST 2


struct idbundle {
  uint64_t nid;
  uint64_t cid;
  uint64_t tid;
};

namespace fpga {

template<typename T>
  void pop_front(std::vector<T> &v)
  {
    if (v.size() > 0) {
      v.erase(v.begin());
    }
  }

  void wait_for_enter(const std::string& msg)
  {
    std::cout << msg << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

class txnTask {

  private:
    bool iszipfian, isnaive;
    double ztheta, wrratio;
    uint64_t gtsize;
    uint32_t txnlen, txncnt;

  // TODO: change to map
    std::vector<uint64_t> keys;
  std::vector<bool> rw; // read or write

  foedus::assorted::ZipfianRandom zipfian_rng;
  foedus::assorted::UniformRandom uniform_rng;

public:

  txnTask(bool iszipfian, double ztheta, bool isnaive, uint64_t gtsize) : iszipfian(iszipfian), ztheta(ztheta), isnaive(isnaive), gtsize(gtsize) {
    // srand(3);
    if(iszipfian) {
      zipfian_rng.init(gtsize, ztheta, 1238);
    } else {
      uniform_rng.set_current_seed(1238);
    }
  }

  ~txnTask(){};

  // initialize key gen
  void keyInit(uint64_t gtsize, double wrratio, uint32_t txnlen, uint32_t txncnt){
    if(!isnaive){
      for (int ii=0; ii<txnlen*txncnt; ii++){
        if(iszipfian){
          keys.push_back(zipfian_rng.next());
        } else {
          keys.push_back(uniform_rng.uniform_within(0, gtsize - 1));
        }
        rw.push_back(((double)rand() / ((double)RAND_MAX + 1)<wrratio)?true:false);
      }
    } else{
      // naive: continuous tid
      for (int ii=0; ii<txncnt; ii++){
        for (int jj=0; jj<txnlen; jj++){
          keys.push_back(ii*txnlen+jj);
          // keys.push_back(jj);
          rw.push_back(((double)rand() / ((double)RAND_MAX + 1)<wrratio)?true:false);
        }
      }
    }

    if(iszipfian){
      // remove repeat
      int nrpt = 0;
      // confirm there's no repeat key in one txn
      for (int ii=0; ii<txncnt;){
        bool ckflag = false;
        for (int jj=0; jj<txnlen; jj++){
          for (int kk=jj+1; kk<txnlen; kk++){
            if(keys[ii*txnlen+jj]==keys[ii*txnlen+kk]){
              keys[ii*txnlen+kk] += 1;
              ckflag = true;
              nrpt++;
            }
          }
        }
        if (!ckflag) ii++;
      }
      if(nrpt){DBG("replaced intra-txn repeat number =" << nrpt);}
    }
  }

  uint64_t getKey(){
    uint64_t val = keys.front();
    pop_front(keys);
    return val;
  }

  bool getRW(){
    bool val = rw.front();
    pop_front(rw);
    return val;
  }

  uint64_t getSize(){
    return keys.size();
  }  


};

}


namespace tpcc {

  // cnt for insert_row
  uint64_t cnt_hist = 0;
  uint64_t cnt_orde = 0;
  uint64_t cnt_nord = 0;
  uint64_t cnt_ordl = 0;

  std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
  
  uint64_t RAND(uint64_t max, uint64_t thd_id) {
    int64_t rint64 = generator();
    return rint64 % max;
  }

  uint64_t URand(uint64_t x, uint64_t y, uint64_t thd_id) {
    return x + RAND(y - x + 1, thd_id);
  }

  uint64_t NURand(uint64_t A, uint64_t x, uint64_t y, uint64_t thd_id) {
    static bool C_255_init = false;
    static bool C_1023_init = false;
    static bool C_8191_init = false;
    static uint64_t C_255, C_1023, C_8191;
    int C = 0;
    switch(A) {
      case 255:
      if(!C_255_init) {
        C_255 = (uint64_t) URand(0,255, thd_id);
        C_255_init = true;
      }
      C = C_255;
      break;
      case 1023:
      if(!C_1023_init) {
        C_1023 = (uint64_t) URand(0,1023, thd_id);
        C_1023_init = true;
      }
      C = C_1023;
      break;
      case 8191:
      if(!C_8191_init) {
        C_8191 = (uint64_t) URand(0,8191, thd_id);
        C_8191_init = true;
      }
      C = C_8191;
      break;
      default:
      exit(-1);
    }
    return(((URand(0,A, thd_id) | URand(x,y, thd_id))+C)%(y-x+1))+x;
  }

  struct Item_no {
    uint64_t ol_i_id;
    uint64_t ol_supply_w_id;
    uint64_t ol_quantity;
  };

  enum TPCCTxnType {TPCC_ALL, 
    TPCC_PAYMENT, 
    TPCC_NEW_ORDER, 
    TPCC_ORDER_STATUS, 
    TPCC_DELIVERY, 
    TPCC_STOCK_LEVEL};


  class tpcc_query {
  public:
    TPCCTxnType type;
    /**********************************************/  
    // common txn input for both payment & new-order
    /**********************************************/  
    uint64_t w_id;
    uint64_t d_id;
    uint64_t c_id;
    /**********************************************/  
    // txn input for payment
    /**********************************************/  
    uint64_t d_w_id;
    uint64_t c_w_id;
    uint64_t c_d_id;
    double h_amount;
    bool by_last_name;
    uint64_t hist_id;
    /**********************************************/  
    // txn input for new-order
    /**********************************************/
    Item_no * items;
    bool rbk;
    bool remote;
    uint64_t ol_cnt;
    uint64_t o_entry_d;
    // Input for delivery
    uint64_t o_carrier_id;
    uint64_t ol_delivery_d;
    // for order-status

    uint64_t orde_id;
    uint64_t nord_id;
    uint64_t ordl_id;

    //  uint64_t wh_to_part(uint64_t wid);
    void gen_payment(uint64_t thd_id);
    void gen_new_order(uint64_t thd_id);
    void gen_order_status(uint64_t thd_id);
  };



  uint64_t qry2Bin (idbundle ids, bool rw, bool lk_upgrd, uint64_t w_len) {
  // nid, cid, tid, rd/wr, lkUpgrade, wLen
    uint64_t inst = (ids.nid + (ids.cid << uint32_t(log2(N_NODE))) + (ids.tid << uint32_t(log2(N_NODE) + log2(N_CH))) + ((uint64_t)(rw? 1: 0) << uint32_t(log2(N_NODE) + log2(N_CH) + log2(N_TUPLE))) + ((uint64_t)(lk_upgrd? 1: 0) << uint32_t(log2(N_NODE) + log2(N_CH) + log2(N_TUPLE) + 1)) + (w_len << uint32_t(log2(N_NODE) + log2(N_CH) + log2(N_TUPLE) + 2)));
  // std::cout << "Inst: " << inst << std::endl;
    return inst;
  }

// one warehouse per channel
  idbundle qry2Id (const tpcc_query &q, const char *lktarget, uint32_t i_ol) {
    idbundle ids;
  // payment
    if (!strcmp(lktarget, "pm_wh")) {
      ids.nid = q.d_w_id / (N_CH);
      ids.cid = q.d_w_id % (N_CH);
      ids.tid = 0;
    }
    if (!strcmp(lktarget, "pm_dist")) {
      ids.nid = q.d_w_id / (N_CH);
      ids.cid = q.d_w_id % (N_CH);
      ids.tid = OFFS_DIST + q.d_id * SIZE_DIST;
    }
    if (!strcmp(lktarget, "pm_cust")) {
      ids.nid = q.c_w_id / (N_CH);
      ids.cid = q.c_w_id % (N_CH);
      ids.tid = OFFS_CUST + (q.c_d_id * g_cust_per_dist + q.c_id) * SIZE_CUST;
    }
    if (!strcmp(lktarget, "pm_hist")) {
      ids.nid = q.d_w_id / (N_CH);
      ids.cid = q.d_w_id % (N_CH);
      ids.tid = OFFS_HIST + q.hist_id * SIZE_HIST;
    }

  // new order
    if (!strcmp(lktarget, "nord_wh")) {
      ids.nid = q.w_id / (N_CH);
      ids.cid = q.w_id % (N_CH);
      ids.tid = 0;
    }
    if (!strcmp(lktarget, "nord_dist")) {
      ids.nid = q.w_id / (N_CH);
      ids.cid = q.w_id % (N_CH);
    ids.tid = OFFS_DIST + q.d_id * SIZE_DIST + 1; // +1 is the offset in dist
    }
    if (!strcmp(lktarget, "nord_cust")) {
      ids.nid = q.w_id / (N_CH);
      ids.cid = q.w_id % (N_CH);
      ids.tid = OFFS_CUST + q.c_id * SIZE_CUST;
    }
    if (!strcmp(lktarget, "nord_orde")) {
      ids.nid = q.w_id / (N_CH);
      ids.cid = q.w_id % (N_CH);
      ids.tid = OFFS_ORDE + q.orde_id * SIZE_ORDE;
    }
    if (!strcmp(lktarget, "nord_nord")) {
      ids.nid = q.w_id / (N_CH);
      ids.cid = q.w_id % (N_CH);
      ids.tid = OFFS_NORD + q.nord_id * SIZE_NORD;
    }
    if (!strcmp(lktarget, "nord_ordl")) {
      ids.nid = q.w_id / (N_CH);
      ids.cid = q.w_id % (N_CH);
      ids.tid = OFFS_ORDL + (q.ordl_id + i_ol) * SIZE_ORDL;
    }
    if (!strcmp(lktarget, "nord_item")) {
      ids.nid = q.w_id / (N_CH);
      ids.cid = q.w_id % (N_CH);
      ids.tid = OFFS_ITEM + q.items[i_ol].ol_i_id * SIZE_ITEM;
    }
    // may contain remote access
    if (!strcmp(lktarget, "nord_stok")) {
      ids.nid = q.items[i_ol].ol_supply_w_id / (N_CH);
      ids.cid = q.items[i_ol].ol_supply_w_id % (N_CH);
      ids.tid = OFFS_STOK + q.items[i_ol].ol_i_id * SIZE_STOK;
    }
    return ids;
  }

}


















































