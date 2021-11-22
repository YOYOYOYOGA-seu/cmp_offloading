/*
 * @Author Shi Zhangkun
 * @Date 2021-09-16 13:55:47
 * @LastEditTime 2021-11-22 17:24:45
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /client/src/cmpOffloading.cpp
 */
#include "cmpOffloading.h"
#include <chrono>
#include <cstring>
#include <deque>
#include <netdb.h>
#include <cassert>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include "CJsonObject.hpp"
#include "cmpOffProto.hpp"
#include "base64.h"
#define CENTER_SCHEDER_PORT 1120
#define OFD_SERV_PORT       1118
#define REDIS_PASSWOAD      "nangao_box"
#define REFREASH_IP_TIME_OUT 500  //单位ms

/**
 * @brief  
 * @note  
 * @param {int} fd
 * @retval none
 */
inline bool fd_setNoBlock(int fd)
{
  int opts = fcntl(fd,F_GETFL);
  opts = opts|O_NONBLOCK;
  if(opts < 0 || fcntl(fd,F_SETFL,opts) < 0) //设置为非阻塞socket
  {
    return false;
  }
  return true;
}

/**
 * @brief  
 * @note  
 * @param {string} url
 * @retval none
 */
std::string urlToIp(std::string url)
{
  hostent * ent = gethostbyname(url.c_str());
  if (!ent) return "";
	sockaddr_in addr;
	memcpy(&addr.sin_addr,ent->h_addr,ent->h_length);
	auto ip_addr = inet_ntoa(addr.sin_addr);
	return ip_addr;
}

/**
 * @brief  
 * @note  
 * @param {string} url
 * @param {bool} distr
 * @retval none
 */
