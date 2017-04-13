#pragma once

#include <vector>
#include <algorithm>
#include <type_traits>
#include <functional>

#include <Common/Exception.h>
#include <Core/Types.h>
#include <common/strong_typedef.h>


namespace DB
{

namespace ErrorCodes
{
    extern const int BAD_TYPE_OF_FIELD;
    extern const int BAD_GET;
    extern const int NOT_IMPLEMENTED;
}

class Field;
using Array = std::vector<Field>;
using TupleBackend = std::vector<Field>;
STRONG_TYPEDEF(TupleBackend, Tuple); /// Array and Tuple are different types with equal representation inside Field.


/** 32 is enough. Round number is used for alignment and for better arithmetic inside std::vector.
  */
#define DBMS_MIN_FIELD_SIZE 32


/** Discriminated union из нескольких типов.
  * Сделан для замены boost::variant:
  *  является не обобщённым,
  *  зато несколько более эффективным, и более простым.
  *
  * Используется для представления единичного значения одного из нескольких типов в оперативке.
  * Внимание! Предпочтительно вместо единичных значений хранить кусочки столбцов. См. Column.h
  */
class Field
{
public:
    struct Types
    {
        /// Type tag.
        enum Which
        {
            Null                = 0,
            UInt64              = 1,
            Int64               = 2,
            Float64             = 3,

            /// Non-POD types.

            String              = 16,
            Array               = 17,
            Tuple               = 18,
        };

        static const int MIN_NON_POD = 16;

        static const char * toString(Which which)
        {
            switch (which)
            {
                case Null:                 return "Null";
                case UInt64:             return "UInt64";
                case Int64:             return "Int64";
                case Float64:             return "Float64";
                case String:             return "String";
                case Array:             return "Array";
                case Tuple:             return "Tuple";

                default:
                    throw Exception("Bad type of Field", ErrorCodes::BAD_TYPE_OF_FIELD);
            }
        }
    };


    /// Позволяет получить идентификатор для типа или наоборот.
    template <typename T> struct TypeToEnum;
    template <Types::Which which> struct EnumToType;


    Field()
        : which(Types::Null)
    {
    }

    /** Не смотря на наличие шаблонного конструктора, этот конструктор всё-равно нужен,
      *  так как при его отсутствии, компилятор всё-равно сгенерирует конструктор по-умолчанию.
      */
    Field(const Field & rhs)
    {
        create(rhs);
    }

    Field(Field && rhs)
    {
        create(std::move(rhs));
    }

    template <typename T>
    Field(T && rhs,
        typename std::enable_if<!std::is_same<typename std::decay<T>::type, Field>::value, void>::type * unused = nullptr)
    {
        createConcrete(std::forward<T>(rhs));
    }

    /// Создать строку inplace.
    Field(const char * data, size_t size)
    {
        create(data, size);
    }

    Field(const unsigned char * data, size_t size)
    {
        create(data, size);
    }

    /// NOTE In case when field already has string type, more direct assign is possible.
    void assignString(const char * data, size_t size)
    {
        destroy();
        create(data, size);
    }

    void assignString(const unsigned char * data, size_t size)
    {
        destroy();
        create(data, size);
    }

    Field & operator= (const Field & rhs)
    {
        if (this != &rhs)
        {
            if (which != rhs.which)
            {
                destroy();
                create(rhs);
            }
            else
                assign(rhs);    /// This assigns string or vector without deallocation of existing buffer.
        }
        return *this;
    }

    Field & operator= (Field && rhs)
    {
        if (this != &rhs)
        {
            if (which != rhs.which)
            {
                destroy();
                create(std::move(rhs));
            }
            else
                assign(std::move(rhs));
        }
        return *this;
    }

    template <typename T>
    typename std::enable_if<!std::is_same<typename std::decay<T>::type, Field>::value, Field &>::type
    operator= (T && rhs)
    {
        if (which != TypeToEnum<typename std::decay<T>::type>::value)
        {
            destroy();
            createConcrete(std::forward<T>(rhs));
        }
        else
            assignConcrete(std::forward<T>(rhs));

        return *this;
    }

