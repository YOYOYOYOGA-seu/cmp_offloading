/*
 * @Author Shi Zhangkun
 * @Date 2021-08-31 19:04:49
 * @LastEditTime 2021-08-31 19:04:49
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/inc/spinLock.h
 */
#ifndef __SPINLOCK_HPP
#define __SPINLOCK_HPP
#include <atomic>

/* 
 *  自旋锁
 */
class SpinLock {
private:
  //声明自旋锁不可被复制
  SpinLock(SpinLock &) {}  
  SpinLock& operator= (const SpinLock &) {return *this;} 
  SpinLock(SpinLock &&) {} 
  
 std::atomic<bool> flag_;
public:
 SpinLock() : flag_(false){}
 void lock(){
  bool expect = false;
  while (!flag_.compare_exchange_weak(expect, true))
  {
    //这里一定要将expect复原，执行失败时expect结果是未定的
    expect = false;
  }
 }
 void unlock(){
 flag_.store(false);
 }
};
#endif