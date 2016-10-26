#pragma once

#include <memory>

#include <DB/Core/Row.h>
#include <DB/DataTypes/IDataType.h>
#include <DB/Common/typeid_cast.h>
#include <DB/Common/Arena.h>


namespace DB
{

namespace ErrorCodes
{
	extern const int AGGREGATE_FUNCTION_DOESNT_ALLOW_PARAMETERS;
	extern const int NUMBER_OF_ARGUMENTS_DOESNT_MATCH;
	extern const int ILLEGAL_TYPE_OF_ARGUMENT;
	extern const int ARGUMENT_OUT_OF_BOUND;
}

using AggregateDataPtr = char *;
using ConstAggregateDataPtr = const char *;


struct GroupOfStates
{
	size_t group_size;
	AggregateDataPtr state;

	GroupOfStates(size_t group_size_ = 0, AggregateDataPtr state_ = nullptr)
	: group_size(group_size_), state(state_) {}
};

using GroupedStates = std::vector<GroupOfStates>;


/** Интерфейс для агрегатных функций.
  * Экземпляры классов с этим интерфейсом не содержат самих данных для агрегации,
  *  а содержат лишь метаданные (описание) агрегатной функции,
  *  а также методы для создания, удаления и работы с данными.
  * Данные, получающиеся во время агрегации (промежуточные состояния вычислений), хранятся в других объектах
  *  (которые могут быть созданы в каком-нибудь пуле),
  *  а IAggregateFunction - внешний интерфейс для манипулирования ими.
  */
class IAggregateFunction
{
public:
	/// Получить основное имя функции.
	virtual String getName() const = 0;

	/** Указать типы аргументов. Если функция неприменима для данных аргументов - кинуть исключение.
	  * Необходимо вызывать перед остальными вызовами.
	  */
	virtual void setArguments(const DataTypes & arguments) = 0;

	/** Указать параметры - для параметрических агрегатных функций.
	  * Если параметры не предусмотрены или переданные параметры недопустимы - кинуть исключение.
	  * Если параметры есть - необходимо вызывать перед остальными вызовами, иначе - не вызывать.
	  */
	virtual void setParameters(const Array & params)
	{
		throw Exception("Aggregate function " + getName() + " doesn't allow parameters.",
			ErrorCodes::AGGREGATE_FUNCTION_DOESNT_ALLOW_PARAMETERS);
	}

	/// Получить тип результата.
	virtual DataTypePtr getReturnType() const = 0;

	virtual ~IAggregateFunction() {};


	/** Функции по работе с данными. */

	/** Создать пустые данные для агрегации с помощью placement new в заданном месте.
	  * Вы должны будете уничтожить их с помощью метода destroy.
	  */
	virtual void create(AggregateDataPtr place) const = 0;

	/// Уничтожить данные для агрегации.
	virtual void destroy(AggregateDataPtr place) const noexcept = 0;

	/// Уничтожать данные не обязательно.
	virtual bool hasTrivialDestructor() const = 0;

	/// Получить sizeof структуры с данными.
	virtual size_t sizeOfData() const = 0;

	/// Как должна быть выровнена структура с данными. NOTE: Сейчас не используется (структуры с состоянием агрегации кладутся без выравнивания).
	virtual size_t alignOfData() const = 0;

	/** Adds a value into aggregation data on which place points to.
	 *  columns points to columns containing arguments of aggregation function.
	 *  row_num is number of row which should be added.
	 *  Additional parameter arena should be used instead of standard memory allocator if the addition requires memory allocation.
	 */
	virtual void add(AggregateDataPtr place, const IColumn ** columns, size_t row_num, Arena * arena) const = 0;

	/// Merges state (on which place points to) with other state of current aggregation function.
	virtual void merge(AggregateDataPtr place, ConstAggregateDataPtr rhs, Arena * arena) const = 0;

	/// Serializes state (to transmit it over the network, for example).
	virtual void serialize(ConstAggregateDataPtr place, WriteBuffer & buf) const = 0;

	/// Deserializes state. This function is called only for empty (just created) states.
	virtual void deserialize(AggregateDataPtr place, ReadBuffer & buf, Arena * arena) const = 0;

	/// Returns true if a function requires Arena to handle own states (see add(), merge(), deserialize()).
	virtual bool allocatesMemoryInArena() const
	{
		return false;
	}

	/// Inserts results into a column.
	virtual void insertResultInto(ConstAggregateDataPtr place, IColumn & to) const = 0;

	/** Возвращает true для агрегатных функций типа -State.
	  * Они выполняются как другие агрегатные функции, но не финализируются (возвращают состояние агрегации, которое может быть объединено с другим).
	  */
	virtual bool isState() const { return false; }


	/** Внутренний цикл, использующий указатель на функцию, получается лучше, чем использующий виртуальную функцию.
	  * Причина в том, что в случае виртуальных функций, GCC 5.1.2 генерирует код,
	  *  который на каждой итерации цикла заново грузит из памяти в регистр адрес функции (значение по смещению в таблице виртуальных функций).
	  * Это даёт падение производительности на простых запросах в районе 12%.
	  * После появления более хороших компиляторов, код можно будет убрать.
	  */
	using AddFunc = void (*)(const IAggregateFunction *, AggregateDataPtr, const IColumn **, size_t, Arena *);
	virtual AddFunc getAddressOfAddFunction() const = 0;

	virtual void addChunks(const IColumn ** columns, GroupedStates & states, Arena * arena) const
	{
		throw Exception("Calling aggragte function doesn't support chunk processing");
	}

	virtual void createChunk(AggregateDataPtr first_place, size_t num_elems) const
	{
		throw Exception("Calling aggragte function doesn't support chunk processing");
	}

	virtual bool supportsChunks() const
	{
		return false;
	}
};


/// Реализует несколько методов. T - тип структуры с данными для агрегации.
template <typename T>
class IAggregateFunctionHelper : public IAggregateFunction
{
protected:
	using Data = T;

	static Data & data(AggregateDataPtr place) 				{ return *reinterpret_cast<Data*>(place); }
	static const Data & data(ConstAggregateDataPtr place) 	{ return *reinterpret_cast<const Data*>(place); }

public:
	void create(AggregateDataPtr place) const override
	{
		new (place) Data;
	}

	void destroy(AggregateDataPtr place) const noexcept override
	{
		data(place).~Data();
	}

	bool hasTrivialDestructor() const override
	{
		return __has_trivial_destructor(Data);
	}

	size_t sizeOfData() const override
	{
		return sizeof(Data);
	}

	/// NOTE: Сейчас не используется (структуры с состоянием агрегации кладутся без выравнивания).
	size_t alignOfData() const override
	{
		return __alignof__(Data);
	}
};


using AggregateFunctionPtr = std::shared_ptr<IAggregateFunction>;
using AggregateFunctionsPlainPtrs = std::vector<IAggregateFunction *>;
using AggregateFunctionsPlainConstPtrs = std::vector<const IAggregateFunction *>;

}
