#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <typeindex>

#include "metatype.h"

using namespace std;

// type_index to MetaType::type register
typedef unordered_map<type_index, int> TypeIndexContainer;
typedef TypeIndexContainer::const_iterator TypeIndexIterator;

static TypeIndexContainer metaTypeContainer = {
    make_pair(type_index(typeid(void)), MetaType::Undefined),
    make_pair(type_index(typeid(bool)), MetaType::Bool),
    make_pair(type_index(typeid(char)), MetaType::Char),
    make_pair(type_index(typeid(unsigned char)), MetaType::UChar),
    make_pair(type_index(typeid(short)), MetaType::Short),
    make_pair(type_index(typeid(unsigned short)), MetaType::Word),
    make_pair(type_index(typeid(int)), MetaType::Int),
    make_pair(type_index(typeid(unsigned int)), MetaType::UInt),
    make_pair(type_index(typeid(long int)), MetaType::Long),
    make_pair(type_index(typeid(unsigned long int)), MetaType::ULong),
    make_pair(type_index(typeid(long long)), MetaType::LongLong),
    make_pair(type_index(typeid(unsigned long long)), MetaType::ULongLong),
    make_pair(type_index(typeid(double)), MetaType::Double),
    make_pair(type_index(typeid(float)), MetaType::Float),
    make_pair(type_index(typeid(void*)), MetaType::VoidStar),
    make_pair(type_index(typeid(char*)), MetaType::CharStar),
    make_pair(type_index(typeid(int*)), MetaType::IntStar),
    make_pair(type_index(typeid(std::string)), MetaType::String),
    make_pair(type_index(typeid(std::vector<int>)), MetaType::IntVector),
};

int MetaType::fromTypeIndex(const type_index &type)
{
    TypeIndexIterator i = metaTypeContainer.find(type);
    return i == metaTypeContainer.cend() ? -1 : i->second;
}
