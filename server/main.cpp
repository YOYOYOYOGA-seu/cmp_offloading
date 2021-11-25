/*
 * @Author Shi Zhangkun
 * @Date 2021-02-04 11:14:24
 * @LastEditTime 2021-11-25 17:49:32
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /cmp_offloading/server/main.cpp
 */

#include "eventHandler.h"
#include "eventLoop.h"
#include "scheduler.h"
#include "fdOperate.hpp"
#include "cpuInfo.h"
#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <netdb.h>
#include <cassert>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#define CENTER_SCHEDER_PORT 1120
#define LISTEN_PORT         1118
#define STR1(R)  #R  
#define STR2(R)  STR1(R)
void serv_exit(int sig)
{
  Scheduler sch;
  sch.stop();
  exit(0);
}

std::string urlToIp(std::string url)
{
  hostent * ent = gethostbyname(url.c_str());
  if (!ent) return "";
	sockaddr_in addr;
	memcpy(&addr.sin_addr,ent->h_addr,ent->h_length);
	auto ip_addr = inet_ntoa(addr.sin_addr);
	return ip_addr;
}


bool createListenEvent(Event& acceptEvent)
{
  int fd;
  struct sockaddr_in  addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(LISTEN_PORT);
  addr.sin_addr.s_addr = inet_addr("0.0.0.0");
  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (!fd_setNoBlock(fd)) 
  {
    close(fd);
    return false;
  }
  
  bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  listen(fd, 10);
  acceptEvent = Event(fd, "0.0.0.0:" STR2(LISTEN_PORT));
  acceptEvent.handlers[EVENT_READ] = HANDLER_ACCEPT;
  acceptEvent.handlers[EVENT_CLOSE] = HANDLER_CLOSE;
  acceptEvent.mask = Event::MASK_EVENT_IN;
  return true;
}

bool connectToMasterServ(Event& event, std::string url)
{
  int fd;
  struct sockaddr_in  addr;
  std::string ip;
  ip = urlToIp(url);
  if (ip.size() == 0) return false;
  
  addr.sin_family = AF_INET;
  addr.sin_port = htons(CENTER_SCHEDER_PORT);
  addr.sin_addr.s_addr = inet_addr(ip.c_str());
  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 || !fd_setNoBlock(fd))
  {
    close(fd);
    return false;
  }

  std::string name = ip + ":" STR2(CENTER_SCHEDER_PORT);
  event = Event(fd, name.c_str());
  event.handlers[EVENT_READ] = HANDLER_CMD;
  event.handlers[EVENT_WRITE] = HANDLER_REPLY;
  event.handlers[EVENT_CLOSE] = HANDLER_CLOSE;
  event.mask = Event::MASK_EVENT_IN;
  return true;
}

int main(int argc, const char* argv[])
{
  signal(SIGINT, serv_exit);
  std::string masterUrl;

   for (int i = 1; i < argc; i++)
  {
    std::string temp(argv[i]);
    if (temp[0] != '-')
    {
      masterUrl = temp;
    }
  }

  Event acceptEvent;
  if (!createListenEvent(acceptEvent)) return 0;
  

  std::shared_ptr<EventLoop> pLoop = std::make_shared<EventLoop>();
  pLoop->addEventHandler(HANDLER_ACCEPT, std::make_unique<EventHandlerAccept>(0));
  pLoop->addEventHandler(HANDLER_SCH_ACCEPT, std::make_unique<EventHandlerSchedAccept>(0));
  pLoop->addEventHandler(HANDLER_CMD, std::make_unique<EventHandlerCMD>(CPUInfo::processors()));
  pLoop->addEventHandler(HANDLER_REPLY, std::make_unique<EventHandlerReply>(0));
  pLoop->addEventHandler(HANDLER_CLOSE, std::make_unique<EventHandlerClose>(0));
  pLoop->addEvent(std::move(acceptEvent));

  
  if (masterUrl.size()) // 从服务器模式
  {
    Event eventSchCon;
    if(!connectToMasterServ(eventSchCon, masterUrl)) 
    {
      std::cerr << "[server boot] failed to connect to master server." << std::endl;
      return 0;
    }
    pLoop->addEvent(std::move(eventSchCon));
  }
  else   // master服务器模式
  {
    Scheduler sch;
    if(!sch.run(CENTER_SCHEDER_PORT, pLoop)) 
    {
      std::cerr << "[server boot] failed to center scheduler." << std::endl;
      return 0;
    }
  }

  pLoop->loop();
}