/*
 * @Author Shi Zhangkun
 * @Date 2021-09-09 15:15:18
 * @LastEditTime 2021-09-09 17:35:59
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/inc/fdOperate.hpp
 */

#ifndef __FDOPERATE_H
#define __FDOPERATE_H
#include <fcntl.h>
#include <unistd.h>

inline bool fd_setNoBlock(int fd)
{
  int opts = fcntl(fd,F_GETFL);
  opts = opts|O_NONBLOCK;
  if(opts < 0 || fcntl(fd,F_SETFL,opts) < 0) //设置为非阻塞socket
  {
    return false;
  }
  return true;
}

#endif