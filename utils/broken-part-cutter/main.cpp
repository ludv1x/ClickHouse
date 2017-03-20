#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>

#include <Poco/Util/XMLConfiguration.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/OptionCallback.h>
#include <Poco/String.h>

#include <DB/Databases/DatabaseOrdinary.h>

#include <DB/Storages/System/StorageSystemNumbers.h>
#include <DB/Storages/System/StorageSystemTables.h>
#include <DB/Storages/System/StorageSystemDatabases.h>
#include <DB/Storages/System/StorageSystemProcesses.h>
#include <DB/Storages/System/StorageSystemEvents.h>
#include <DB/Storages/System/StorageSystemOne.h>
#include <DB/Storages/System/StorageSystemSettings.h>
#include <DB/Storages/System/StorageSystemDictionaries.h>
#include <DB/Storages/System/StorageSystemColumns.h>
#include <DB/Storages/System/StorageSystemFunctions.h>
#include <DB/Storages/System/StorageSystemBuildOptions.h>

#include <DB/Interpreters/Context.h>
#include <DB/Interpreters/ProcessList.h>
#include <DB/Interpreters/executeQuery.h>
#include <DB/Interpreters/loadMetadata.h>

#include <DB/Common/Exception.h>
#include <DB/Common/Macros.h>
#include <DB/Common/ConfigProcessor.h>
#include <DB/Common/escapeForFileName.h>

#include <DB/IO/ReadBufferFromString.h>
#include <DB/IO/WriteBufferFromFileDescriptor.h>

#include <DB/Parsers/parseQuery.h>
#include <DB/Parsers/IAST.h>

#include <common/ErrorHandlers.h>
#include <common/ApplicationServerExt.h>

#include <DB/Storages/StorageMergeTree.h>
#include <DB/Storages/MergeTree/MergeTreeBlockInputStream.h>
#include <DB/Storages/MergeTree/MergedBlockOutputStream.h>
#include <DB/DataStreams/MaterializingBlockInputStream.h>
#include <DB/DataStreams/ExpressionBlockInputStream.h>
#include <DB/Interpreters/Settings.h>
#include <DB/Common/escapeForFileName.h>
#include <Poco/Util/Application.h>

using namespace DB;

static size_t getFileSize(const std::string & file_path)
{
	std::ifstream file(file_path, std::ifstream::ate | std::ifstream::binary);
	if (!file.is_open())
		throw Exception("Can't open file " + file_path);
	return file.tellg();
}

