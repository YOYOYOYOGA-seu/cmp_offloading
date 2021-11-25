/*
 * @Author Shi Zhangkun
 * @Date 2021-02-04 11:14:24
 * @LastEditTime 2021-11-25 15:45:59
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /cmp_offloading/client/main.cpp
 */
#include <map>
#include <string>
#include <unistd.h>
#include <iostream>
#include "cmpOffloading.h"
#include "common.h"

std::string detachName(std::string& path)
{
  std::string ret;
  for (unsigned int i = path.size() - 1; i >= 0; i--)
  {
    if (path[i] == '/')
    {
      ret = path.substr(i + 1);
      path = path.substr(0,i + 1);
      break;
    }
  }
  return ret;
}
int main(int args, const char** argv){
  int counts = 1;
  std::string url("127.0.0.1");
  std::string path;
  for (int i = 1; i < args; i++)
  {
    std::string temp(argv[i]);
    if (temp == "-c")
    {
      if (i + 1 < args)
      {
        counts = std::stoi(argv[++i]);
        if (counts < 1) counts = 1;
      }
      else
      {
        std::cerr << "need args after -c." << std::endl;
        return -1;
      }
    }
    else if (temp == "-s")
    {
      if (i + 1 < args)
      {
        url = argv[++i];
      }
      else
      {
        std::cerr << "need args after -s." << std::endl;
        return -1;
      }
    }
    else
    {
      path = temp;
    }
  }
  
  
  std::string name = std::move(detachName(path));
  
  
  std::cout << "Exec scripts : " << name << " to : " << url << ", path : " << path << " , counts : " << counts << std::endl;
	CmpOffloadingCil cli(url,true);
  std::map<std::string, CComVar> outputs;
  CComVar test1(DEVICE_DATA,"testInt");
  test1.setValue(100);
  CComVar test2(DEVICE_DATA,"testStr");
  std::vector<std::string> strVect = {"hello world", " 22222"};
  std::map<std::string, const CComVar*> inputs = {{"dev-testInt", &test1},{"dev-testStr", &test2}};
  test2.setValue(strVect);
  
  for (int i = 0; i <= counts; i++)
  { 
    cli.reFreshServIP();
    if(cli.exec(inputs, outputs, name, path) == SUCCESS_WITH_DATA_CHANGE)
    {
      std::cout << "exec results (" << i << "):" << std::endl;
      for (auto& v : outputs)
      {
        std::cout << "[" << v.first << "] : " << v.second.curVar().toString() << " ";
      }
      std::cout << std::endl;
      std::cout << "using time: <" <<cli.execTimeCost()/1000 << "s:" << cli.execTimeCost()%1000 << "ms>." << std::endl; 
    } 
  }
	return 0;
}