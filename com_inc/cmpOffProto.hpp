/*
 * @Author Shi Zhangkun
 * @Date 2021-04-06 10:55:05
 * @LastEditTime 2021-09-23 20:13:16
 * @LastEditors Shi Zhangkun
 * @Description 串行拓展协议解析api
 * @FilePath /client/inc/cmpOffProto.hpp
 */

#ifndef __SERIALEXTPROTO_H
#define __SERIALEXTPROTO_H
#include <vector>
#include <algorithm>
#include <queue>
#include "CRC8.h"

typedef unsigned char scmd_t;
typedef unsigned char sstatus_t;
class ProPack{
public: 
  static const unsigned  int NO_CRC_LENGTH = 4;
  static const unsigned  int MIN_ACK_PACK_SIZE = 7;  
  static const unsigned  int MIN_REQ_PACK_SIZE = 6;  
  static const unsigned  int MAX_LEN = 0xFFFF;
  static const int LOCATE_HEAD = 0;
  static const int LOCATE_LENGTH = 1;
  static const int LOCATE_CMD = 3;
  static const int LOCATE_UNUSE = 4;
  static const int LOCATE_STATUS = 5;
  static const int LOCATE_REQ_DATA = 5;
  static const int LOCATE_ACK_DATA = 6;
  

  static const unsigned char PROTO_HEAD = 0xEB;
  
  static const scmd_t CMD_STATUS = 0x00;
  static const scmd_t CMD_MOD_INFO = 0x01;
  static const scmd_t CMD_GET_LOAD_INFO = 0x02;
  static const scmd_t CMD_SET = 0x20;
  static const scmd_t CMD_EXEC = 0x30;
  static const scmd_t CMD_GET_IP = 0x35;
  static const scmd_t CMD_INVAILD = 0xFF;

  static const sstatus_t STATUS_OK = 0x80;
  static const sstatus_t STATUS_CRC_ERR = 0x81;
  static const sstatus_t STATUS_PACK_TOO_LEN = 0x82;
  static const sstatus_t STATUS_CMD_UNKNOW = 0x83;
  static const sstatus_t STATUS_CMD_FAIL = 0x84;
  static const sstatus_t STATUS_HARD_FAULT = 0x85;
  static const sstatus_t STATUS_FORMAT_ERR = 0x86;
  static const sstatus_t STATUS_UNINIT = 0x87;

private:

  bool ifAck = true;
  bool vaildFlag = false;
  std::vector<unsigned char> pack;
  
  static bool ifVaildCmd(scmd_t cmd)
  {
    return (cmd == ProPack::CMD_STATUS || cmd == ProPack::CMD_GET_LOAD_INFO || cmd == ProPack::CMD_MOD_INFO || \
            cmd == ProPack::CMD_EXEC || cmd == ProPack::CMD_SET || cmd == ProPack::CMD_GET_IP);
  }
  
  bool packCRC(const std::vector<unsigned char>& pack)
  {
    return (CRC8(pack.data() + LOCATE_CMD, pack.size() - NO_CRC_LENGTH) == pack.back());
  }

  /**
   * @brief  判断是否为合法的包
   * @note  
   * @param {const} vector
   * @retval none
   */
  bool judgePackValidation(void)
  {
    vaildFlag = (pack.size() >= (ifAck ? MIN_ACK_PACK_SIZE : MIN_REQ_PACK_SIZE)  &&
    pack[0] == PROTO_HEAD && length() == pack.size() - NO_CRC_LENGTH && ifVaildCmd(pack[LOCATE_CMD]) && packCRC(pack));
    return vaildFlag;
  }

public:
  ProPack(){}
  ProPack(const std::vector<unsigned char>& buf, bool ifAck = true){
    parse(buf, ifAck);
  }

  /**
   * @brief  解析一个协议包
   * @note  
   * @param {const} std
   * @param {bool} ifAck
   * @retval 是否是一个有效的包
   */
  bool parse(const std::vector<unsigned char>& buf, bool ifAck = true)
  {
    pack = buf;
    this->ifAck = ifAck;
    if(!judgePackValidation()) pack.clear(); //不是一个合法的包
    return vaildFlag;
  }

  /**
   * @brief  
   * @note  从传入的buffer迭代器遍历获得第一个有效的包
   * @param {std::vector<unsigned char>::iterator&} first
   * @param {std::vector<unsigned char>::iterator} last
   * @retval true 剩余的buffer中可能还有有效的包  false 剩余的buffer中已经没有其他有效的包了
   */
  bool parseFrom(std::deque<unsigned char>& buffer, bool ifAck = true)
  {
    this->ifAck = ifAck;
    vaildFlag = false;
    pack.clear();
    while (buffer.size() >= (ifAck ? MIN_ACK_PACK_SIZE : MIN_REQ_PACK_SIZE))
    {
      if(buffer[0] == PROTO_HEAD)
      {
        auto len2 = buffer[LOCATE_LENGTH + 1];
        auto len1 = buffer[LOCATE_LENGTH];
        auto totalLen = ((static_cast<unsigned int>(len2) << 8) + len1) + NO_CRC_LENGTH;
        if(buffer.size() < totalLen)
          return false;
        for(unsigned int i = 0; i < totalLen; i++)
          pack.push_back(buffer[i]);
        buffer.erase(buffer.begin(), buffer.begin() + totalLen);
        if(judgePackValidation())
          return true;
        else
          pack.clear();
      }
      else buffer.erase(buffer.begin());
    }
    return false;
  }

