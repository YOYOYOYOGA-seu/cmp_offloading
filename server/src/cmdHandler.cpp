/*
 * @Author Shi Zhangkun
 * @Date 2021-09-13 16:06:03
 * @LastEditTime 2021-10-12 14:28:15
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/src/cmdHandler.cpp
 */

#include "eventHandler.h"
#include "cmpOffProto.hpp"
#include "base64.h"
#include "scheduler.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#define REDIS_PORT 6379
#define REDIS_PASSWOAD  "nangao_box"


EventHandlerCMD::EventHandlerCMD(int parallels)
:EventHandler(parallels)
{
  cleanCacheDir();
  //nftw(scriptsCacheDir, implementFunc, 50, FTW_DEPTH|FTW_PHYS); //清空脚本缓存文件夹
  mkdir(scriptsCacheDir, 0755);
  redisConnect();
}

EventHandlerCMD::~EventHandlerCMD()
{
  redisDisConnect();
}

bool EventHandlerCMD::checkRedisConnection()
{
  return (redisCli != nullptr && redisCli->err == REDIS_OK && (redisCli->flags & REDIS_CONNECTED) > 0);
}

bool EventHandlerCMD::redisConnect()
{
  if(checkRedisConnection())
    return true;
  redisDisConnect();
  redisCli = redisConnectWithTimeout("127.0.0.1",REDIS_PORT,{.tv_sec = 0, .tv_usec = 200000});
  if(checkRedisConnection())
  {
    auto* p = static_cast<redisReply*>(redisCommand(redisCli,"AUTH " REDIS_PASSWOAD)); //密钥验证
    if(p == nullptr || std::string(p->str) != "OK")
    {
      redisDisConnect();
      return false;
    }
    std::cerr << "[CMD Handler] connected to redis server." << std::endl;
    return true;
  }
  redisDisConnect();
  return false;
}

bool EventHandlerCMD::redisDisConnect()
{
  if(redisCli)
  {
    redisFree(redisCli);
    redisCli = nullptr;
    std::cout << "[CMD Handler] disconnected from redis server." << std::endl;
  }
  return 0;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
bool EventHandlerCMD::downloadScript(const std::string& from, const std::string& name)
{
  auto ret = false;
  std::string cmd;
  cmd += from;
  cmd += '/';
  cmd += name;
  if (!checkRedisConnection())
  {
    if(!redisConnect()) return false;
  }

  redisReply* pRet = static_cast<redisReply*>(redisCommand(redisCli, "GET %s", cmd.c_str()));
  if (pRet == nullptr ) return false;
  if (pRet->str != nullptr && pRet->type == REDIS_REPLY_STRING) 
  {
    ret = true;
    std::string path = scriptsCacheDir;
    path += from + '/';
    if(access(path.c_str(), F_OK) < 0)
      mkdir(path.c_str(), 0755);
    path += name;
    auto&& srciptContent = base64_decode(std::string(pRet->str));
    std::ofstream file(path.c_str());
    file << srciptContent;
    file.close();
  }
  freeReplyObject(pRet);
  pRet = static_cast<redisReply*>(redisCommand(redisCli, "DEL %s", cmd.c_str()));
  freeReplyObject(pRet);
  return ret;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
bool EventHandlerCMD::handler_callback(Event&& event, std::queue<Event>& waits)
{
  ProPack pack;
  while (pack.parseFrom(event.recv,false))
  {
    if (pack.cmd() == ProPack::CMD_EXEC)  //运行脚本指令
    {
      auto&& data = pack.data();
      std::string ret;
      //std::cout << "[handler]recv cmd from <" << event.name << "> : ";
      /*
      for (auto& c : data)
        std::cout << c ;
      std::cout << std::endl;
      */
      switch (exec(reinterpret_cast<char*>(data.data()), event.name, ret))
      {
      case EXEC_OK:
        {
          std::vector<unsigned char> temp(ret.size() + 1, 0);
          for (unsigned int i = 0; i < ret.size(); i++)
          {
            temp[i] = ret[i];
          }
          pack.generate(ProPack::CMD_EXEC, std::move(temp),true);
        }
        break;
      case EXEC_NO_FILE:  
        {
          std::vector<unsigned char> name;
          for (auto c : event.name)
            name.push_back(c);
          name.push_back('\0');
          pack.generate(ProPack::CMD_EXEC, ProPack::STATUS_UNINIT, std::move(name));
        }
        break;
      case EXEC_INPUT_FMT_ERR:
        pack.generate(ProPack::CMD_EXEC, ProPack::STATUS_FORMAT_ERR);
        break;
      default:
        pack.generate(ProPack::CMD_EXEC, ProPack::STATUS_CMD_FAIL);
      }
      event.send.insert(event.send.end(),pack.package().begin(), pack.package().end());
    }
    else if (pack.cmd() == ProPack::CMD_GET_LOAD_INFO)  //获取当前服务器负载指令
    {
      std::ifstream loadavg("/proc/loadavg");
      unsigned int loadPercent;
      if (loadavg.is_open())
      {
        std::string avg;
        loadavg >> avg;
        loadPercent = 100 * std::stod(avg);
        std::vector<unsigned char> data = {static_cast<unsigned char>(0xFF&loadPercent), 
                                          static_cast<unsigned char>(0xFF&(loadPercent >> 8)),
                                          static_cast<unsigned char>(0xFF&(loadPercent >> 16)), 
                                          static_cast<unsigned char>(0xFF&(loadPercent >> 24))};
        pack.generate(ProPack::CMD_GET_LOAD_INFO, std::move(data), true);
      }
      else
      {
        pack.generate(ProPack::CMD_GET_LOAD_INFO, ProPack::STATUS_CMD_FAIL);
      }
      event.send.insert(event.send.end(),pack.package().begin(), pack.package().end());
    }
    else if (pack.cmd() == ProPack::CMD_GET_IP)
    {
      Scheduler sch;
      if (sch.ifRunning())
      {
        auto ip = sch.serverFit();
        std::vector<unsigned char> data = {static_cast<unsigned char>(0xFF&ip), 
                                          static_cast<unsigned char>(0xFF&(ip >> 8)),
                                          static_cast<unsigned char>(0xFF&(ip >> 16)), 
                                          static_cast<unsigned char>(0xFF&(ip >> 24))};
        pack.generate(ProPack::CMD_GET_IP, std::move(data), true);
      }
      else
      {
        pack.generate(ProPack::CMD_GET_IP, ProPack::STATUS_CMD_FAIL);
      }
      event.send.insert(event.send.end(),pack.package().begin(), pack.package().end());
    }
  }
  return true;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
Event EventHandlerCMD::beforeHandlerEvent(Event& event)
{
  Event temp(event);
  temp.read();
  
  //裁剪末端不完整的数据包数据放回event
  return temp;
}
