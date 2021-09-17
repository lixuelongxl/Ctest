/*
 * =====================================================================================
 *
 *       Filename:  rocksdb_direct_context.h
 *
 *    Description:  rocksdb direct context class definition.
 *
 *        Version:  1.0
 *        Created:  09/08/2020
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zhulin, shzhulin3@jd.com
 *        Company:  JD.com, Inc.
 *
 * =====================================================================================
 */

#ifndef __ROCKSDB_DIRECT_CONTEXT_H__
#define __ROCKSDB_DIRECT_CONTEXT_H__

#if 1

#include <stdint.h>
#include <string>
#include <vector>
#include <deque>
#include <assert.h>
#include <string.h>

static const uint16_t sgMagicNum = 12345; // global magic number

// operator must be matched with DTC with the same order
enum CondOpr
{
  eEQ, // ==
  eNE, // !=
  eLT, // <
  eLE, // <=
  eGT, // >
  eGE // >=
};

#pragma pack(push, 1)
struct QueryCond
{
  uint8_t sFieldIndex;
  uint8_t sCondOpr; // CondOpr
  std::string sCondValue;
 
private:
  int binarySize()
  {
    static int fixHeaderLen = sizeof(sFieldIndex) + sizeof(sCondOpr) + sizeof(int)/* value len */;
    return fixHeaderLen + sCondValue.length();
  }

  void serializeTo(char* &data)
  {
    *(uint8_t*)data = sFieldIndex;
    data += sizeof(uint8_t);

    *(uint8_t*)data = sCondOpr;
    data += sizeof(uint8_t);
    
    *(int*)data = sCondValue.length();
    data += sizeof(int);

    memmove((void*)data, (void*)sCondValue.c_str(), sCondValue.length());
    data += sCondValue.length();
  }
  
  void serializeFrom(const char* data, int& condLen)
  {
    const char *begPos = data;

    sFieldIndex = *(uint8_t*)data;
    data += sizeof(uint8_t);

    sCondOpr = *(uint8_t*)data;
    data += sizeof(uint8_t);

    int len = *(int*)data;
    data += sizeof(int);
    
    sCondValue.assign(data, len);
    condLen = data - begPos + len;
  }

  friend class DirectRequestContext;
};

struct LimitCond
{
  int sLimitStart;
  int sLimitStep;
  LimitCond(){
    sLimitStart = 0;
    sLimitStep = 10;
  }

  void reset() { sLimitStart = -1, sLimitStep = -1; }
};

struct DirectRequestContext
{
  uint16_t sMagicNum;
  uint64_t sSequenceId;

  std::vector<QueryCond> sFieldConds;
  std::vector<std::pair<int, bool/* asc or not*/> > sOrderbyFields;
  LimitCond sLimitCond;

  void reset()
  {
    sMagicNum = 0;
    sSequenceId = 0;
    sFieldConds.clear();
    sLimitCond.reset();
  }
  
  // binary format size for transporting in
  int binarySize()
  {
    static int fixHeaderLen = sizeof(sMagicNum) + sizeof(sSequenceId) + sizeof(uint8_t) * 2;
    
    int len = fixHeaderLen;
    for ( size_t idx = 0; idx < sFieldConds.size(); idx++ )
    {
      len += sFieldConds[idx].binarySize();
    }
    for (size_t idx = 0; idx < sOrderbyFields.size(); idx++)
    {
      len += (sizeof(int) + sizeof(bool));      
    }  
    len += sizeof(LimitCond);

    return len;
  }
  
  // before call this function, should call 'binarySize' to evaluate the size of the struct
  void serializeTo(char *data, int len)
  {
    *(uint16_t*)data = sMagicNum;
    data += sizeof(uint16_t);

    *(uint64_t*)data = sSequenceId;
    data += sizeof(uint64_t);
    
    *(uint8_t*)data = sFieldConds.size();
    data += sizeof(uint8_t);

    for ( size_t idx = 0; idx < sFieldConds.size(); idx++ )
    {
      sFieldConds[idx].serializeTo(data);
    }

    *(uint8_t*)data = sOrderbyFields.size();
    data += sizeof(uint8_t);
    for ( size_t idx = 0; idx < sOrderbyFields.size(); idx++ )
    {
      *(int*)data = sOrderbyFields[idx].first;
      data += sizeof(int);
      *(bool*)data = sOrderbyFields[idx].second;
      data += sizeof(bool);
    }

    memmove((void*)data, (void*)&sLimitCond, sizeof(LimitCond));
    data += sizeof(LimitCond);
    log_debug("serializeTo, sLimitStart: %d, sLimitStep: %d", sLimitCond.sLimitStart, sLimitCond.sLimitStep);
  }

  void serializeFrom(const char *data, int dataLen)
  {
    sMagicNum = *(uint16_t*)data;
    data += sizeof(uint16_t);
    dataLen -= sizeof(uint16_t);

    sSequenceId = *(uint64_t*)data;
    data += sizeof(uint64_t);
    dataLen -= sizeof(uint64_t);
    
    uint8_t condNum = *(uint8_t*)data;
    data += sizeof(uint8_t);
    dataLen -= sizeof(uint8_t);

    QueryCond cond;
    int condLen = 0;
    for ( uint8_t idx = 0; idx < condNum; idx++ )
    {
      cond.serializeFrom(data, condLen);
      data += condLen;
      dataLen -= condLen;

      sFieldConds.push_back(cond);
    }
    std::pair<int, bool> orPair;
    uint8_t orderNum = *(uint8_t*)data;
    data += sizeof(uint8_t);
    dataLen -= sizeof(uint8_t);
    for ( uint8_t idx = 0; idx < orderNum; idx++ )
    {
      orPair.first = *(int*)data;
      data += sizeof(int);
      dataLen -= sizeof(int);

      orPair.second = *(bool*)data;
      data += sizeof(bool);
      dataLen -= sizeof(bool);

      sOrderbyFields.push_back(orPair);
    }
    memmove((void*)&sLimitCond, (void*)data, sizeof(LimitCond));
    dataLen -= sizeof(LimitCond);
    log_debug("serializeFrom, sLimitStart: %d, sLimitStep: %d", sLimitCond.sLimitStart, sLimitCond.sLimitStep);

    assert( dataLen == 0 );
  }
};

struct DirectResponseContext
{
  uint16_t sMagicNum;
  uint64_t sSequenceId;
  int16_t sRowNums; // number of matched rows or errno
  std::deque<std::string> sRowValues;
  
  void serializeTo(std::string& data)
  {
    static int headerLen = sizeof(uint16_t) + sizeof(uint64_t) + sizeof(int16_t);

    data.clear();
    data = (std::string((char*)this, headerLen));
    
    for ( int16_t idx = 0; idx < sRowNums; idx++ )
    {
      data.append(sRowValues.front());
      sRowValues.pop_front();
    }
  }
  
  void free()
  {
    sMagicNum = 0;
    sSequenceId = 0;
    sRowNums = -1;
    sRowValues.clear();
  }
};
#pragma pack(pop)

#endif

#endif // __ROCKSDB_DIRECT_CONTEXT_H__
