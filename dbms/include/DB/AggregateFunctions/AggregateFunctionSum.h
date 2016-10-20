#pragma once

#include <DB/IO/WriteHelpers.h>
#include <DB/IO/ReadHelpers.h>

#include <DB/DataTypes/DataTypesNumberFixed.h>

#include <DB/AggregateFunctions/IUnaryAggregateFunction.h>


namespace DB
{

template <typename T>
struct AggregateFunctionSumData
{
	T sum;

	AggregateFunctionSumData() : sum(0) {}
};


/// Считает сумму чисел.
template <typename T>
class AggregateFunctionSum final : public IUnaryAggregateFunction<AggregateFunctionSumData<typename NearestFieldType<T>::Type>, AggregateFunctionSum<T> >
{
public:
	String getName() const override { return "sum"; }

	DataTypePtr getReturnType() const override
	{
		return std::make_shared<typename DataTypeFromFieldType<typename NearestFieldType<T>::Type>::Type>();
	}

	void setArgument(const DataTypePtr & argument)
	{
		if (!argument->behavesAsNumber())
			throw Exception("Illegal type " + argument->getName() + " of argument for aggregate function " + getName(),
				ErrorCodes::ILLEGAL_TYPE_OF_ARGUMENT);
	}


	void addImpl(AggregateDataPtr place, const IColumn & column, size_t row_num, Arena *) const
	{
		this->data(place).sum += static_cast<const ColumnVector<T> &>(column).getData()[row_num];
	}

	void merge(AggregateDataPtr place, ConstAggregateDataPtr rhs, Arena * arena) const override
	{
		this->data(place).sum += this->data(rhs).sum;
	}

	void serialize(ConstAggregateDataPtr place, WriteBuffer & buf) const override
	{
		writeBinary(this->data(place).sum, buf);
	}

	void deserialize(AggregateDataPtr place, ReadBuffer & buf, Arena *) const override
	{
		readBinary(this->data(place).sum, buf);
	}

	void insertResultInto(ConstAggregateDataPtr place, IColumn & to) const override
	{
		static_cast<ColumnVector<typename NearestFieldType<T>::Type> &>(to).getData().push_back(this->data(place).sum);
	}

	using Data = AggregateFunctionSumData<typename NearestFieldType<T>::Type>;

	bool supportsChunks() const override
	{
		return true;
	}

	void addChunks(const IColumn ** columns, StatesList & states, Arena * arena) const override
	{
		const T * column_data = &static_cast<const ColumnVector<T> &>(*columns[0]).getData()[0];

		size_t i = 0;
		for (size_t group_num = 0; group_num < states.size(); group_num++)
		{
			Data & state_data = *reinterpret_cast<Data *>(states[group_num].state);
			size_t group_end = i + states[group_num].group_size;

			for (; i < group_end; ++i)
				state_data.sum += column_data[i];
		}
	}
};


}
