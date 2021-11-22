/*
 * @Author Shi Zhangkun
 * @Date 2021-08-31 14:56:34
 * @LastEditTime 2021-10-11 10:27:20
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/inc/event.h
 */
#ifndef __EVENT_H
#define __EVENT_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <map>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <string>
#include "poller.h"
#include "spinLock.h"

#define MAX_EVENT_SIZE 1024

class EventLoop;



enum handlerType_t {
  HANDLER_NULL = -1,
  HANDLER_ACCEPT,
  HANDLER_SCH_ACCEPT,
  HANDLER_CMD,
  HANDLER_REPLY,
  HANDLER_CLOSE,
  HANDLER_TYPE_COUNTS
};

class Event{
public:
  static const short MASK_EVENT_ADD = 0x01;
  static const short MASK_EVENT_DEL = 0x02;
  static const short MASK_EVENT_IN = 0x04;
  static const short MASK_EVENT_OUT = 0x08;
public:
  int fd = -1;
  short mask = 0;
  handlerType_t handlers[EVENT_TYPE_COUNTS] = {HANDLER_NULL};
  std::deque<unsigned char> recv;
  std::vector<unsigned char> send;
  std::string name;
  Event() = default;
  Event(Event&& event);
  Event(const Event& event);
  Event(int fd, std::string str = std::string());
  Event& operator=(Event&& event);
  Event& operator=(const Event& event);
  int read(void);
  int write(void);
};

#endif