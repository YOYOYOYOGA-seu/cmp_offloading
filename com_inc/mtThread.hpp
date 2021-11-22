/*
 * @Author Shi Zhangkun
 * @Date 2021-08-31 19:58:23
 * @LastEditTime 2021-09-02 20:50:01
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/inc/mtThread.hpp
 */
#ifndef __MTTHREAD_H
#define __MTTHREAD_H
#include <queue>
#include "spinLock.h"

template<typename T, class LCK = SpinLock, typename QUE = std::queue<T>>
bool getQueTopSafely(QUE& que, LCK& lck, T& res)
{
  lck.lock();
  auto ret = false;
  if (que.size())
  {
    res = que.front();
    que.pop();
    ret = true;
  }
  else
  {
    ret = false;
  }
  lck.unlock();
  return ret;
}

#endif