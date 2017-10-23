#ifndef METACLASS_H
#define METACLASS_H

#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <typeindex>

#include "metatype.h"

using namespace std;

namespace traits
{

template<typename Type>
struct function_traits;//: function_traits< decltype<&Type::operator ()) > {};

template<typename Ret, typename... Args>
struct function_traits<Ret (Args...)>
{
    static constexpr size_t arity = sizeof... (Args);
    using return_type = Ret;
    using arg_types = tuple<Args...>;
};

template<typename Ret, typename... Args>
struct function_traits<Ret (*)(Args...)> : function_traits<Ret (Args...)> {};

template<typename Ret, typename... Args>
struct function_traits<Ret (&)(Args...)> : function_traits<Ret (Args...)> {};

template<class Class, typename Ret, typename... Args>
struct function_traits<Ret (Class::*)(Args...)> : function_traits<Ret (Args...)>
{
    using class_type = Class;
};

template<class Class, typename Ret, typename... Args>
struct function_traits<Ret (Class::*)(Args...) const> : function_traits<Ret (Args...)>
{
    using class_type = Class;
};

template<class Class, typename Ret, typename... Args>
struct function_traits<Ret (Class::*)(Args...) volatile> : function_traits<Ret (Args...)>
{
    using class_type = Class;
};

template<class Class, typename Ret, typename... Args>
struct function_traits<Ret (Class::*)(Args...) const volatile> : function_traits<Ret (Args...)>
{
    using class_type = Class;
};

template<typename Type>
struct function_traits<std::function<Type>> : function_traits<Type> {};

//////////////////////////////////////////////////////////
/// arguments
template<typename Func, size_t Index>
struct param_types
{
    using type = typename tuple_element<Index, typename function_traits<Func>::arg_types>::type;
};

template<typename Func, size_t Index>
using param_types_t = typename param_types<Func, Index>::type;

} // namespace impl

//////////////////////////////////////////////////////////////////////////////////////
/// expand the tuple
///
namespace tuple_invoke
{

template<size_t N>
struct ApplyMember
{
    template<class Class, typename Func, typename Tuple, typename... Arguments>
    static inline auto apply(Class&& c, Func&& f, Tuple&& t, Arguments&&... a) ->
        decltype(ApplyMember<N-1>::apply(forward<Class>(c), forward<Func>(f), forward<Tuple>(t), get<N-1>(forward<Tuple>(t)), forward<Arguments...>(a)...))
    {
        return ApplyMember<N-1>::apply(forward<Class>(c), forward<Func>(f), forward<Tuple>(t), get<N-1>(forward<Tuple>(t)), forward<Arguments...>(a)...);
    }
};

// call the method
template<>
struct ApplyMember<0>
{
    template<class Class, typename Func, typename Tuple, typename... Arguments>
    static inline auto apply(Class&& c, Func&& f, Tuple&&, Arguments&&... a) ->
        decltype((forward<Class>(c)->*forward<Func>(f))(forward<Arguments>(a)...))
    {
        return(forward<Class>(c)->*forward<Func>(f))(forward<Arguments>(a)...);
    }
};

// the main function
template<class Class, typename Func, typename Tuple>
inline auto apply(Class&& c, Func&& f, Tuple&& t) ->
    decltype(ApplyMember<tuple_size<typename decay<Tuple>::type>::value>::apply(forward<Class>(c), forward<Func>(f), forward<Tuple>(t)))
{
    return ApplyMember<tuple_size<typename decay<Tuple>::type>::value>::apply(forward<Class>(c), forward<Func>(f), forward<Tuple>(t));
}

} // namespace tuple_invoke
//////////////////////////////////////////////////////////////////////////////////////
///
///
class MetaMethodBase
{
public:
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

    virtual ~MetaMethodBase() {}
    explicit MetaMethodBase(const string &name)
        : m_name(name)
    {
    }

    template<typename... Arguments>
    static constexpr vector<ArgumentType> argumentTypes(Arguments && ...args)
    {
        array<ArgumentType, sizeof... (Arguments)> aa = { ArgumentType::value<Arguments>()... };
        return vector<ArgumentType>(aa.begin(), aa.end());
    }

    string name() const
    {
        return m_name;
    }

    typedef vector<ArgumentType>::const_iterator ArgumentIterator;
    ArgumentIterator argumentsBegin() const
    {
        return ++m_arguments.begin();
    }
    ArgumentIterator argumentsEnd() const
    {
        return m_arguments.end();
    }

    int argumentCount() const
    {
        return m_arguments.size() - 1;
    }

    bool isReturnType(const ArgumentType &retType)
    {
        return (m_arguments[0] == retType);
    }

