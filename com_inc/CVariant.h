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

#ifndef __VARIANT_H
#define __VARIANT_H
#include <string>
#include <vector>
#include <iostream>
#include <memory>
typedef enum {
  /* base data type ( compatibility with open62541) */
  DATATYPEKIND_BOOLEAN = 0,
  DATATYPEKIND_SBYTE = 1,
  DATATYPEKIND_BYTE = 2,
  DATATYPEKIND_INT16 = 3,
  DATATYPEKIND_UINT16 = 4,
  DATATYPEKIND_INT32 = 5,
  DATATYPEKIND_UINT32 = 6,
  DATATYPEKIND_INT64 = 7,
  DATATYPEKIND_UINT64 = 8,
  DATATYPEKIND_FLOAT = 9,
  DATATYPEKIND_DOUBLE = 10,
  DATATYPEKIND_STRING = 11,
  DATATYPE_BASE_END,
  
  /* vector type */
  DATATYPEKIND_BOOLEAN_VECTOR = 100,
  DATATYPEKIND_SBYTE_VECTOR = 101,
  DATATYPEKIND_BYTE_VECTOR = 102,
  DATATYPEKIND_INT16_VECTOR = 103,
  DATATYPEKIND_UINT16_VECTOR = 104,
  DATATYPEKIND_INT32_VECTOR = 105,
  DATATYPEKIND_UINT32_VECTOR = 106,
  DATATYPEKIND_INT64_VECTOR = 107,
  DATATYPEKIND_UINT64_VECTOR = 108,
  DATATYPEKIND_FLOAT_VECTOR = 109,
  DATATYPEKIND_DOUBLE_VECTOR = 110,
  DATATYPEKIND_STRING_VECTOR = 111,
  DATATYPE_VECTOR_END,

  /* sign a empty CVariant object */
  DATATYPEKIND_NOTYPE = 255,

}datatype_t;

namespace gva
{
  extern const std::size_t BASE_TYPE_SIZE[DATATYPE_BASE_END];
  /* Used to limit the input type of a template function while allowing 
  the template function to automatically get a type number based on the 
  parameter(typedatatype_t) */
  template <typename T>struct dataKind;  
  template <>struct dataKind<bool> { enum { type = DATATYPEKIND_BOOLEAN }; };
  template <>struct dataKind<char> { enum { type = DATATYPEKIND_SBYTE }; };
  template <>struct dataKind<unsigned char> { enum { type = DATATYPEKIND_BYTE }; };
  template <>struct dataKind<short> { enum { type = DATATYPEKIND_INT16 }; };
  template <>struct dataKind<unsigned short> { enum { type = DATATYPEKIND_UINT16 }; };
  template <>struct dataKind<int> { enum { type = DATATYPEKIND_INT32 }; };
  template <>struct dataKind<unsigned int> { enum { type = DATATYPEKIND_UINT32 }; };
  template <>struct dataKind<long> { enum { type = sizeof(long) == 4 ? DATATYPEKIND_INT32:DATATYPEKIND_INT64 }; };
  template <>struct dataKind<unsigned long> { enum { type = sizeof(long) == 4 ? DATATYPEKIND_UINT32:DATATYPEKIND_UINT64}; };
  template <>struct dataKind<long long> { enum { type = DATATYPEKIND_INT64 }; };
  template <>struct dataKind<unsigned long long> { enum { type = DATATYPEKIND_UINT64 }; };
  template <>struct dataKind<float> { enum { type = DATATYPEKIND_FLOAT }; };
  template <>struct dataKind<double> { enum { type = DATATYPEKIND_DOUBLE }; };
  template <>struct dataKind<std::string> { enum { type = DATATYPEKIND_STRING }; };

    /* used for throw CVariant bad cast error */
  class bad_variant_cast : public std::exception {
  public:
    datatype_t request;
    datatype_t current;
    std::string errStr;
    bad_variant_cast(datatype_t req, datatype_t cur)noexcept
    {
      request = req; 
      current = cur;
      errStr = "CVariant: bad value read type,request(";
      errStr += std::to_string(request) + "), current(" + std::to_string(current) + ")";
    } 
    const char * what () const throw ()
    {
      return errStr.c_str();
    }
  };