static void fixStorage(StorageMergeTree & storage)
{
	MergeTreeData & data = storage.getData();
	MergeTreeData::DataPartsVector parts = data.getDataPartsVector();
	MergeTreeData::DataPartPtr part;
	
	if (parts.size() != 1)
		throw Exception("Expected 1 part");;
	part = parts[0];

	size_t index_granularity = data.index_granularity;
	std::string storage_path = storage.getFullPath();
	std::string part_path =  storage_path + part->name + "/";
	
	std::cout << "Fixing " << part_path << " with index_granularity " << index_granularity << "\n";

	std::set<size_t> columns_elements;
	std::map<std::string, size_t> column_mrk_sizes;
	for (const auto & name_and_type : part->columns)
	{
		std::string column_mrk_file = part_path + escapeForFileName(name_and_type.name) + ".mrk";
		size_t size_of_column_bytes = getFileSize(column_mrk_file);
		size_t size_of_column_marks = size_of_column_bytes / 16;
		
		std::cout << name_and_type.name << " " << name_and_type.type->getName() << ":\t" << size_of_column_marks << " marks " << "\n";
		
		if (!size_of_column_bytes || size_of_column_bytes % 16 != 0)
			throw Expected("Unexped size of " + size_of_column_bytes);
		
		columns_elements.emplace(size_of_column_marks);
		column_mrk_sizes.emplace(name_and_type.name, size_of_column_marks);
	}
	
	if (columns_elements.size() != 2)
		throw Exception("Expeted two different sizes of mrk files");
	
	size_t min_marks = *columns_elements.begin();
	size_t max_marks = *columns_elements.rbegin();
	
	std::cout << "min_marks: " << min_marks << ", max_marks: " << max_marks << "\n";
	
	if (min_marks != part->size && max_marks != part->size)
		throw Exception("Unexpected declared part of size");
	
	Names min_columns, max_columns;
	for (const auto & elem : column_mrk_sizes)
	{
		if (elem.second == max_marks)
			max_columns.emplace_back(elem.first);
		else
			min_columns.emplace_back(elem.first);
	}
	
	size_t num_max_rows = 0, num_min_rows = 0;
	{
		MergeTreeBlockInputStream stream_max_cloumns(
			part_path, index_granularity, max_columns, data, part,
			MarkRanges(1, MarkRange(0, max_marks)), false, nullptr, "", true, 0, DBMS_DEFAULT_BUFFER_SIZE, false);
		
		Block block;
		while ((block = stream_max_cloumns.read()))
		{
			num_max_rows += block.rows();
		}
	}
	{
		MergeTreeBlockInputStream stream_min_cloumns(
			part_path, index_granularity, min_columns, data, part,
			MarkRanges(1, MarkRange(0, max_marks)), false, nullptr, "", true, 0, DBMS_DEFAULT_BUFFER_SIZE, false);
		
		Block block;
		while ((block = stream_min_cloumns.read()))
		{
			num_min_rows += block.rows();
		}
	}
	
	std::cout << "min_rows: " << num_min_rows << " " << "max_rows: " << num_max_rows << "\n";
	if (!(num_min_rows < num_max_rows && (num_max_rows - num_min_rows) % index_granularity == 0))
	{
		throw Exception("Unexpected real size of columns");
	}
	std::cout << "Will cut first " << num_max_rows - num_min_rows << " rows of PK columns\n";
	
	//std::string backup_dir = storage_path + part->name + "_backup/";
	std::string backup_dir = Poco::Path().current() + "/" + part->name + "_backup/";
	std::cout << "Creating backup in " << backup_dir << "\n";
	
	Poco::File(backup_dir).createDirectories();
	Poco::File(part_path + "primary.idx").copyTo(backup_dir);
	Poco::File(part_path + "checksums.txt").copyTo(backup_dir);
	
	for (const auto & column : max_columns)
	{
		Poco::File(part_path + escapeForFileName(column) + ".mrk").copyTo(backup_dir);
		Poco::File(part_path + escapeForFileName(column) + ".bin").copyTo(backup_dir);
	}
	
	/// Initilize readers and writers
	/// Write to tmp dir
	std::string tmp_dir = part_path + "tmp_pk/";
	Poco::File(tmp_dir).createDirectories();
	
	MergeTreeData::DataPart::Checksums pk_checksums;
	{
		auto compression_method = data.context.chooseCompressionMethod(
			part->size_in_bytes,
			static_cast<double>(part->size_in_bytes) / data.getTotalActiveSizeInBytes());
		MergeTreeData::DataPart::ColumnToSize merged_column_to_size;
		for (const MergeTreeData::DataPartPtr & part : parts)
			part->accumulateColumnSizes(merged_column_to_size);
		
		NamesAndTypesList max_columns_with_types;
		{
			Block sample_block = data.getSampleBlock();
			for (const auto & column : max_columns)
				max_columns_with_types.emplace_back(column, sample_block.getByName(column).type);
		}
		
		MergedBlockOutputStream pk_writer{
			data, tmp_dir, max_columns_with_types, compression_method, merged_column_to_size, 0};
		
		auto input = std::make_unique<MergeTreeBlockInputStream>(
			part_path, index_granularity, max_columns, data, part,
			MarkRanges(1, MarkRange(0, max_marks)), false, nullptr, "", true, 0, DBMS_DEFAULT_BUFFER_SIZE, false);
		BlockInputStreamPtr pk_reader;
			
		if (data.merging_params.mode != MergeTreeData::MergingParams::Unsorted)
			pk_reader = std::make_shared<MaterializingBlockInputStream>(
				std::make_shared<ExpressionBlockInputStream>(BlockInputStreamPtr(std::move(input)), data.getPrimaryExpression()));
			else
				pk_reader = std::move(input);
		
		Block block;
		size_t num_blocks_to_skip = max_marks - min_marks;
		size_t rows_written = 0;
		for (size_t i = 0; i < max_marks; ++i)
		{
			block = pk_reader->read();
			
			if (!block || (i < max_marks - 1 && block.rows() != index_granularity))
				throw Exception("Unexpected size of first blocks");
			
			if (i < num_blocks_to_skip)
				continue;
			
			pk_writer.write(block);
			rows_written += block.rows();
			if (i % std::max(num_max_rows / 10, 1ul) == 0 || i == max_marks - 1)
				std::cout << "Written " << rows_written << "/" << num_min_rows << " new rows\n";
		}
		
		pk_reader->readSuffix();
		pk_checksums = pk_writer.writeSuffixAndGetChecksums();
	}
	
	/// Move from tmp
	Poco::File(tmp_dir + "primary.idx").moveTo(part_path);
	for (const std::string & column : max_columns)
	{
		Poco::File(tmp_dir + escapeForFileName(column) + ".bin").moveTo(part_path);
		Poco::File(tmp_dir + escapeForFileName(column) + ".mrk").moveTo(part_path);
	}
	Poco::File(tmp_dir).remove(true);
	
	for (const std::string & column : max_columns)
	{
		if (getFileSize(part_path + escapeForFileName(column) + ".mrk") != min_marks * 16)
			throw Exception("Unexpected size of " + part_path + escapeForFileName(column) + ".mrk");
	}
	
	/// Update checksums
	MergeTreeData::DataPart::Checksums & res_checksums = const_cast<MergeTreeData::DataPart::Checksums &>(part->checksums);
	for (const auto & elem : pk_checksums.files)
	{
		std::cout << "Update checksum for file " << elem.first << "\n";
		
		auto it = res_checksums.files.find(elem.first);
		if (it == res_checksums.files.end())
			throw Exception("Checksum for file " + elem.first + " not found");
		
		it->second = elem.second;
	}
	res_checksums.checkSizes(part_path);
	{
		WriteBufferFromFile checksum_file(part_path + "checksums.txt");
		res_checksums.write(checksum_file);
	}
}


