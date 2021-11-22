/*
 * @Author Shi Zhangkun
 * @Date 2021-09-01 10:37:46
 * @LastEditTime 2021-09-24 21:30:15
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/src/eventLoop.cpp
 */

#include "event.h"
#include "eventLoop.h"
#include "eventHandler.h"
#include "mtThread.hpp"
#include "fdOperate.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <sys/stat.h>
#define LISTEN_PORT 1118

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
EventLoop::EventLoop()
:handlers(HANDLER_TYPE_COUNTS)
{
  
}



/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
void EventLoop::eventLoopThreadTask(void)
{
  std::queue<std::pair<int,eventType_t>> fired;  //已激发事件队列
  while (loopEnable)
  {
    beforePoll();
    poller.poll(fired, 100);
    while (fired.size())
    {
      int ifExist = false;
      eventsMapLock.lock();
      auto itor = events.find(fired.front().first);
      auto type = fired.front().second;
      if (itor != events.end())
      {
        ifExist = true;
      }
      eventsMapLock.unlock();
      if (ifExist)
      {
        auto& eve = itor->second;
        auto handlerIndex = eve.handlers[type];
        if (handlerIndex != HANDLER_NULL && handlerIndex < HANDLER_TYPE_COUNTS
            && handlers[handlerIndex] != nullptr)
        {
          handlers[handlerIndex]->handlerEvent(this, eve);
        }
      }
      fired.pop();
    }
  }
}

void EventLoop::beforePoll()
{
  handlerReplys();
  handlerWaits();
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
void EventLoop::handlerReplys(void)
{
  int fd = -1;
  while (getQueTopSafely(replys, replysLock, fd) && fd >= 0)
  {
    bool ifExist = false;
    eventsMapLock.lock();
    auto itor = events.find(fd);
    if (itor != events.end())
    {
      ifExist = true;
    }
    eventsMapLock.unlock();

    if (ifExist)
    {
      auto handler = itor->second.handlers[EVENT_WRITE];
      handlers[handler]->handlerEvent(this, itor->second);
      /* 回复操作 */
    }
  }

}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
void EventLoop::handlerWaits(void)
{
  Event event;
  while (getQueTopSafely(waits, waitsQueLock, event))
  {
    if (event.fd < 0) continue;
    auto& mask = event.mask;
    if (mask & Event::MASK_EVENT_ADD) //新增
    {
      std::cout << "[Event Loop] : new event from <" << event.name << ">." << std::endl;
      eventsMapLock.lock();
      events[event.fd] = std::move(event);
      eventsMapLock.unlock();
      poller.addEvent(event);
    }
    else if (mask & Event::MASK_EVENT_DEL)
    {
      
      eventsMapLock.lock();
      auto itor = events.find(event.fd);
      if (itor != events.end())
      {
        event.name = std::move(itor->second.name);
        events.erase(itor);
      }
      eventsMapLock.unlock();
      poller.deletEvent(event);
      std::cout << "[Event Loop] : event deleted, name: <" << event.name << ">." << std::endl;
      close(event.fd);
    }
    else
    {
      eventsMapLock.lock();
      bool ifExist = false;
      auto itor = events.find(event.fd);
      if (itor != events.end())
      {
        ifExist = true;
        itor->second = std::move(event);
      }
      eventsMapLock.unlock();
      if (ifExist)
      {
        poller.modifyEvent(event);
      }
    }
  }
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
void EventLoop::addReplys(Event&& event)
{
  
  bool ifExist = false;
  eventsMapLock.lock();
  auto itor = events.find(event.fd);
  if (itor != events.end())
  {
    ifExist = true;
    itor->second.send = std::move(event.send);
  }
  eventsMapLock.unlock();

  if (ifExist)
  {
    replysLock.lock();
    replys.push(event.fd);
    replysLock.unlock();
    poller.aweak();
  }
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
bool EventLoop::addEvent(Event&& event)
{
  if (event.fd < 0) return false;
  event.mask |= Event::MASK_EVENT_ADD;
  event.mask &= ~(Event::MASK_EVENT_DEL);
  waitsQueLock.lock();
  waits.push(std::forward<Event>(event));
  waitsQueLock.unlock();
  return true;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
bool EventLoop::modifyEvent(Event&& event)
{
  if (event.fd < 0) return false;
  event.mask &= ~(Event::MASK_EVENT_DEL | Event::MASK_EVENT_ADD);
  waitsQueLock.lock();
  waits.push(std::forward<Event>(event));
  waitsQueLock.unlock();
  return true;
}

/**
 * @brief  
 * @note  
 * @param {int} fd
 * @retval none
 */
bool EventLoop::deletEvent(int fd)
{
  if (fd < 0) return false;
  Event event(fd);
  event.mask |= Event::MASK_EVENT_DEL;
  waitsQueLock.lock();
  waits.push(std::move(event));
  waitsQueLock.unlock();
  return true;
}

/**
 * @brief  
 * @note  
 * @param {handlerType_t} han
 * @retval none
 */
bool EventLoop::addEventHandler(handlerType_t han, std::unique_ptr<EventHandler>&& pEvent)
{
  handlers[han] = std::forward<std::unique_ptr<EventHandler>>(pEvent);
  handlers[han]->setEventLoop(this);
  return true;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
void EventLoop::loop()
{
  loopEnable = true;
  eventLoopThreadTask();
  loopEnable = false;
}