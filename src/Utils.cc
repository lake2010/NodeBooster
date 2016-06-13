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
#include "Utils.h"

#include "bitcoin/util.h"

static const char _hexchars[] = "0123456789abcdef";

static inline int _hex2bin_char(const char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return (c - 'a') + 10;
  if (c >= 'A' && c <= 'F')
    return (c - 'A') + 10;
  return -1;
}

bool Hex2Bin(const char *in, size_t size, vector<char> &out) {
  out.clear();
  out.reserve(size/2);

  uint8 h, l;
  // skip space, 0x
  const char *psz = in;
  while (isspace(*psz))
    psz++;
  if (psz[0] == '0' && tolower(psz[1]) == 'x')
    psz += 2;

  // convert
  while (psz + 2 <= (char *)in + size) {
    h = _hex2bin_char(*psz++);
    l = _hex2bin_char(*psz++);
    out.push_back((h << 4) | l);
  }
  return true;

}

bool Hex2Bin(const char *in, vector<char> &out) {
  out.clear();
  out.reserve(strlen(in)/2);

  uint8 h, l;
  // skip space, 0x
  const char *psz = in;
  while (isspace(*psz))
    psz++;
  if (psz[0] == '0' && tolower(psz[1]) == 'x')
    psz += 2;

  if (strlen(psz) % 2 == 1) { return false; }

  // convert
  while (*psz != '\0' && *(psz + 1) != '\0') {
    h = _hex2bin_char(*psz++);
    l = _hex2bin_char(*psz++);
    out.push_back((h << 4) | l);
  }
  return true;
}

void Bin2Hex(const uint8 *in, size_t len, string &str) {
  str.clear();
  const uint8 *p = in;
  while (len--) {
    str.push_back(_hexchars[p[0] >> 4]);
    str.push_back(_hexchars[p[0] & 0xf]);
    ++p;
  }
}

void Bin2Hex(const vector<char> &in, string &str) {
  Bin2Hex((uint8 *)in.data(), in.size(), str);
}

void Bin2HexR(const vector<char> &in, string &str) {
  vector<char> r;
  for (auto it = in.rbegin(); it != in.rend(); ++it) {
    r.push_back(*it);
  }
  Bin2Hex(r, str);
}

string EncodeHexTx(const CTransaction& tx) {
  CDataStream ssTx(SER_NETWORK, BITCOIN_PROTOCOL_VERSION);
  ssTx << tx;
  return HexStr(ssTx.begin(), ssTx.end());
}

string EncodeHexBlock(const CBlock &block) {
  CDataStream ssBlock(SER_NETWORK, BITCOIN_PROTOCOL_VERSION);
  ssBlock << block;
  return HexStr(ssBlock.begin(), ssBlock.end());
}


static bool _DecodeBinTx(CTransaction& tx, vector<unsigned char> &data) {
  CDataStream ssData(data, SER_NETWORK, BITCOIN_PROTOCOL_VERSION);
  try {
    ssData >> tx;
  }
  catch (const std::exception &) {
    return false;
  }
  return true;
}

bool DecodeBinTx(CTransaction& tx, const unsigned char *data, size_t len) {
  vector<unsigned char> txData(data, data + len);
  return _DecodeBinTx(tx, txData);
}

bool DecodeHexTx(CTransaction& tx, const std::string& strHexTx)
{
  if (!IsHex(strHexTx))
    return false;

  vector<unsigned char> txData(ParseHex(strHexTx));
  return _DecodeBinTx(tx, txData);
}


static bool _DecodeBinBlk(CBlock& block, vector<unsigned char> &data) {
  CDataStream ssBlock(data, SER_NETWORK, BITCOIN_PROTOCOL_VERSION);
  try {
    ssBlock >> block;
  }
  catch (const std::exception &) {
    return false;
  }
  return true;
}

bool DecodeBinBlk(CBlock& block, const unsigned char *data, size_t len) {
  vector<unsigned char> blkData(data, data + len);
  return _DecodeBinBlk(block, blkData);
}

bool DecodeHexBlk(CBlock& block, const std::string& strHexBlk)
{
  if (!IsHex(strHexBlk))
    return false;

  std::vector<unsigned char> blkData(ParseHex(strHexBlk));
  return _DecodeBinBlk(block, blkData);
}

//  Receive 0MQ string from socket and convert into string
std::string s_recv (zmq::socket_t & socket) {
  zmq::message_t message;
  socket.recv(&message);

  return std::string(static_cast<char*>(message.data()), message.size());
}

//  Convert string to 0MQ string and send to socket
bool s_send(zmq::socket_t & socket, const std::string & string) {
  zmq::message_t message(string.size());
  memcpy(message.data(), string.data(), string.size());

  bool rc = socket.send(message);
  return (rc);
}

//  Sends string as 0MQ string, as multipart non-terminal
bool s_sendmore (zmq::socket_t & socket, const std::string & string) {
  zmq::message_t message(string.size());
  memcpy(message.data(), string.data(), string.size());

  bool rc = socket.send(message, ZMQ_SNDMORE);
  return (rc);
}
