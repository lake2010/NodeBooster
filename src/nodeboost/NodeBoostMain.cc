#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstdio>

#include <glog/logging.h>
#include <libconfig.h++>

#include "zmq.hpp"

#include "Utils.h"
#include "NodeBoost.h"

using namespace std;
using namespace libconfig;

NodeBoost *gNodeBoost = nullptr;

void handler(int sig) {
  if (gNodeBoost) {
    gNodeBoost->stop();
  }
}

void usage() {
  fprintf(stderr, "Usage:\n\tnodeboost -c \"nodeboost.cfg\" -l \"log_dir\"\n");
}

int main(int argc, char **argv) {
  char *optLogDir = NULL;
  char *optConf   = NULL;
  int c;

  if (argc <= 1) {
    usage();
    return 1;
  }
  while ((c = getopt(argc, argv, "c:l:h")) != -1) {
    switch (c) {
      case 'c':
        optConf = optarg;
        break;
      case 'l':
        optLogDir = optarg;
        break;
      case 'h': default:
        usage();
        exit(0);
    }
  }

  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);
  FLAGS_log_dir = string(optLogDir);
  FLAGS_logbuflevel = -1;

  // Read the file. If there is an error, report it and exit.
  Config cfg;
  try
  {
    cfg.readFile(optConf);
  } catch(const FileIOException &fioex) {
    std::cerr << "I/O error while reading file." << std::endl;
    return(EXIT_FAILURE);
  } catch(const ParseException &pex) {
    std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
    << " - " << pex.getError() << std::endl;
    return(EXIT_FAILURE);
  }

  TxRepo txrepo;
  gNodeBoost = new NodeBoost(cfg.lookup("server.publish_addr"),
                             cfg.lookup("server.response_addr"),
                             cfg.lookup("bitcoind.sub_addr"),
                             &txrepo);

  // connect peers
  {
    const Setting &peers = cfg.lookup("peers");
    for (int i = 0; i < peers.getLength(); i++) {
      string subAddr, reqAddr;
      peers[i].lookupValue("publish_addr",  subAddr);
      peers[i].lookupValue("response_addr", reqAddr);
      gNodeBoost->peerConnect(subAddr, reqAddr);
    }
  }

  gNodeBoost->run();

  return 0;
}



int main1(int argc, char **argv) {
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);
  FLAGS_log_dir = "./log";

  LOG(INFO) << "NodeBoost";

  Config cfg;
  // Read the file. If there is an error, report it and exit.
  try
  {
    cfg.readFile("example.cfg");
  }
  catch(const FileIOException &fioex)
  {
    std::cerr << "I/O error while reading file." << std::endl;
    return(EXIT_FAILURE);
  }
  catch(const ParseException &pex)
  {
    std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
    << " - " << pex.getError() << std::endl;
    return(EXIT_FAILURE);
  }

  //  Prepare our context and subscriber
  zmq::context_t context(1);
  zmq::socket_t subscriber(context, ZMQ_SUB);
  subscriber.connect("tcp://10.45.15.220:10000");
  subscriber.setsockopt(ZMQ_SUBSCRIBE, "rawblock", 8);
  subscriber.setsockopt(ZMQ_SUBSCRIBE, "rawtx",    5);

  while (1) {
    //  Read message contents
    std::string type    = s_recv(subscriber);
    std::string content = s_recv(subscriber);

    std::cout << "[" << type << "]: " << content.length() << std::endl;

    if (type == "rawtx") {
      CTransaction tx;
      DecodeBinTx(tx, (const unsigned char *)content.data(), content.length());
      cout << "tx: " << tx.GetHash().ToString() << endl << EncodeHexTx(tx) << endl;
    }
//    std::cout << "[" << type << "]: " << content << std::endl;
  }

  return 0;
}
