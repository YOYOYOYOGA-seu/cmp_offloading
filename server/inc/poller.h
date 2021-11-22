/*
 * @Author Shi Zhangkun
 * @Date 2021-08-31 17:37:38
 * @LastEditTime 2021-09-09 17:35:50
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/inc/poller.h
 */
#ifndef __POLLER_H
#define __POLLER_H
#include <vector>
#include <queue>
#include <ctime>
#include <sys/epoll.h>
class EventLoop;
class Event;
enum eventType_t : short{
  EVENT_NULL = -1,
  EVENT_READ,
  EVENT_WRITE,
  EVENT_CLOSE,
  EVENT_TYPE_COUNTS
};
class Poller{
private:
  int listenCounts = 0;
  int aweakfd[2] = {-1};
  int epfd = -1;
  struct epoll_event *events = nullptr;
public:
  Poller();
  ~Poller();
  bool addEvent(const Event& event);
  void deletEvent(const Event& event);
  void modifyEvent(const Event& event);
  void poll(std::queue<std::pair<int,eventType_t>>& fired, std::time_t tm);
  void aweak();
};
#endif