    bool compatibleArguments(const vector<ArgumentType> &invokeArgs)
    {
        if (m_arguments.size() - 1 == invokeArgs.size()) {
            vector<ArgumentType>::const_iterator argsThis = ++m_arguments.cbegin();
            vector<ArgumentType>::const_iterator argsThat = invokeArgs.cbegin();
            bool ok = true;
            while (ok && (argsThis != m_arguments.cend()) && (argsThat != invokeArgs.cend())) {
                ok = argsThis->isCompatible((*argsThat));
                ++argsThat;
                ++argsThis;
            }
            return ok;
        }
        return false;
    }

    template <class Class, typename Func, typename Tuple>
    inline bool apply(Class && c, Func && f, Tuple && t)
    {
        if (traits::function_traits<typename decay<Func>::type>::arity == m_arguments.size() - 1) {
            tuple_invoke::apply(forward<Class>(c), forward<Func>(f), forward<Tuple>(t));
            return true;
        }
        return false;
    }

protected:
    template<class TObject, typename TReturnType, typename... Arguments>
    void extractArguments(TReturnType (TObject::*)(Arguments...))
    {
        const array<ArgumentType, sizeof... (Arguments) + 1> args =
                { ArgumentType::value<TReturnType>(), ArgumentType::value<Arguments>()... };
        m_arguments = {args.begin(), args.end()};
    }

private:
    string m_name;
    vector<ArgumentType> m_arguments;
};

//////////////////////////////////////////////////////////////////////////////////////
///
///
template <class TObject, typename TReturnType, typename... Arguments>
class MetaMethod : public MetaMethodBase
{
    TReturnType (TObject::*m_method)(Arguments...);
public:
    explicit MetaMethod(TReturnType (TObject::*method)(Arguments...), const string &name)
        : MetaMethodBase(name)
        , m_method(method)
    {
        extractArguments(method);
    }
    virtual ~MetaMethod() {}

    inline TReturnType invoke(TObject *object, Arguments... args)
    {
        if (std::is_same<TReturnType, void>::value) {
            (object->*m_method)(args...);
        } else {
            return (object->*m_method)(args...);
        }
    }

    template<typename Tuple>
    inline auto apply(TObject *object, Tuple&& t)
    {
        if (std::is_same<TReturnType, void>::value) {
            tuple_invoke::apply(object, m_method, forward<Tuple>(t));
        } else {
            return tuple_invoke::apply(object, m_method, forward<Tuple>(t));
        }
    }
};

//////////////////////////////////////////////////////////////////////////////////////
///
///
class MetaObject;
class MetaClass
{
    typedef multimap<string, MetaMethodBase*> MetaMethodContainer;
    const MetaClass *m_superClass = nullptr;
    MetaMethodContainer m_methods;

public:
    explicit MetaClass(const MetaClass *super = nullptr)
        : m_superClass(super)
    {
    }
    ~MetaClass()
    {
        for (auto mm : m_methods) {
            delete mm.second;
        }
    }
    void addMetaMethod(MetaMethodBase *method)
    {
        m_methods.insert(pair<string, MetaMethodBase*>(method->name(), method));
    }

    typedef MetaMethodContainer::const_iterator MetaMethodIterator;
    typedef pair<MetaMethodIterator, MetaMethodIterator> MetaMethodRange;
    static MetaMethodRange methodRange(MetaObject *object, const string &name);

    const MetaClass *superClass() const
    {
        return m_superClass;
    }

    template<typename TReturnType = void, typename... Arguments>
    MetaMethodBase *metaMethod(MetaObject *object, const string &signature);

    template<typename TReturnType, typename... Arguments>
    static bool invoke(MetaObject *o, const string &signature, Arguments... args);
    template<typename TReturnType, typename... Arguments>
    static bool invoke(MetaObject *o, TReturnType &ret, const string &signature, Arguments... args);

    template<class TObject, typename Tuple>
    static bool apply(TObject *object, const string &name, Tuple&& arguments)
    {
        const MetaClass *mo = object->metaClass();
        MetaMethodRange range = mo->m_methods.equal_range(name);
        if (range.first == range.second) {
            return false;
        }
        for (MetaMethodIterator i = range.first; i != range.second; ++i) {
            size_t argCount = tuple_size<typename decay<Tuple>::type>::value;
            if (i->second->argumentCount() == argCount) {
                tuple_invoke::apply(object, i->second->m_method, forward<Tuple>(arguments));
                return true;
            }
        }
        return false;
    }

private:

    template<typename TReturnType>
    MetaMethodBase *getMethod(const string &name, const vector<MetaMethodBase::ArgumentType> &argTypes) const
    {
        MetaMethodBase::ArgumentType returnType = MetaMethodBase::ArgumentType::value<TReturnType>();
        for (auto mm : m_methods) {
            if ((mm.first == name) &&
                mm.second->isReturnType(returnType) &&
                mm.second->compatibleArguments(argTypes)) {
                return mm.second;
            }
        }
        return nullptr;
    }
};