  class CVariant {
  private:
    void* data;
    std::shared_ptr<unsigned int> refCount;
    datatype_t type;
    /* for base type, the size is 1; for vector type, the size is the vector size.
      It will only refreash after call CVariant::getSize() */
    mutable unsigned int size;  //mutable(const CVariant can call CVariant::getSize())
    /* insert a object to a vector type CVariant object, internal function, and must 
    check ifinsertable before call this function*/
    inline void _insert(CVariant& var, unsigned int locate) 
    {
      unsigned int n = var.getSize();
      CVariant temp;
      if (*refCount > 1)
        _copyself();
      for (unsigned int i = 0; i < n; i++)
      {
        temp = var[i];
        static_cast<std::vector<CVariant>*>(data)->insert(static_cast<std::vector<CVariant>*>(data)->begin()+locate, temp);
        locate++;
      }
      getSize(); //refreash size
    }
    template <typename T>inline void _insert(const T var, unsigned int locate) 
    {
      CVariant temp;
      temp = (var);
      if (*refCount > 1)
        _copyself();
      static_cast<std::vector<CVariant>*>(data)->insert(static_cast<std::vector<CVariant>*>(data)->begin()+locate, temp);
      getSize(); //refreash size
    }
    template <typename T> inline void _insert(const std::vector<T>& var, unsigned int locate) 
    {
      if (*refCount > 1)
        _copyself();
      for (int i = 0; i < var.size(); i++)
      {
        CVariant temp;
        temp = var[i];
        static_cast<std::vector<CVariant>*>(data)->insert(static_cast<std::vector<CVariant>*>(data)->begin()+locate, temp);
        locate++;
      }
      getSize(); //refreash size
    }
    template <typename T> inline void _insert(const T* var, unsigned int n, unsigned int locate) 
    {
      if (*refCount > 1)
        _copyself();
      for (unsigned int i = 0; i < n; i++)
      {
        CVariant temp;
        temp = var[i];
        static_cast<std::vector<CVariant>*>(data)->insert(static_cast<std::vector<CVariant>*>(data)->begin()+locate, temp);
        locate++;
      }
      getSize(); //refreash size
    }
    /* deep copy */
    CVariant &_copy(const CVariant& var);

    /* do a deep copy of itself(called before change the data while the refCount > 1) */
    void _copyself(void);

    /* upgrade a base type CVariant obj to a corresponding vetor type obj */
    void _upgrade(void);

    /* _cast_1 is the first step of cast between numberic type CVariant  */
    bool _cast_1(datatype_t tp);
    /* _cast_2 is the second step of cast between numberic type CVariant, called by _cast_1  */
    template <typename T>static CVariant _cast_2(datatype_t tp, const T var)
    {
      CVariant temp;
      switch (tp)
      {
      case DATATYPEKIND_SBYTE:
        temp = static_cast<char>(var);
        break;
      case DATATYPEKIND_BYTE:
        temp = static_cast<unsigned char>(var);
        break;
      case DATATYPEKIND_INT16:
        temp = static_cast<short>(var);
        break;
      case DATATYPEKIND_UINT16:
        temp = static_cast<unsigned short>(var);
        break;
      case DATATYPEKIND_INT32:
        temp = static_cast<int>(var);
        break;
      case DATATYPEKIND_UINT32:
        temp = static_cast<unsigned int>(var);
        break;
      case DATATYPEKIND_INT64:
        temp = static_cast<long long>(var);
        break;
      case DATATYPEKIND_UINT64:
        temp = static_cast<unsigned long long>(var);
        break;
      case DATATYPEKIND_FLOAT:
        temp = static_cast<float>(var);
        break;
      case DATATYPEKIND_DOUBLE:
        temp = static_cast<double>(var);
        break;
      default:
        break;
      }        
      return temp;
    }
    static CVariant _cast_2(datatype_t tp, const std::string var) // for passing  compile (std::string can't cast)
    {
      abort();
      return CVariant();
    }

  public:
    CVariant();
    CVariant(datatype_t tp);
    template<typename T> CVariant(const T var)
    {
      data = nullptr;
      size = 0;
      type = DATATYPEKIND_NOTYPE;
      if (static_cast<datatype_t>(dataKind<T>::type) < DATATYPE_BASE_END)
      {
        operator=(var);
      }
    }
    template<typename T> CVariant(const std::vector<T>& var)
    {
      data = nullptr;
      size = 0;
      type = DATATYPEKIND_NOTYPE;
      if (static_cast<datatype_t>(dataKind<T>::type) < DATATYPE_BASE_END)
      {
        operator=(var);
      }
    }
    template<typename T> CVariant(const T* var, unsigned int n)
    {
      if (static_cast<datatype_t>(dataKind<T>::type) < DATATYPE_BASE_END)
      {
        clear();
        type = static_cast<datatype_t>(dataKind<T>::type + DATATYPEKIND_BOOLEAN_VECTOR);
        data = new std::vector<CVariant>;
        refCount = std::make_shared<unsigned int>(1);
        size = n;
        CVariant temp;
        for (int i = 0; i < n; i++)
        {
          temp = var[i];
          static_cast<std::vector<CVariant>*>(data)->push_back(temp);
        }
      }
    }
    CVariant(const CVariant& var);
    ~CVariant();
    void clear();