class Fixer : public Poco::Util::Application
{
public:
	
	int main(const std::vector<std::string> & args) override
	{
		if (args.size() != 2)
		{
			std::cerr << "usage: <db> <table_to_fix>\n";
			return -1;
		}
		
		std::string database_name = args[0];
		std::string table_name = args[1];
		
		ConfigurationPtr processed_config = ConfigProcessor(false, true).loadConfig("/etc/clickhouse-server/config.xml");
		config().add(processed_config.duplicate(), 0, false);
		
		/// Disable merges
		config().setUInt64("merge_tree.max_bytes_to_merge_at_max_space_in_pool", 1ul);
		config().setUInt64("merge_tree.max_bytes_to_merge_at_min_space_in_pool", 1ul);
		
		auto context = std::make_unique<Context>();
		context->setGlobalContext(*context);
		context->setApplicationType(Context::ApplicationType::LOCAL_SERVER);
		context->getSettingsRef().background_pool_size = 1;
		
		std::cout << "Will fix table " << database_name << "." << table_name << "\n";
		std::cout << "Path " << config().getString("path") << "\n";
		
		//context->setSetting("profile", config().getString("default_profile", "default"));
		
		context->setPath(config().getString("path"));
		std::cout << "Loading metadata from " << context->getPath() << "\n";
		loadMetadata(*context);
		std::cout << "Loaded metadata from " << context->getPath() << "\n";
		
		StoragePtr table = context->getTable(database_name, table_name);
		StorageMergeTree & storage = dynamic_cast<StorageMergeTree &>(*table);
		fixStorage(storage);
		
		context->shutdown();
		context.reset();
		
		return 0;
	}
};


int main(int argc, char ** argv)
try
{
	Fixer app;
	app.init(argc, argv);
	app.run();
}
catch (...)
{
	std::cerr << getCurrentExceptionMessage(true);
	return -1;
}
