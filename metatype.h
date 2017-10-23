#ifndef METATYPE_H
#define METATYPE_H

#include <typeindex>

#include <boost/variant.hpp>

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////
///
///
class MetaType
{
    int m_typeId = -1;

public:
    enum TypeId {
        Undefined = 0, // also void
        Bool,
        Char,
        UChar,
        Short,
        Word,
        Int,
        UInt,
        Long,
        ULong,
        LongLong,
        ULongLong,
        Double,
        Float,
        VoidStar,
        CharStar,
        IntStar,
        String,
        IntVector
    };
    explicit MetaType(int typeId)
        : m_typeId(typeId)
    {
    }
    explicit MetaType(const type_index &type)
        : m_typeId(MetaType::fromTypeIndex(type))
    {
    }

    static int fromTypeIndex(const type_index &type);
};

#endif // METATYPE_H
