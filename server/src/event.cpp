/*
 * @Author Shi Zhangkun
 * @Date 2021-08-31 20:23:25
 * @LastEditTime 2021-10-12 17:55:06
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/src/event.cpp
 */
#include "mtThread.hpp"
#include "event.h"
#include "config.h"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

/**
 * @brief  
 * @note  
 * @param {int} fd
 * @retval none
 */
Event::Event(int fd, std::string str) 
:fd(fd), name(std::move(str))
{
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
Event::Event(Event&& event)
:fd(event.fd), mask(event.mask)
{
  for (int i = 0; i < EVENT_TYPE_COUNTS; i++)
  {
    handlers[i] = event.handlers[i];
  }
  recv = std::move(event.recv);
  send = std::move(event.send);
  name = std::move(event.name);
  event.recv.clear();
  event.send.clear();
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
Event::Event(const Event& event)
:fd(event.fd), mask(event.mask), recv(event.recv), send(event.send), name(event.name)
{
  for (int i = 0; i < EVENT_TYPE_COUNTS; i++)
  {
    handlers[i] = event.handlers[i];
  }
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
Event& Event::operator=(Event&& event)
{
  fd = event.fd;
  mask = event.mask;
  for (int i = 0; i < EVENT_TYPE_COUNTS; i++)
  {
    handlers[i] = event.handlers[i];
  }
  recv = std::move(event.recv);
  send = std::move(event.send);
  name = std::move(event.name);
  event.recv.clear();
  event.send.clear();
  return *this;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
Event& Event::operator=(const Event& event)
{
  fd = event.fd;
  mask = event.mask;
  for (int i = 0; i < EVENT_TYPE_COUNTS; i++)
  {
    handlers[i] = event.handlers[i];
  }
  recv = event.recv;
  send = event.send;
  name = event.name;
  return *this;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
int Event::read()
{
#define BUF_SIZE 1024
  unsigned char buf[BUF_SIZE];
  int nsize = 0;
  ioctl(fd, FIONREAD, &nsize);
  int Count = nsize % BUF_SIZE;
  for (int j = 0; j <= Count; j++)
  {
    auto toRead = nsize > BUF_SIZE ? BUF_SIZE : nsize;
    nsize -= BUF_SIZE;
    auto size = ::read(fd, buf, toRead);
    if (size <= 0) break;
    for (int i = 0; i < size; i++)
      recv.push_back(buf[i]);
  }
  if (ECHO_RW)
  {
    std::cout << "[event read]<" << name << "> : ";
    for(auto& c : recv)
    {
      std::cout << std::hex << static_cast<int>(c) << ' ';;
    }
    std::cout << std::dec << std::endl;
  }
  return recv.size();
#undef BUF_SIZE
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
int Event::write()
{
  if (ECHO_RW)
  {
    std::cout << "[event write]<" << name << "> : ";
    for(auto& c : send)
    {
      std::cout << std::hex << static_cast<int>(c) << ' ';
    }
    std::cout << std::dec << std::endl;
  }
  return ::write(fd, send.data(), send.size());
}