    ~Field()
    {
        destroy();
    }


    Types::Which getType() const { return which; }
    const char * getTypeName() const { return Types::toString(which); }

    bool isNull() const { return which == Types::Null; }


    template <typename T> T & get()
    {
        using TWithoutRef = typename std::remove_reference<T>::type;
        TWithoutRef * __attribute__((__may_alias__)) ptr = reinterpret_cast<TWithoutRef*>(storage);
        return *ptr;
    };

    template <typename T> const T & get() const
    {
        using TWithoutRef = typename std::remove_reference<T>::type;
        const TWithoutRef * __attribute__((__may_alias__)) ptr = reinterpret_cast<const TWithoutRef*>(storage);
        return *ptr;
    };

    template <typename T> T & safeGet()
    {
        const Types::Which requested = TypeToEnum<typename std::decay<T>::type>::value;
        if (which != requested)
            throw Exception("Bad get: has " + std::string(getTypeName()) + ", requested " + std::string(Types::toString(requested)), ErrorCodes::BAD_GET);
        return get<T>();
    }

    template <typename T> const T & safeGet() const
    {
        const Types::Which requested = TypeToEnum<typename std::decay<T>::type>::value;
        if (which != requested)
            throw Exception("Bad get: has " + std::string(getTypeName()) + ", requested " + std::string(Types::toString(requested)), ErrorCodes::BAD_GET);
        return get<T>();
    }


    bool operator< (const Field & rhs) const
    {
        if (which < rhs.which)
            return true;
        if (which > rhs.which)
            return false;

        switch (which)
        {
            case Types::Null:                 return false;
            case Types::UInt64:             return get<UInt64>()                 < rhs.get<UInt64>();
            case Types::Int64:                 return get<Int64>()                 < rhs.get<Int64>();
            case Types::Float64:             return get<Float64>()                 < rhs.get<Float64>();
            case Types::String:             return get<String>()                 < rhs.get<String>();
            case Types::Array:                 return get<Array>()                 < rhs.get<Array>();
            case Types::Tuple:                 return get<Tuple>()                 < rhs.get<Tuple>();

            default:
                throw Exception("Bad type of Field", ErrorCodes::BAD_TYPE_OF_FIELD);
        }
    }

    bool operator> (const Field & rhs) const
    {
        return rhs < *this;
    }

    bool operator<= (const Field & rhs) const
    {
        if (which < rhs.which)
            return true;
        if (which > rhs.which)
            return false;

        switch (which)
        {
            case Types::Null:                 return true;
            case Types::UInt64:             return get<UInt64>()                 <= rhs.get<UInt64>();
            case Types::Int64:                 return get<Int64>()                 <= rhs.get<Int64>();
            case Types::Float64:             return get<Float64>()                 <= rhs.get<Float64>();
            case Types::String:             return get<String>()                 <= rhs.get<String>();
            case Types::Array:                 return get<Array>()                 <= rhs.get<Array>();
            case Types::Tuple:                 return get<Tuple>()                 <= rhs.get<Tuple>();

            default:
                throw Exception("Bad type of Field", ErrorCodes::BAD_TYPE_OF_FIELD);
        }
    }

    bool operator>= (const Field & rhs) const
    {
        return rhs <= *this;
    }

    bool operator== (const Field & rhs) const
    {
        if (which != rhs.which)
            return false;

        switch (which)
        {
            case Types::Null:                 return true;
            case Types::UInt64:
            case Types::Int64:
            case Types::Float64:            return get<UInt64>()                 == rhs.get<UInt64>();
            case Types::String:             return get<String>()                 == rhs.get<String>();
            case Types::Array:                 return get<Array>()                 == rhs.get<Array>();
            case Types::Tuple:                 return get<Tuple>()                 == rhs.get<Tuple>();

            default:
                throw Exception("Bad type of Field", ErrorCodes::BAD_TYPE_OF_FIELD);
        }
    }

