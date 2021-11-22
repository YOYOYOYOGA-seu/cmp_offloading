/*
 * @Author Shi Zhangkun
 * @Date 2021-09-15 15:19:44
 * @LastEditTime 2021-11-22 18:51:34
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /cmp_offloading/client/inc/cmpOffloading.h
 */
#ifndef __CMPOFFLOADING_H
#define __CMPOFFLOADING_H
#include <hiredis/hiredis.h>
#include <netinet/in.h>
#include <string>
#include <shared_mutex>
#include <vector>
#include <map>
#include "CJsonObject/CJsonObject.hpp"
#include "edgeCptFunc.h"
#include "cmpOffProto.hpp"
#include "common.h"
class CmpOffloadingCil{
private:
  in_addr_t centerServIp = 0; //中心调度服务器IP
  in_addr_t servIP = 0; //当前活动执行服务器IP
  int centerServfd = -1; //中心调度服务器连接
  int servfd = -1;  //当前活动执行服务连接

  unsigned int lastTimeCost = -1; //上次执行耗时（-1代表失败）
  bool execLocally(neb::CJsonObject inputs, std::string path, std::string script, std::string& ret);
  bool sendPack(ProPack& pack, struct timeval timeo);
  bool sendScript(std::string& path, std::string& script, std::string prefix);
  bool connectToCenterServ();
  bool connectToServ();
  CmpOffloadingCil(const CmpOffloadingCil& cli)  = delete;
  CmpOffloadingCil& operator=(const CmpOffloadingCil& cli) = delete;
public:
  CmpOffloadingCil(std::string url, bool distr = false);
  ~CmpOffloadingCil();
  bool reFreshServIP();
  bool available();
  unsigned int execTimeCost(); 
  cptResultCode_t exec(std::map<std::string,const CComVar*>& inputs, std::map<std::string, CComVar>& outputs, std::string script, \
                         std::string path, std::shared_timed_mutex *mutex = nullptr, struct timeval timeo = {5,0});
};

#endif