CmpOffloadingCil::CmpOffloadingCil(std::string url, bool distr)
{
  auto&& ip = urlToIp(url);
  if (ip.size())
  {
    if (distr)
    {
      centerServIp = inet_addr(ip.c_str());
      connectToCenterServ();
    }
    else
    {
      servIP = inet_addr(ip.c_str());
      connectToServ();
    }
  }
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
CmpOffloadingCil::~CmpOffloadingCil()
{
  if (centerServfd)
    close(centerServfd);
  if (servfd)
    close(servfd);

}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
bool CmpOffloadingCil::connectToCenterServ()
{
  return false;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
bool CmpOffloadingCil::connectToServ()
{
  if (servfd > 0) return true;
  if (servIP == 0) return false;
  struct sockaddr_in  addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = servIP;
  addr.sin_port = htons(OFD_SERV_PORT);
  servfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (servfd <= 0) return false;

  if(fd_setNoBlock(servfd))
  {
    auto ret = connect(servfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (ret == 0) 
    {
      return true;
    }
    else
    {
      struct timeval timeo;
      fd_set fdw;
      int err;
      FD_ZERO(&fdw);
      FD_SET(servfd,&fdw);
      timeo.tv_sec = 1;
      timeo.tv_usec = 0;
      // 判断是否连接成功，超时时间为1s
      if(select(servfd + 1, nullptr, &fdw, nullptr, &timeo) > 0)
      {
        socklen_t error_len = sizeof(int);
        if(FD_ISSET(servfd,&fdw) && getsockopt(servfd, SOL_SOCKET, SO_ERROR, &err, &error_len) == 0 && err == 0)
          return true;
        err = 0;
      }
    }
  }

  close(servfd);
  servfd = -1;
  return false;

}

/**
 * @brief  
 * @note  
 * @param {timeval} timeo
 * @retval none
 */
bool CmpOffloadingCil::sendPack(ProPack& pack, struct timeval timeo)
{
  std::deque<unsigned char> recvBuf;
  auto ret = write(servfd, pack.package().data(), pack.package().size());
  if (ret < 0)
  {
    close(servfd);
    servfd = -1;
    return false;
  }

  fd_set fdw;
  FD_ZERO(&fdw);
  FD_SET(servfd,&fdw);
  if(select(servfd + 1, &fdw, nullptr, nullptr, &timeo) > 0)
  {
#define BUF_SIZE 1024
    unsigned char buf[BUF_SIZE];
    int nsize = 0;
    ioctl(servfd, FIONREAD, &nsize);
    int Count = nsize % BUF_SIZE;
    for (int j = 0; j <= Count; j++)
    {
      auto toRead = nsize > BUF_SIZE ? BUF_SIZE : nsize;
      nsize -= BUF_SIZE;
      auto size = ::read(servfd, buf, toRead);
      if (size <= 0) break;
      for (int i = 0; i < size; i++)
        recvBuf.push_back(buf[i]);
    }
#undef BUF_SIZE
    pack.parseFrom(recvBuf);
    if (pack.ifVaild()) return true;
  }
  return false;

}

/**
 * @brief  
 * @note  
 * @param {string} prefix
 * @retval none
 */
bool CmpOffloadingCil::sendScript(std::string& path, std::string& script, std::string prefix)
{

  if (servIP == 0) return false;
  auto redisCon = redisConnectWithTimeout(inet_ntoa({servIP}), 6379, timeval{.tv_sec = 1, .tv_usec = 0});
  if (!redisCon) return false;
  redisReply* p = static_cast<redisReply*>(redisCommand(redisCon,"AUTH " REDIS_PASSWOAD)); //密钥验证
  if(p == nullptr || std::string(p->str) != "OK")
  {
    redisFree(redisCon);
    if (p) freeReplyObject(p);
    return false;
  }
    
  std::string url = path + script;
  std::ifstream scrFile(url,std::ios::in);
  if (scrFile.is_open() == false) {
    return false;
  }
  
  std::stringstream buffer;
  buffer << scrFile.rdbuf();
  std::string text(buffer.str());
  scrFile.close();
  text = base64_encode(text);
  freeReplyObject(p);

  prefix += '/';
  prefix += script;
  p = static_cast<redisReply*>(redisCommand(redisCon, "SET %s %s", prefix.c_str(), text.c_str()));
  if(p == nullptr || std::string(p->str) != "OK")
  {
    redisFree(redisCon);
    if (p) freeReplyObject(p);
    return false;
  }
  
  redisFree(redisCon);
  freeReplyObject(p);
  return true;
}



/**
 * @brief  执行任务
 * @note  
 * @param {*}
 * @retval none
 */
cptResultCode_t CmpOffloadingCil::exec(std::map<std::string,const CComVar*>& inputs, std::map<std::string, CComVar>& outputs, \
                                  std::string script, std::string path, std::shared_timed_mutex *mutex, struct timeval timeo)
{
  using namespace std::literals;
  auto parseReturnJson = [](neb::CJsonObject payload, std::map<std::string,const CComVar*>& inputs, std::map<std::string, CComVar>& outputs, std::shared_timed_mutex *mutex)
  {
    cptResultCode_t ret =  SUCCESS_WITHOUT_DATA_CHANGE;
    auto& outputJson = payload["outputs"];
    if (mutex) mutex->lock();
    for (int i = 0; i < outputJson.GetArraySize(); i++)
    {
      gva::CVariant var(DATATYPEKIND_NOTYPE);
      std::string name;
      std::string type;
      std::string value;
      if (!outputJson[i].Get("name", name) || !outputJson[i].Get("type", type) || !outputJson[i].Get("value", value)) 
        continue; 
      if(name.find("cpt-") != 0)  //如果键开头不是“cpt-"前缀则加入前缀
        name = std::string("cpt-") + name;

      switch (type[0])
      {
      case 'b':
        var = (value == "true" ? true : false);
        break;
      case 'c':
        var = (value[0]);
        break;
      case 'i':
        var = (std::stoi(value));
        break;
      case 'l':
        var = (std::stol(value));
        break;
      case 'f':
        var = (std::stof(value));
        break;
      case 'd':
        var = (std::stod(value));
        break;
      case 's':
        var = (value);
        break;

      default:
        break;
      }
      if(outputs.find(name) == outputs.end())
      {
        outputs.emplace(name, CComVar(EDGE_COMPUTING_NODE, name));
        inputs[name] = &outputs[name];
      }
      outputs[name].setValue(var);
      if(outputs[name].dirty)
        ret = SUCCESS_WITH_DATA_CHANGE;
    }
    if (mutex) mutex->unlock();  
    return ret;   
  };

  neb::CJsonObject payload;
  ProPack pack;
  std::vector<unsigned char> temp;
  payload.Add("script", script);
  payload.AddEmptySubArray("inputs");
  if (mutex) mutex->lock_shared();
  for (auto itor = inputs.begin() ; itor != inputs.end(); itor++)
  {
    neb::CJsonObject jsonTemp;
    jsonTemp.Add("name", itor->first);
    if (itor->second->curVar().getSize() > 100) //限制单个vector变量的数组长度
      continue;
    if (itor->second->curVar().getType() == DATATYPEKIND_BOOLEAN) 
      jsonTemp.Add("type", "b");
    else if (itor->second->curVar().getType() == DATATYPEKIND_SBYTE)
      jsonTemp.Add("type", "c");
    else if (itor->second->curVar().getType() <= DATATYPEKIND_INT32)
      jsonTemp.Add("type", "i");
    else if (itor->second->curVar().getType() <= DATATYPEKIND_UINT64)
      jsonTemp.Add("type", "l");
    else if (itor->second->curVar().getType() == DATATYPEKIND_FLOAT)
      jsonTemp.Add("type", "f");
    else if (itor->second->curVar().getType() == DATATYPEKIND_DOUBLE)
      jsonTemp.Add("type", "d");
    else if (itor->second->curVar().getType() == DATATYPEKIND_STRING)
      jsonTemp.Add("type", "s");
    else if (itor->second->curVar().getType() == DATATYPEKIND_BOOLEAN_VECTOR) 
      jsonTemp.Add("type", "B");
    else if (itor->second->curVar().getType() <= DATATYPEKIND_INT32_VECTOR)
      jsonTemp.Add("type", "I");
    else if (itor->second->curVar().getType() <= DATATYPEKIND_UINT64_VECTOR)
      jsonTemp.Add("type", "L");
    else if (itor->second->curVar().getType() <= DATATYPEKIND_DOUBLE_VECTOR)
      jsonTemp.Add("type", "d");
    else if (itor->second->curVar().getType() == DATATYPEKIND_STRING_VECTOR)
      jsonTemp.Add("type", "S");
    else
      continue;
    if (itor->second->curVar().ifVectorType())
    {
      auto& vect = itor->second->curVar();
      jsonTemp.AddEmptySubArray("value");
      for (unsigned int i = 0; i < vect.getSize(); i++)
      {
        if (vect.getType() == DATATYPEKIND_BOOLEAN_VECTOR)
        {
          bool var;
          vect[i].get(var);
          jsonTemp["value"].Add(var);
        }
        else if (vect.getType() <= DATATYPEKIND_INT32_VECTOR)
        {
          int var;
          vect[i].get(var);
          jsonTemp["value"].Add(var);
        }
        else if (vect.getType() <= DATATYPEKIND_UINT64_VECTOR)
        {
          int64 var;
          vect[i].get(var);
          jsonTemp["value"].Add(var);
        }
        else if (vect.getType() <= DATATYPEKIND_DOUBLE_VECTOR)
        {
          double var;
          vect[i].get(var);
          jsonTemp["value"].Add(var);
        }
        else if (vect.getType() == DATATYPEKIND_STRING_VECTOR)
        {
          std::string var;
          vect[i].get(var);
          jsonTemp["value"].Add(var);
        }
      }
    }
    else
    {
      jsonTemp.Add("value", itor->second->curVar().toString());
    }
      
    payload["inputs"].Add(jsonTemp);
  }
  if (mutex) mutex->unlock_shared();
  auto&& payloadStr = payload.ToString();
  
  for (auto& c : payloadStr)
  {
    temp.push_back(c);
  }
  pack.generate(ProPack::CMD_EXEC, std::move(temp), false);
  lastTimeCost = -1;
  const auto startTime = std::chrono::steady_clock::now();
  if (!available())
  {
    std::string retString;
    if (execLocally(payload["inputs"], path, script, retString))
    {
      if (payload.Parse(retString))
      {
        auto ret = parseReturnJson(payload, inputs, outputs, mutex);
        const auto endTime = std::chrono::steady_clock::now(); 
        lastTimeCost = ((endTime - startTime) / 1ms);
        return ret;
      }
    }
  }
  else if (sendPack(pack, timeo))
  {
    if (pack.status() == ProPack::STATUS_OK)
    {
      if (payload.Parse(reinterpret_cast<char*>(pack.data().data())))
      {
        auto ret = parseReturnJson(payload, inputs, outputs, mutex);
        const auto endTime = std::chrono::steady_clock::now(); 
        lastTimeCost = ((endTime - startTime) / 1ms);
        return ret;
      }
    }
    else if (pack.status() == ProPack::STATUS_UNINIT)
    {
      std::string prefixName(reinterpret_cast<char*>(pack.data().data()));
      sendScript(path, script, prefixName);
    }
  }
  return EXEC_FAILURE;
}

bool CmpOffloadingCil::execLocally(neb::CJsonObject inputs, std::string path, std::string script, std::string& ret)
{
  neb::CJsonObject json;
  std::string from = "localhost";
  std::string fileUrl = path + script;
  if(access(fileUrl.c_str(), F_OK) < 0)
  {
    return false;
  }

  if (script.substr(script.size() - 3) == ".py")
  {
    char buf[1024];
    std::string formatJson = inputs.ToString();
    formatJson = base64_encode(formatJson);
    std::string cmd = "./pythonRunner.elf ";
    cmd += formatJson + ' ';
    cmd += path + ' ';
    cmd += script + ' ';
    cmd += from;
    auto fp = popen(cmd.c_str(), "r");
    if(fp){
      
      while (1)
      {
        auto size = fread(buf,1,sizeof(buf),fp);
        if (size <= 0) break;
        for (decltype(size) i = 0; i < size; i++)
          ret.push_back(buf[i]);
      }
    }
    else
    {
      return false;
    }
    pclose(fp);
    return true;
  }
  return false;
}
/**
 * @brief  获取上一次卸载执行的耗时
 * @note  
 * @param {*}
 * @retval none
 */
unsigned int CmpOffloadingCil::execTimeCost()
{
  return lastTimeCost;
}
/**
 * @brief  刷新当前执行服务器IP
 * @note  
 * @param {*}
 * @retval none
 */
bool CmpOffloadingCil::reFreshServIP()
{
  if (centerServIp) //如果有中心调度服务器
  {
    if (centerServfd >= 0 || connectToServ())
    {
      ProPack pack;
      pack.generate(ProPack::CMD_GET_IP);
      timeval timeo = {.tv_sec = REFREASH_IP_TIME_OUT/1000, .tv_usec = (REFREASH_IP_TIME_OUT%1000) * 1000};
      if (sendPack(pack, timeo)) //从中心调度服务器获取新的卸载服务器ip
      {
        if (pack.status() == ProPack::STATUS_OK)
        {
          auto&& temp = pack.data();
          decltype(servIP) newServIP = (static_cast<unsigned int>(temp[3]) << 24) + (static_cast<int>(temp[2]) << 16) + \
                    (static_cast<unsigned int>(temp[1]) << 8) + (static_cast<unsigned int>(temp[0]));
          if (newServIP != servIP) //新的IP地址，如果变更了需要将现有的连接关闭
          {
            close(servfd);
            servfd = -1;
          }
        }
      }
    }
  }
   
  return connectToServ(); //连接到新的serv，同时也起到断线重连的作用
}

/**
 * @brief  返回当前卸载是否可用
 * @note  
 * @param {*}
 * @retval none
 */
bool CmpOffloadingCil::available()
{
  return (servfd >= 0);
}