    bool operator!= (const Field & rhs) const
    {
        return !(*this == rhs);
    }

private:
    static const size_t storage_size = std::max({
        DBMS_MIN_FIELD_SIZE - sizeof(Types::Which),
        sizeof(Null), sizeof(UInt64), sizeof(Int64), sizeof(Float64), sizeof(String), sizeof(Array), sizeof(Tuple)});

    char storage[storage_size] __attribute__((aligned(8)));
    Types::Which which;


    /// Assuming there was no allocated state or it was deallocated (see destroy).
    template <typename T>
    void createConcrete(T && x)
    {
        using JustT = typename std::decay<T>::type;
        JustT * __attribute__((__may_alias__)) ptr = reinterpret_cast<JustT *>(storage);
        new (ptr) JustT(std::forward<T>(x));
        which = TypeToEnum<JustT>::value;
    }

    /// Assuming same types.
    template <typename T>
    void assignConcrete(T && x)
    {
        using JustT = typename std::decay<T>::type;
        JustT * __attribute__((__may_alias__)) ptr = reinterpret_cast<JustT *>(storage);
        *ptr = std::forward<T>(x);
    }


    template <typename F, typename Field>    /// Field template parameter may be const or non-const Field.
    static void dispatch(F && f, Field & field)
    {
        switch (field.which)
        {
            case Types::Null:         f(field.template get<Null>());        return;
            case Types::UInt64:     f(field.template get<UInt64>());    return;
            case Types::Int64:         f(field.template get<Int64>());        return;
            case Types::Float64:     f(field.template get<Float64>());    return;
            case Types::String:     f(field.template get<String>());    return;
            case Types::Array:         f(field.template get<Array>());        return;
            case Types::Tuple:         f(field.template get<Tuple>());        return;

            default:
                throw Exception("Bad type of Field", ErrorCodes::BAD_TYPE_OF_FIELD);
        }
    }


    void create(const Field & x)
    {
        dispatch([this] (auto & value) { createConcrete(value); }, x);
    }

    void create(Field && x)
    {
        dispatch([this] (auto & value) { createConcrete(std::move(value)); }, x);
    }

    void assign(const Field & x)
    {
        dispatch([this] (auto & value) { assignConcrete(value); }, x);
    }

    void assign(Field && x)
    {
        dispatch([this] (auto & value) { assignConcrete(std::move(value)); }, x);
    }


    void create(const char * data, size_t size)
    {
        String * __attribute__((__may_alias__)) ptr = reinterpret_cast<String*>(storage);
        new (ptr) String(data, size);
        which = Types::String;
    }

    void create(const unsigned char * data, size_t size)
    {
        create(reinterpret_cast<const char *>(data), size);
    }

    __attribute__((__always_inline__)) void destroy()
    {
        if (which < Types::MIN_NON_POD)
            return;

        switch (which)
        {
            case Types::String:
                destroy<String>();
                break;
            case Types::Array:
                destroy<Array>();
                break;
            case Types::Tuple:
                destroy<Tuple>();
                break;
            default:
                 break;
        }

        which = Types::Null;    /// for exception safety in subsequent calls to destroy and create, when create fails.
    }