  /**
   * @brief  生成一个数据包
   * @note  
   * @param {unsigned char} cmd
   * @param {vector<unsigned char>} data
   * @param {bool} ifAck
   * @retval 返回生成的数据包
   */
  bool generate(scmd_t cmd,std::vector<unsigned char> data = {},bool ifAck = false)
  {
    this->ifAck = ifAck;
    pack.clear();
    vaildFlag = false;
    if(ifVaildCmd(cmd))
    {
      auto len = data.size() + (ifAck ? MIN_ACK_PACK_SIZE : MIN_REQ_PACK_SIZE) - NO_CRC_LENGTH;
      if(len <= MAX_LEN)
      {
        vaildFlag = true;
        auto head = PROTO_HEAD;
        pack.push_back(head);
        pack.push_back(static_cast<unsigned char>(len & 0xFF));
        pack.push_back(static_cast<unsigned char>((len >> 8) & 0xFF));
        pack.push_back(cmd);
        pack.push_back(0x0);
        if (ifAck)
          pack.push_back(0x80);
        std::copy(data.begin(),data.end(),std::back_inserter(pack));
        pack.push_back(CRC8(pack.data() + LOCATE_CMD,len));
      }
    }
    return vaildFlag;
  }

  /**
   * @brief  
   * @note  
   * @param {scmd_t} cmd
   * @param {sstatus_t} error
   * @param {bool} ifAck
   * @retval none
   */  
  bool generate(scmd_t cmd, sstatus_t error, std::vector<unsigned char> data = {})
  {
    this->ifAck = true;
    pack.clear();
    auto len = MIN_ACK_PACK_SIZE + data.size() - NO_CRC_LENGTH;
    vaildFlag = false;
    if(ifVaildCmd(cmd))
    {
        vaildFlag = true;
        auto head = PROTO_HEAD;
        pack.push_back(head);
        pack.push_back(static_cast<unsigned char>(len & 0xFF));
        pack.push_back(static_cast<unsigned char>((len >> 8) & 0xFF));
        pack.push_back(cmd);
        pack.push_back(0x0);
        pack.push_back(error);
        std::copy(data.begin(),data.end(),std::back_inserter(pack));
        pack.push_back(CRC8(pack.data() + LOCATE_CMD,len));
    }
    return vaildFlag;
  }
  /**
   * @brief  获取包长度(不包含包头、长度码和CRC)
   * @note  需要提前验证是否为有效包
   * @param {const} vector
   * @retval none
   */
  unsigned int length(void)
  {
    if(pack.size() < NO_CRC_LENGTH  || pack[LOCATE_HEAD] != PROTO_HEAD) return 0;
    auto ret = ((static_cast<unsigned int>(pack[LOCATE_LENGTH + 1]) << 8) + pack[LOCATE_LENGTH]);
    return ret == pack.size() - NO_CRC_LENGTH ? ret : 0;
  }

  /**
   * @brief  获取原始数据包的数据
   * @note  
   * @param {*}
   * @retval none
   */
  const std::vector<unsigned char>& package(void)
  {
    return pack;
  }
  /**
   * @brief  获取数据,注意是负载的数据而不是原始包的数据
   * @note  需要提前验证是否为有效包
   * @param {const} std
   * @param {bool} ifAck
   * @retval none
   */
  std::vector<unsigned char> data(void)
  {
    auto len = length() - (ifAck ? MIN_ACK_PACK_SIZE : MIN_REQ_PACK_SIZE) + NO_CRC_LENGTH;
    std::vector<unsigned char> ret(len);
    std::copy_n(pack.begin() + (ifAck ? LOCATE_ACK_DATA : LOCATE_REQ_DATA), len, ret.begin());
    return ret;
  }
  
  /**
   * @brief  当前是否存储了一个有效包
   * @note  
   * @param {*}
   * @retval none
   */
  bool ifVaild(void)
  {
    return vaildFlag;
  }

  /**
   * @brief  获取数据包类型
   * @note  
   * @param {*}
   * @retval true :recvPack  false : sendPack
   */
  bool type(void)
  {
    return ifAck;
  }

  /**
   * @brief  获得状态码
   * @note  
   * @param {*}
   * @retval none
   */
 sstatus_t status(void)
  {
    if(vaildFlag && ifAck)
    {
      return pack[LOCATE_STATUS];
    }
    return 0;
  }

/**
 * @brief  获得指令码
 * @note  
 * @param {*}
 * @retval none
 */  
  scmd_t cmd(void)
  {
    if(vaildFlag )
    {
      return pack[LOCATE_CMD];
    }
    return 0;
  }
  
};

#endif