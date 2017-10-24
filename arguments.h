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

#endif // ARGUMENTS_H
