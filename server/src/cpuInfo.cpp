/*
 * @Author Shi Zhangkun
 * @Date 2021-11-25 18:12:48
 * @LastEditTime 2021-11-25 18:16:58
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /cmp_offloading/server/src/cpuInfo.cpp
 */
#include "cpuInfo.h"

unsigned int CPUInfo::_processors = 0;

unsigned int  CPUInfo::processors()
{
  if (!_processors)
  {
    FILE* fp = popen("cat /proc/cpuinfo| grep \"processor\"| wc -l", "r");
    if (fp)
    {
      char buf[16] = {0};
      fread(buf,1, sizeof(buf) - 1, fp);
      auto ret = std::stoul(std::string(buf));
      if (ret <= MAX_PROCESSORS_NUM)
        _processors = ret;
    }
  }
  return _processors > 0 ? _processors : 1;
}

double CPUInfo::avrgLoad_1min()
{
  std::ifstream loadavg("/proc/loadavg");
  double load = -1;
  if (loadavg.is_open())
  {
    std::string avg;
    loadavg >> avg;
    load = std::stod(avg);
  }
  return load;
}