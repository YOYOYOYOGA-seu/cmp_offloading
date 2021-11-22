/*
MIT License

Copyright (c) 2021 Shi Zhangkun (https://github.com/YOYOYOYOGA-seu)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Please visit https://github.com/YOYOYOYOGA-seu/CVariant for more information 
about this software.
*/

#include "CVariant.h"
#include <cstring>
#include <string>
#include <vector>
using namespace gva;
const std::size_t gva::BASE_TYPE_SIZE[] = {sizeof(bool), sizeof(char), sizeof(unsigned char),
                                      sizeof(short), sizeof(unsigned short), sizeof(int), sizeof(unsigned int),
                                      sizeof(long long), sizeof(unsigned long long), sizeof(float), sizeof(double),
                                      sizeof(std::string)};

/**
 * @brief
 * @note
 * @param {type} none
 * @retval none
 */
CVariant::CVariant()
{
  refCount.reset();
  data = nullptr;
  type = DATATYPEKIND_NOTYPE;
  size = 0;
}

/**
 * @brief
 * @note
 * @param {type} none
 * @retval none
 */
CVariant::CVariant(datatype_t tp)
{
  refCount.reset();
  data = nullptr;
  type = DATATYPEKIND_NOTYPE;
  size = 0;
  setType(tp);
}

/**
 * @brief
 * @note
 * @param {type} none
 * @retval none
 */
CVariant::CVariant(const CVariant &var)
{
  refCount.reset();
  data = nullptr;
  type = DATATYPEKIND_NOTYPE;
  size = 0;
  operator=(var); //do a shadow copy (copy-on-write)
}

/**
 * @brief
 * @note
 * @param {type} none
 * @retval none
 */
CVariant::~CVariant()
{
  clear();
}

/**
 * @brief
 * @note
 * @param {type} none
 * @retval none
 */