    CVariant& operator=(const CVariant& var);
    CVariant& operator=(const char* var);
    template<typename T> CVariant& operator=(const T var)
    {
      if (static_cast<datatype_t>(dataKind<T>::type) < DATATYPE_BASE_END)
      {
        if (static_cast<datatype_t>(dataKind<T>::type) == type && data != nullptr)
        {
          if (*refCount > 1)
            _copyself();
          *static_cast<T*>(data) = var;
        }
        else
        {
          clear();
          type = static_cast<datatype_t>(dataKind<T>::type);
          data = new T(var);
          refCount = std::make_shared<unsigned int>(1);
        }
        size = 1;
      }
      return *this;
    }
    template<typename T> CVariant& operator=(const std::vector<T>& var)
    {
      if (static_cast<datatype_t>(dataKind<T>::type) < DATATYPE_BASE_END)
      {
        clear();
        type = static_cast<datatype_t>(dataKind<T>::type + DATATYPEKIND_BOOLEAN_VECTOR);
        data = new std::vector<CVariant>;
        refCount = std::make_shared<unsigned int>(1);
        size = var.size();
        CVariant temp;
        for (unsigned int i = 0; i < var.size(); i++)
        {
          temp = var[i];
          static_cast<std::vector<CVariant>*>(data)->push_back(temp);
        }
      }
      return *this;
    }

    /** cast to another base type value ,a packaging of CVariant::value<T>()
     * warning!! call CVariant::value(), so ifcast failed will case process abort.
     */
    template<typename T> operator T() const
    {
      return value<T>();
    }
    
    /* for base type , it amount to the data type's own +=(like int += int; string+=string;)
      for a vector type, it amount to call CVariant::append*/
    CVariant& operator+=(const char* var);
    template<typename T> CVariant& operator+=(T var)
    {
      if (static_cast<datatype_t>(dataKind<T>::type) == type && type < DATATYPE_BASE_END)
      {
        if (*refCount > 1)
          _copyself();
        *static_cast<T*>(data) += var;
      }
      else if (ifNumType(dataKind<T>::type) && ifNumType(type))   // numerical value can transform
      {
        CVariant temp = _cast_2(type,var);
        if (temp.getType() == type)
          operator+=(temp);
      }
      else if (static_cast<datatype_t>(dataKind<T>::type + DATATYPEKIND_BOOLEAN_VECTOR) == type)
      {
        append(var);
      }
      return *this;
    }
    template<typename T> CVariant& operator+=(const std::vector<T>& var)
    {
      append(var);
      return *this;
    }
    CVariant& operator+=(const CVariant& var);

    /* it amount to the data type's own -=(like int += int;),and only vaild when the data type can -=*/
    template<typename T> CVariant& operator-=(T var)
    {
      if (static_cast<datatype_t>(dataKind<T>::type) == type && type < DATATYPE_BASE_END)
      {
        if (*refCount > 1)
          _copyself();
        *static_cast<T*>(data) -= var;
      }   
      else if (ifNumType(dataKind<T>::type) && ifNumType(type))   // numerical value can transform
      {
        CVariant temp = _cast_2(type,var);
        if (temp.getType() == type)
          operator-=(temp);
      }
      return *this;
    }
    CVariant& operator-=(const CVariant& var);

    /* only for base type, return true when type = type, data = data */
    bool operator==(const CVariant& var) const;
    
    /* for base type , return itself, for vector type, return the number n CVariant object */
    CVariant& operator[](std::size_t n);
    const CVariant& operator[](std::size_t n) const; //const CVariant use
    /* insert elements to a vector type CVariant(must has the same base type) */
    bool insert(const char* var, unsigned int locate);
    bool insert(const char* var, std::size_t n, unsigned int locate);
    template <typename T> bool insert(const T var, unsigned int locate)
    {
      if (static_cast<datatype_t>(dataKind<T>::type + DATATYPEKIND_BOOLEAN_VECTOR) == type)
      {
        _insert(var, locate);
        return true;
      }
      else if (ifNumType(type - DATATYPEKIND_BOOLEAN_VECTOR)&&ifNumType(dataKind<T>::type))
      {
        CVariant temp = var;
        temp._cast_1(static_cast<datatype_t>(type - DATATYPEKIND_BOOLEAN_VECTOR));
        _insert(temp, locate);
        return true;
      }
      return false;
    }
    template<typename T> bool insert(const std::vector<T>& var, unsigned int locate)
    {
      if (static_cast<datatype_t>(dataKind<T>::type + DATATYPEKIND_BOOLEAN_VECTOR) == type)
      {
        _insert(var, locate);
        return true;
      }
      return false;
    }
    template<typename T> bool insert(const T* var, unsigned int n, unsigned int locate)
    {
      if (static_cast<datatype_t>(dataKind<T>::type + DATATYPEKIND_BOOLEAN_VECTOR) == type)
      {
        _insert(var, n, locate);
        return true;
      }
      return false;
    }
    bool insert(const CVariant& var, unsigned int locate);

