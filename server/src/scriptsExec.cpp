/*
 * @Author Shi Zhangkun
 * @Date 2021-09-13 16:31:10
 * @LastEditTime 2021-11-23 16:57:13
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /cmp_offloading/server/src/scriptsExec.cpp
 */

#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include "CJsonObject/CJsonObject.hpp"
#include "eventHandler.h"
#include "base64.h"
#include <wait.h>


/**
 * @brief  
 * @note  
 * @param {string} input : 输入json串，格式{"script":"aaa.py", "inputs":[{"name":"xxx", "type":"l", "value":123}]}}
 * @param {string} from : 来自边缘设备的URL
 * @retval none
 */
EventHandlerCMD::ExecStatus EventHandlerCMD::exec(std::string input, std::string from, std::string& ret)
{
  neb::CJsonObject json;
  std::string scriptName;
  std::string scriptPath;
  std::string ip;

  if (!json.Parse(input) || !json.Get("script", scriptName)) return EXEC_INPUT_FMT_ERR;

  for (auto& c : from)
  {
    if (c == ':') break;
    ip.push_back(c);
  }
  scriptPath = scriptsCacheDir;
  scriptPath += from + '/';
  {
    std::string path = scriptPath + scriptName;
    if(access(path.c_str(), F_OK) < 0)
    {
      if (!downloadScript(from, scriptName)) return EXEC_NO_FILE;
    }
  }
  
  if (scriptName.substr(scriptName.size() - 3) == ".py")
  {
    char buf[1024];
    std::string formatJson = json["inputs"].ToString();
    formatJson = base64_encode(formatJson);
    std::string cmd = "./pythonRunner.elf ";
    cmd += formatJson + ' ';
    cmd += scriptPath + ' ';
    cmd += scriptName + ' ';
    cmd += ip;
    auto fp = popen(cmd.c_str(), "r");
    if(fp){
      
      while (1)
      {
        auto size = fread(buf,1,sizeof(buf),fp);
        if (size <= 0) break;
        for (decltype(size) i = 0; i < size; i++)
          ret.push_back(buf[i]);
      }
    }
    else
    {
      return EXEC_FAIL;
    }
    pclose(fp);
    return EXEC_OK;
  }
  return EXEC_FAIL;
}

