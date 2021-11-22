/*
 * @Author Shi Zhangkun
 * @Date 2021-09-16 18:15:28
 * @LastEditTime 2021-09-23 16:06:58
 * @LastEditors Shi Zhangkun
 * @Description none
 * @FilePath /client/inc/common.h
 */
#ifndef __COMMON_H
#define __COMMON_H
#include <string>
#include "CVariant.h"
#include <time.h>
typedef enum {
    DEVICE_DATA,
    SENSOR_DATA,
    EDGE_COMPUTING_NODE
  }comVarSource_t;

/**
 *  设备变量类
 */
class CComVar{
protected:
  gva::CVariant var;   //储存当前最近一次采集到的值
  comVarSource_t source;
  void saveToSQL(){};
public:
  std::string name;
  mutable volatile bool dirty;
  const gva::CVariant& curVar (void) const {return var;}  //获取当前值
  //封装过的赋值函数，将会自动设置脏位（在值发生变化时）
  void setValue(const gva::CVariant& value){
    if(value == var) 
      return;
    else 
    {
      var = value; 
      dirty = true;
      saveToSQL();
    }
  }
  template<typename T>void setValue(const T value)
  {
    gva::CVariant temp(value); 
    if(temp == var) return; 
    else 
    {
      var = temp; 
      dirty = true;
      saveToSQL();
    }
  }

  CComVar(comVarSource_t sor = EDGE_COMPUTING_NODE):var(){dirty = false;source = sor;}
  CComVar(comVarSource_t sor, std::string nm, datatype_t tp=DATATYPEKIND_NOTYPE):var(tp){
    source = sor;
    name = nm;
    dirty = false;
  }
};

#endif