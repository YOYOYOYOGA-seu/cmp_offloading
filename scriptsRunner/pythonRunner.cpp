/*
 * @Author Shi Zhangkun
 * @Date 2021-09-24 20:05:39
 * @LastEditTime 2021-11-17 18:31:27
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /server/scriptsRunner/pythonRunner.cpp
 */
#if  defined (__arm__)
#include "python3.5m/Python.h"
#else
#include "python3.8/Python.h"
#endif
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include "CJsonObject.hpp"
#include "base64.h"
#include <string>
#include <vector>
#include <iostream>

/**
 * @brief  判断是否为python脚本文件
 * @note  
 * @param {string} name
 * @retval none
 */
bool ifPythonFile(std::string name)
{
  if(name.size() < 4)
    return false;
  if(name.substr(name.size() - 3, name.size()) != ".py")
    return false;
  return true;
}

/**
 * @brief  去除python脚本拓展名后缀
 * @note  
 * @param {string} &name
 * @retval none
 */
bool pyFileRemoveSuffix(std::string &name)
{
  if(ifPythonFile(name))
  {
    name = name.substr(0, name.size() - 3);
    return true;
  }
  return false;
}

//int main(neb::CJsonObject& inputs, const std::string& path, std::string name, std::string& ret)
int main(int args, const char** argv)
{
  std::string inputsStr = argv[1];
  inputsStr = base64_decode(inputsStr);
  neb::CJsonObject inputs(inputsStr);
  std::string path(argv[2]);
  std::string name(argv[3]);
  std::string from(argv[4]);
  std::string ret;
  Py_Initialize();
	// 检查初始化是否成功  
	if (!Py_IsInitialized())
	{
		Py_Finalize();
    return -1;
	}
  
	PyObject *pModule;
	PyObject*pFunc = NULL;
	PyRun_SimpleString("import sys");
  std::string pathAppend = "sys.path.append('";
  pathAppend += path;
  pathAppend += "')";
	PyRun_SimpleString(pathAppend.c_str());//设置python模块，搜寻位置，文件放在.cpp文件一起
	
  if(pyFileRemoveSuffix(name) == false)
  {
    Py_Finalize();
    return -1;
  }
    
  
	pModule = PyImport_ImportModule(name.c_str());//Python文件名     
	if (!pModule) {
		Py_Finalize();
    return -1;
	}
	else
  {
		pFunc = PyObject_GetAttrString(pModule, "main");//Python文件中的函数名  
		if (!pFunc) {
			Py_Finalize();
      return -1;
		}
		
    PyObject* pArgs = PyTuple_New(1);
    PyObject* pDict = PyDict_New();
    PyDict_SetItemString(pDict, "__from_ip", Py_BuildValue("s", from.c_str()));

    for(auto i = 0; i < inputs.GetArraySize(); i++)
    {
      std::string name;
      std::string type;
      inputs[i].Get("name", name);
      inputs[i].Get("type", type);
      if (type[0] >= 'a' && type[0] <= 'z')
      {
        std::string value;
        inputs[i].Get("value", value);
        switch (type[0])
        {
        case 'b':
          PyDict_SetItemString(pDict, name.c_str(), Py_BuildValue("i", value == "true" ? 1 : 0));
          break;
        case 'c':
          PyDict_SetItemString(pDict, name.c_str(), Py_BuildValue("c", value[0]));
          break;
        case 'i':
          PyDict_SetItemString(pDict, name.c_str(), Py_BuildValue("i", std::stoi(value)));
          break;
        case 'l':
          PyDict_SetItemString(pDict, name.c_str(), Py_BuildValue("l", std::stol(value)));
          break;
        case 'f':
          PyDict_SetItemString(pDict,name.c_str(), Py_BuildValue("f", std::stof(value)));
          break;
        case 'd':
          PyDict_SetItemString(pDict, name.c_str(), Py_BuildValue("d", std::stod(value)));
          break;
        case 's':
          PyDict_SetItemString(pDict, name.c_str(), Py_BuildValue("s", value.c_str()));
          break;
        default:
          break;
        }
      }
      else if (type[0] >= 'A' && type[0] <= 'Z')
      {
        neb::CJsonObject value;
        inputs[i].Get("value", value);
        auto size = value.GetArraySize();
        if (size > 0)
        {
          auto list = PyList_New(size);
          switch (type[0])
          {
          case 'I':
            for (int i = 0; i < size; i++)
            {
              int temp;
              value.Get(i, temp);
              PyList_SetItem(list, i, Py_BuildValue("i", temp));
            }
            PyDict_SetItemString(pDict, name.c_str(), list);
            break;
          case 'L':
            for (int i = 0; i < size; i++)
            {
              long temp;
              value.Get(i, temp);
              PyList_SetItem(list, i, Py_BuildValue("l", temp));
            }
            PyDict_SetItemString(pDict, name.c_str(), list);
            break;  
          case 'S':
            for (int i = 0; i < size; i++)
            {
              std::string temp;
              value.Get(i, temp);
              PyList_SetItem(list, i, Py_BuildValue("s", temp.c_str()));
            }
            PyDict_SetItemString(pDict, name.c_str(), list);
            break;
          case 'd':
            for (int i = 0; i < size; i++)
            {
              double temp;
              value.Get(i, temp);
              PyList_SetItem(list, i, Py_BuildValue("d", temp));
            }
            PyDict_SetItemString(pDict, name.c_str(), list);
            break;
          default:
            break;
          }
        }
        
      }
    }

    PyTuple_SetItem(pArgs, 0, pDict);
		PyObject *pReturn = nullptr;
		pReturn = PyEval_CallObject(pFunc, pArgs);//调用函数

    PyObject* value = nullptr;
    PyObject* key = nullptr;
    Py_ssize_t pos = 0;
    neb::CJsonObject retJson;
    retJson.AddEmptySubArray("outputs");
    while(pReturn != nullptr && PyDict_Next(pReturn, &pos, &key, &value))
    { 
      
      std::string keyStr;
      if(PyUnicode_Check(key))
      {
        neb::CJsonObject node;
        std::string keyTemp = PyUnicode_AsUTF8(key);
        if(keyTemp.size() <= 0)
          continue;
        
        if(keyTemp.find("cpt-") != 0)  //如果键开头不是“cpt-"前缀则加入前缀
          keyStr = "cpt-";
        keyStr.append(keyTemp);
        node.Add("name",keyStr);

        if(PyUnicode_Check(value))
        {
          const char* valueTemp = PyUnicode_AsUTF8(value);
          if(valueTemp == nullptr)
            continue;
          node.Add("type", "s");
          node.Add("value", std::string(valueTemp));
        }
        else if(PyLong_Check(value))
        {
          node.Add("type", "l");
          node.Add("value", std::to_string(PyLong_AsLong(value)));
        }
        else if(PyFloat_Check(value))
        {
          node.Add("type", "d");
          node.Add("value", std::to_string(PyFloat_AsDouble(value)));
        }
        else
        {
          continue;
        }
        retJson["outputs"].Add(node);
      }
    }
    ret = retJson.ToString();
    std::cout << ret;
    Py_Finalize();
    return 0;
  }
}