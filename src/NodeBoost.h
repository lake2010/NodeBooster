/*
 The MIT License (MIT)

 Copyright (c) [2016] [BTC.COM]

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/
#include "bitcoin/base58.h"
#include "bitcoin/core.h"
#include "bitcoin/util.h"

#include "zmq.hpp"

#include "Common.h"
#include "Utils.h"


#define MSG_PUB_THIN_BLOCK "BLOCK_THIN"

#define MSG_CMD_LEN 20
#define MSG_CMD_GET_TXS "get_txs"


class NodePeer;
class TxRepo;
class NodeBoost;

/////////////////////////////////// NodePeer ///////////////////////////////////
class NodePeer {
  atomic<bool> running_;
  zmq::socket_t  *zmqSub_;  // sub-pub
  zmq::socket_t  *zmqReq_;  // req-rep

  string subAddr_;
  string reqAddr_;

  NodeBoost *nodeBoost_;
  TxRepo *txRepo_;

  void sendMissingTxs(const vector<uint256> &missingTxs);
  void recvMissingTxs();

  bool buildBlock(const string &thinBlock, CBlock &block);

public:
  NodePeer(const string &subAddr, const string &reqAddr,
           zmq::context_t *zmqContext, NodeBoost *nodeBoost,
           TxRepo *txRepo);
  ~NodePeer();

  bool isFinish();
  void stop();
  void run();
};


//////////////////////////////////// TxRepo ////////////////////////////////////
class TxRepo {
  mutex lock_;
  std::map<uint256, CTransaction> txsPool_;

public:
  TxRepo();
  ~TxRepo();

  bool isExist(const uint256 &hash);
  bool getTx(const uint256 &hash, CTransaction &tx);
  void AddTx(const CTransaction &tx);
};


/////////////////////////////////// NodeBoost //////////////////////////////////
class NodeBoost {
  atomic<bool> running_;

  zmq::context_t zmqContext_;
  zmq::socket_t *zmqRep_;  // response
  zmq::socket_t *zmqPub_;  // publish

  string zmqPubAddr_;
  string zmqRepAddr_;
  string zmqBitcoind_;

  TxRepo *txRepo_;

  std::set<NodePeer *> peers_;

  void peerCloseAll();

  void threadZmqPublish();
  void threadZmqResponse();
  void threadListenBitcoind();

  void threadPeerConnect(const string &subAddr, const string &reqAddr);

  void buildRepGetTxs(const zmq::message_t &zin, zmq::message_t &zout);

public:
  NodeBoost(const string &zmqPubAddr, const string &zmqRepAddr,
            const string &zmqBitcoind,
            TxRepo *txRepo);
  ~NodeBoost();

  void peerConnect(const string &subAddr, const string &reqAddr);

  void findMissingTxs(const string &thinBlock, vector<uint256> &missingTxs);

  void run();
  void stop();
  void submitBlock(const CBlock &block);
};

