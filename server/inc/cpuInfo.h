/*
 * @Author Shi Zhangkun
 * @Date 2021-11-25 17:40:46
 * @LastEditTime 2021-11-25 18:15:44
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /cmp_offloading/server/inc/cpuInfo.h
 */
#ifndef __CPU_INFO_H
#define __CPU_INFO_H

#include <cstdlib>
#include <stdio.h>
#include <string>
#include <fstream>

class CPUInfo
{
  #define MAX_PROCESSORS_NUM 128
private:
  static unsigned int _processors;
public:
  CPUInfo()=default;
  static unsigned int processors();
  static double avrgLoad_1min();
};

#endif
// cat /proc/cpuinfo| grep "processor"| wc -l