    template <typename T>
    void destroy()
    {
        T * __attribute__((__may_alias__)) ptr = reinterpret_cast<T*>(storage);
        ptr->~T();
    }
};

#undef DBMS_MIN_FIELD_SIZE


template <> struct Field::TypeToEnum<Null>                                 { static const Types::Which value = Types::Null; };
template <> struct Field::TypeToEnum<UInt64>                             { static const Types::Which value = Types::UInt64; };
template <> struct Field::TypeToEnum<Int64>                             { static const Types::Which value = Types::Int64; };
template <> struct Field::TypeToEnum<Float64>                             { static const Types::Which value = Types::Float64; };
template <> struct Field::TypeToEnum<String>                             { static const Types::Which value = Types::String; };
template <> struct Field::TypeToEnum<Array>                             { static const Types::Which value = Types::Array; };
template <> struct Field::TypeToEnum<Tuple>                             { static const Types::Which value = Types::Tuple; };

template <> struct Field::EnumToType<Field::Types::Null>                 { using Type = Null                 ; };
template <> struct Field::EnumToType<Field::Types::UInt64>                 { using Type = UInt64                 ; };
template <> struct Field::EnumToType<Field::Types::Int64>                 { using Type = Int64                 ; };
template <> struct Field::EnumToType<Field::Types::Float64>             { using Type = Float64                 ; };
template <> struct Field::EnumToType<Field::Types::String>                 { using Type = String                 ; };
template <> struct Field::EnumToType<Field::Types::Array>                 { using Type = Array                 ; };
template <> struct Field::EnumToType<Field::Types::Tuple>                 { using Type = Tuple                 ; };


template <typename T>
T get(const Field & field)
{
    return field.template get<T>();
}

template <typename T>
T get(Field & field)
{
    return field.template get<T>();
}

template <typename T>
T safeGet(const Field & field)
{
    return field.template safeGet<T>();
}

template <typename T>
T safeGet(Field & field)
{
    return field.template safeGet<T>();
}


template <> struct TypeName<Array> { static std::string get() { return "Array"; } };
template <> struct TypeName<Tuple> { static std::string get() { return "Tuple"; } };


template <typename T> struct NearestFieldType;

template <> struct NearestFieldType<UInt8>         { using Type = UInt64 ; };
template <> struct NearestFieldType<UInt16>     { using Type = UInt64 ; };
template <> struct NearestFieldType<UInt32>     { using Type = UInt64 ; };
template <> struct NearestFieldType<UInt64>     { using Type = UInt64 ; };
template <> struct NearestFieldType<Int8>         { using Type = Int64 ; };
template <> struct NearestFieldType<Int16>         { using Type = Int64 ; };
template <> struct NearestFieldType<Int32>         { using Type = Int64 ; };
template <> struct NearestFieldType<Int64>         { using Type = Int64 ; };
template <> struct NearestFieldType<Float32>     { using Type = Float64 ; };
template <> struct NearestFieldType<Float64>     { using Type = Float64 ; };
template <> struct NearestFieldType<String>     { using Type = String ; };
template <> struct NearestFieldType<Array>         { using Type = Array ; };
template <> struct NearestFieldType<Tuple>         { using Type = Tuple    ; };
template <> struct NearestFieldType<bool>         { using Type = UInt64 ; };
template <> struct NearestFieldType<Null>        { using Type = Null; };


template <typename T>
typename NearestFieldType<T>::Type nearestFieldType(const T & x)
{
    return typename NearestFieldType<T>::Type(x);
}


class ReadBuffer;
class WriteBuffer;

/// Предполагается что у всех элементов массива одинаковый тип.
void readBinary(Array & x, ReadBuffer & buf);

inline void readText(Array & x, ReadBuffer & buf)             { throw Exception("Cannot read Array.", ErrorCodes::NOT_IMPLEMENTED); }
inline void readQuoted(Array & x, ReadBuffer & buf)         { throw Exception("Cannot read Array.", ErrorCodes::NOT_IMPLEMENTED); }

/// Предполагается что у всех элементов массива одинаковый тип.
void writeBinary(const Array & x, WriteBuffer & buf);

void writeText(const Array & x, WriteBuffer & buf);

inline void writeQuoted(const Array & x, WriteBuffer & buf) { throw Exception("Cannot write Array quoted.", ErrorCodes::NOT_IMPLEMENTED); }

void readBinary(Tuple & x, ReadBuffer & buf);

inline void readText(Tuple & x, ReadBuffer & buf)             { throw Exception("Cannot read Tuple.", ErrorCodes::NOT_IMPLEMENTED); }
inline void readQuoted(Tuple & x, ReadBuffer & buf)         { throw Exception("Cannot read Tuple.", ErrorCodes::NOT_IMPLEMENTED); }

void writeBinary(const Tuple & x, WriteBuffer & buf);

void writeText(const Tuple & x, WriteBuffer & buf);

inline void writeQuoted(const Tuple & x, WriteBuffer & buf) { throw Exception("Cannot write Tuple quoted.", ErrorCodes::NOT_IMPLEMENTED); }

}
