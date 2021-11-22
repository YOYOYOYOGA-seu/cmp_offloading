/*
 * @Author Shi Zhangkun
 * @Date 2021-09-06 16:32:54
 * @LastEditTime 2021-09-26 14:44:35
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/inc/eventHandler.h
 */
#ifndef __EVENTHANDLER_H
#define __EVENTHANDLER_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <map>
#include <atomic>
#include <unordered_map>
#include <memory>
#include "hiredis/hiredis.h"
#include "event.h"
#include "poller.h"
#include "spinLock.h"

class EventHandler{
protected:
  static const char* scriptsCacheDir;
  bool cleanCacheDir(std::string name = {});
private:
  EventLoop* pEventLoop = nullptr;
  std::condition_variable_any cv; //通用条件变量（可使用任何满足要求的锁）
  SpinLock firedLock;
  std::queue<Event> fired;
  std::atomic<int> parallels; // -1 析构 0 非异步  >0 并行数
  std::vector<std::thread> handlerThreads;
  virtual bool handler_callback(Event&& event, std::queue<Event>& waits) = 0;
  virtual Event beforeHandlerEvent(Event& event); //预处理（如读取原始数据等）
  void eventHandlerThreadTask(void);
  void execCallback(Event&& event);
  EventHandler& operator=(const EventHandler& hd) = delete;
  EventHandler(const EventHandler& hd) = delete;
public:
  EventHandler(int parallels = 1);
  virtual ~EventHandler();
  bool setEventLoop(EventLoop* p);
  bool handlerEvent(const EventLoop* p, Event& event);

  void replysEvent(Event&& event);
};

class EventHandlerAccept : public EventHandler{
  bool handler_callback(Event&& event, std::queue<Event>& waits) override;
public:
  EventHandlerAccept(int parallels = 1) : EventHandler(parallels){}
};

class EventHandlerCMD : public EventHandler{
public:
  enum ExecStatus{
    EXEC_OK,
    EXEC_FAIL,
    EXEC_NO_FILE,
    EXEC_INPUT_FMT_ERR
  };
private:

  redisContext *redisCli = nullptr;
  
  bool checkRedisConnection();
  bool redisConnect();
  bool redisDisConnect();
  bool downloadScript(const std::string& from, const std::string& name);
  ExecStatus exec(std::string input, std::string from, std::string& ret);
  bool handler_callback(Event&& event, std::queue<Event>& waits) override;
  Event beforeHandlerEvent(Event& event) override;
public:
  EventHandlerCMD(int parallels = 1);
  ~EventHandlerCMD() override;
};

class EventHandlerReply : public EventHandler{
  bool handler_callback(Event&& event, std::queue<Event>& waits) override;
  Event beforeHandlerEvent(Event& event) override;
public:
  EventHandlerReply(int parallels = 1) : EventHandler(parallels){}
};

class EventHandlerClose : public EventHandler{
  bool handler_callback(Event&& event, std::queue<Event>& waits) override;
public:
  EventHandlerClose(int parallels = 1) : EventHandler(parallels){}
};
#endif