#include "common.hpp"


int main(int argc, char *argv[]) {

  // Read arguments
  boost::program_options::options_description programDescription("Options:");
  programDescription.add_options()
    ("node,d", boost::program_options::value<uint32_t>(), "Node ID")
    ("cnttxn,c", boost::program_options::value<uint32_t>(), "Number of transactions")
    ("fname,f", boost::program_options::value<string>(), "Instruction binary filename");
  boost::program_options::variables_map commandLineArgs;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
  boost::program_options::notify(commandLineArgs);

  uint32_t node_id = commandLineArgs["node"].as<uint32_t>();
  uint32_t cnt_txn = commandLineArgs["cnttxn"].as<uint32_t>();
  string fname_task = commandLineArgs["fname"].as<string>();

  uint64_t insoffs = (((uint64_t)512 << 10) << 10);

  PR_HEADER("PARAMS");

  // get fpga handle
  cProcess cproc(0, getpid());

  initTxnCmd(cproc, cnt_txn, fname_task, insoffs);
  txnManCnfg(cproc, node_id, cnt_txn, insoffs);
  std::cout<< "Txn Manager Config Finnished."<< std::endl;
  txnManStart(cproc);
  std::cout<< "Txn Manager Started."<< std::endl;
  sleep(2);
  txnManPrtStatus(cproc);
  std::cout<< "Txn Manager Status Printed."<< std::endl;

  return EXIT_SUCCESS;
}

