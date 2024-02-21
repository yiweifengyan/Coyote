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

using namespace tpcc;
using namespace std;

void tpcc_query::gen_payment(uint64_t qid) {
	type = TPCC_PAYMENT;
	w_id = qid % g_num_wh;
	d_w_id = w_id;

	d_id = URand(0, DIST_PER_WARE-1, w_id-1);
	h_amount = URand(1, 5000, w_id-1);
	int x = URand(1, 100, w_id-1);
	int y = URand(1, 100, w_id-1);


	if(x <= 85) {
		// home warehouse
		c_d_id = d_id;
		c_w_id = w_id;
	} else {	
		// remote warehouse
		c_d_id = URand(0, DIST_PER_WARE-1, w_id-1);
		if(g_num_wh > 1) {
			while((c_w_id = URand(0, g_num_wh-1, w_id-1)) == w_id) {}
		} else 
			c_w_id = w_id;
	}
// rbshi: temp disable the by_last_name
//	if(y <= 60) {
//		// by last name
//		by_last_name = true;
//		Lastname(NURand(255,0,999,w_id-1),c_last);
//	} else {
		// by cust id
		by_last_name = false;
		c_id = NURand(1023, 0, g_cust_per_dist-1,w_id-1);
//	}
		hist_id = cnt_hist;
		cnt_hist++;
}

void tpcc_query::gen_new_order(uint64_t qid) {
	type = TPCC_NEW_ORDER;
	w_id = qid % g_num_wh;
	d_id = URand(0, DIST_PER_WARE-1, w_id-1);
	c_id = NURand(1023, 0, g_cust_per_dist-1, w_id-1);
	rbk = URand(1, 100, w_id-1);
	ol_cnt = URand(5, 14, w_id-1);
	o_entry_d = 2013;
	items = (Item_no *) _mm_malloc(sizeof(Item_no) * ol_cnt, 64);
	remote = false;

	for (uint32_t oid = 0; oid < ol_cnt; oid ++) {
		items[oid].ol_i_id = NURand(8191, 0, g_max_items-1, w_id-1);
		uint32_t x = URand(1, 100, w_id-1);
		if (x > 1 || g_num_wh == 1)
			items[oid].ol_supply_w_id = w_id;
		else  {
			while((items[oid].ol_supply_w_id = URand(0, g_num_wh-1, w_id-1)) == w_id) {}
				remote = true;
		}
		items[oid].ol_quantity = URand(1, 10, w_id-1);
	}
	// Remove duplicate items
	for (uint32_t i = 0; i < ol_cnt; i ++) {
		for (uint32_t j = 0; j < i; j++) {
			if (items[i].ol_i_id == items[j].ol_i_id) {
				for (uint32_t k = i; k < ol_cnt - 1; k++)
					items[k] = items[k + 1];
				ol_cnt --;
				i--;
			}
		}
	}
	for (uint32_t i = 0; i < ol_cnt; i ++) 
		for (uint32_t j = 0; j < i; j++) 
			assert(items[i].ol_i_id != items[j].ol_i_id);
	
	orde_id = cnt_orde;
	cnt_orde++;

	nord_id = cnt_nord;
	cnt_nord++;
	
	ordl_id = cnt_ordl;
	cnt_ordl += ol_cnt;
}



int main() {
	std::vector<tpcc_query> qry;

	// generate query vector
	for (uint32_t i=0; i< 1280; i++){
		tpcc_query q;
		// q.gen_payment(N_CH+(i%N_CH));
		q.gen_new_order((i%N_CH));
		qry.push_back(q);
	}
	std::cout << "[INFO] Txn task generated." << std::endl;


	// translate and dump to binary file
	uint64_t* inst_mem = (uint64_t*) malloc(qry.size()*64*sizeof(uint64_t)); /*maxLen=64*/

	uint64_t idx_qry = 0;

	for (auto q = qry.begin(); q != qry.end(); ++q, idx_qry++) {
		uint64_t* qry_mem = (uint64_t*) malloc(64*sizeof(uint64_t));
		switch(q->type) {
			case TPCC_PAYMENT: {
				*qry_mem = 6;
				// get_row(warehouse, RD)
				*(qry_mem+1) = qry2Bin(qry2Id(*q, "pm_wh", 0), false, false, 0);
				// get_row(district, RD)
				*(qry_mem+2) = qry2Bin(qry2Id(*q, "pm_dist", 0), false, false, 0);
				// get_row(district, WR)
				*(qry_mem+3) = qry2Bin(qry2Id(*q, "pm_dist", 0), true, true, 0);
				// get_row(customer, RD)
				*(qry_mem+4) = qry2Bin(qry2Id(*q, "pm_cust", 0), false, false, 0);
				// get_row(customer, WR)
				*(qry_mem+5) = qry2Bin(qry2Id(*q, "pm_cust", 0), true, true, 0);
				// get_row(hist, WR)
				*(qry_mem+6) = qry2Bin(qry2Id(*q, "pm_hist", 0), true, false, 1);
				break;
			}
			// TPCC_NEW_ORDER
			default: {
				*qry_mem = 6 + q->ol_cnt * 4; /*calculate the query length here*/;
				// get_row(warehouse, RD)
				*(qry_mem+1) = qry2Bin(qry2Id(*q, "nord_wh", 0), false, false, 0);
				// get_row(customer, RD)
				*(qry_mem+2) = qry2Bin(qry2Id(*q, "nord_cust", 0), false, false, 0);
				// get_row(district, RD, WR)
				*(qry_mem+3) = qry2Bin(qry2Id(*q, "nord_dist", 0), false, false, 0);
				*(qry_mem+4) = qry2Bin(qry2Id(*q, "nord_dist", 0), true, true, 0);
				// get_row(order, WR)
				*(qry_mem+5) = qry2Bin(qry2Id(*q, "nord_orde", 0), true, false, 0);
				// get_row(nord, WR)
				*(qry_mem+6) = qry2Bin(qry2Id(*q, "nord_nord", 0), true, false, 0);

				for (uint32_t ii=0; ii<q->ol_cnt; ii++) {
					*(qry_mem+7+ii*4+0) = qry2Bin(qry2Id(*q, "nord_item", ii), false, false, 0);
					*(qry_mem+7+ii*4+1) = qry2Bin(qry2Id(*q, "nord_stok", ii), false, false, 0);
					*(qry_mem+7+ii*4+2) = qry2Bin(qry2Id(*q, "nord_stok", ii), true, true, 0);
					*(qry_mem+7+ii*4+3) = qry2Bin(qry2Id(*q, "nord_ordl", ii), true, false, 1); // 128B
				}
			}
		}
		// copy the qry_mem to target
		memcpy (inst_mem + idx_qry * 64, qry_mem, 64 * sizeof(uint64_t));
	}

	// write to file
	ofstream wf("test.bin", ios::out | ios::binary);
	if(!wf) {
		std::cout << "Cannot open file!" << endl;
		return 1;
	}

	wf.write((char*) inst_mem, qry.size()*64*sizeof(uint64_t));
	wf.close();

	return 0;
}






