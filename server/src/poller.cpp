/*
 * @Author Shi Zhangkun
 * @Date 2021-09-03 16:36:44
 * @LastEditTime 2021-09-10 13:48:19
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/src/poller.cpp
 */
#include "poller.h"
#include "event.h"
#include "fdOperate.hpp"
#include <sys/epoll.h>
#include <unistd.h>

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
Poller::Poller()
{
  epfd = epoll_create(MAX_EVENT_SIZE);
  pipe(aweakfd);
  if (aweakfd[0] >= 0 && epfd >= 0)
  {
    struct epoll_event ev;
    ev.data.fd = aweakfd[0];
    ev.events = EPOLLET|EPOLLRDHUP|EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,aweakfd[0],&ev);
  }
  
  events = new struct epoll_event[MAX_EVENT_SIZE];
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
Poller::~Poller()
{
  close(aweakfd[1]);
  close(aweakfd[0]);
  close(epfd);
  
  
  delete events;
}
/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
bool Poller::addEvent(const Event& event)
{
  struct epoll_event ev;
  ev.data.fd = event.fd;
  ev.events = EPOLLET|EPOLLRDHUP;
  if (event.mask & Event::MASK_EVENT_IN)
    ev.events |= EPOLLIN;
  if (event.mask & Event::MASK_EVENT_OUT)
    ev.events |= EPOLLOUT;
  auto ret = epoll_ctl(epfd,EPOLL_CTL_ADD,event.fd,&ev);
  return ret >= 0;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
void Poller::deletEvent(const Event& event)
{
  struct epoll_event ev;
  ev.data.fd = event.fd;
  epoll_ctl(epfd,EPOLL_CTL_DEL,event.fd,&ev);
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
void Poller::modifyEvent(const Event& event)
{
  struct epoll_event ev;
  ev.data.fd = event.fd;
  ev.events = EPOLLET|EPOLLHUP;
  if (event.mask & Event::MASK_EVENT_IN)
    ev.events |= EPOLLIN;
  if (event.mask & Event::MASK_EVENT_OUT)
    ev.events |= EPOLLOUT;
  epoll_ctl(epfd,EPOLL_CTL_MOD,event.fd,&ev);
}

void Poller::poll(std::queue<std::pair<int,eventType_t>>& fired, std::time_t tm)
{

  int retval = epoll_wait(epfd, events, MAX_EVENT_SIZE, tm);
  for (int i = 0; i < retval; i++)
  {
    if (events[i].data.fd == aweakfd[0])
    {
      char buf[64];
      while (read(aweakfd[0], buf, sizeof(buf)) >= 64);
    }
    
    if (events[i].events & EPOLLRDHUP)
      fired.push(std::pair<int,eventType_t>(static_cast<int>(events[i].data.fd), EVENT_CLOSE));
    else if (events[i].events & EPOLLIN)
      fired.push(std::pair<int,eventType_t>(static_cast<int>(events[i].data.fd), EVENT_READ));
    else if (events[i].events & EPOLLOUT)
      fired.push(std::pair<int,eventType_t>(static_cast<int>(events[i].data.fd), EVENT_WRITE));
  }
}

void Poller::aweak()
{
  if (aweakfd[1] >= 0)
  {
    write(aweakfd[1], "w", sizeof("w"));
  }
}