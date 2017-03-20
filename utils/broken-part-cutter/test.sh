#!/bin/bash
set -e

function gen_src {
local name="$1"

clickhouse-client -q "DROP TABLE IF EXISTS $name"
clickhouse-client -q "CREATE TABLE $name (
p Date, Sign Int8, ks String, ki UInt64,
ds String,
di01 UInt64,
di02 UInt64,
di03 UInt64,
di04 UInt64,
di05 UInt64,
di06 UInt64,
di07 UInt64,
di08 UInt64,
di09 UInt64,
di10 UInt64,
t Nested(
da11 UInt64,
da12 String
)
)
ENGINE = CollapsingMergeTree(p, (ks, intHash64(ki)), 8192, Sign)"

clickhouse-client -q "INSERT INTO $name SELECT
toDate(0) AS date,
toInt8(1) AS Sign,
toString(number) AS ks,
number AS ki,
hex(number) AS ds,
number AS di01,
number AS di02,
number AS di03,
number AS di04,
number AS di05,
number AS di06,
number AS di07,
number AS di08,
number AS di09,
number AS di10,
[number] AS \`t.da11\`,
[hex(number)] AS \`t.da12\`
FROM system.numbers LIMIT 4000000"

while [[ `clickhouse-client -q "select count() from system.parts where active AND database = 'test' AND table = 'broken_part'"` -ne 1 ]] ; do
	clickhouse-client -q "OPTIMIZE TABLE $name PARTITION 197001"
	echo "Parts:" `clickhouse-client -q "select count() from system.parts where active AND database = 'test' AND table = 'broken_part'"`
done
}

function get_hash {
	clickhouse-client --max_threads=1 -q "SELECT cityHash64(h1)
	FROM (SELECT groupArray(cityHash64(p, Sign, ks, ki, ds, di01, di02, di03, di04, di05, di06, di07, di08, di09, di10, \`t.da11\`, \`t.da12\`)) AS h1 FROM test.broken_part_ref)"
}


sudo service clickhouse-server stop
rm -rf /opt/clickhouse//data/test/broken_part_src/*
sudo service clickhouse-server start
sleep 2

gen_src test.broken_part

clickhouse-client --max_threads=1 -q "DROP TABLE IF EXISTS test.broken_part_ref"
clickhouse-client --max_threads=1 -q "CREATE TABLE test.broken_part_ref AS test.broken_part"
clickhouse-client --max_threads=1 -q "INSERT INTO test.broken_part_ref SELECT * FROM test.broken_part LIMIT 16384, 3983616"

clickhouse-client -q "OPTIMIZE TABLE test.broken_part PARTITION 197001 FINAL"
clickhouse-client -q "OPTIMIZE TABLE test.broken_part PARTITION 197001 FINAL"
# check that part is broken
clickhouse-client -q "SELECT sum(ignore(*)) FROM test.broken_part" || true

ref_hash=`get_hash`
sudo service clickhouse-server stop &> /dev/null

# build util and run fixer
(cd ~/distcc-ClickHouse; ./make-distcc broken-part-cutter && ./utils/broken-part-cutter/broken-part-cutter test broken_part)

sudo service clickhouse-server start &> /dev/null
sleep 2

echo $ref_hash
get_hash


