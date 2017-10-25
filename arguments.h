#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <vector>
#include <functional>

#include "function_traits.h"

//////////////////////////////////////////////////////////////////////////////////////
///
///
namespace arguments
{

struct ArgumentType
{
    type_index m_type;
    bool m_isConst:1;
    bool m_isRef:1;

    ArgumentType()
        : m_type(typeid(void))
        , m_isConst(false)
        , m_isRef(false)
    {}
    ArgumentType(const ArgumentType &other)
        : m_type(other.m_type)
        , m_isConst(other.m_isConst)
        , m_isRef(other.m_isRef)
    {}
    ArgumentType(const type_index &type, bool isConst, bool isRef)
        : m_type(type)
        , m_isConst(isConst)
        , m_isRef(isRef)
    {}

    template<typename Type>
    static ArgumentType &&value()
    {
        return move( ArgumentType{
                         typeid(Type),
                         is_const<typename remove_reference<Type>::type>::value,
                         is_reference<typename remove_const<Type>::type>::value
                     });
    }

    bool operator ==(const ArgumentType &that) const
    {
        return (m_type == that.m_type)
               && (m_isRef == that.m_isRef)
               && (m_isConst == that.m_isConst);
    }

    bool isCompatible(const ArgumentType &invoked) const
    {
        if (m_type != invoked.m_type) {
            return false;
        }
        if (m_isRef && m_isRef != invoked.m_isRef) {
            // non-reference invokes are allowed only if the declaration is const
            return m_isConst;
        }
        return true;
    }
};

typedef vector<ArgumentType> ArgContainer;
typedef ArgContainer::const_iterator ArgIterator;

template<typename... Arguments>
static constexpr arguments::ArgContainer argumentTypes(Arguments && ...args)
{
    array<arguments::ArgumentType, sizeof... (Arguments)> aa = { arguments::ArgumentType::value<Arguments>()... };
    return arguments::ArgContainer(aa.begin(), aa.end());
}

// extracts the return type and the arguments of a given method
template<class TClass, typename TReturnType, typename... Arguments>
static constexpr arguments::ArgContainer argumentTypes(TReturnType (TClass::*)(Arguments...))
{
    const array<arguments::ArgumentType, sizeof... (Arguments) + 1> aa =
            { arguments::ArgumentType::value<TReturnType>(), arguments::ArgumentType::value<Arguments>()... };
    return arguments::ArgContainer(aa.begin(), aa.end());
}

} // namespace arguments

// generic argument holding values in variants
class ArgumentBase
{
protected:
    const char *m_name;
    const void *m_data;
    arguments::ArgumentType m_type;
public:
    explicit ArgumentBase(const char *name = nullptr, const void *data = nullptr)
        : m_name(name)
        , m_data(data)
    {}
    virtual ~ArgumentBase() {}
    inline bool isValid() const
    {
        return m_name != nullptr;
    }
    inline const void *data() const
    {
        return const_cast<void*>(m_data);
    }
    inline const char *name() const
    {
        return m_name;
    }

    const arguments::ArgumentType &type() const
    {
        return m_type;
    }
};

class ReturnArgumentBase : public ArgumentBase
{
public:
    explicit ReturnArgumentBase(const char *name = nullptr, const void *data = nullptr)
        : ArgumentBase(name, data)
    {
    }
};

// specializations
template<class T>
class Argument : public ArgumentBase
{
public:
    inline Argument(const char *name, T &data)
        : ArgumentBase(name, static_cast<const void*>(&data))
    {
        m_type = arguments::ArgumentType::value<T>();
    }
};

template<class T>
class Argument<T &> : public ArgumentBase
{
public:
    inline Argument(const char *name, T &data)
        : ArgumentBase(name, static_cast<const void*>(&data))
    {
        m_type = arguments::ArgumentType::value<T>();
    }

    T value() const
    {
        static_cast<T>(data);
    }
};

template<class T>
class ReturnArgument : public ReturnArgumentBase
{
public:
    inline ReturnArgument(const char *name, T &data)
        : ReturnArgumentBase(name, static_cast<const void*>(data))
    {
    }
};

#define ARG(type, value)        Argument<type>(#type, value)
#define RET_ARG(type, value)    ReturnArgument<type>(#type, value)

#define MAX_ARGS    3


#endif // ARGUMENTS_H
