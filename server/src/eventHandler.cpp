/*
 * @Author Shi Zhangkun
 * @Date 2021-09-06 16:45:26
 * @LastEditTime 2021-09-26 15:06:06
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/src/eventHandler.cpp
 */
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "mtThread.hpp"
#include "eventHandler.h"
#include "eventLoop.h"
#include "fdOperate.hpp"

const char* EventHandler::scriptsCacheDir = "./scripts/";

/**
 * @brief  
 * @note  
 * @param {bool} ifAsync
 * @param {callback_t} callback
 * @retval none
 */
EventHandler::EventHandler(int parallels) 
{
  this->parallels = parallels;
  for (int i = 0; i < parallels; i++)
  {
    handlerThreads.push_back(std::thread(&EventHandler::eventHandlerThreadTask, this));
  }

}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
EventHandler::~EventHandler()
{
  parallels = -1;
  cv.notify_all();
  for (auto& th : handlerThreads)
  {
    if (th.joinable())
      th.join();
  }
}

/**
 * @brief  
 * @note  
 * @param {string} name
 * @retval none
 */
bool EventHandler::cleanCacheDir(std::string name)
{
  std::string cmd = "rm -rf ";
  cmd += scriptsCacheDir;
  cmd += name;
  system(cmd.c_str());
  return true;

}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
Event EventHandler::beforeHandlerEvent(Event& event)
{
  return event;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
void EventHandler::eventHandlerThreadTask()
{
  auto pred = [this](){return (this->fired.size() > 0);};
  while(parallels > 0)
  {
    std::unique_lock<decltype(firedLock)> lck(firedLock);
    cv.wait(lck, pred);
    Event event = std::move(fired.front());
    fired.pop();
    lck.unlock(); //修改完队列后（条件）一定要释放锁！！！！

    execCallback(std::move(event));
  }
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
void EventHandler::execCallback(Event&& event)
{
  
  std::queue<Event> waits;
  if(handler_callback(std::forward<Event>(event),waits))
    replysEvent(std::forward<Event>(event));
  
  while (pEventLoop && waits.size())
  {
    if (waits.front().mask & Event::MASK_EVENT_DEL)
      pEventLoop->deletEvent(waits.front().fd);
    else if (waits.front().mask & Event::MASK_EVENT_ADD)
      pEventLoop->addEvent(std::move(waits.front()));
    else
      pEventLoop->modifyEvent(std::move(waits.front()));
    waits.pop();
  }
}

/**
 * @brief  
 * @note  
 * @param {EventLoop*} p
 * @retval none
 */
bool EventHandler::setEventLoop(EventLoop* p)
{
  if (pEventLoop != nullptr)
  {
    std::unique_lock<decltype(firedLock)> lck(firedLock);
    fired = std::queue<Event>();
  }
  pEventLoop = p;
  return true;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
bool EventHandler::handlerEvent(const EventLoop* p, Event& event)
{
  if (p != pEventLoop) return false;
  auto&& temp = beforeHandlerEvent(event);
  if (parallels == 0) //非异步
  {
    execCallback(std::forward<Event>(temp));
  }
  else
  {
    std::unique_lock<decltype(firedLock)> lck(firedLock);
    fired.push(temp);
    lck.unlock(); //notify_one不属于被保护的范围，提前解锁以减小锁的粒度
    cv.notify_one();
  }
  return true;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
void EventHandler::replysEvent(Event&& event)
{
  if (!pEventLoop) return;
  pEventLoop->addReplys(std::forward<Event>(event));
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
bool EventHandlerAccept::handler_callback(Event&& event, std::queue<Event>& waits)
{
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
      std::string name = inet_ntoa(addr.sin_addr);
      name += ':';
      name += std::to_string(addr.sin_port);
      Event temp(fd, name);
      temp.mask = Event::MASK_EVENT_ADD | Event::MASK_EVENT_IN;
      temp.handlers[EVENT_READ] = HANDLER_CMD;
      temp.handlers[EVENT_WRITE] = HANDLER_REPLY;
      temp.handlers[EVENT_CLOSE] = HANDLER_CLOSE;
      waits.push(std::move(temp));
    }
  }
  return false;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
bool EventHandlerReply::handler_callback(Event&& event, std::queue<Event>& waits)
{
  event.write();
  event.send.clear();
  return false;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
Event EventHandlerReply::beforeHandlerEvent(Event& event)
{
  Event ret(event.fd, event.name);
  ret.send = std::move(event.send);
  return ret;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
bool EventHandlerClose::handler_callback(Event&& event, std::queue<Event>& waits)
{
  event.mask = Event::MASK_EVENT_DEL;
  cleanCacheDir(event.name);
  waits.push(std::forward<Event>(event));
  return false;
}