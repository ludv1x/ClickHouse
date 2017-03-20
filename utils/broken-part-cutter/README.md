broken-part-cutter util fixes storage after incorrect OPTIMIZE FINAL (ClickHouse version v1.1.54181 and earlier)

Binary for Ubuntu 14 could be downloaded here https://yadi.sk/d/52cz8HbY3GAcNP

When the util can work:
* Table contains only single part (in single partition)
* This part was broken after several OPTIMIZE FINAL calls, you can observers errors like `Index file /opt/clickhouse/../.../.../primary.idx is unexpectedly long`.

Note:
* This fixer removes first 8192*N first rows of your table (they cannot be restored)
* N - less or equal than number of OPTIMIZE FINAL queries called for before v1.1.54181
* The exact number of removed lines will be printed by the util: `Will cut first 16384 rows of PK columns`

How to use:
* Stop clickhouse-server: `sudo service clickhouse-server stop`
* Move deatached parts (i.e. parts from deateached directory) to storagre dir
`mv  /opt/clickhouse/data/db/broken_table/detached/* /opt/clickhouse/data/db/broken_table/`
* Check that only single part in the broken table
* run broken-part-cutter `./broken-part-cutter <db> <broken_table>`
* It should be finished sucessfully
* If it fails, you could revert changes, just copy content of created backup dir into broken partition (it will looks like `19700101_19700101_2_8_3_backup`), contact with me
