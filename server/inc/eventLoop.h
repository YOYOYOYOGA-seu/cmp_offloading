/*
 * @Author Shi Zhangkun
 * @Date 2021-09-06 16:33:02
 * @LastEditTime 2021-09-24 17:10:01
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/inc/eventLoop.h
 */
#ifndef __EVENTLOOP_H
#define __EVENTLOOP_H

#include <vector>
#include <thread>
#include <queue>
#include <atomic>
#include <unordered_map>
#include <memory>
#include "event.h"
#include "eventHandler.h"
#include "poller.h"
#include "spinLock.h"

class EventLoop{
  friend void EventHandler::replysEvent(Event&& event);
private:
  std::vector<std::unique_ptr<EventHandler>> handlers;
  SpinLock waitsQueLock;
  std::queue<Event> waits;  //待操作队列
  SpinLock eventsMapLock;
  std::unordered_map<int, Event> events;
  SpinLock replysLock;
  std::queue<int> replys; //待回复事件队列
  Poller poller;
  bool loopEnable = false;
  void eventLoopThreadTask(void);
  void beforePoll(void);
  void handlerWaits(void);
  void handlerReplys(void);
  void addReplys(Event&& event);
  
public:
  EventLoop();
  void loop();
  bool addEvent(Event&& event);
  bool modifyEvent(Event&& event);
  bool deletEvent(int fd);
  bool addEventHandler(handlerType_t han, std::unique_ptr<EventHandler>&& pEvent);
};

#endif