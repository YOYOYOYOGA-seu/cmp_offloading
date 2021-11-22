/*
 * @Author Shi Zhangkun
 * @Date 2021-10-09 16:48:01
 * @LastEditTime 2021-10-12 17:36:57
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/src/shceduler.cpp
 */

#include "scheduler.h"
#include "fdOperate.hpp"
#include "cmpOffProto.hpp"
#include <limits>
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

bool Scheduler::ifRun = false;
int Scheduler::listenFd = -1;
std::list<struct Scheduler::ServerDesc> Scheduler::servs;
std::mutex Scheduler::servsMut;
std::thread Scheduler::tickThread;
std::timed_mutex Scheduler::threadWait;
std::weak_ptr<EventLoop> Scheduler::pEventLoop;

Scheduler::Scheduler()
{
}

Scheduler::~Scheduler()
{
}

bool Scheduler::addServer(int fd, unsigned short port, unsigned int ipv4)
{
  if (!ifRun) return false;

  struct ServerDesc serv{.fd = fd, .port = port, .ipv4 = ipv4, .load = 0xFFFFFFFF};
  servsMut.lock();
  servs.push_back(serv);
  servsMut.unlock();
  return true;
}

unsigned int Scheduler::serverFit()
{
  return servs.front().ipv4;
}

bool Scheduler::run(unsigned short port, std::weak_ptr<EventLoop> eventLoop)
{
  auto pLoop = eventLoop.lock();
  if (pLoop == nullptr) return false;    
  stop();
  
  struct sockaddr_in  addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr("0.0.0.0");
  listenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  
  if (!fd_setNoBlock(listenFd)) 
  {
    close(listenFd);
    return false;
  }
  
  bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  listen(listenFd, 10);
  std::string name = "0.0.0.0:";
  name += std::to_string(port);
  Event listenEvent = Event(listenFd, name);
  listenEvent.handlers[EVENT_READ] = HANDLER_SCH_ACCEPT;
  listenEvent.handlers[EVENT_CLOSE] = HANDLER_CLOSE;
  listenEvent.mask = Event::MASK_EVENT_IN;

  if(pLoop->addEvent(std::move(listenEvent)))
  {
    ifRun = true;
    threadWait.try_lock();
    tickThread = std::thread(&Scheduler::tickThreadFunc, this);
    if(tickThread.joinable())
    {
      return true;
    }
  }

  stop();
  return false;
}

void Scheduler::stop()
{
  if (!ifRun) return;

  ifRun = false;
  threadWait.unlock();
  if(tickThread.joinable())
  {
    tickThread.join();
  }
  servsMut.lock();
  for (auto& serv : servs)
  {
    close(serv.fd);
  }
  servsMut.unlock();

  servs.clear();
  auto p = pEventLoop.lock();
  if (p != nullptr)
  {
    p->deletEvent(listenFd);
  }
}

void Scheduler::tickThreadFunc()
{
  addServer(-1, 0, 0);
  while (ifRun)
  {
    ProPack pack;
    pack.generate(ProPack::CMD_GET_LOAD_INFO);
    for (auto& serv : servs)
    {
      if (serv.fd > 0)
      {
        Event temp(serv.fd);
        temp.send = pack.package();
        temp.write();
      }
    }
    sleep(1);
    for (auto itor = servs.begin(); itor != servs.end(); )
    {
      auto next = itor++;
      std::swap(next, itor);
      auto& serv = *itor;
      if (serv.fd > 0)
      {
        Event temp(serv.fd);
        temp.read();
        if (temp.recv.size())
        {
          serv.faildCount = 0;
          ProPack ack;
          ack.parseFrom(temp.recv);
          if (ack.ifVaild() && ack.cmd() == ProPack::CMD_GET_LOAD_INFO)
          {
            auto data = ack.data();
            serv.load = static_cast<unsigned int>(data[0]) + static_cast<unsigned int>(data[1] << 8) \
                        + static_cast<unsigned int>(data[2] << 16) \
                        + static_cast<unsigned int>(data[3] << 24);
          }
          temp.recv.clear();
        }
        else
        {
          if(++serv.faildCount >= MAX_FAILD_COUNT)
          {
            close(serv.fd);
            servs.erase(itor);
          }
        }
      }
      else
      {
        std::ifstream loadavg("/proc/loadavg");
        std::string avg;
        loadavg >> avg;
        serv.load = 100 * std::stod(avg);
      }
      itor = next;
    }
    servs.sort([](ServerDesc& a, ServerDesc& b){
      return a.load <= b.load;
    });
    for (auto& serv : servs)
    {
      in_addr temp;
      temp.s_addr = serv.ipv4;
      std::cout << inet_ntoa(temp) << ':' << serv.port << '<' << serv.load << '>' << "\t";
    }
    std::cout << std::endl;
    threadWait.try_lock_for(std::chrono::seconds(5));
  }
}

bool EventHandlerSchedAccept::handler_callback(Event&& event, std::queue<Event>& waits)
{
  Scheduler sch;
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  auto fd = 0;
  while (fd >= 0)
  {
    fd = accept(event.fd, reinterpret_cast<sockaddr*>(&addr), &len);
    if (fd >= 0)
    {
      if (!fd_setNoBlock(fd))
      {
        close(fd);
        continue;
      }
      sch.addServer(fd, addr.sin_port, addr.sin_addr.s_addr);
    }
  }
  return false;
}