#if defined(__clang__)
    #define WARNING_PUSH                _Pragma("clang diagnostic push")
    #define DISABLE_OVERRIDE_WARNING    _Pragma("clang diagnostic ignored \"-Winconsistent-missing-override\"")
    #define WARNING_POP                 _Pragma("clang diagnostic pop")
#else
    #define WARNING_PUSH
    #define DISABLE_OVERRIDE_WARNING
    #define WARNING_POP
#endif

#define METACLASS_BEGIN(Class, SuperClass) \
    public: \
    static const MetaClass staticMetaObject; \
    WARNING_PUSH \
    DISABLE_OVERRIDE_WARNING \
    virtual const MetaClass *metaObject() const { return &staticMetaObject; } \
    virtual void initMetaClass(Class *thisClass) \
    { \
        typedef class Class TClass; \
        SuperClass::initMetaClass(static_cast<SuperClass*>(thisClass)); \
        MetaClass *mo = const_cast<MetaClass*>(thisClass->metaObject()); \
        (void)(mo);

#define METACLASS_END() \
    } \
    WARNING_POP

#define METAOBJECT(Class, SuperClass) \
const MetaClass Class::staticMetaObject { &SuperClass::staticMetaObject };

#define META_METHOD(Method, ReturnType, ...) \
    mo->addMetaMethod(new MetaMethod<TClass, ReturnType, ##__VA_ARGS__>(&TClass::Method, #Method));

//////////////////////////////////////////////////////////////////////////////////////
///
///
class MetaObject
{
public:
    // metadata section for the default class
    static const MetaClass staticMetaObject;
    virtual const MetaClass *metaObject() const { return &staticMetaObject; }
    virtual void initMetaClass(MetaObject *)
    {
        typedef class MetaObject TClass;
        MetaClass *mo = const_cast<MetaClass*>(metaObject());
        META_METHOD(abstractMethod, int, const vector<int>&)
    }

public:
    explicit MetaObject() {}
    virtual ~MetaObject() {}

    virtual int abstractMethod(const vector<int> &) = 0;
};
const MetaClass MetaObject::staticMetaObject;

//////////////////////////////////////////////////////////////////////////////////////
///
///
MetaClass::MetaMethodRange MetaClass::methodRange(MetaObject *object, const string &name)
{
    return object->metaObject()->m_methods.equal_range(name);
}

template<typename TReturnType = void, typename... Arguments>
MetaMethodBase *metaMethod(MetaObject *object, const string &signature)
{
    array<MetaMethodBase::ArgumentType, sizeof... (Arguments)> aa = { MetaMethodBase::ArgumentType::value<Arguments>()... };
    vector<MetaMethodBase::ArgumentType> argTypes = {aa.begin(), aa.end()};
    MetaClass *mo = const_cast<MetaClass*>(object->metaObject());
    return mo->getMethod<TReturnType>(signature, argTypes);
}


template<typename TReturnType, typename... Arguments>
bool MetaClass::invoke(MetaObject *o, const string &signature, Arguments... args)
{
    vector<MetaMethodBase::ArgumentType> argTypes = MetaMethodBase::argumentTypes(forward<Arguments>(args)...);
    MetaClass *mo = const_cast<MetaClass*>(o->metaObject());
    while (mo) {
        MetaMethodBase *method = mo->getMethod<TReturnType>(signature, argTypes);
        if (method) {
            MetaMethod<MetaObject, TReturnType, Arguments...> *mCasted =
                    static_cast<MetaMethod<MetaObject, TReturnType, Arguments...>*>(method);
            if (mCasted) {
                mCasted->invoke(o, args...);
                return true;
            }
        }
        // check superclass
        mo = const_cast<MetaClass*>(mo->superClass());
    }
    return false;
}

template<typename TReturnType, typename... Arguments>
bool MetaClass::invoke(MetaObject *o, TReturnType &ret, const string &signature, Arguments... args)
{
    vector<MetaMethodBase::ArgumentType> argTypes = MetaMethodBase::argumentTypes(forward<Arguments>(args)...);
    MetaClass *mo = const_cast<MetaClass*>(o->metaObject());
    while (mo) {
        MetaMethodBase *method = o->metaObject()->getMethod<TReturnType>(signature, argTypes);
        if (method) {
            MetaMethod<MetaObject, TReturnType, Arguments...> *mCasted =
                    static_cast<MetaMethod<MetaObject, TReturnType, Arguments...>*>(method);
            if (mCasted) {
                if (is_void<TReturnType>::value) {
                    mCasted->invoke(o, args...);
                } else {
                    ret = mCasted->invoke(o, args...);
                }
                return true;
            }
        }
        // continue in superclass
        cout << "super-" << signature;
        mo = const_cast<MetaClass*>(mo->superClass());
    }
    return false;
}

#endif // METACLASS_H