void CVariant::clear()
{
  if (data != nullptr && *refCount <= 1) // if no CVariant object ref this data memory, then delete it
  {
    switch (type)
    {
    case DATATYPEKIND_BOOLEAN:
      delete static_cast<bool *>(data);
      break;
    case DATATYPEKIND_SBYTE:
      delete static_cast<char *>(data);
      break;
    case DATATYPEKIND_BYTE:
      delete static_cast<unsigned char *>(data);
      break;
    case DATATYPEKIND_INT16:
      delete static_cast<short *>(data);
      break;
    case DATATYPEKIND_UINT16:
      delete static_cast<unsigned short *>(data);
      break;
    case DATATYPEKIND_INT32:
      delete static_cast<int *>(data);
      break;
    case DATATYPEKIND_UINT32:
      delete static_cast<unsigned int *>(data);
      break;
    case DATATYPEKIND_INT64:
      delete static_cast<long long *>(data);
      break;
    case DATATYPEKIND_UINT64:
      delete static_cast<unsigned long long *>(data);
      break;
    case DATATYPEKIND_FLOAT:
      delete static_cast<float *>(data);
      break;
    case DATATYPEKIND_DOUBLE:
      delete static_cast<double *>(data);
      break;
    case DATATYPEKIND_STRING:
      delete static_cast<std::string *>(data);
      break;
    default:
      if (type < DATATYPE_VECTOR_END && type >= DATATYPEKIND_BOOLEAN_VECTOR)
        delete static_cast<std::vector<CVariant> *>(data);
      break;
    }
  }
  else if(refCount != nullptr)// else only change reference count
  {
    (*refCount)--;
  }
  refCount.reset();
  data = nullptr;
  size = 0;
  type = DATATYPEKIND_NOTYPE;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
void CVariant::_upgrade(void)
{
  if (type < DATATYPE_BASE_END)
  {
    CVariant temp = *this;
    setType(static_cast<datatype_t>(type + DATATYPEKIND_BOOLEAN_VECTOR));
    _insert(temp, 0);
  }
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::_cast_1(datatype_t tp)
{
  if (!ifNumType(tp))
    return false;
  switch (type)
  {
  case DATATYPEKIND_SBYTE:
    *this = _cast_2(tp, *static_cast<char *>(data));
    break;
  case DATATYPEKIND_BYTE:
    *this = _cast_2(tp, *static_cast<unsigned char *>(data));
    break;
  case DATATYPEKIND_INT16:
    *this = _cast_2(tp, *static_cast<short *>(data));
    break;
  case DATATYPEKIND_UINT16:
    *this = _cast_2(tp, *static_cast<unsigned short *>(data));
    break;
  case DATATYPEKIND_INT32:
    *this = _cast_2(tp, *static_cast<int *>(data));
    break;
  case DATATYPEKIND_UINT32:
    *this = _cast_2(tp, *static_cast<unsigned int *>(data));
    break;
  case DATATYPEKIND_INT64:
    *this = _cast_2(tp, *static_cast<long long *>(data));
    break;
  case DATATYPEKIND_UINT64:
    *this = _cast_2(tp, *static_cast<unsigned long long *>(data));
    break;
  case DATATYPEKIND_FLOAT:
    *this = _cast_2(tp, *static_cast<float *>(data));
    break;
  case DATATYPEKIND_DOUBLE:
    *this = _cast_2(tp, *static_cast<double *>(data));
    break;
  default:
    return false;
  }
  return true;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
void CVariant::_copyself(void)
{
  CVariant temp;
  temp._copy(*this);
  *this = temp;
}

/**
 * @brief  deep copy
 * @note  
 * @param {type} none
 * @retval none
 */
CVariant &CVariant::_copy(const CVariant &var)
{
  clear();
  if (var.data != nullptr)
  {
    type = var.type;
    size = var.size;
    switch (type)
    {
    case DATATYPEKIND_BOOLEAN:
      data = new bool(*static_cast<bool *>(var.data));
      break;
    case DATATYPEKIND_SBYTE:
      data = new char(*static_cast<char *>(var.data));
      break;
    case DATATYPEKIND_BYTE:
      data = new unsigned char(*static_cast<unsigned char *>(var.data));
      break;
    case DATATYPEKIND_INT16:
      data = new short(*static_cast<short *>(var.data));
      break;
    case DATATYPEKIND_UINT16:
      data = new unsigned short(*static_cast<unsigned short *>(var.data));
      break;
    case DATATYPEKIND_INT32:
      data = new int(*static_cast<int *>(var.data));
      break;
    case DATATYPEKIND_UINT32:
      data = new unsigned int(*static_cast<unsigned int *>(var.data));
      break;
    case DATATYPEKIND_INT64:
      data = new long long(*static_cast<long long *>(var.data));
      break;
    case DATATYPEKIND_UINT64:
      data = new unsigned long long(*static_cast<unsigned long long *>(var.data));
      break;
    case DATATYPEKIND_FLOAT:
      data = new float(*static_cast<float *>(var.data));
      break;
    case DATATYPEKIND_DOUBLE:
      data = new double(*static_cast<double *>(var.data));
      break;
    case DATATYPEKIND_STRING:
      data = new std::string(*static_cast<std::string *>(var.data));
      break;
    default:
      if (type < DATATYPE_VECTOR_END && type >= DATATYPEKIND_BOOLEAN_VECTOR)
      {
        data = new std::vector<CVariant>(*static_cast<std::vector<CVariant> *>(var.data));
      }
      else
      {
        data = nullptr;
        type = DATATYPEKIND_NOTYPE;
      }
      break;
    }
    if(data != nullptr)  
      refCount = std::make_shared<unsigned int>(1);
  }
  return *this;
}

/**
 * @brief  shallow copy
 * @note  
 * @param {*}
 * @retval none
 */
CVariant& CVariant::operator=(const CVariant& var)
{
  clear();
  if (var.data == nullptr)
  {
    data = nullptr;
    type = DATATYPEKIND_NOTYPE;
    size = 0;
  }
  else
  {
    refCount = var.refCount;
    (*refCount)++;  // ref add
    type = var.type;
    size = var.size;
    data = var.data;
  }
  return *this;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
CVariant &CVariant::operator=(const char *var)
{
  if (type == DATATYPEKIND_STRING && data != nullptr)
  {
    if(*refCount > 1)
      _copyself();
    *static_cast<std::string *>(data) = var;
  }
  else
  {
    clear();
    type = DATATYPEKIND_STRING;
    data = new std::string(var);
    refCount = std::make_shared<unsigned int>(1);
  }
  size = 1;
  return *this;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
CVariant &CVariant::operator+=(const CVariant &var)
{
  const CVariant *pvar = &var;
  CVariant temp;
  if (var.type != type && ifNumType(type) && ifNumType(var.type))
  {
    temp = var;
    temp._cast_1(type);
    pvar = &temp;
  }
  if ((pvar->type == type && type < DATATYPE_BASE_END))
  {
    if(*refCount > 1)
      _copyself();
    switch (type)
    {
      // bool type can't do + or -
    case DATATYPEKIND_SBYTE:
      *static_cast<char *>(data) += *static_cast<char *>(pvar->data);
      break;
    case DATATYPEKIND_BYTE:
      *static_cast<unsigned char *>(data) += *static_cast<unsigned char *>(pvar->data);
      break;
    case DATATYPEKIND_INT16:
      *static_cast<short *>(data) += *static_cast<short *>(pvar->data);
      break;
    case DATATYPEKIND_UINT16:
      *static_cast<unsigned short *>(data) += *static_cast<unsigned short *>(pvar->data);
      break;
    case DATATYPEKIND_INT32:
      *static_cast<int *>(data) += *static_cast<int *>(pvar->data);
      break;
    case DATATYPEKIND_UINT32:
      *static_cast<unsigned int *>(data) += *static_cast<unsigned int *>(pvar->data);
      break;
    case DATATYPEKIND_INT64:
      *static_cast<long long *>(data) += *static_cast<long long *>(pvar->data);
      break;
    case DATATYPEKIND_UINT64:
      *static_cast<unsigned long long *>(data) += *static_cast<unsigned long long *>(pvar->data);
      break;
    case DATATYPEKIND_FLOAT:
      *static_cast<float *>(data) += *static_cast<float *>(pvar->data);
      break;
    case DATATYPEKIND_DOUBLE:
      *static_cast<double *>(data) += *static_cast<double *>(pvar->data);
      break;
    case DATATYPEKIND_STRING:
      *static_cast<std::string *>(data) += *static_cast<std::string *>(pvar->data);
      break;
    default:
      break;
    }
  }
  else if ((var.type + DATATYPEKIND_BOOLEAN_VECTOR) == type)
    append(var);
  return *this;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
CVariant &CVariant::operator+=(const char *var)
{

  if (type == DATATYPEKIND_STRING)
  {
    if(*refCount > 1)
      _copyself();
    *static_cast<std::string *>(data) += var;
  }
  else if (type == DATATYPEKIND_STRING_VECTOR)
    append(var);
  return *this;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
CVariant &CVariant::operator-=(const CVariant &var)
{
  const CVariant *pvar = &var;
  CVariant temp;
  if (var.type != type && ifNumType(type) && ifNumType(var.type))
  {
    temp = var;
    temp._cast_1(type);
    pvar = &temp;
  }
  if ((pvar->type == type && type < DATATYPE_BASE_END))
  {
    if(*refCount > 1)
      _copyself();
    switch (type)
    {
      // bool type can't do + or -
    case DATATYPEKIND_SBYTE:
      *static_cast<char *>(data) -= *static_cast<char *>(pvar->data);
      break;
    case DATATYPEKIND_BYTE:
      *static_cast<unsigned char *>(data) -= *static_cast<unsigned char *>(pvar->data);
      break;
    case DATATYPEKIND_INT16:
      *static_cast<short *>(data) -= *static_cast<short *>(pvar->data);
      break;
    case DATATYPEKIND_UINT16:
      *static_cast<unsigned short *>(data) -= *static_cast<unsigned short *>(pvar->data);
      break;
    case DATATYPEKIND_INT32:
      *static_cast<int *>(data) -= *static_cast<int *>(pvar->data);
      break;
    case DATATYPEKIND_UINT32:
      *static_cast<unsigned int *>(data) -= *static_cast<unsigned int *>(pvar->data);
      break;
    case DATATYPEKIND_INT64:
      *static_cast<long long *>(data) -= *static_cast<long long *>(pvar->data);
      break;
    case DATATYPEKIND_UINT64:
      *static_cast<unsigned long long *>(data) -= *static_cast<unsigned long long *>(pvar->data);
      break;
    case DATATYPEKIND_FLOAT:
      *static_cast<float *>(data) -= *static_cast<float *>(pvar->data);
      break;
    case DATATYPEKIND_DOUBLE:
      *static_cast<double *>(data) -= *static_cast<double *>(pvar->data);
      break;
      // std::string type can't do -=
    default:
      break;
    }
  }
  return *this;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::operator==(const CVariant &var) const
{
  if (type == var.type && type < DATATYPE_BASE_END && type != DATATYPEKIND_STRING) //是相同的基础类型
  {
    if (std::memcmp(data, var.data, BASE_TYPE_SIZE[type]) == 0)
    {
      return true;
    }
  }
  else if (ifNumType(var.type) && ifNumType(type))
  { // if the two CVariant object are both numberic type, cast them to double
    CVariant temp1 = var;
    CVariant temp2 = *this;
    temp1._cast_1(DATATYPEKIND_DOUBLE);
    temp2._cast_1(DATATYPEKIND_DOUBLE);
    if (std::memcmp(temp1.data, temp2.data, BASE_TYPE_SIZE[DATATYPEKIND_DOUBLE]) == 0)
    {
      return true;
    }
  }
  else if (type == var.type && type == DATATYPEKIND_STRING)
  {
    if (strcmp(static_cast<std::string *>(data)->c_str(), static_cast<std::string *>(var.data)->c_str()) == 0)
    {
      return true;
    }
  }
  else if(type == var.type)
  {
    if(ifVectorType(type) && ifVectorType(var.type))
    {
      auto& vct1 = *static_cast<std::vector<CVariant>*>(data);
      auto& vct2 = *static_cast<std::vector<CVariant>*>(var.data);
      auto itor1 = vct1.begin();
      auto itor2 = vct2.begin();
      while(itor1 != vct1.end() && itor2 != vct2.end())
      {
        if(*itor1 == *itor2)
        {
          itor1++; itor2++;
          continue;
        }
        else
        {
          break;
        }
      }
      if(itor1 == vct1.end() && itor1 == vct1.end())
        return true;
    }
  }
  return false;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
const CVariant& CVariant::operator[](std::size_t n) const
{
  if (!(type >= DATATYPEKIND_BOOLEAN_VECTOR && type < DATATYPE_VECTOR_END))
    return *this;
  else
    return static_cast<std::vector<CVariant> *>(data)->at(n);
}


CVariant& CVariant::operator[](std::size_t n) 
{
  if (!(type >= DATATYPEKIND_BOOLEAN_VECTOR && type < DATATYPE_VECTOR_END))
    return *this;
  else
    return static_cast<std::vector<CVariant> *>(data)->at(n);
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::insert(const CVariant &var, unsigned int locate)
{
  if (type > DATATYPEKIND_BOOLEAN_VECTOR && (var.type + DATATYPEKIND_BOOLEAN_VECTOR == type || var.type == type) && type != DATATYPEKIND_NOTYPE)
  {
    _insert(var, locate);
    return true;
  }
  return false;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::insert(const char *var, unsigned int locate)
{
  if (type == DATATYPEKIND_STRING_VECTOR)
  {
    std::string temp(var);
    _insert(temp, locate);
    return true;
  }
  return false;
}
bool CVariant::insert(const char *var, std::size_t n, unsigned int locate)
{
  if (type == DATATYPEKIND_STRING_VECTOR)
  {
    std::string temp(var,n);
    _insert(temp, locate);
    return true;
  }
  return false;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::append(const CVariant &var)
{
  if ((var.type + DATATYPEKIND_BOOLEAN_VECTOR == type || var.type == type) && type != DATATYPEKIND_NOTYPE)
  {
    if (type < DATATYPE_BASE_END)
      _upgrade();
    _insert(var, getSize());
    return true;
  }
  return false;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::append(const char *var)
{
  if (type == DATATYPEKIND_STRING)
  {
    _upgrade();
  }
  if (type == DATATYPEKIND_STRING_VECTOR)
  {
    std::string temp(var);
    _insert(temp, getSize());
    return true;
  }

  return false;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::append(const char *var, std::size_t n)
{
  if (type == DATATYPEKIND_STRING)
  {
    _upgrade();
  }
  if (type == DATATYPEKIND_STRING_VECTOR)
  {
    std::string temp(var,n);
    _insert(temp, getSize());
    return true;
  }
  return false;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::erease(unsigned int locate)
{
  if (ifVectorType(type) && locate < static_cast<std::vector<CVariant> *>(data)->size())
  {
    if(*refCount > 1)
      _copyself();
    std::vector<CVariant>::iterator itr = static_cast<std::vector<CVariant> *>(data)->begin() + locate;
    static_cast<std::vector<CVariant> *>(data)->erase(itr);
    return true;
  }
  return false;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::erease(unsigned int first, unsigned int last)
{
  if (ifVectorType(type) && first <= last && last < static_cast<std::vector<CVariant> *>(data)->size())
  {
    if(*refCount > 1)
      _copyself();
    std::vector<CVariant>::iterator ftr = static_cast<std::vector<CVariant> *>(data)->begin() + first;
    std::vector<CVariant>::iterator ltr = static_cast<std::vector<CVariant> *>(data)->begin() + last;
    static_cast<std::vector<CVariant> *>(data)->erase(ftr, ltr);
    return true;
  }
  return false;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
const void *CVariant::getPtr(void) const
{
  if (type < DATATYPE_BASE_END)
    return data;
  return nullptr;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
template <char>
const char *CVariant::getPtr(void) const //单独列出获取c类型字符串指针（存储采用std::string，没有c类型字符串的datatype_t）
{
  if (type == DATATYPEKIND_STRING)
    return static_cast<std::string *>(data)->c_str();
}


/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
std::string CVariant::toString(const char* fmt) const
{
  char buf[32] = {0};
  if(ifVectorType()) //向量型，直接递归
  {
    std::string ret;
    auto sz = getSize();
    ret = "[";
    for(std::size_t i = 0; i < sz; i++)
    {
      if(i == 0) ret += '\"';
      else ret += ", \"";
      ret += operator[](i).toString(fmt);
      ret += "\"";
    }
    ret +="]";
    return ret;
  }

  if(std::strcmp(fmt,"hex") == 0) //fmt -> "nex"
  {
    if(ifNumType() && type <= DATATYPEKIND_UINT32)
      std::sprintf(buf,"%x", value<unsigned int>());
    else if(ifNumType() && type <= DATATYPEKIND_UINT64)
      std::sprintf(buf,"%llx", value<unsigned long long>());
    else //其他，不支持hex的类型
      return toString();
    return std::string(buf);
  }
  else if(std::strcmp(fmt,"trunc") == 0 && (type == DATATYPEKIND_FLOAT || type == DATATYPEKIND_DOUBLE)) //fmt -> "trunc"
  {
    std::string ret;
    ret = toString();
    auto point = ret.find('.');
    if(point != ret.npos)
    {
      while(ret.size() > point)
      {
        if(ret.back() != '0' && ret.back() != '.') 
          break;
        ret.pop_back();
      }
    }
    return ret;
  }

  return toString(); //fmt无法识别，调用默认toString()函数
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
std::string CVariant::toString(int prec) const
{
  std::string ret;
  if(ifVectorType()) //向量型，直接递归
  {
    std::string ret;
    auto sz = getSize();
    ret = "[";
    for(std::size_t i = 0; i < sz; i++)
    {
      if(i == 0) 
        ret += '\"';
      else 
        ret += ", \"";
      ret += operator[](i).toString(prec);
      ret += "\"";
    }
    ret +="]";
    return ret;
  }

  if(prec >= 0 && (type == DATATYPEKIND_FLOAT || type == DATATYPEKIND_DOUBLE)) 
  {
    auto temp = value<double>();
    std::string ret = "%.";
    ret += std::to_string(prec) + "lf";
    auto bufSize = snprintf(NULL, 0, ret.c_str(), temp);
    char* buf = new char[bufSize];
    sprintf(buf, ret.c_str(), temp);
    ret = std::string(buf);
    delete[] buf;
    return ret;
  }
  return toString();
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
std::string CVariant::toString(void) const
{
  std::string ret;
  if(type == DATATYPEKIND_STRING)
    return value<std::string>();
  else if(type == DATATYPEKIND_BOOLEAN)
    ret = value<bool>() ? "true" : "false";
  else if(type == DATATYPEKIND_SBYTE)
    ret = value<char>();
  else if(type >= DATATYPEKIND_BYTE && type <= DATATYPEKIND_INT32)
    ret = std::to_string(value<int>());
  else if(type >= DATATYPEKIND_BYTE && type <= DATATYPEKIND_INT32)
    ret = std::to_string(value<int>());
  else if(type == DATATYPEKIND_UINT32)
    ret = std::to_string(value<unsigned int>());
  else if(type == DATATYPEKIND_INT64)
    ret = std::to_string(value<long long>());
  else if(type == DATATYPEKIND_UINT64)
    ret = std::to_string(value<unsigned long long>());
  else if(type == DATATYPEKIND_FLOAT)
    ret = std::to_string(value<float>());
  else if(type == DATATYPEKIND_DOUBLE)
    ret = std::to_string(value<double>());
  else if(ifVectorType())
  {
    auto sz = getSize();
    ret = "[";
    for(std::size_t i = 0; i < sz; i++)
    {
      if(i == 0) 
        ret += '\"';
      else 
        ret += ", \"";
      ret += operator[](i).toString();
      ret += "\"";
    }
    ret +="]";
  } 
  return ret;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::setValue(void *dat, std::size_t size)
{
  if (type < DATATYPE_BASE_END) //基础数据类型才可以设置值
  {
    if (BASE_TYPE_SIZE[type] == size) //保证内存对齐
    {
      if(*refCount > 1)
        _copyself();
      if (type == DATATYPEKIND_STRING) //string类型不能直接copy
      {
        *static_cast<std::string *>(data) = *static_cast<std::string *>(dat);
      }
      else
      {
        std::memcpy(data, dat, size);
      }
      return true;
    }
  }
  return false;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::setValue(const char *value) //单独列出通过c类型字符串给字符串赋值（存储采用std::string）
{
  if (type == DATATYPEKIND_STRING)
  {
    if(*refCount > 1)
      _copyself();
    static_cast<std::string *>(data)->assign(value);
    return true;
  }
  return false;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::setValue(const char *value, std::size_t n) //单独列出通过c类型字符串给字符串赋值（存储采用std::string）
{
  if (type == DATATYPEKIND_STRING)
  {
    if(*refCount > 1)
      _copyself();
    static_cast<std::string *>(data)->assign(value,n);
    return true;
  }
  return false;
}
/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::setType(datatype_t tp)
{
  if (ifType(tp) == false)
    return false;
  if (tp == type)
    return true;
  clear();
  size = 1;
  type = tp;
  switch (tp)
  {
  case DATATYPEKIND_BOOLEAN:
    data = new bool;
    break;
  case DATATYPEKIND_SBYTE:
    data = new char;
    break;
  case DATATYPEKIND_BYTE:
    data = new unsigned char;
    break;
  case DATATYPEKIND_INT16:
    data = new short;
    break;
  case DATATYPEKIND_UINT16:
    data = new unsigned short;
    break;
  case DATATYPEKIND_INT32:
    data = new int;
    break;
  case DATATYPEKIND_UINT32:
    data = new unsigned int;
    break;
  case DATATYPEKIND_INT64:
    data = new long long;
    break;
  case DATATYPEKIND_UINT64:
    data = new unsigned long long;
    break;
  case DATATYPEKIND_FLOAT:
    data = new float;
    break;
  case DATATYPEKIND_DOUBLE:
    data = new double;
    break;
  case DATATYPEKIND_STRING:
    data = new std::string;
    break;
  default:
    size = 0;
    if (type < DATATYPE_VECTOR_END && type >= DATATYPEKIND_BOOLEAN_VECTOR)
    {
      data = new std::vector<CVariant>;
    }
    else
    {
      type = DATATYPEKIND_NOTYPE;
    }
    break;
  }
  if(data != nullptr)
    refCount = std::make_shared<unsigned int>(1);
  return true;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
datatype_t CVariant::getType(void) const
{
  return type;
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
unsigned int CVariant::getSize(void) const
{
  if (type < DATATYPE_VECTOR_END && type >= DATATYPEKIND_BOOLEAN_VECTOR)
  {
    size = static_cast<std::vector<CVariant> *>(data)->size();
  }
  return size;
}

/**
 * @brief  
 * @note  
 * @param {*}
 * @retval none
 */
unsigned int CVariant::getTypeSize(int type)
{
  if(!ifBaseType(type)) return -1;
  if(type >= DATATYPEKIND_BOOLEAN && type <= DATATYPEKIND_BYTE)
    return 1;
  else if(type >= DATATYPEKIND_INT16 && type <= DATATYPEKIND_UINT16)
    return 2;
  else if((type >= DATATYPEKIND_INT32 && type <= DATATYPEKIND_UINT32) || type == DATATYPEKIND_FLOAT)
    return 4;
  else if(type >= DATATYPEKIND_INT32 && type <= DATATYPEKIND_DOUBLE)
    return 8;
  else  
    return -1;   
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::ifBaseType(void) const
{
  return ifBaseType(type);
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::ifNumType(void) const
{
  return ifNumType(type);
}

/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::ifVectorType(void) const
{
  return ifVectorType(type);
}
/**
 * @brief  
 * @note  
 * @param {type} none
 * @retval none
 */
bool CVariant::ifType(void) const
{
  return ifType(type);
}