    /* append elements to the end of a vector type CVariant object, ifthe CVariant is a 
    base type(but must same as the input or can do a cast), the function will upgrade 
    the CVariant to vector type. */
    bool append(const char* var);
    bool append(const char* var, std::size_t n);
    template<typename T>bool append(const T var)
    {
      if (static_cast<datatype_t>(dataKind<T>::type) == type ||
          (ifNumType(dataKind<T>::type) && ifNumType(type)))
      { //input type = current type or can do a cast
        _upgrade();
      }
      return insert(var,getSize());
    }
    template<typename T>bool append(const std::vector<T>& var)
    {
      if (static_cast<datatype_t>(dataKind<T>::type) == type)
      {
        _upgrade();
      }
      return insert(var,getSize());
    }
    bool append(const CVariant& var);

    /* Delete a pointed element or a range of elements in vector type CVariant object */
    bool erease(unsigned int locate);
    bool erease(unsigned int first, unsigned int last);

    /* Get the ptr to the origin data memory area. */
    const void* getPtr(void) const;
    template<char> const char* getPtr(void) const; 
    template<typename T> const T* getPtr(void) const
    {
      if (static_cast<datatype_t>(dataKind<T>::type) == type)
        return static_cast<const T*>(data);
      return nullptr;
    }

    /* Get value of a CVariant object. for a vector type, you can call as: 
    `var[i].get`.return true ifsuccess.*/
    template<typename T>bool get(T& retVal) const 
    {
      if (static_cast<datatype_t>(dataKind<T>::type) == type)
      {
        retVal = *static_cast<T*>(data);
        return true;
      }
      return false;
    }

    /* Directly return the value,but iffailed, will abort the whole 
    proccess(current type of CVariant object can't cast to the given
     type T) */
    template<typename T>T value(void) const 
    {
      if (static_cast<datatype_t>(dataKind<T>::type) == type)
      {
        return  *static_cast<T*>(data);
      }
      else if (ifNumType(dataKind<T>::type)&&ifNumType(type))
      {
        CVariant temp = *this;
        temp._cast_1(static_cast<datatype_t>(dataKind<T>::type));
        return *static_cast<T*>(temp.data);
      }
      
      bad_variant_cast err(static_cast<datatype_t>(dataKind<T>::type), type);
      throw err;
      return 0;
    }

    /* cast CVariant to string for print or other use 
     * fmt : "hex" -> cast number type CVariant to hex format string;
     *       "trunc" -> auto trunc the tail zero after decimal point.
     * prec : the number of digits after the decimal point be keeped
     */
    std::string toString(const char* fmt) const;
    std::string toString(int prec) const;
    std::string toString(void) const;

    /* Set a CVariant object's value, different form operator = ,it can't
     change the CVariant object's type, so ifthe input's type is different 
     form object's current type and can't do a static_cast, the operate will 
     not success. In other words, this function will do type checking and 
     not a copy assignment function */
    bool setValue(void* dat, std::size_t size);
    bool setValue(const char* value);
    bool setValue(const char* value, std::size_t n = 0);
    template<typename T>bool setValue(T value) 
    {
      if (static_cast<datatype_t>(dataKind<T>::type) == type)
      {
        if (*refCount > 1)
          _copyself();
        *static_cast<T*>(data) = value;
        return true;
      }
      else if (ifNumType(dataKind<T>::type)&&ifNumType(type))
      {
        *this = _cast_2(type,value);
        return true;
      }
      return false;
    }

    /* Set a CVariant to a pointed type, and will call clear() ifthe CVariant 
    object is not empty. */
    bool setType(datatype_t tp);

    /* Get a CVariant object's type */
    datatype_t getType(void) const;

    /* Get a vector type CVariant object's size ( base type always 1) */
    unsigned int getSize(void) const;

    bool ifBaseType(void)const;
    bool ifNumType(void)const;
    bool ifVectorType(void)const;
    bool ifType(void)const;
    static unsigned int getTypeSize(int type);
    static inline bool ifBaseType(int type) 
    {
      return (type >= DATATYPEKIND_BOOLEAN && type < DATATYPE_BASE_END);
    }
    static inline bool ifNumType(int type) 
    {
      bool temp =(type >= DATATYPEKIND_SBYTE && type <= DATATYPEKIND_DOUBLE);
      return temp;
    }
    static inline bool ifVectorType(int type) 
    {
      return(type >= DATATYPEKIND_BOOLEAN_VECTOR && type < DATATYPE_VECTOR_END);
    }
    static inline bool ifType(int type) 
    {
      return (ifVectorType(type) || ifBaseType(type));
    }
  };
}
#endif