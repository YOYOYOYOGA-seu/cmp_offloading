/*
 * @Author Shi Zhangkun
 * @Date 2021-10-09 16:48:37
 * @LastEditTime 2021-11-25 18:09:15
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /cmp_offloading/server/inc/scheduler.h
 */
#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <list>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include "eventHandler.h"
#include "eventLoop.h"

class Scheduler
{
  const int MAX_FAILD_COUNT = 5;
  struct ServerDesc
  {
    
    int fd;
    unsigned short port;
    unsigned int ipv4;
    double load;
    unsigned int processors;
    int faildCount = 0;
  };
private:
  static bool ifRun;
  static int listenFd;
  static std::list<struct ServerDesc> servs;
  static std::mutex servsMut;
  static std::timed_mutex threadWait;
  static std::thread tickThread;
  static std::weak_ptr<EventLoop> pEventLoop;
  /* data */
  void tickThreadFunc();
public:
  Scheduler();
  ~Scheduler();
  bool addServer(int fd, unsigned short port, unsigned int ipv4);
  unsigned int serverFit();
  bool run(unsigned short port, std::weak_ptr<EventLoop> eventLoop);
  void stop();
  bool ifRunning(){return ifRun;}
};

class EventHandlerSchedAccept : public EventHandler 
{
  bool handler_callback(Event&& event, std::queue<Event>& waits) override;
public:
  EventHandlerSchedAccept(int parallels = 1) : EventHandler(parallels){}
};


#endif


