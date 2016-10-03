![ ](https://mc.yandex.ru/watch/18343495)

[ ](/)

[ Russian ](reference_ru.html)

[ English ](reference_en.html)

# [ClickHouse](/)

## Reference Manual

— Alexey Milovidov

# Contents

  

# Introduction

==What is ClickHouse?== ClickHouse is a columnar DBMS for OLAP. In a "normal"
row-oriented DBMS, data is stored in this order: %% 5123456789123456789 1
Eurobasket - Greece - Bosnia and Herzegovina - example.com 1 2011-09-01
01:03:02 6274717 1294101174 11409 612345678912345678 0 33 6
http://www.example.com/basketball/team/123/match/456789.html
http://www.example.com/basketball/team/123/match/987654.html 0 1366 768 32 10
3183 0 0 13 0\0 1 1 0 0 2011142 -1 0 0 01321 613 660 2011-09-01 08:01:17 0 0 0
0 utf-8 1466 0 0 0 5678901234567890123 277789954 0 0 0 0 0 5234985259563631958
0 Consulting, Tax assessment, Accounting, Law 1 2011-09-01 01:03:02 6320881
2111222333 213 6458937489576391093 0 3 2 http://www.example.ru/ 0 800 600 16
10 2 153.1 0 0 10 63 1 1 0 0 2111678 000 0 588 368 240 2011-09-01 01:03:17 4 0
60310 0 windows-1251 1466 0 000 778899001 0 0 0 0 0 ... %% In other words, all
the values related to a row are stored next to each other. Examples of a row-
oriented DBMS are MySQL, Postgres, MS SQL Server, and others. In a column-
oriented DBMS, data is stored like this:

    
    
    **WatchID:**    5385521489354350662     5385521490329509958     5385521489953706054     5385521490476781638     5385521490583269446     5385521490218868806     5385521491437850694   5385521491090174022      5385521490792669254     5385521490420695110     5385521491532181574     5385521491559694406     5385521491459625030     5385521492275175494   5385521492781318214      5385521492710027334     5385521492955615302     5385521493708759110     5385521494506434630     5385521493104611398
    **JavaEnable:** 1       0       1       0       0       0       1       0       1       1       1       1       1       1       0       1       0       0       1       1
    **Title:**      Yandex  Announcements - Investor Relations - Yandex     Yandex — Contact us — Moscow    Yandex — Mission        Ru      Yandex — History — History of Yandex    Yandex Financial Releases - Investor Relations - Yandex Yandex — Locations      Yandex Board of Directors - Corporate Governance - Yandex       Yandex — Technologies
    **GoodEvent:**  1       1       1       1       1       1       1       1       1       1       1       1       1       1       1       1       1       1       1       1
    **EventTime:**  2016-05-18 05:19:20     2016-05-18 08:10:20     2016-05-18 07:38:00     2016-05-18 01:13:08     2016-05-18 00:04:06     2016-05-18 04:21:30     2016-05-18 00:34:16     2016-05-18 07:35:49     2016-05-18 11:41:59     2016-05-18 01:13:32
    ...
    

These examples only show the order that data is arranged in. The values from
different columns are stored separately, and data from the same column is
stored together. Examples of a column-oriented DBMS: Vertica, Paraccel (Actian
Matrix) (Amazon Redshift), Sybase IQ, Exasol, Infobright, InfiniDB, MonetDB
(VectorWise) (Actian Vector), LucidDB, SAP HANA, Google Dremel, Google
PowerDrill, Druid, kdb+ and others. Different orders for storing data are
better suited to different scenarios. The data access scenario refers to what
queries are made, how often, and in what proportion; how much data is read for
each type of query - rows, columns, and bytes; the relationship between
reading and updating data; the working size of the data and how locally it is
used; whether transactions are used, and how isolated they are; requirements
for data replication and logical integrity; requirements for latency and
throughput for each type of query, and so on. The higher the load on the
system, the more important it is to customize the system to the scenario, and
the more specific this customization becomes. There is no system that is
equally well-suited to significantly different scenarios. If a system is
adaptable to a wide set of scenarios, under a high load, the system will
handle all the scenarios equally poorly, or will work well for just one of the
scenarios. We'll say that the following is true for the OLAP (online
analytical processing) scenario: \- The vast majority of requests are for read
access. \- Data is updated in fairly large batches (> 1000 rows), not by
single rows; or it is not updated at all. \- Data is added to the DB but is
not modified. \- For reads, quite a large number of rows are extracted from
the DB, but only a small subset of columns. \- Tables are "wide," meaning they
contain a large number of columns. \- Queries are relatively rare (usually
hundreds of queries per server or less per second). \- For simple queries,
latencies around 50 ms are allowed. \- Column values are fairly small -
numbers and short strings (for example, 60 bytes per URL). \- Requires high
throughput when processing a single query (up to billions of rows per second
per server). \- There are no transactions. \- Low requirements for data
consistency. \- There is one large table per query. All tables are small,
except for one. \- A query result is significantly smaller than the source
data. That is, data is filtered or aggregated. The result fits in a single
server's RAM. It is easy to see that the OLAP scenario is very different from
other popular scenarios (such as OLTP or Key-Value access). So it doesn't make
sense to try to use OLTP or a Key-Value DB for processing analytical queries
if you want to get decent performance. For example, if you try to use MongoDB
or Elliptics for analytics, you will get very poor performance compared to
OLAP databases. Columnar-oriented databases are better suited to OLAP
scenarios (at least 100 times better in processing speed for most queries),
for the following reasons: 1\. For I/O. 1.1. For an analytical query, only a
small number of table columns need to be read. In a column-oriented database,
you can read just the data you need. For example, if you need 5 columns out of
100, you can expect a 20-fold reduction in I/O. 1.2. Since data is read in
packets, it is easier to compress. Data in columns is also easier to compress.
This further reduces the I/O volume. 1.3. Due to the reduced I/O, more data
fits in the system cache. For example, the query "count the number of records
for each advertising platform" requires reading one "advertising platform ID"
column, which takes up 1 byte uncompressed. If most of the traffic was not
from advertising platforms, you can expect at least 10-fold compression of
this column. When using a quick compression algorithm, data decompression is
possible at a speed of at least several gigabytes of uncompressed data per
second. In other words, this query can be processed at a speed of
approximately several billion rows per second on a single server. This speed
is actually achieved in practice.

    
    
    milovidov@████████.yandex.ru:~$ clickhouse-client
    ClickHouse client version 0.0.52053.
    Connecting to localhost:9000.
    Connected to ClickHouse server version 0.0.52053.
    
    :) SELECT CounterID, count() FROM hits GROUP BY CounterID ORDER BY count() DESC LIMIT 20
    
    SELECT
        CounterID,
        count()
    FROM hits
    GROUP BY CounterID
    ORDER BY count() DESC
    LIMIT 20
    
    ┌─CounterID─┬──count()─┐
    │    114208 │ 56057344 │
    │    115080 │ 51619590 │
    │      3228 │ 44658301 │
    │     38230 │ 42045932 │
    │    145263 │ 42042158 │
    │     91244 │ 38297270 │
    │    154139 │ 26647572 │
    │    150748 │ 24112755 │
    │    242232 │ 21302571 │
    │    338158 │ 13507087 │
    │     62180 │ 12229491 │
    │     82264 │ 12187441 │
    │    232261 │ 12148031 │
    │    146272 │ 11438516 │
    │    168777 │ 11403636 │
    │   4120072 │ 11227824 │
    │  10938808 │ 10519739 │
    │     74088 │  9047015 │
    │    115079 │  8837972 │
    │    337234 │  8205961 │
    └───────────┴──────────┘
    
    20 rows in set. Elapsed: 0.153 sec. Processed 1.00 billion rows, 4.00 GB (6.53 billion rows/s., 26.10 GB/s.)
    
    :)
    

2\. For CPU. Since executing a query requires processing a large number of
rows, it helps to dispatch all operations for entire vectors instead of for
separate rows, or to implement the query engine so that there is almost no
dispatching cost. If you don't do this, with any half-decent disk subsystem,
the query interpreter inevitably stalls the CPU. It makes sense to both store
data in columns and process it, when possible, by columns. There are two ways
to do this: 1\. A vector engine. All operations are written for vectors,
instead of for separate values. This means you don't need to call operations
very often, and dispatching costs are negligible. Operation code contains an
optimized internal cycle. 2\. Code generation. The code generated for the
query has all the indirect calls in it. This is not done in "normal"
databases, because it doesn't make sense when running simple queries. However,
there are exceptions. For example, MemSQL uses code generation to reduce
latency when processing SQL queries. (For comparison, analytical DBMSs require
optimization of throughput, not latency.) Note that for CPU efficiency, the
query language must be declarative (SQL or MDX), or at least a vector (J, K).
The query should only contain implicit loops, allowing for optimization.
==Distinctive features of ClickHouse== 1\. True column-oriented DBMS. 2\. Data
compression. 3\. Disk storage of data. 4\. Parallel processing on multiple
cores. 5\. Distributed processing on multiple servers. 6\. SQL support. 7\.
Vector engine. 8\. Real-time data updates. 9\. Indexes. 10\. Suitable for
online queries. 11\. Support for approximated calculations. 12\. Support for
nested data structures. Support for arrays as data types. 13\. Support for
restrictions on query complexity, along with quotas. 14\. Data replication and
support for data integrity on replicas. Let's look at some of these features
in detail.

### 1\. True column-oriented DBMS.

In a true column-oriented DBMS, there isn't any "garbage" stored with the
values. For example, constant-length values must be supported, to avoid
storing their length "number" next to the values. As an example, a billion
UInt8-type values should actually consume around 1 GB uncompressed, or this
will strongly affect the CPU use. It is very important to store data compactly
(without any "garbage") even when uncompressed, since the speed of
decompression (CPU usage) depends mainly on the volume of uncompressed data.
This is worth noting because there are systems that can store values of
separate columns separately, but that can't effectively process analytical
queries due to their optimization for other scenarios. Example are HBase,
BigTable, Cassandra, and HyperTable. In these systems, you will get throughput
around a hundred thousand rows per second, but not hundreds of millions of
rows per second. Also note that ClickHouse is a DBMS, not a single database.
ClickHouse allows creating tables and databases in runtime, loading data, and
running queries without reconfiguring and restarting the server.

### 2\. Data compression.

Some column-oriented DBMSs (InfiniDB CE and MonetDB) do not use data
compression. However, data compression really improves performance.

### 3\. Disk storage of data.

Many column-oriented DBMSs (SAP HANA, and Google PowerDrill) can only work in
RAM. But even on thousands of servers, the RAM is too small for storing all
the pageviews and sessions in Yandex.Metrica.

### 4\. Parallel processing on multiple cores.

Large queries are parallelized in a natural way.

### 5\. Distributed processing on multiple servers.

Almost none of the columnar DBMSs listed above have support for distributed
processing. In ClickHouse, data can reside on different shards. Each shard can
be a group of replicas that are used for fault tolerance. The query is
processed on all the shards in parallel. This is transparent for the user.

### 6\. SQL support.

If you are familiar with standard SQL, we can't really talk about SQL support.
NULLs are not supported. All the functions have different names. However, this
is a declarative query language based on SQL that can't be differentiated from
SQL in many instances. JOINs are supported. Subqueries are supported in FROM,
IN, JOIN clauses; and scalar subqueries. Correllated subqueries are not
supported.

### 7\. Vector engine.

Data is not only stored by columns, but is processed by vectors - parts of
columns. This allows us to achieve high CPU performance.

### 8\. Real-time data updates.

ClickHouse supports primary key tables. In order to quickly perform queries on
the range of the primary key, the data is sorted incrementally using the merge
tree. Due to this, data can continually be added to the table. There is no
locking when adding data.

### 9\. Indexes.

Having a primary key allows, for example, extracting data for specific clients
(Metrica counters) for a specific time range, with low latency less than
several dozen milliseconds.

### 10\. Suitable for online queries.

This lets us use the system as the back-end for a web interface. Low latency
means queries can be processed without delay, while the Yandex.Metrica
interface page is loading (in online mode).

### 11\. Support for approximated calculations.

1\. The system contains aggregate functions for approximated calculation of
the number of various values, medians, and quantiles. 2\. Supports running a
query based on a part (sample) of data and getting an approximated result. In
this case, proportionally less data is retrieved from the disk. 3\. Supports
running an aggregation for a limited number of random keys, instead of for all
keys. Under certain conditions for key distribution in the data, this provides
a reasonably accurate result while using fewer resources.

### 14\. Data replication and support for data integrity on replicas.

Uses asynchronous multimaster replication. After being written to any
available replica, data is distributed to all the remaining replicas. The
system maintains identical data on different replicas. Data is restored
automatically after a failure, or using a "button" for complex cases. For more
information, see the section "Data replication". ==ClickHouse features that
can be considered disadvantages== 1\. No transactions. 2\. For aggregation,
query results must fit in the RAM on a single server. However, the volume of
source data for a query may be indefinitely large. 3\. Lack of full-fledged
UPDATE/DELETE implementation. ==The Yandex.Metrica task== We need to get
custom reports based on hits and sessions, with custom segments set by the
user. Data for the reports is updated in real-time. Queries must be run
immediately (in online mode). We must be able to build reports for any time
period. Complex aggregates must be calculated, such as the number of unique
visitors. At this time (April 2014), Yandex.Metrica receives approximately 12
billion events (pageviews and mouse clicks) daily. All these events must be
stored in order to build custom reports. A single query may require scanning
hundreds of millions of rows over a few seconds, or millions of rows in no
more than a few hundred milliseconds. ===Aggregated and non-aggregated data===
There is a popular opinion that in order to effectively calculate statistics,
you must aggregate data, since this reduces the volume of data. But data
aggregation is a very limited solution, for the following reasons: \- You must
have a pre-defined list of reports the user will need. The user can't make
custom reports. \- When aggregating a large quantity of keys, the volume of
data is not reduced, and aggregation is useless. \- For a large number of
reports, there are too many aggregation variations (combinatorial explosion).
\- When aggregating keys with high cardinality (such as URLs), the volume of
data is not reduced by much (less than twofold). For this reason, the volume
of data with aggregation might grow instead of shrink. \- Users will not view
all the reports we calculate for them. A large portion of calculations are
useless. \- The logical integrity of data may be violated for various
aggregations. If we do not aggregate anything and work with non-aggregated
data, this might actually reduce the volume of calculations. However, with
aggregation, a significant part of the work is taken offline and completed
relatively calmly. In contrast, online calculations require calculating as
fast as possible, since the user is waiting for the result. Yandex.Metrica has
a specialized system for aggregating data called Metrage, which is used for
the majority of reports. Starting in 2009, Yandex.Metrica also used a
specialized OLAP database for non-aggregated data called OLAPServer, which was
previously used for the report builder. OLAPServer worked well for non-
aggregated data, but it had many restrictions that did not allow it to be used
for all reports as desired. These included the lack of support for data types
(only numbers), and the inability to incrementally update data in real-time
(it could only be done by rewriting data daily). OLAPServer is not a DBMS, but
a specialized DB. To remove the limitations of OLAPServer and solve the
problem of working with non-aggregated data for all reports, we developed the
ClickHouse DBMS. ==Usage in Yandex.Metrica and other Yandex services==
ClickHouse is used for multiple purposes in Yandex.Metrica. Its main task is
to build reports in online mode using non-aggregated data. It uses a cluster
of 374 servers, which store over 8 trillion rows (more than a quadrillion
values) in the database. The volume of compressed data, without counting
duplication and replication, is about 800 TB. The volume of uncompressed data
(in TSV format) would be approximately 7 PB. ClickHouse is also used for: \-
Storing WebVisor data. \- Processing intermediate data. \- Building global
reports with Analytics. \- Running queries for debugging the Metrica engine.
\- Analyzing logs from the API and the user interface. ClickHouse has at least
a dozen installations in other Yandex services: in search verticals, Market,
Direct, business analytics, mobile development, AdFox, personal services, and
others. ==Possible counterparts== There are no analogs to ClickHouse
available. At this time (May 2016), there aren't any available open-source and
free systems that have all the features listed above. However, these features
are absolutely necessary for Yandex.Metrica. ==Possible silly questions==

### 1\. Why not to use systems like map-reduce?

Systems like map-reduce are distributed computing systems, where the reduce
phase is performed using distributed sorting. Regading this aspect, map-reduce
is similar to other systems like YAMR, [Hadoop](http://hadoop.apache.org/),
[YT](https://yandexdataschool.ru/about/conference/program/babenko). These
systems are not suitable for online queries because of latency, So they can't
be used in backend-level for web interface. Systems like this also are not
suitable for realtime updates. Distributed sorting is not optimal solution for
reduce operations, if the result of the operation and all intermediate
results, shall they exist, fit in operational memory of a single server, as
usually happens in case of online analytical queries. In this case the optimal
way to perform reduce operations is by using a hash-table. A common
optimization method for map-reduce tasks is combine operation (partial reduce)
which uses hash-tables in memory. This optimization is done by the user
manually. Distributed sorting is the main reason for long latencies of simple
map-reduce jobs. Systems similar to map-reduce enable running any code on the
cluster. But for OLAP use-cases declerative query languages are better suited
as they allow to carry out investigations faster. For example, for Hadoop
there are [Hive](https://hive.apache.org/) and [Pig](https://pig.apache.org/).
There are others: [Cloudera Impala](http://impala.io/), [Shark
(depricated)](http://shark.cs.berkeley.edu/) and [Spark
SQL](http://spark.apache.org/sql/) for [Spark](http://spark.apache.org/),
[Presto](https://prestodb.io/), [Apache Drill](https://drill.apache.org/).
However, performance of such tasks is highly sub-optimal compared to the
performance of specialized systems and relatively high latency does not allow
the use of these systems as a backend for the web interface. YT allows you to
store separate groups of columns. But YT is not a truly columnar storage
system, as the system has no fixed length data types (so you can efficiently
store a number without "garbage"), and there is no vector engine. Tasks in YT
are performed by arbitrary code in streaming mode, so can not be suficiently
optimized (up to hundreds of millions of lines per second per server). In
2014-2016 YT is to develop "dynamic table sorting" functionality using Merge
Tree, strongly typed values ​​and SQL-like language support. Dynamicly sorted
tables are not suited for OLAP tasks, since the data is stored in rows. Query
language development in YT is still in incubating phase, which does not allow
it to focus on this functionality. YT developers are considering dynamicly
sorted tables for use in OLTP and Key-Value scenarios. ==Performance==
According to internal testing results, ClickHouse shows the best performance
for comparable operating scenarios among systems of its class that were
available for testing. This includes the highest throughput for long queries,
and the lowest latency on short queries. Testing results are shown [on this
page](benchmark.html). ===Throughput for a single large query=== Throughput
can be measured in rows per second or in megabytes per second. If the data is
placed in the page cache, a query that is not too complex is processed on
modern hardware at a speed of approximately 2-10 GB/s of uncompressed data on
a single server (for the simplest cases, the speed may reach 30 GB/s). If data
is not placed in the page cache, the speed depends on the disk subsystem and
the data compression rate. For example, if the disk subsystem allows reading
data at 400 MB/s, and the data compression rate is 3, the speed will be around
1.2 GB/s. To get the speed in rows per second, divide the speed in bytes per
second by the total size of the columns used in the query. For example, if 10
bytes of columns are extracted, the speed will be around 100-200 million rows
per second. The processing speed increases almost linearly for distributed
processing, but only if the number of rows resulting from aggregation or
sorting is not too large. ===Latency when processing short queries.=== If a
query uses a primary key and does not select too many rows to process
(hundreds of thousands), and does not use too many columns, we can expect less
than 50 milliseconds of latency (single digits of milliseconds in the best
case) if data is placed in the page cache. Otherwise, latency is calculated
from the number of seeks. If you use rotating drives, for a system that is not
overloaded, the latency is calculated by this formula: seek time (10 ms) *
number of columns queried * number of data parts. ===Throughput when
processing a large quantity of short queries.=== Under the same conditions,
ClickHouse can handle several hundred queries per second on a single server
(up to several thousand in the best case). Since this scenario is not typical
for analytical DBMSs, we recommend expecting a maximum of 100 queries per
second. ===Performance on data insertion.=== We recommend inserting data in
packets of at least 1000 rows, or no more than a single request per second.
When inserting to a MergeTree table from a tab-separated dump, the insertion
speed will be from 50 to 200 MB/s. If the inserted rows are around 1 Kb in
size, the speed will be from 50,000 to 200,000 rows per second. If the rows
are small, the performance will be higher in rows per second (on Yandex Banner
System data -> 500,000 rows per second, on Graphite data -> 1,000,000 rows per
second). To improve performance, you can make multiple INSERT queries in
parallel, and performance will increase linearly.

# Getting started

==System requirements== This is not a cross-platform system. It requires Linux
Ubuntu Precise (12.04) or newer, x86_64 architecture with SSE 4.2 instruction
set. To test for SSE 4.2 support, do %%grep -q sse4_2 /proc/cpuinfo && echo
"SSE 4.2 supported" || echo "SSE 4.2 not supported"%% We recommend using
Ubuntu Trusty or Ubuntu Xenial or Ubuntu Precise. The terminal must use UTF-8
encoding (the default in Ubuntu). ==Installation== For testing and
development, the system can be installed on a single server or on a desktop
computer. ===Installing from packages=== In %%/etc/apt/sources.list%% (or in a
separate %%/etc/apt/sources.list.d/clickhouse.list%% file), add the
repository: On Ubuntu Trusty (14.04): %% deb
http://repo.yandex.ru/clickhouse/trusty stable main %% For other Ubuntu
versions, replace %%trusty%% to %%xenial%% or %%precise%%. Then run: %% sudo
apt-key adv --keyserver keyserver.ubuntu.com --recv E0C56BD4 # optional sudo
apt-get update sudo apt-get install clickhouse-client clickhouse-server-common
%% You can also download and install packages manually from here:
<http://repo.yandex.ru/clickhouse/trusty/pool/main/c/clickhouse/>, [http://rep
o.yandex.ru/clickhouse/xenial/pool/main/c/clickhouse/](http://repo.yandex.ru/c
lickhouse/precise/pool/main/c/clickhouse/)
<http://repo.yandex.ru/clickhouse/precise/pool/main/c/clickhouse/>. ClickHouse
contains access restriction settings. They are located in the 'users.xml' file
(next to 'config.xml'). By default, access is allowed from everywhere for the
default user without a password. See 'user/default/networks'. For more
information, see the section "Configuration files". ===Installing from
source=== Build following the instructions in
[build.md](https://github.com/yandex/ClickHouse/blob/master/doc/build.md) You
can compile packages and install them. You can also use programs without
installing packages. Client: src/dbms/src/Client/ Server: src/dbms/src/Server/
For the server, create a catalog with data, such as: %%
/opt/clickhouse/data/default/ /opt/clickhouse/metadata/default/ %% (Configured
in the server config.) Run 'chown' for the desired user. Note the path to logs
in the server config (src/dbms/src/Server/config.xml). ===Other methods of
installation=== The Docker image is located here:
<https://hub.docker.com/r/yandex/clickhouse-server/> There is Gentoo overlay
located here: <https://github.com/kmeaw/clickhouse-overlay> ===Launch=== To
start the server (as a daemon), run:

    
    
    sudo service clickhouse-server start
    

View the logs in the catalog %% /var/log/clickhouse-server/ %% If the server
doesn't start, check the configurations in the file %% /etc/clickhouse-
server/config.xml %% You can also launch the server from the console:

    
    
    clickhouse-server --config-file=/etc/clickhouse-server/config.xml
    

In this case, the log will be printed to the console, which is convenient
during development. If the configuration file is in the current directory, you
don't need to specify the '\--config-file' parameter. By default, it uses
'./config.xml'. You can use the command-line client to connect to the server:

    
    
    clickhouse-client
    

The default parameters indicate connecting with localhost:9000 on behalf of
the user 'default' without a password. The client can be used for connecting
to a remote server. For example:

    
    
    clickhouse-client --host=example.com
    

For more information, see the section "Command-line client". Checking the
system:

    
    
    milovidov@milovidov-Latitude-E6320:~/work/metrica/src/dbms/src/Client$ ./clickhouse-client
    ClickHouse client version 0.0.18749.
    Connecting to localhost:9000.
    Connected to ClickHouse server version 0.0.18749.
    
    :) SELECT 1
    
    _SELECT_ 1
    
    ┌─_1_─┐
    │ 1 │
    └───┘
    
    1 rows in set. Elapsed: 0.003 sec.
    
    :)
    

Congratulations, it works! ==Test data== If you are Yandex employee, you can
use Yandex.Metrica test data to explore the system's capabilities. You can
find instructions for using the test data [here](https://github.yandex-
team.ru/Metrika/ClickHouse_private/tree/master/tests). Otherwise, you could
use one of available public datasets, described
[here](https://github.com/yandex/ClickHouse/tree/master/doc/example_datasets).
==If you have questions== If you are Yandex employee, use internal ClickHouse
maillist. You can subscribe to this list to get announcements, information on
new developments, and questions that other users have. Otherwise, you could
ask questions on [Stackoverflow](https://stackoverflow.com/); discuss in
[Google Groups](https://groups.google.com/group/clickhouse); or send private
message to developers to address [clickhouse-feedback@yandex-team.com](mailto
:clickhouse-feedback@yandex-team.com).

# Interfaces

To explore the system's capabilities, download data to tables, or make manual
queries, use the clickhouse-client program. ==HTTP interface== The HTTP
interface lets you use ClickHouse on any platform from any programming
language. We use it for working from Java and Perl, as well as shell scripts.
In other departments, the HTTP interface is used from Perl, Python, and Go.
The HTTP interface is more limited than the native interface, but it has
better compatibility. By default, clickhouse-server listens for HTTP on port
8123 (this can be changed in the config). If you make a GET / request without
parameters, it returns the string "Ok" (with a line break at the end). You can
use this in health-check scripts.

    
    
    $ curl 'http://localhost:8123/'
    Ok.
    

Send the request as a URL 'query' parameter, or as a POST. Or send the
beginning of the request in the 'query' parameter, and the rest in the POST
(we'll explain later why this is necessary). If successful, you receive the
200 response code and the result in the response body. If an error occurs, you
receive the 500 response code and an error description text in the response
body. When using the GET method, 'readonly' is set. In other words, for
queries that modify data, you can only use the POST method. You can send the
query itself either in the POST body, or in the URL parameter. Examples:

    
    
    $ curl 'http://localhost:8123/?query=SELECT%201'
    1
    
    $ wget -O- -q 'http://localhost:8123/?query=SELECT 1'
    1
    
    $ GET 'http://localhost:8123/?query=SELECT 1'
    1
    
    $ echo -ne 'GET /?query=SELECT%201 HTTP/1.0\r\n\r\n' | nc localhost 8123
    HTTP/1.0 200 OK
    Connection: Close
    Date: Fri, 16 Nov 2012 19:21:50 GMT
    
    1
    

As you can see, curl is not very convenient because spaces have to be URL-
escaped. Although wget escapes everything on its own, we don't recommend it
because it doesn't work well over HTTP 1.1 when using keep-alive and Transfer-
Encoding: chunked.

    
    
    $ echo 'SELECT 1' | curl 'http://localhost:8123/' --data-binary @-
    1
    
    $ echo 'SELECT 1' | curl 'http://localhost:8123/?query=' --data-binary @-
    1
    
    $ echo '1' | curl 'http://localhost:8123/?query=SELECT' --data-binary @-
    1
    

If part of the query is sent in the parameter, and part in the POST, a line
break is inserted between these two data parts. Example (this won't work):

    
    
    $ echo 'ECT 1' | curl 'http://localhost:8123/?query=SEL' --data-binary @-
    Code: 59, e.displayText() = DB::Exception: Syntax error: failed at position 0: SEL
    ECT 1
    , expected One of: SHOW TABLES, SHOW DATABASES, SELECT, INSERT, CREATE, ATTACH, RENAME, DROP, DETACH, USE, SET, OPTIMIZE., e.what() = DB::Exception
    

By default, data is returned in TabSeparated format (for more information, see
the "Formats" section). You use the FORMAT clause of the query to request any
other format.

    
    
    $ echo 'SELECT 1 FORMAT Pretty' | curl 'http://localhost:8123/?' --data-binary @-
    ┏━━━┓
    ┃ 1 ┃
    ┡━━━┩
    │ 1 │
    └───┘
    

The POST method of transmitting data is necessary for INSERT queries. In this
case, you can write the beginning of the query in the URL parameter, and use
POST to pass the data to insert. The data to insert could be, for example, a
tab-separated dump from MySQL. In this way, the INSERT query replaces LOAD
DATA LOCAL INFILE from MySQL. Examples: Creating a table:

    
    
    echo 'CREATE TABLE t (a UInt8) ENGINE = Memory' | POST 'http://localhost:8123/'
    

Using the familiar INSERT query for data insertion:

    
    
    echo 'INSERT INTO t VALUES (1),(2),(3)' | POST 'http://localhost:8123/'
    

Data can be sent separately from the query:

    
    
    echo '(4),(5),(6)' | POST 'http://localhost:8123/?query=INSERT INTO t VALUES'
    

You can specify any data format. The 'Values' format is the same as what is
used when writing INSERT INTO t VALUES:

    
    
    echo '(7),(8),(9)' | POST 'http://localhost:8123/?query=INSERT INTO t FORMAT Values'
    

To insert data from a tab-separated dump, specify the corresponding format:

    
    
    echo -ne '10\n11\n12\n' | POST 'http://localhost:8123/?query=INSERT INTO t FORMAT TabSeparated'
    

Reading the table contents. Data is output in random order due to parallel
query processing:

    
    
    $ GET 'http://localhost:8123/?query=SELECT a FROM t'
    7
    8
    9
    10
    11
    12
    1
    2
    3
    4
    5
    6
    

Deleting the table.

    
    
    POST 'http://localhost:8123/?query=DROP TABLE t'
    

For successful requests that don't return a data table, an empty response body
is returned. You can use compression when transmitting data. The compressed
data has a non-standard format, and you will need to use a special compressor
program to work with it (%%sudo apt-get install compressor-metrika-yandex%%).
If you specified 'compress=1' in the URL, the server will compress the data it
sends you. If you specified 'decompress=1' in the URL, the server will
decompress the same data that you pass in the POST method. You can use this to
reduce network traffic when transmitting a large amount of data, or for
creating dumps that are immediately compressed. You can use the 'database' URL
parameter to specify the default database.

    
    
    $ echo 'SELECT number FROM numbers LIMIT 10' | curl 'http://localhost:8123/?database=system' --data-binary @-
    0
    1
    2
    3
    4
    5
    6
    7
    8
    9
    

By default, the database that is registered in the server settings is used as
the default database. By default, this is the database called 'default'.
Alternatively, you can always specify the database using a dot before the
table name. The username and password can be indicated in one of two ways: 1\.
Using HTTP Basic Authentication. Example:

    
    
    echo 'SELECT 1' | curl 'http://user:password@localhost:8123/' -d @-
    

2\. In the 'user' and 'password' URL parameters. Example:

    
    
    echo 'SELECT 1' | curl 'http://localhost:8123/?user=user&password=password' -d @-
    

If the user name is not indicated, the username 'default' is used. If the
password is not indicated, an empty password is used. You can also use the URL
parameters to specify any settings for processing a single query, or entire
profiles of settings. Example: %%http://localhost:8123/?profile=web&max_rows_t
o_read=1000000000&query=SELECT+1%% For more information, see the section
"Settings".

    
    
    $ echo 'SELECT number FROM system.numbers LIMIT 10' | curl 'http://localhost:8123/?' --data-binary @-
    0
    1
    2
    3
    4
    5
    6
    7
    8
    9
    

For information about other parameters, see the section "SET". In contrast to
the native interface, the HTTP interface does not support the concept of
sessions or session settings, does not allow aborting a query (to be exact, it
allows this in only a few cases), and does not show the progress of query
processing. Parsing and data formatting are performed on the server side, and
using the network might be ineffective. The optional 'query_id' parameter can
be passed as the query ID (any string). For more information, see the section
"Settings, replace_running_query". The optional 'quota_key' parameter can be
passed as the quota key (any string). For more information, see the section
"Quotas". The HTTP interface allows passing external data (external temporary
tables) for querying. For more information, see the section "External data for
query processing". ==JDBC driver== There is official JDBC driver for
ClickHouse. See [here](https://github.com/yandex/clickhouse-jdbc). ==Third-
party client libraries== There exist third-party client libraries for
[Python](https://github.com/Infinidat/infi.clickhouse_orm), PHP
([1](https://github.com/8bitov/clickhouse-php-client),
[2](https://github.com/SevaCode/PhpClickHouseClient),
[3](https://github.com/smi2/phpClickHouse)), [Go](https://github.com/roistat
/go-clickhouse), Node.js ([1](https://github.com/TimonKK/clickhouse),
[2](https://github.com/apla/node-clickhouse)),
[Perl](https://github.com/elcamlost/perl-DBD-ClickHouse). Libraries was not
tested by us. Ordering is arbitrary. ==Third-party GUI== There exist
minimalistic web UI for ClickHouse by [SMI2](https://github.com/smi2
/clickhouse-frontend). ==Native interface (TCP)== The native interface is used
in the "clickhouse-client" command-line client for interaction between servers
with distributed query processing, and also in C++ programs. We will only
cover the command-line client. ==Command-line client==

    
    
    $ clickhouse-client
    ClickHouse client version 0.0.26176.
    Connecting to localhost:9000.
    Connected to ClickHouse server version 0.0.26176.
    
    :) SELECT 1
    

The "clickhouse-client" program accepts the following parameters, which are
all optional: \--host, -h - server name, by default - 'localhost'. You can use
either the name or the IPv4 or IPv6 address. \--port - The port to connect to,
by default - '9000'. Note that the HTTP interface and the native interface use
different ports. \--user, -u - The username, by default - 'default'.
\--password - The password, by default - empty string. \--query, -q - Query to
process when using non-interactive mode. \--database, -d - Select the current
default database, by default - the current DB from the server settings (by
default, the 'default' DB). \--multiline, -m - If specified, allow multiline
queries (do not send request on Enter). \--multiquery, -n - If specified,
allow processing multiple queries separated by semicolons. Only works in non-
interactive mode. \--format, -f - Use the specified default format to output
the result. \--vertical, -E - If specified, use the Vertical format by default
to output the result. This is the same as '\--format=Vertical'. In this
format, each value is printed on a separate line, which is helpful when
displaying wide tables. \--time, -t - If specified, print the query execution
time to 'stderr' in non-interactive mode. \--stacktrace - If specified, also
prints the stack trace if an exception occurs. \--config-file - Name of the
configuration file that has additional settings or changed defaults for the
settings listed above. By default, files are searched for in this order:
./clickhouse-client.xml ~/./clickhouse-client/config.xml /etc/clickhouse-
client/config.xml Settings are only taken from the first file found. You can
also specify any settings that will be used for processing queries. For
example, %%clickhouse-client --max_threads=1%%. For more information, see the
section "Settings". The client can be used in interactive and non-interactive
(batch) mode. To use batch mode, specify the 'query' parameter, or send data
to 'stdin' (it verifies that 'stdin' is not a terminal), or both. Similar to
the HTTP interface, when using the 'query' parameter and sending data to
'stdin', the request is a concatenation of the 'query' parameter, a line
break, and the data in 'stdin'. This is convenient for large INSERT queries.
Examples for insert data via clickhouse-client: %% echo -ne "1, 'some text',
'2016-08-14 00:00:00'\n2, 'some more text', '2016-08-14 00:00:01'" |
clickhouse-client --database=test --query="INSERT INTO test FORMAT CSV"; cat
<<_EOF | clickhouse-client --database=test --query="INSERT INTO test FORMAT
CSV"; 3, 'some text', '2016-08-14 00:00:00' 4, 'some more text', '2016-08-14
00:00:01' _EOF cat file.csv | clickhouse-client --database=test
--query="INSERT INTO test FORMAT CSV"; %% In batch mode, the default data
format is TabSeparated. You can set the format in the FORMAT clause of the
query. By default, you can only process a single query in batch mode. To make
multiple queries from a "script," use the 'multiquery' parameter. This works
for all queries except INSERT. Query results are output consecutively without
additional separators. Similarly, to process a large number of queries, you
can run 'clickhouse-client' for each query. Note that it may take tens of
milliseconds to launch the 'clickhouse-client' program. In interactive mode,
you get a command line where you can enter queries. If 'multiline' is not
specified (the default): To run a query, press Enter. The semicolon is not
necessary at the end of the query. To enter a multiline query, enter a
backslash %%\%% before the line break - after you press Enter, you will be
asked to enter the next line of the query. If 'multiline' is specified: To run
a query, end it with a semicolon and press Enter. If the semicolon was omitted
at the end of the entered line, you will be asked to enter the next line of
the query. Only a single query is run, so everything after the semicolon is
ignored. You can specify %%\G%% instead of or after the semicolon. This
indicates using Vertical format. In this format, each value is printed on a
separate line, which is convenient for wide tables. This unusual feature was
added for compatibility with the MySQL CLI. The command line is based on
'readline' (and 'history') (or 'libedit', or even nothing, depending on
build). In other words, it uses the familiar keyboard shortcuts and keeps a
history. The history is written to /.clickhouse-client-history. By default,
the format used is PrettyCompact. You can change the format in the FORMAT
clause of the query, or by specifying '\G' at the end of the query, using the
'\--format' or '\--vertical' argument in the command line, or using the client
configuration file. To exit the client, press Ctrl+D (or Ctrl+C), or enter one
of the following : "exit", "quit", "logout", "учше", "йгше", "дщпщге",
"exit;", "quit;", "logout;", "учшеж", "йгшеж", "дщпщгеж", "q", "й", "\q",
"\Q", ":q", "\й", "\Й", "Жй" When processing a query, the client shows: 1\.
Progress, which is updated no more than 10 times per second (by default). For
quick queries, the progress might not have time to be displayed. 2\. The
formatted query after parsing, for debugging. 3\. The result in the specified
format. 4\. The number of lines in the result, the time passed, and the
average speed of query processing. To cancel a lengthy query, press Ctrl+C.
However, you will still need to wait a little for the server to abort the
request. It is not possible to cancel a query at certain stages. If you don't
wait and press Ctrl+C a second time, the client will exit. The command-line
client allows passing external data (external temporary tables) for querying.
For more information, see the section "External data for request processing".

# Query language

==Syntax== There are two types of parsers in the system: a full SQL parser (a
recursive descent parser), and a data format parser (a fast stream parser). In
all cases except the INSERT query, only the full SQL parser is used. The
INSERT query uses both parsers: %%INSERT INTO t VALUES (1, 'Hello, world'),
(2, 'abc'), (3, 'def')%% The %%INSERT INTO t VALUES%% fragment is parsed by
the full parser, and the data %%(1, 'Hello, world'), (2, 'abc'), (3, 'def')%%
is parsed by the fast stream parser. Data can have any format. When a query is
received, the server calculates no more than 'max_query_size' bytes of the
request in RAM (by default, 1 MB), and the rest is stream parsed. This means
the system doesn't have problems with large INSERT queries, like MySQL does.
When using the Values format in an INSERT query, it may seem that data is
parsed the same as expressions in a SELECT query, but this is not true. The
Values format is much more limited. Next we will cover the full parser. For
more information about format parsers, see the section "Formats". ===Spaces===
There may be any number of space symbols between syntactical constructions
(including the beginning and end of a query). Space symbols include the space,
tab, line break, CR, and form feed. ===Comments=== SQL-style and C-style
comments are supported. SQL-style comments: from %%--%% to the end of the
line. The space after %%--%% can be omitted. C-style comments: from %%/*%% to
%%*/%%. These comments can be multiline. Spaces are not required here, either.
===Keywords=== Keywords (such as SELECT) are not case-sensitive. Everything
else (column names, functions, and so on), in contrast to standard SQL, is
case-sensitive. Keywords are not reserved (they are just parsed as keywords in
the corresponding context). ===Identifiers=== Identifiers (column names,
functions, and data types) can be quoted or non-quoted. Non-quoted identifiers
start with a Latin letter or underscore, and continue with a Latin letter,
underscore, or number. In other words, they must match the regex %%^[a-zA-
Z_][0-9a-zA-Z_]*$%%. Examples: %%x%%, %%_1%%, %%X_y__Z123_%%. Quoted
identifiers are placed in reversed quotation marks %%`id`%% (the same as in
MySQL), and can indicate any set of bytes (non-empty). In addition, symbols
(for example, the reverse quotation mark) inside this type of identifier can
be backslash-escaped. Escaping rules are the same as for string literals (see
below). We recommend using identifiers that do not need to be quoted.
===Literals=== There are numeric literals, string literals, and compound
literals.

#### Numeric literals

A numeric literal tries to be parsed: \- first as a 64-bit signed number,
using the 'strtoull' function. \- if unsuccessful, as a 64-bit unsigned
number, using the 'strtoll' function. \- if unsuccessful, as a floating-point
number using the 'strtod' function. \- otherwise, an error is returned. The
corresponding value will have the smallest type that the value fits in. For
example, 1 is parsed as UInt8, but 256 is parsed as UInt16. For more
information, see "Data types". Examples: %%1%%, %%18446744073709551615%%,
%%0xDEADBEEF%%, %%01%%, %%0.1%%, %%1e100%%, %%-1e-100%%, %%inf%%, %%nan%%.

#### String literals

Only string literals in single quotes are supported. The enclosed characters
can be backslash-escaped. The following escape sequences have special
meanings: %%\b%%, %%\f%%, %%\r%%, %%\n%%, %%\t%%, %%\0%%, %%\a%%, %%\v%%,
\x_HH_. In all other cases, escape sequences like \_c_, where _c_ is any
character, are transformed to _c_. This means that the sequences %%\'%% and
%%\\\%% can be used. The value will have the String type. Minimum set of
symbols that must be escaped in string literal is %%'%% and %%\%%.

#### Compound literals

Constructions are supported for arrays: %%[1, 2, 3]%% and tuples: %%(1,
'Hello, world!', 2)%%. Actually, these are not literals, but expressions with
the array creation operator and the tuple creation operator, respectively. For
more information, see the section "Operators2". An array must consist of at
least one item, and a tuple must have at least two items. Tuples have a
special purpose for use in the IN clause of a SELECT query. Tuples can be
obtained as the result of a query, but they can't be saved to a database (with
the exception of Memory-type tables). ===Functions=== Functions are written
like an identifier with a list of arguments (possibly empty) in brackets. In
contrast to standard SQL, the brackets are required, even for an empty
arguments list. Example: %%now()%%. There are regular and aggregate functions
(see the section "Aggregate functions"). Some aggregate functions can contain
two lists of arguments in brackets. Example: %%quantile(0.9)(x)%%. These
aggregate functions are called "parametric" functions, and the arguments in
the first list are called "parameters". The syntax of aggregate functions
without parameters is the same as for regular functions. ===Operators===
Operators are converted to their corresponding functions during query parsing,
taking their priority and associativity into account. For example, the
expression %%1 + 2 * 3 + 4%% is transformed to %%plus(plus(1, multiply(2, 3)),
4)%%. For more information, see the section "Operators2" below. ===Data types
and database table engines=== Data types and table engines in the CREATE query
are written the same way as identifiers or functions. In other words, they may
or may not contain an arguments list in brackets. For more information, see
the sections "Data types," "Table engines," and "CREATE". ===Synonyms=== In
the SELECT query, expressions can specify synonyms using the AS keyword. Any
expression is placed to the left of AS. The identifier name for the synonym is
placed to the right of AS. As opposed to standard SQL, synonyms are not only
declared on the top level of expressions: %%SELECT (1 AS n) + 2, n%% In
contrast to standard SQL, synonyms can be used in all parts of a query, not
just SELECT. ===Asterisk=== In a SELECT query, an asterisk can replace the
expression. For more information, see the section "SELECT". ===Expressions===
An expression is a function, identifier, literal, application of an operator,
expression in brackets, subquery, or asterisk. It can also contain a synonym.
A list of expressions is one or more expressions separated by commas.
Functions and operators, in turn, can have expressions as arguments.
==Queries== ===CREATE DATABASE=== %%CREATE DATABASE [IF NOT EXISTS] db_name%%
\- Creates the 'db_name' database. A database is just a directory for tables.
If "IF NOT EXISTS" is included, the query won't return an error if the
database already exists. ===CREATE TABLE=== The CREATE TABLE query can have
several forms. %%CREATE [TEMPORARY] TABLE [IF NOT EXISTS] [db.]name ( name1
[type1] [DEFAULT|MATERIALIZED|ALIAS expr1], name2 [type2]
[DEFAULT|MATERIALIZED|ALIAS expr2], ... ) ENGINE = engine%% Creates a table
named 'name' in the 'db' database or the current database if 'db' is not set,
with the structure specified in brackets and the 'engine' engine. The
structure of the table is a list of column descriptions. If indexes are
supported by the engine, they are indicated as parameters for the table
engine. A column description is %%name type%% in the simplest case. For
example: %%RegionID UInt32%%. Expressions can also be defined for default
values (see below). %%CREATE [TEMPORARY] TABLE [IF NOT EXISTS] [db.]name AS
[db2.]name2 [ENGINE = engine]%% Creates a table with the same structure as
another table. You can specify a different engine for the table. If the engine
is not specified, the same engine will be used as for the 'db2.name2' table.
%%CREATE [TEMPORARY] TABLE [IF NOT EXISTS] [db.]name ENGINE = engine AS SELECT
...%% Creates a table with a structure like the result of the SELECT query,
with the 'engine' engine, and fills it with data from SELECT. In all cases, if
IF NOT EXISTS is specified, the query won't return an error if the table
already exists. In this case, the query won't do anything.

#### Default values

The column description can specify an expression for a default value, in one
of the following ways: %%DEFAULT expr%%, %%MATERIALIZED expr%%, %%ALIAS
expr%%. Example: %%URLDomain String DEFAULT domain(URL)%%. If an expression
for the default value is not defined, the default values will be set to zeros
for numbers, empty strings for strings, empty arrays for arrays, and
0000-00-00 for dates or 0000-00-00 00:00:00 for dates with time. NULLs are not
supported. If the default expression is defined, the column type is optional.
If there isn't an explicitly defined type, the default expression type is
used. Example: %%EventDate DEFAULT toDate(EventTime)%% - the 'Date' type will
be used for the 'EventDate' column. If the data type and default expression
are defined explicitly, this expression will be cast to the specified type
using type casting functions. Example: %%Hits UInt32 DEFAULT 0%% means the
same thing as %%Hits UInt32 DEFAULT toUInt32(0)%%. Default expressions may be
defined as an arbitrary expression from table constants and columns. When
creating and changing the table structure, it checks that expressions don't
contain loops. For INSERT, it checks that expressions are resolvable - that
all columns they can be calculated from have been passed. %%DEFAULT expr%%
Normal default value. If the INSERT query doesn't specify the corresponding
column, it will be filled in by computing the corresponding expression.
%%MATERIALIZED expr%% Materialized expression. Such a column can't be
specified for INSERT, because it is always calculated. For an INSERT without a
list of columns, these columns are not considered. In addition, this column is
not substituted when using an asterisk in a SELECT query. This is to preserve
the invariant that the dump obtained using SELECT * can be inserted back into
the table using INSERT without specifying the list of columns. %%ALIAS expr%%
Synonym. Such a column isn't stored in the table at all. Its values can't be
inserted in a table, and it is not substituted when using an asterisk in a
SELECT query. It can be used in SELECTs if the alias is expanded during query
parsing. When using the ALTER query to add new columns, old data for these
columns is not written. Instead, when reading old data that does not have
values for the new columns, expressions are computed on the fly by default.
However, if running the expressions requires different columns that are not
indicated in the query, these columns will additionally be read, but only for
the blocks of data that need it. If you add a new column to a table but later
change its default expression, the values used for old data will change (for
data where values were not stored on the disk). Note that when running
background merges, data for columns that are missing in one of the merging
parts is written to the merged part. It is not possible to set default values
for elements in nested data structures.

#### Temporary tables

In all cases, if TEMPORARY is specified, a temporary table will be created.
Temporary tables have the following characteristics: \- Temporary tables
disappear when the session ends, including if the connection is lost. \- A
temporary table is created with the Memory engine. The other table engines are
not supported. \- The DB can't be specified for a temporary table. It is
created outside of databases. \- If a temporary table has the same name as
another one and a query specifies the table name without specifying the DB,
the temporary table will be used. \- For distributed query processing,
temporary tables used in a query are passed to remote servers. In most cases,
temporary tables are not created manually, but when using external data for a
query, or for distributed (GLOBAL) IN. For more information, see the
appropriate sections. ===CREATE VIEW=== %%CREATE [MATERIALIZED] VIEW [IF NOT
EXISTS] [db.]name [ENGINE = engine] [POPULATE] AS SELECT ...%% Creates a view.
There are two types of views: normal and MATERIALIZED. Normal views don't
store any data, but just perform a read from another table. In other words, a
normal view is nothing more than a saved query. When reading from a view, this
saved query is used as a subquery in the FROM clause. As an example, assume
you've created a view: %%CREATE VIEW view AS SELECT ...%% and written a query:
%%SELECT a, b, c FROM view%% This query is fully equivalent to using the
subquery: %%SELECT a, b, c FROM (SELECT ...)%% Materialized views store data
transformed by the corresponding SELECT query. When creating a materialized
view, you can specify ENGINE - the table engine for storing data. By default,
it uses the same engine as for the table that the SELECT query is made from. A
materialized view is arranged as follows: when inserting data to the table
specified in SELECT, part of the inserted data is converted by this SELECT
query, and the result is inserted in the view. If you specify POPULATE, the
existing table data is inserted in the view when creating it, as if making a
CREATE TABLE ... AS SELECT ... query. Otherwise, the query contains only the
data inserted in the table after creating the view. We don't recommend using
POPULATE, since data inserted in the table during the view creation will not
be inserted in it. The SELECT query can contain DISTINCT, GROUP BY, ORDER BY,
LIMIT ... Note that the corresponding conversions are performed independently
on each block of inserted data. For example, if GROUP BY is set, data is
aggregated during insertion, but only within a single packet of inserted data.
The data won't be further aggregated. The exception is when using an ENGINE
that independently performs data aggregation, such as SummingMergeTree. The
execution of ALTER queries on materialized views has not been fully developed,
so they might be inconvenient. Views look the same as normal tables. For
example, they are listed in the result of the SHOW TABLES query. There isn't a
separate query for deleting views. To delete a view, use DROP TABLE.
===ATTACH=== The query is exactly the same as CREATE, except \- The word
ATTACH is used instead of CREATE. \- The query doesn't create data on the
disk, but assumes that data is already in the appropriate places, and just
adds information about the table to the server. After executing an ATTACH
query, the server will know about the existence of the table. This query is
used when starting the server. The server stores table metadata as files with
ATTACH queries, which it simply runs at launch (with the exception of system
tables, which are explicitly created on the server). ===DROP=== This query has
two types: DROP DATABASE and DROP TABLE. %%DROP DATABASE [IF EXISTS] db%%
Deletes all tables inside the 'db' database, then deletes the 'db' database
itself. If IF EXISTS is specified, it doesn't return an error if the database
doesn't exist. %%DROP TABLE [IF EXISTS] [db.]name%% Deletes the table. If IF
EXISTS is specified, it doesn't return an error if the table doesn't exist or
the database doesn't exist. ===DETACH=== %%DETACH TABLE [IF EXISTS]
[db.]name%% Deletes information about the table from the server. The server
stops knowing about the table's existence. This does not delete the table's
data or metadata. On the next server launch, the server will read the metadata
and find out about the table again. Similarly, a "detached" table can be re-
attached using the ATTACH query (with the exception of system tables, which do
not have metadata stored for them). There is no DETACH DATABASE query.
===RENAME=== %%RENAME TABLE [db11.]name11 TO [db12.]name12, [db21.]name21 TO
[db22.]name22, ...%% Renames one or more tables. All tables are renamed under
global locking. Renaming tables is a light operation. If you indicated another
database after TO, the table will be moved to this database. However, the
directories with databases must reside in the same file system (otherwise, an
error is returned). ===ALTER=== The ALTER query is only supported for
*MergeTree type tables, as well as for Merge and Distributed types. The query
has several variations.

#### Column manipulations

%%ALTER TABLE [db].name ADD|DROP|MODIFY COLUMN ...%% Lets you change the table
structure. In the query, specify a list of one or more comma-separated
actions. Each action is an operation on a column. The following actions are
supported: %%ADD COLUMN name [type] [default_expr] [AFTER name_after]%% Adds a
new column to the table with the specified name, type, and default expression
(see the section "Default expressions"). If you specify 'AFTER name_after'
(the name of another column), the column is added after the specified one in
the list of table columns. Otherwise, the column is added to the end of the
table. Note that there is no way to add a column to the beginning of a table.
For a chain of actions, 'name_after' can be the name of a column that is added
in one of the previous actions. Adding a column just changes the table
structure, without performing any actions with data. The data doesn't appear
on the disk after ALTER. If the data is missing for a column when reading from
the table, it is filled in with default values (by performing the default
expression if there is one, or using zeros or empty strings). The column
appears on the disk after merging data parts (see MergeTree). This approach
allows us to complete the ALTER query instantly, without increasing the volume
of old data. %%DROP COLUMN name%% Deletes the column with the name 'name'.
Deletes data from the file system. Since this deletes entire files, the query
is completed almost instantly. %%MODIFY COLUMN name [type] [default_expr]%%
Changes the 'name' column's type to 'type' and/or the default expression to
'default_expr'. When changing the type, values are converted as if the
'to_Type_' function were applied to them. If only the default expression is
changed, the query doesn't do anything complex, and is completed almost
instantly. Changing the column type is the only complex action - it changes
the contents of files with data. For large tables, this may take a long time.
There are several stages of execution: \- Preparing temporary (new) files with
modified data. \- Renaming old files. \- Renaming the temporary (new) files to
the old names. \- Deleting the old files. Only the first stage takes time. If
there is a failure at this stage, the data is not changed. If there is a
failure during one of the successive stages, data can be restored manually.
The exception is if the old files were deleted from the file system but the
data for the new files did not get written to the disk and was lost. There is
no support for changing the column type in arrays and nested data structures.
The ALTER query lets you create and delete separate elements (columns) in
nested data structures, but not whole nested data structures. To add a nested
data structure, you can add columns with a name like 'name.nested_name' and
the type 'Array(_T_)'. A nested data structure is equivalent to multiple array
columns with a name that has the same prefix before the dot. There is no
support for deleting or changing the type for columns in the primary key or
the sampling key (columns that are in the ENGINE expression). If the ALTER
query is not sufficient for making the table changes you need, you can create
a new table, copy the data to it using the INSERT SELECT query, then switch
the tables using the RENAME query and delete the old table. The ALTER query
blocks all reads and writes for the table. In other words, if a long SELECT is
running at the time of the ALTER query, the ALTER query will wait for the
SELECT to complete. At the same time, all new queries to the same table will
wait while this ALTER is running. For tables that don't store data themselves
(Merge and Distributed), ALTER just changes the table structure, and does not
change the structure of subordinate tables. For example, when running ALTER
for a Distributed table, you will also need to run ALTER for the tables on all
remote servers. The ALTER query for changing columns is replicated. The
instructions are saved in ZooKeeper, then each replica applies them. All ALTER
queries are run in the same order. The query waits for the appropriate actions
to be completed on the other replicas. However, a query to change columns in a
replicated table can be interrupted, and all actions will be performed
asynchronously.

#### Manipulations with partitions and parts

Only works for tables in the MergeTree family. The following operations are
available: %%DETACH PARTITION%% - Move a partition to the 'detached' directory
and forget it. %%DROP PARTITION%% - Delete a partition. %%ATTACH
PART|PARTITION%% - Add a new part or partition from the 'detached' directory
to the table. %%FREEZE PARTITION%% - Create a backup of a partition. %%FETCH
PARTITION%% - Download a partition from another server. Each type of query is
covered separately below. A partition in a table is data for a single calendar
month. This is determined by the values of the date key specified in the table
engine parameters. Each month's data is stored separately in order to simplify
manipulations with this data. A "part" in the table is part of the data from a
single partition, sorted by the primary key. You can use the system.parts
table to view the set of table parts and partitions: %%SELECT * FROM
system.parts WHERE active%% active - Only count active parts. Inactive parts
are, for example, source parts remaining after merging to a larger part -
these parts are deleted approximately 10 minutes after merging. Another way to
view a set of parts and partitions is to go into the directory with table
data. The directory with data is /opt/clickhouse/data/_database_/_table_/,
where /opt/clickhouse/ is the path to ClickHouse data, 'database' is the
database name, and 'table' is the table name. Example: %% $ ls -l
/opt/clickhouse/data/test/visits/ total 48 drwxrwxrwx 2 metrika metrika 20480
may 13 02:58 20140317_20140323_2_2_0 drwxrwxrwx 2 metrika metrika 20480 may 13
02:58 20140317_20140323_4_4_0 drwxrwxrwx 2 metrika metrika 4096 may 13 02:55
detached -rw-rw-rw- 1 metrika metrika 2 may 13 02:58 increment.txt %% Here
20140317_20140323_2_2_0 and 20140317_20140323_4_4_0 are directories of parts.
Let's look at the name of the first part: 20140317_20140323_2_2_0. 20140317 -
minimum date of part data 20140323 - maximum date of part data 2 - minimum
number of the data block 2 - maximum number of the data block 0 - part level -
depth of the merge tree that formed it Each part corresponds to a single
partition and contains data for a single month. 201403 - The partition name. A
partition is a set of parts for a single month. On an operating server, you
can't manually change the set of parts or their data on the file system, since
the server won't know about it. For non-replicated tables, you can do this
when the server is stopped, but we don't recommended it. For replicated
tables, the set of parts can't be changed in any case. The 'detached'
directory contains parts that are not used by the server - detached from the
table using the ALTER ... DETACH query. Parts that are damaged are also moved
to this directory, instead of deleting them. You can add, delete, or modify
the data in the 'detached' directory at any time - the server won't know about
this until you make the ALTER TABLE ... ATTACH query. For replicated tables,
there is also an 'unreplicated' directory. This directory can hold the portion
of table data that does not participate in replication. This data is present
only if the table was converted from a non-replicated one (see the section
"Conversion from MergeTree to ReplicatedMergeTree"). %%ALTER TABLE [db.]table
DETACH [UNREPLICATED] PARTITION 'name'%% Move all data for partitions named
'name' to the 'detached' directory and forget about them. The partition name
is specified in YYYYMM format. It can be indicated in single quotes or without
them. After the query is executed, you can do whatever you want with the data
in the 'detached' directory — delete it from the file system, or just leave
it. If UNREPLICATED is specified (this only works for replicatable tables),
the query is run for the unreplicated part of a replicatable table. The query
is replicated - data will be moved to the 'detached' directory and forgotten
on all replicas. The query can only be sent to a leader replica. To find out
if a replica is a leader, perform SELECT to the 'system.replicas' system
table. Alternatively, it is easier to make a query on all replicas, and all
except one will throw an exception. %%ALTER TABLE [db.]table DROP
[UNREPLICATED] PARTITION 'name'%% Similar to the DETACH operation. Deletes
data from the table. Data parts will be tagged as inactive and will be
completely deleted in approximately 10 minutes. The query is replicated - data
will be deleted on all replicas. %%ALTER TABLE [db.]table ATTACH
[UNREPLICATED] PARTITION|PART 'name'%% Adds data to the table from the
'detached' directory. Or, if UNREPLICATED is specified, data is moved from an
'unreplicated' part to replicated data. It is possible to add data for an
entire partition or a separate part. For a part, specify the full name of the
part in single quotes. The query is replicated. Each replica checks whether
there is data in the 'detached' directory. If there is data, it checks the
integrity, verifies that it matches the data on the server that initiated the
query, and then adds it if everything is correct. If not, it downloads data
from the query requestor replica, or from another replica where the data has
already been added. So you can put data in the 'detached' directory on one
replica, and use the ALTER ... ATTACH query to add it to the table on all
replicas. %%ALTER TABLE [db.]table FREEZE PARTITION 'name'%% Creates a local
backup of one or multiple partitions. The name can be the full name of the
partition (for example, 201403), or its prefix (for example, 2014) - then the
backup will be created for all the corresponding partitions. The query does
the following: for a data snapshot at the time of execution, it creates
hardlinks to table data in the directory /opt/clickhouse/shadow/N/...
/opt/clickhouse/ is the working ClickHouse directory from the config. N is the
incremental number of the backup. The same structure of directories is created
inside the backup as inside /opt/clickhouse/. It also performs 'chmod' for all
files, forbidding writes to them. The backup is created almost instantly (but
first it waits for current queries to the corresponding table to finish
running). At first, the backup doesn't take any space on the disk. As the
system works, the backup can take disk space, as data is modified. If the
backup is made for old enough data, it won't take space on the disk. After
creating the backup, data from /opt/clickhouse/shadow/ can be copied to the
remote server and then deleted on the local server. The entire backup process
is performed without stopping the server. The ALTER ... FREEZE PARTITION query
is not replicated. A local backup is only created on the local server. As an
alternative, you can manually copy data from the
/opt/clickhouse/data/database/table directory. But if you do this while the
server is running, race conditions are possible when copying directories with
files being added or changed, and the backup may be inconsistent. You can do
this if the server isn't running - then the resulting data will be the same as
after the ALTER TABLE t FREEZE PARTITION query. ALTER TABLE ... FREEZE
PARTITION only copies data, not table metadata. To make a backup of table
metadata, copy the file /opt/clickhouse/metadata/database/table.sql To restore
from a backup: \- Use the CREATE query to create the table if it doesn't
exist. The query can be taken from an .sql file (replace ATTACH in it with
CREATE). \- Copy data from the data/database/table/ directory inside the
backup to the /opt/clickhouse/data/database/table/detached/ directory. \- Run
ALTER TABLE ... ATTACH PARTITION YYYYMM queries where YYYYMM is the month, for
every month. In this way, data from the backup will be added to the table.
Restoring from a backup doesn't require stopping the server. **Backups and
replication** Replication provides protection from device failures. If all
data disappeared on one of your replicas, follow the instructions in the
"Restoration after failure" section to restore it. For protection from device
failures, you must use replication. For more information about replication,
see the section "Data replication". Backups protect against human error
(accidentally deleting data, deleting the wrong data or in the wrong cluster,
or corrupting data). For high-volume databases, it can be difficult to copy
backups to remote servers. In such cases, to protect from human error, you can
keep a backup on the same server (it will reside in /opt/clickhouse/shadow/).
%%ALTER TABLE [db.]table FETCH PARTITION 'name' FROM 'path-in-zookeeper'%%
This query only works for replicatable tables. It downloads the specified
partition from the shard that has its ZooKeeper path specified in the FROM
clause, then puts it in the 'detached' directory for the specified table.
Although the query is called ALTER TABLE, it does not change the table
structure, and does not immediately change the data available in the table.
Data is placed in the 'detached' directory. You can use the ALTER TABLE ...
ATTACH query to attach the data. The path to ZooKeeper is specified in the
FROM clause. For example, %%/clickhouse/tables/01-01/visits%%. Before
downloading, the system checks that the partition exists and the table
structure matches. The most appropriate replica is selected automatically from
the healthy replicas. The ALTER ... FETCH PARTITION query is not replicated.
The partition will be downloaded to the 'detached' directory only on the local
server. Note that if after this you use the ALTER TABLE ... ATTACH query to
add data to the table, the data will be added on all replicas (on one of the
replicas it will be added from the 'detached' directory, and on the rest it
will be loaded from neighboring replicas).

#### Synchronicity of ALTER queries

For non-replicatable tables, all ALTER queries are performed synchronously.
For replicatable tables, the query just adds instructions for the appropriate
actions to ZooKeeper, and the actions themselves are performed as soon as
possible. However, the query can wait for these actions to be completed on all
the replicas. For ALTER ... ATTACH|DETACH|DROP queries, you can use the
'replication_alter_partitions_sync' setting to set up waiting. Possible
values: 0 - do not wait, 1 - wait for own completion (default), 2 - wait for
all. ===SHOW DATABASES=== %%SHOW DATABASES [FORMAT format]%% Prints a list of
all databases. This query is identical to the query SELECT name FROM
system.databases [FORMAT format] See the section "Formats". ===SHOW TABLES===
%%SHOW TABLES [FROM db] [LIKE 'pattern'] [FORMAT format]%% Outputs a list of
\- tables from the current database, or from the 'db' database if "FROM db" is
specified. \- all tables, or tables whose name matches the pattern, if "LIKE
'pattern'" is specified. The query is identical to the query SELECT name FROM
system.tables WHERE database = 'db' [AND name LIKE 'pattern'] [FORMAT format]
See the section "LIKE operator". ===SHOW PROCESSLIST=== %%SHOW PROCESSLIST
[FORMAT format]%% Outputs a list of queries currently being processed, other
than SHOW PROCESSLIST queries. Prints a table containing the columns: **user**
is the user who made the query. Keep in mind that for distributed processing,
queries are sent to remote servers under the 'default' user. SHOW PROCESSLIST
shows the username for a specific query, not for a query that this query
initiated. **address** is the name of the host that the query was sent from.
For distributed processing, on remote servers, this is the name of the query
requestor host. To track where a distributed query was originally made from,
look at SHOW PROCESSLIST on the query requestor server. **elapsed** \- The
execution time, in seconds. Queries are output in order of decreasing
execution time. **rows_read**, **bytes_read** \- How many rows and bytes of
uncompressed data were read when processing the query. For distributed
processing, data is totaled from all the remote servers. This is the data used
for restrictions and quotas. **memory_usage** \- Current RAM usage in bytes.
See the setting 'max_memory_usage'. **query** \- The query itself. In INSERT
queries, the data for insertion is not output. **query_id** \- The query
identifier. Non-empty only if it was explicitly defined by the user. For
distributed processing, the query ID is not passed to remote servers. This
query is exactly the same as: SELECT * FROM system.processes [FORMAT format].
Tip (execute in the console): %%watch -n1 "clickhouse-client --query='SHOW
PROCESSLIST'"%% ===SHOW CREATE TABLE=== %%SHOW CREATE TABLE [db.]table [FORMAT
format]%% Returns a single String-type 'statement' column, which contains a
single value - the CREATE query used for creating the specified table.
===DESCRIBE TABLE=== %%DESC|DESCRIBE TABLE [db.]table [FORMAT format]%%
Returns two String-type columns: 'name' and 'type', which indicate the names
and types of columns in the specified table. Nested data structures are output
in "expanded" format. Each column is shown separately, with the name after a
dot. ===EXISTS=== %%EXISTS TABLE [db.]name [FORMAT format]%% Returns a single
UInt8-type column, which contains the single value 0 if the table or database
doesn't exist, or 1 if the table exists in the specified database. ===USE===
%%USE db%% Lets you set the current database for the session. The current
database is used for searching for tables if the database is not explicitly
defined in the query with a dot before the table name. This query can't be
made when using the HTTP protocol, since there is no concept of a session.
===SET=== %%SET [GLOBAL] param = value%% Lets you set the 'param' setting to
'value'. You can also make all the settings from the specified settings
profile in a single query. To do this, specify 'profile' as the setting name.
For more information, see the section "Settings". The setting is made for the
session, or for the server (globally) if GLOBAL is specified. When making a
global setting, the setting is not applied to sessions already running,
including the current session. It will only be used for new sessions. Settings
made using SET GLOBAL have a lower priority compared with settings made in the
config file in the user profile. In other words, user settings can't be
overridden by SET GLOBAL. When the server is restarted, global settings made
using SET GLOBAL are lost. To make settings that persist after a server
restart, you can only use the server's config file. (This can't be done using
a SET query.) ===OPTIMIZE=== %%OPTIMIZE TABLE [db.]name%% Asks the table
engine to do something for optimization. Supported only by *MergeTree engines,
in which this query initializes a non-scheduled merge of data parts. For
replicated tables, the OPTIMIZE query is only applied to non-replicated data
(this data exists only after converting non-replicated tables to replicated
ones). If there isn't any, the query doesn't do anything. ===INSERT=== This
query has several variations. %%INSERT INTO [db.]table [(c1, c2, c3)] VALUES
(v11, v12, v13), (v21, v22, v23), ...%% Inserts rows with the listed values in
the 'table' table. This query is exactly the same as: %%INSERT INTO [db.]table
[(c1, c2, c3)] FORMAT Values (v11, v12, v13), (v21, v22, v23), ...%% %%INSERT
INTO [db.]table [(c1, c2, c3)] FORMAT format ...%% Inserts data in any
specified format. The data itself comes after 'format', after all space
symbols up to the first line break if there is one and including it, or after
all space symbols if there isn't a line break. We recommend writing data
starting from the next line (this is important if the data starts with space
characters). Example: %%INSERT INTO t FORMAT TabSeparated 11 Hello, world! 22
Qwerty %% For more information about data formats, see the section "Formats".
The "Interfaces" section describes how to insert data separately from the
query when using the command-line client or the HTTP interface. The query may
optionally specify a list of columns for insertion. In this case, the default
values are written to the other columns. Default values are calculated from
DEFAULT expressions specified in table definitions, or, if the DEFAULT is not
explicitly defined, zeros and empty strings are used. If the
'strict_insert_default' setting is set to 1, all the columns that do not have
explicit DEFAULTS must be specified in the query. %%INSERT INTO [db.]table
[(c1, c2, c3)] SELECT ...%% Inserts the result of the SELECT query into a
table. The names and data types of the SELECT result must exactly match the
table structure that data is inserted into, or the specified list of columns.
To change column names, use synonyms (AS) in the SELECT query. To change data
types, use type conversion functions (see the section "Functions"). None of
the data formats allows using expressions as values. In other words, you can't
write INSERT INTO t VALUES (now(), 1 + 1, DEFAULT). There is no support for
other data part modification queries: UPDATE, DELETE, REPLACE, MERGE, UPSERT,
INSERT UPDATE. However, you can delete old data using ALTER TABLE ... DROP
PARTITION. ===SELECT=== His Highness, the SELECT query. %%SELECT [DISTINCT]
expr_list [FROM [db.]table | (subquery) | table_function] [FINAL] [SAMPLE
sample_coeff] [ARRAY JOIN ...] [GLOBAL] ANY|ALL INNER|LEFT JOIN
(subquery)|table USING columns_list [PREWHERE expr] [WHERE expr] [GROUP BY
expr_list] [WITH TOTALS] [HAVING expr] [ORDER BY expr_list] [LIMIT [n, ]m]
[UNION ALL ...] [FORMAT format]%% All the clauses are optional, except for the
required list of expressions immediately after SELECT. The clauses below are
described in almost the same order as in the query execution conveyor. If the
query omits the DISTINCT, GROUP BY, and ORDER BY clauses and the IN and JOIN
subqueries, the query will be completely stream processed, using O(1) amount
of RAM. Otherwise, the query may consume too much RAM, if appropriate
restrictions are not defined (max_memory_usage, max_rows_to_group_by,
max_rows_to_sort, max_rows_in_distinct, max_bytes_in_distinct,
max_rows_in_set, max_bytes_in_set, max_rows_in_join, max_bytes_in_join,
max_bytes_before_external_sort). For more information, see the section
"Settings". It is possible to use external sorting (saving temporary tables to
a disk). The system does not have external aggregation or merge join.

#### FROM clause

If the FROM clause is omitted, data will be read from the 'system.one' table.
The 'system.one' table contains exactly one row (this table fulfills the same
purpose as the DUAL table found in other DBMSs). The FROM clause specifies the
table to read data from, or a subquery, or a table function; ARRAY JOIN and
the regular JOIN may also be included (see below). Instead of a table, the
SELECT subquery may be specified in brackets. In this case, the subquery
processing pipeline will be built into the processing pipeline of an external
query. In contrast to standard SQL, a synonym does not need to be specified
after a subquery. For compatibility, it is possible to write 'AS name' after a
subquery, but the specified name isn't used anywhere. A table function may be
specified instead of a table. For more information, see the section "Table
functions". To execute a query, all the columns listed in the query are
extracted from the appropriate table. Any columns not needed for the external
query are thrown out of the subqueries. If a query does not list any columns
(for example, SELECT count() FROM t), some column is extracted from the table
anyway (the smallest one is preferred), in order to calculate the number of
rows. The FINAL modifier can be used only for a SELECT from a
CollapsingMergeTree table. When you specify FINAL, data is selected fully
"collapsed". Keep in mind that using FINAL leads to a selection that includes
columns related to the primary key, in addition to the columns specified in
the SELECT. Additionally, the query will be executed in a single stream, and
data will be merged during query execution. This means that when using FINAL,
the query is processed more slowly. In most cases, you should avoid using
FINAL. For more information, see the section "CollapsingMergeTree engine".

#### SAMPLE clause

The SAMPLE clause allows for approximated query processing. Approximated query
processing is only supported by MergeTree* type tables, and only if the
sampling expression was specified during table creation (see the section
"MergeTree engine"). SAMPLE has the format %%SAMPLE k%%, where 'k' is a
decimal number from 0 to 1, or %%SAMPLE n%%, where 'n' is a sufficiently large
integer. In the first case, the query will be executed on 'k' percent of data.
For example, %%SAMPLE 0.1%% runs the query on 10% of data. In the second case,
the query will be executed on a sample of no more than 'n' rows. For example,
%%SAMPLE 10000000%% runs the query on a maximum of 10,000,000 rows. Example:
%%SELECT Title, count() * 10 AS PageViews FROM hits_distributed SAMPLE 0.1
WHERE CounterID = 34 AND toDate(EventDate) >= toDate('2013-01-29') AND
toDate(EventDate) <= toDate('2013-02-04') AND NOT DontCountHits AND NOT
Refresh AND Title != '' GROUP BY Title ORDER BY PageViews DESC LIMIT 1000%% In
this example, the query is executed on a sample from 0.1 (10%) of data. Values
of aggregate functions are not corrected automatically, so to get an
approximate result, the value 'count()' is manually multiplied by 10. When
using something like %%SAMPLE 10000000%%, there isn't any information about
which relative percent of data was processed or what the aggregate functions
should be multiplied by, so this method of writing is not always appropriate
to the situation. A sample with a relative coefficient is "consistent": if we
look at all possible data that could be in the table, a sample (when using a
single sampling expression specified during table creation) with the same
coefficient always selects the same subset of possible data. In other words, a
sample from different tables on different servers at different times is made
the same way. For example, a sample of user IDs takes rows with the same
subset of all the possible user IDs from different tables. This allows using
the sample in subqueries in the IN clause, as well as for manually correlating
results of different queries with samples.

#### ARRAY JOIN clause

Allows executing JOIN with an array or nested data structure. The intent is
similar to the 'arrayJoin' function, but its functionality is broader. ARRAY
JOIN is essentially INNER JOIN with an array. Example: %% :) CREATE TABLE
arrays_test (s String, arr Array(UInt8)) ENGINE = Memory CREATE TABLE
arrays_test ( s String, arr Array(UInt8) ) ENGINE = Memory Ok. 0 rows in set.
Elapsed: 0.001 sec. :) INSERT INTO arrays_test VALUES ('Hello', [1,2]),
('World', [3,4,5]), ('Goodbye', []) INSERT INTO arrays_test VALUES Ok. 3 rows
in set. Elapsed: 0.001 sec. :) SELECT * FROM arrays_test SELECT * FROM
arrays_test ┌─s───────┬─arr─────┐ │ Hello │ [1,2] │ │ World │ [3,4,5] │ │
Goodbye │ [] │ └─────────┴─────────┘ 3 rows in set. Elapsed: 0.001 sec. :)
SELECT s, arr FROM arrays_test ARRAY JOIN arr SELECT s, arr FROM arrays_test
ARRAY JOIN arr ┌─s─────┬─arr─┐ │ Hello │ 1 │ │ Hello │ 2 │ │ World │ 3 │ │
World │ 4 │ │ World │ 5 │ └───────┴─────┘ 5 rows in set. Elapsed: 0.001 sec.
%% An alias can be specified for an array in the ARRAY JOIN clause. In this
case, an array item can be accessed by this alias, but the array itself by the
original name. Example: %% :) SELECT s, arr, a FROM arrays_test ARRAY JOIN arr
AS a SELECT s, arr, a FROM arrays_test ARRAY JOIN arr AS a
┌─s─────┬─arr─────┬─a─┐ │ Hello │ [1,2] │ 1 │ │ Hello │ [1,2] │ 2 │ │ World │
[3,4,5] │ 3 │ │ World │ [3,4,5] │ 4 │ │ World │ [3,4,5] │ 5 │
└───────┴─────────┴───┘ 5 rows in set. Elapsed: 0.001 sec. %% Multiple arrays
of the same size can be comma-separated in the ARRAY JOIN clause. In this
case, JOIN is performed with them simultaneously (the direct sum, not the
direct product). Example: %% :) SELECT s, arr, a, num, mapped FROM arrays_test
ARRAY JOIN arr AS a, arrayEnumerate(arr) AS num, arrayMap(x -> x + 1, arr) AS
mapped SELECT s, arr, a, num, mapped FROM arrays_test ARRAY JOIN arr AS a,
arrayEnumerate(arr) AS num, arrayMap(lambda(tuple(x), plus(x, 1)), arr) AS
mapped ┌─s─────┬─arr─────┬─a─┬─num─┬─mapped─┐ │ Hello │ [1,2] │ 1 │ 1 │ 2 │ │
Hello │ [1,2] │ 2 │ 2 │ 3 │ │ World │ [3,4,5] │ 3 │ 1 │ 4 │ │ World │ [3,4,5]
│ 4 │ 2 │ 5 │ │ World │ [3,4,5] │ 5 │ 3 │ 6 │
└───────┴─────────┴───┴─────┴────────┘ 5 rows in set. Elapsed: 0.002 sec. :)
SELECT s, arr, a, num, arrayEnumerate(arr) FROM arrays_test ARRAY JOIN arr AS
a, arrayEnumerate(arr) AS num SELECT s, arr, a, num, arrayEnumerate(arr) FROM
arrays_test ARRAY JOIN arr AS a, arrayEnumerate(arr) AS num
┌─s─────┬─arr─────┬─a─┬─num─┬─arrayEnumerate(arr)─┐ │ Hello │ [1,2] │ 1 │ 1 │
[1,2] │ │ Hello │ [1,2] │ 2 │ 2 │ [1,2] │ │ World │ [3,4,5] │ 3 │ 1 │ [1,2,3]
│ │ World │ [3,4,5] │ 4 │ 2 │ [1,2,3] │ │ World │ [3,4,5] │ 5 │ 3 │ [1,2,3] │
└───────┴─────────┴───┴─────┴─────────────────────┘ 5 rows in set. Elapsed:
0.002 sec. %% ARRAY JOIN also works with nested data structures. Example: %%
:) CREATE TABLE nested_test (s String, nest Nested(x UInt8, y UInt32)) ENGINE
= Memory CREATE TABLE nested_test ( s String, nest Nested( x UInt8, y UInt32)
) ENGINE = Memory Ok. 0 rows in set. Elapsed: 0.006 sec. :) INSERT INTO
nested_test VALUES ('Hello', [1,2], [10,20]), ('World', [3,4,5], [30,40,50]),
('Goodbye', [], []) INSERT INTO nested_test VALUES Ok. 3 rows in set. Elapsed:
0.001 sec. :) SELECT * FROM nested_test SELECT * FROM nested_test
┌─s───────┬─nest.x──┬─nest.y─────┐ │ Hello │ [1,2] │ [10,20] │ │ World │
[3,4,5] │ [30,40,50] │ │ Goodbye │ [] │ [] │
└─────────┴─────────┴────────────┘ 3 rows in set. Elapsed: 0.001 sec. :)
SELECT s, nest.x, nest.y FROM nested_test ARRAY JOIN nest SELECT s, `nest.x`,
`nest.y` FROM nested_test ARRAY JOIN nest ┌─s─────┬─nest.x─┬─nest.y─┐ │ Hello
│ 1 │ 10 │ │ Hello │ 2 │ 20 │ │ World │ 3 │ 30 │ │ World │ 4 │ 40 │ │ World │
5 │ 50 │ └───────┴────────┴────────┘ 5 rows in set. Elapsed: 0.001 sec. %%
When specifying names of nested data structures in ARRAY JOIN, the meaning is
the same as ARRAY JOIN with all the array elements that it consists of.
Example: %% :) SELECT s, nest.x, nest.y FROM nested_test ARRAY JOIN nest.x,
nest.y SELECT s, `nest.x`, `nest.y` FROM nested_test ARRAY JOIN `nest.x`,
`nest.y` ┌─s─────┬─nest.x─┬─nest.y─┐ │ Hello │ 1 │ 10 │ │ Hello │ 2 │ 20 │ │
World │ 3 │ 30 │ │ World │ 4 │ 40 │ │ World │ 5 │ 50 │
└───────┴────────┴────────┘ 5 rows in set. Elapsed: 0.001 sec. %% This
variation also makes sense: %% :) SELECT s, nest.x, nest.y FROM nested_test
ARRAY JOIN nest.x SELECT s, `nest.x`, `nest.y` FROM nested_test ARRAY JOIN
`nest.x` ┌─s─────┬─nest.x─┬─nest.y─────┐ │ Hello │ 1 │ [10,20] │ │ Hello │ 2 │
[10,20] │ │ World │ 3 │ [30,40,50] │ │ World │ 4 │ [30,40,50] │ │ World │ 5 │
[30,40,50] │ └───────┴────────┴────────────┘ 5 rows in set. Elapsed: 0.001
sec. %% An alias may be used for a nested data structure, in order to select
either the JOIN result or the source array. Example: %% :) SELECT s, n.x, n.y,
nest.x, nest.y FROM nested_test ARRAY JOIN nest AS n SELECT s, `n.x`, `n.y`,
`nest.x`, `nest.y` FROM nested_test ARRAY JOIN nest AS n
┌─s─────┬─n.x─┬─n.y─┬─nest.x──┬─nest.y─────┐ │ Hello │ 1 │ 10 │ [1,2] │
[10,20] │ │ Hello │ 2 │ 20 │ [1,2] │ [10,20] │ │ World │ 3 │ 30 │ [3,4,5] │
[30,40,50] │ │ World │ 4 │ 40 │ [3,4,5] │ [30,40,50] │ │ World │ 5 │ 50 │
[3,4,5] │ [30,40,50] │ └───────┴─────┴─────┴─────────┴────────────┘ 5 rows in
set. Elapsed: 0.001 sec. %% Example of using the arrayEnumerate function: %%
:) SELECT s, n.x, n.y, nest.x, nest.y, num FROM nested_test ARRAY JOIN nest AS
n, arrayEnumerate(nest.x) AS num SELECT s, `n.x`, `n.y`, `nest.x`, `nest.y`,
num FROM nested_test ARRAY JOIN nest AS n, arrayEnumerate(`nest.x`) AS num
┌─s─────┬─n.x─┬─n.y─┬─nest.x──┬─nest.y─────┬─num─┐ │ Hello │ 1 │ 10 │ [1,2] │
[10,20] │ 1 │ │ Hello │ 2 │ 20 │ [1,2] │ [10,20] │ 2 │ │ World │ 3 │ 30 │
[3,4,5] │ [30,40,50] │ 1 │ │ World │ 4 │ 40 │ [3,4,5] │ [30,40,50] │ 2 │ │
World │ 5 │ 50 │ [3,4,5] │ [30,40,50] │ 3 │
└───────┴─────┴─────┴─────────┴────────────┴─────┘ 5 rows in set. Elapsed:
0.002 sec. %% The query can only specify a single ARRAY JOIN clause. The
corresponding conversion can be performed before the WHERE/PREWHERE clause (if
its result is needed in this clause), or after completing WHERE/PREWHERE (to
reduce the volume of calculations).

#### JOIN clause

The normal JOIN, which is not related to ARRAY JOIN described above. %%
[GLOBAL] ANY|ALL INNER|LEFT [OUTER] JOIN (subquery)|table USING columns_list
%% Performs joins with data from the subquery. At the beginning of query
execution, the subquery specified after JOIN is run, and its result is saved
in memory. Then it is read from the "left" table specified in the FROM clause,
and while it is being read, for each of the read rows from the "left" table,
rows are selected from the subquery results table (the "right" table) that
meet the condition for matching the values of the columns specified in USING.
The table name can be specified instead of a subquery. This is equivalent to
the 'SELECT * FROM table' subquery, except in a special case when the table
has the Join engine - an array prepared for joining. All columns that are not
needed for the JOIN are deleted from the subquery. There are several types of
JOINs: INNER or LEFT - the type: If INNER is specified, the result will
contain only those rows that have a matching row in the right table. If LEFT
is specified, any rows in the left table that don't have matching rows in the
right table will be assigned the default value - zeros or empty rows. LEFT
OUTER may be written instead of LEFT; the word OUTER does not affect anything.
ANY or ALL - strictness: If ANY is specified and there are multiple matching
rows in the right table, only the first one will be joined. If ALL is
specified and there are multiple matching rows in the right table, the data
will be multiplied by the number of these rows. Using ALL corresponds to the
normal JOIN semantic from standard SQL. Using ANY is optimal. If the right
table has only one matching row, the results of ANY and ALL are the same. You
must specify either ANY or ALL (neither of them is selected by default).
GLOBAL - distribution: When using a normal %%JOIN%%, the query is sent to
remote servers. Subqueries are run on each of them in order to make the right
table, and the join is performed with this table. In other words, the right
table is formed on each server separately. When using %%GLOBAL ... JOIN%%,
first the requestor server runs a subquery to calculate the right table. This
temporary table is passed to each remote server, and queries are run on them
using the temporary data that was transmitted. Be careful when using GLOBAL
JOINs. For more information, see the section "Distributed subqueries" below.
Any combination of JOINs is possible. For example, %%GLOBAL ANY LEFT OUTER
JOIN%%. When running JOINs, there is no optimization of the order of execution
in relation to other stages of the query. The join (a search in the right
table) is run before filtering in WHERE and before aggregation. In order to
explicitly set the order of execution, we recommend running a JOIN subquery
with a subquery. Example: %% SELECT CounterID, hits, visits FROM ( SELECT
CounterID, count() AS hits FROM test.hits GROUP BY CounterID ) ANY LEFT JOIN (
SELECT CounterID, sum(Sign) AS visits FROM test.visits GROUP BY CounterID )
USING CounterID ORDER BY hits DESC LIMIT 10 ┌─CounterID─┬───hits─┬─visits─┐ │
1143050 │ 523264 │ 13665 │ │ 731962 │ 475698 │ 102716 │ │ 722545 │ 337212 │
108187 │ │ 722889 │ 252197 │ 10547 │ │ 2237260 │ 196036 │ 9522 │ │ 23057320 │
147211 │ 7689 │ │ 722818 │ 90109 │ 17847 │ │ 48221 │ 85379 │ 4652 │ │ 19762435
│ 77807 │ 7026 │ │ 722884 │ 77492 │ 11056 │ └───────────┴────────┴────────┘ %%
Subqueries don't allow you to set names or use them for referencing a column
from a specific subquery. The columns specified in USING must have the same
names in both subqueries, and the other columns must be named differently. You
can use aliases to change the names of columns in subqueries (the example uses
the aliases 'hits' and 'visits'). The USING clause specifies one or more
columns to join, which establishes the equality of these columns. The list of
columns is set without brackets. More complex join conditions are not
supported. The right table (the subquery result) resides in RAM. If there
isn't enough memory, you can't run a JOIN. Only one JOIN can be specified in a
query (on a single level). To run multiple JOINs, you can put them in
subqueries. Each time a query is run with the same JOIN, the subquery is run
again - the result is not cached. To avoid this, use the special 'Join' table
engine, which is a prepared array for joining that is always in RAM. For more
information, see the section "Table engines, Join". In some cases, it is more
efficient to use IN instead of JOIN. Among the various types of JOINs, the
most efficient is ANY LEFT JOIN, then ANY INNER JOIN. The least efficient are
ALL LEFT JOIN and ALL INNER JOIN. If you need a JOIN for joining with
dimension tables (these are relatively small tables that contain dimension
properties, such as names for advertising campaigns), a JOIN might not be very
convenient due to the bulky syntax and the fact that the right table is re-
accessed for every query. For such cases, there is an "external dictionaries"
feature that you should use instead of JOIN. For more information, see the
section "External dictionaries".

#### WHERE clause

If there is a WHERE clause, it must contain an expression with the UInt8 type.
This is usually an expression with comparison and logical operators. This
expression will be used for filtering data before all other transformations.
If indexes are supported by the database table engine, the expression is
evaluated on the ability to use indexes.

#### PREWHERE clause

This clause has the same meaning as the WHERE clause. The difference is in
which data is read from the table. When using PREWHERE, first only the columns
necessary for executing PREWHERE are read. Then the other columns are read
that are needed for running the query, but only those blocks where the
PREWHERE expression is true. It makes sense to use PREWHERE if there are
filtration conditions that are not suitable for indexes that are used by a
minority of the columns in the query, but that provide strong data filtration.
This reduces the volume of data to read. For example, it is useful to write
PREWHERE for queries that extract a large number of columns, but that only
have filtration for a few columns. PREWHERE is only supported by *MergeTree
tables. A query may simultaneously specify PREWHERE and WHERE. In this case,
PREWHERE precedes WHERE. Keep in mind that it does not make much sense for
PREWHERE to only specify those columns that have an index, because when using
an index, only the data blocks that match the index are read. If the
'optimize_move_to_prewhere' setting is set to 1 and PREWHERE is omitted, the
system uses heuristics to automatically move parts of expressions from WHERE
to PREWHERE.

#### GROUP BY clause

This is one of the most important parts of a column-oriented DBMS. If there is
a GROUP BY clause, it must contain a list of expressions. Each expression will
be referred to here as a "key". All the expressions in the SELECT, HAVING, and
ORDER BY clauses must be calculated from keys or from aggregate functions. In
other words, each column selected from the table must be used either in keys
or inside aggregate functions. If a query contains only table columns inside
aggregate functions, the GROUP BY clause can be omitted, and aggregation by an
empty set of keys is assumed. Example: %%SELECT count(), median(FetchTiming >
60 ? 60 : FetchTiming), count() - sum(Refresh) FROM hits%% However, in
contrast to standard SQL, if the table doesn't have any rows (either there
aren't any at all, or there aren't any after using WHERE to filter), an empty
result is returned, and not the result from one of the rows containing the
initial values of aggregate functions. As opposed to MySQL (and conforming to
standard SQL), you can't get some value of some column that is not in a key or
aggregate function (except constant expressions). To work around this, you can
use the 'any' aggregate function (get the first encountered value) or
'min/max'. Example: %%SELECT domainWithoutWWW(URL) AS domain, count(),
any(Title) AS title -- we take the first page title for each domain FROM hits
GROUP BY domain%% For every different key value encountered, GROUP BY
calculates a set of aggregate function values. GROUP BY is not supported for
array columns. A constant can't be specified as arguments for aggregate
functions. Example: sum(1). Instead of this, you can get rid of the constant.
Example: count().

##### WITH TOTALS modifier

If the WITH TOTALS modifier is specified, another row will be calculated. This
row will have key columns containing default values (zeros or empty lines),
and columns of aggregate functions with the values calculated across all the
rows (the "total" values). This extra row is output in JSON*, TabSeparated*,
and Pretty* formats, separately from the other rows. In the other formats,
this row is not output. In JSON* formats, this row is output as a separate
'totals' field. In TabSeparated formats, the row comes after the main result,
preceded by an empty row (after the other data). In Pretty formats, the row is
output as a separate table after the main result. WITH TOTALS can be run in
different ways when HAVING is present. The behavior depends on the
'totals_mode' setting. By default, totals_mode = '**before_having**'. In this
case, 'totals' is calculated across all rows, including the ones that don't
pass through HAVING and 'max_rows_to_group_by'. The other alternatives include
only the rows that pass through HAVING in 'totals', and behave differently
with the setting 'max_rows_to_group_by' and 'group_by_overflow_mode = 'any''.
**after_having_exclusive** \- Don't include rows that didn't pass through
'max_rows_to_group_by'. In other words, 'totals' will have less than or the
same number of rows as it would if 'max_rows_to_group_by' were omitted.
**after_having_inclusive** \- Include all the rows that didn't pass through
'max_rows_to_group_by' in 'totals'. In other words, 'totals' will have more
than or the same number of rows as it would if 'max_rows_to_group_by' were
omitted. **after_having_auto** \- Count the number of rows that passed through
HAVING. If it is more than a certain amount (by default, 50%), include all the
rows that didn't pass through 'max_rows_to_group_by' in 'totals'. Otherwise,
do not include them. **totals_auto_threshold** \- By default, 0.5 is the
coefficient for **after_having_auto**. If 'max_rows_to_group_by' and
'group_by_overflow_mode = 'any'' are not used, all variations of
'after_having' are the same, and you can use any of them (for example,
'after_having_auto'). You can use WITH TOTALS in subqueries, including
subqueries in the JOIN clause. In this case, the respective total values are
combined.

#### HAVING clause

Allows filtering the result received after GROUP BY, similar to the WHERE
clause. WHERE and HAVING differ in that WHERE is performed before aggregation
(GROUP BY), while HAVING is performed after it. If aggregation is not
performed, HAVING can't be used.

#### ORDER BY clause

The ORDER BY clause contains a list of expressions, which can each be assigned
DESC or ASC (the sorting direction). If the direction is not specified, ASC is
assumed. ASC is sorted in ascending order, and DESC in descending order. The
sorting direction applies to a single expression, not to the entire list.
Example: %%ORDER BY Visits DESC, SearchPhrase%% For sorting by String values,
you can specify collation (comparison). Example: %%ORDER BY SearchPhrase
COLLATE 'tr'%% - for sorting by keyword in ascending order, using the Turkish
alphabet, case insensitive, assuming that strings are UTF-8 encoded. COLLATE
can be specified or not for each expression in ORDER BY independently. If ASC
or DESC is specified, COLLATE is specified after it. When using COLLATE,
sorting is always case-insensitive. We only recommend using COLLATE for final
sorting of a small number of rows, since sorting with COLLATE is less
efficient than normal sorting by bytes. Rows that have identical values for
the list of sorting expressions are output in an arbitrary order, which can
also be nondeterministic (different each time). If the ORDER BY clause is
omitted, the order of the rows is also undefined, and may be nondeterministic
as well. When floating point numbers are sorted, NaNs are separate from the
other values. Regardless of the sorting order, NaNs come at the end. In other
words, for ascending sorting they are placed as if they are larger than all
the other numbers, while for descending sorting they are placed as if they are
smaller than the rest. Less RAM is used if a small enough LIMIT is specified
in addition to ORDER BY. Otherwise, the amount of memory spent is proportional
to the volume of data for sorting. For distributed query processing, if GROUP
BY is omitted, sorting is partially done on remote servers, and the results
are merged on the requestor server. This means that for distributed sorting,
the volume of data to sort can be greater than the amount of memory on a
single server. If there is not enough RAM, it is possible to perform sorting
in external memory (creating temporary files on a disk). Use the setting
%%max_bytes_before_external_sort%% for this purpose. If it is set to 0 (the
default), external sorting is disabled. If it is enabled, when the volume of
data to sort reaches the specified number of bytes, the collected data is
sorted and dumped into a temporary file. After all data is read, all the
sorted files are merged and the results are output. Files are written to the
/opt/clickhouse/tmp/ directory in the config (by default, but you can use the
'tmp_path' parameter to change this setting). Running a query may use more
memory than 'max_bytes_before_external_sort'. For this reason, this setting
must have a value significantly smaller than 'max_memory_usage'. As an
example, if your server has 128 GB of RAM and you need to run a single query,
set 'max_memory_usage' to 100 GB, and 'max_bytes_before_external_sort' to 80
GB. External sorting works much less effectively than sorting in RAM.

#### SELECT clause

The expressions specified in the SELECT clause are analyzed after the
calculations for all the clauses listed above are completed. More
specifically, expressions are analyzed that are above the aggregate functions,
if there are any aggregate functions. The aggregate functions and everything
below them are calculated during aggregation (GROUP BY). These expressions
work as if they are applied to separate rows in the result.

#### DISTINCT clause

If DISTINCT is specified, only a single row will remain out of all the sets of
fully matching rows in the result. The result will be the same as if GROUP BY
were specified across all the fields specified in SELECT without aggregate
functions. But there are several differences from GROUP BY: \- DISTINCT can be
applied together with GROUP BY. \- When ORDER BY is omitted and LIMIT is
defined, the query stops running immediately after the required number of
different rows has been read. In this case, using DISTINCT is much more
optimal. \- Data blocks are output as they are processed, without waiting for
the entire query to finish running. DISTINCT is not supported if SELECT has at
least one array column.

#### LIMIT clause

LIMIT m allows you to select the first 'm' rows from the result. LIMIT n, m
allows you to select the first 'm' rows from the result after skipping the
first 'n' rows. 'n' and 'm' must be non-negative integers. If there isn't an
ORDER BY clause that explicitly sorts results, the result may be arbitrary and
nondeterministic.

#### UNION ALL clause

You can use UNION ALL to combine any number of queries. Example: %% SELECT
CounterID, 1 AS table, toInt64(count()) AS c FROM test.hits GROUP BY CounterID
UNION ALL SELECT CounterID, 2 AS table, sum(Sign) AS c FROM test.visits GROUP
BY CounterID HAVING c > 0 %% Only UNION ALL is supported. The regular UNION
(UNION DISTINCT) is not supported. If you need UNION DISTINCT, you can write
SELECT DISTINCT from a subquery containing UNION ALL. Queries that are parts
of UNION ALL can be run simultaneously, and their results can be mixed
together. The structure of results (the number and type of columns) must match
for the queries, but the column names can differ. In this case, the column
names for the final result will be taken from the first query. Queries that
are parts of UNION ALL can't be enclosed in brackets. ORDER BY and LIMIT are
applied to separate queries, not to the final result. If you need to apply a
conversion to the final result, you can put all the queries with UNION ALL in
a subquery in the FROM clause.

#### FORMAT clause

Specify 'FORMAT format' to get data in any specified format. You can use this
for convenience, or for creating dumps. For more information, see the section
"Formats". If the FORMAT clause is omitted, the default format is used, which
depends on both the settings and the interface used for accessing the DB. For
the HTTP interface and the command-line client in batch mode, the default
format is TabSeparated. For the command-line client in interactive mode, the
default format is PrettyCompact (it has attractive and compact tables). When
using the command-line client, data is passed to the client in an internal
efficient format. The client independently interprets the FORMAT clause of the
query and formats the data itself (thus relieving the network and the server
from the load).

#### IN operators

The %%IN%%, %%NOT IN%%, %%GLOBAL IN%%, and %%GLOBAL NOT IN%% operators are
covered separately, since their functionality is quite rich. The left side of
the operator is either a single column or a tuple. Examples: %%SELECT UserID
IN (123, 456) FROM ...%% %%SELECT (CounterID, UserID) IN ((34, 123), (101500,
456)) FROM ...%% If the left side is a single column that is in the index, and
the right side is a set of constants, the system uses the index for processing
the query. Don't list too many values explicitly (i.e. millions). If a data
set is large, put it in a temporary table (for example, see the section
"External data for query processing"), then use a subquery. The right side of
the operator can be a set of constant expressions, a set of tuples with
constant expressions (shown in the examples above), or the name of a database
table or SELECT subquery in brackets. If the right side of the operator is the
name of a table (for example, %%UserID IN users%%), this is equivalent to the
subquery %%UserID IN (SELECT * FROM users)%%. Use this when working with
external data that is sent along with the query. For example, the query can be
sent together with a set of user IDs loaded to the 'users' temporary table,
which should be filtered. If the right side of the operator is a table name
that has the Set engine (a prepared data set that is always in RAM), the data
set will not be created over again for each query. The subquery may specify
more than one column for filtering tuples. Example: %%SELECT (CounterID,
UserID) IN (SELECT CounterID, UserID FROM ...) FROM ...%% The columns to the
left and right of the %%IN%% operator should have the same type. Exception: If
there is an array to the left of IN, it checks that at least one array item
belongs to the data set. For example, %%[1, 2, 3] IN (3, 4, 5)%% returns 1.
This is somewhat illogical, but it is convenient for implementing certain
Yandex.Metrica functionality. If there is an array to the left of NOT IN, it
checks that at least one array item does not belong to the data set. For
example, %%[1, 2, 3] NOT IN (3, 4, 5)%% returns 1. This is completely
illogical, so don't rely on this behavior. We recommend using higher-order
functions instead. Example: arrayAll(x -> x IN (3, 4, 5), [1, 2, 3]) returns 0
- it checks whether all the array items belong to the data set. The IN
operator and subquery may occur in any part of the query, including in
aggregate functions and lambda functions. Example: %%SELECT EventDate,
avg(UserID IN ( SELECT UserID FROM test.hits WHERE EventDate =
toDate('2014-03-17') )) AS ratio FROM test.hits GROUP BY EventDate ORDER BY
EventDate ASC ┌──EventDate─┬────ratio─┐ │ 2014-03-17 │ 1 │ │ 2014-03-18 │
0.807696 │ │ 2014-03-19 │ 0.755406 │ │ 2014-03-20 │ 0.723218 │ │ 2014-03-21 │
0.697021 │ │ 2014-03-22 │ 0.647851 │ │ 2014-03-23 │ 0.648416 │
└────────────┴──────────┘ %% \- for each day after March 17th, count the
percentage of pageviews made by users who visited the site on March 17th. A
subquery in the IN clause is always run just one time on a single server.
There are no dependent subqueries.

#### Distributed subqueries

There are two versions of INs with subqueries (and for JOINs): the regular
%%IN%% / %%JOIN%%, and %%GLOBAL IN%% / %%GLOBAL JOIN%%. They differ in how
they are run for distributed query processing. When using the regular %%IN%%,
the query is sent to remote servers, and each of them runs the subqueries in
the IN or JOIN clause. When using %%GLOBAL IN%% / %%GLOBAL JOIN%%, first all
the subqueries for %%GLOBAL IN%% / %%GLOBAL JOIN%% are run, and the results
are collected in temporary tables. Then the temporary tables are sent to each
remote server, where the queries are run using this temporary data. For a non-
distributed query, use the regular %%IN%% / %%JOIN%%. Be careful when using
subqueries in the %%IN%% / %%JOIN%% clauses for distributed query processing.
Let's look at some examples. Assume that each server in the cluster has a
normal **local_table**. Each server also has a **distributed_table** table
with the **Distributed** type, which looks at all the servers in the cluster.
For a query to the **distributed_table**, the query will be sent to all the
remote servers and run on them using the **local_table**. For example, the
query %%SELECT uniq(UserID) FROM distributed_table%% will be sent to all the
remote servers as %%SELECT uniq(UserID) FROM local_table%% and run on each of
them in parallel, until it reaches the stage where intermediate results can be
combined. Then the intermediate results will be returned to the requestor
server and merged on it, and the final result will be sent to the client. Now
let's examine a query with IN: %%SELECT uniq(UserID) FROM distributed_table
WHERE CounterID = 101500 AND UserID IN (SELECT UserID FROM local_table WHERE
CounterID = 34)%% \- calculates the overlap in the audiences of two websites.
This query will be sent to all the remote servers as %%SELECT uniq(UserID)
FROM local_table WHERE CounterID = 101500 AND UserID IN (SELECT UserID FROM
local_table WHERE CounterID = 34)%% In other words, the data set in the %%IN%%
clause will be collected on each server independently, only across the data
that is stored locally on each of the servers. This will work correctly and
optimally if you are prepared for this case and have spread data across the
cluster servers such that the data for a single UserID resides entirely on a
single server. In this case, all the necessary data will be available locally
on each server. Otherwise, the result will be inaccurate. We refer to this
variation of the query as "local IN". To correct how the query works when data
is spread randomly across the cluster servers, you could specify
**distributed_table** inside a subquery. The query would look like this:
%%SELECT uniq(UserID) FROM distributed_table WHERE CounterID = 101500 AND
UserID IN (SELECT UserID FROM distributed_table WHERE CounterID = 34)%% This
query will be sent to all remote servers as %%SELECT uniq(UserID) FROM
local_table WHERE CounterID = 101500 AND UserID IN (SELECT UserID FROM
distributed_table WHERE CounterID = 34)%% Each of the remote servers will
start running the subquery. Since the subquery uses a distributed table, each
remote server will re-send the subquery to every remote server, as %%SELECT
UserID FROM local_table WHERE CounterID = 34%% For example, if you have a
cluster of 100 servers, executing the entire query will require 10,000
elementary requests, which is generally considered unacceptable. In such
cases, you should always use %%GLOBAL IN%% instead of %%IN%%. Let's look at
how it works for the query %%SELECT uniq(UserID) FROM distributed_table WHERE
CounterID = 101500 AND UserID GLOBAL IN (SELECT UserID FROM distributed_table
WHERE CounterID = 34)%% The requestor server will execute the subquery
%%SELECT UserID FROM distributed_table WHERE CounterID = 34%% and the result
will be put in a temporary table in RAM. Then a query will be sent to each
remote server as %%SELECT uniq(UserID) FROM local_table WHERE CounterID =
101500 AND UserID GLOBAL IN _data1%% and the temporary table '_data1' will be
sent to every remote server together with the query (the name of the temporary
table is implementation-defined). This is more optimal than using the normal
IN. However, keep the following points in mind: 1\. When creating a temporary
table, data is not made unique. To reduce the volume of data transmitted over
the network, specify %%DISTINCT%% in the subquery. (You don't need to do this
for a normal IN.) 2\. The temporary table will be sent to all the remote
servers. Transmission does not account for network topology. For example, if
10 remote servers reside in a datacenter that is very remote in relation to
the requestor server, the data will be sent 10 times over the channel to the
remote datacenter. Try to avoid large data sets when using %%GLOBAL IN%%. 3\.
When transmitting data to remote servers, restrictions on network bandwidth
are not configurable. You might overload the network. 4\. Try to distribute
data across servers so that you don't need to use %%GLOBAL IN%% on a regular
basis. 5\. If you need to use %%GLOBAL IN%% often, plan the location of the
ClickHouse cluster so that in each datacenter, there will be at least one
replica of each shard, and there is a fast network between them - for
possibility to process query with transferring data only inside datacenter. It
also makes sense to specify a local table in the %%GLOBAL IN%% clause, in case
this local table is only available on the requestor server and you want to use
data from it on remote servers.

#### Extreme values

In addition to results, you can also get minimum and maximum values for the
results columns. To do this, set the 'extremes' setting to '1'. Minimums and
maximums are calculated for numeric types, dates, and dates with times. For
other columns, the default values are output. An extra two rows are calculated
- the minimums and maximums, respectively. These extra two rows are output in
JSON*, TabSeparated*, and Pretty* formats, separate from the other rows. They
are not output for other formats. In JSON* formats, the extreme values are
output in a separate 'extremes' field. In TabSeparated formats, the row comes
after the main result, and after 'totals' if present. It is preceded by an
empty row (after the other data). In Pretty formats, the row is output as a
separate table after the main result, and after 'totals' if present. Extreme
values are calculated for rows that have passed through LIMIT. However, when
using 'LIMIT offset, size', the rows before 'offset' are included in
'extremes'. In stream requests, the result may also include a small number of
rows that passed through LIMIT.

#### Notes

The GROUP BY and ORDER BY clauses do not support positional arguments. This
contradicts MySQL, but conforms to standard SQL. For example, 'GROUP BY 1, 2'
will be interpreted as grouping by constants (i.e. aggregation of all rows
into one). You can use synonyms (AS aliases) in any part of a query. You can
put an asterisk in any part of a query instead of an expression. When the
query is analyzed, the asterisk is expanded to a list of all table columns
(excluding the MATERIALIZED and ALIAS columns). There are only a few cases
when using an asterisk is justified: \- When creating a table dump. \- For
tables containing just a few columns, such as system tables. \- For getting
information about what columns are in a table. In this case, set 'LIMIT 1'.
But it is better to use the **DESC TABLE** query. \- When there is strong
filtration on a small number of columns using PREWHERE. \- In subqueries
(since columns that aren't needed for the external query are excluded from
subqueries). In all other cases, we don't recommend using the asterisk, since
it only gives you the drawbacks of a columnar DBMS instead of the advantages.

# External data for query processing

ClickHouse allows sending a server the data that is needed for processing a
query, together with a SELECT query. This data is put in a temporary table
(see the section "Temporary tables") and can be used in the query (for
example, in IN operators). For example, if you have a text file with important
user identifiers, you can upload it to the server along with a query that uses
filtration by this list. If you need to run more than one query with a large
volume of external data, don't use this feature. It is better to upload the
data to the DB ahead of time. External data can be uploaded using the command-
line client (in non-interactive mode), or using the HTTP interface. In the
command-line client, you can specify a parameters section in the format
%%--external --file=... [--name=...] [--format=...]
[--types=...|--structure=...]%% You may have multiple sections like this, for
the number of tables being transmitted. **\--external** \- Marks the beginning
of the section. **\--file** \- Path to the file with the table dump, or %%-%%,
which refers to stdin. Only a single table can be retrieved from stdin. The
following parameters are optional: **\--name** \- Name of the table. If
omitted, %%_data%% is used. **\--format** \- Data format in the file. If
omitted, %%TabSeparated%% is used. One of the following parameters is
required: **\--types** \- A comma-separated list of column types. For example,
%%UInt64,String%%. Columns will be named %%_1%%, %%_2%%, ... **\--structure**
\- Table structure, in the format %%UserID UInt64, URL String%%. Defines the
column names and types. The files specified in %%file%% will be parsed by the
format specified in %%format%%, using the data types specified in %%types%% or
%%structure%%. The table will be uploaded to the server and accessible there
as a temporary table with the name %%name%%. Examples: %%echo -ne "1\n2\n3\n"
| clickhouse-client --query="SELECT count() FROM test.visits WHERE
TraficSourceID IN _data" \--external --file=- --types=Int8 849897 %% %%cat
/etc/passwd | sed 's/:/\t/g' | clickhouse-client --query="SELECT shell,
count() AS c FROM passwd GROUP BY shell ORDER BY c DESC" \--external --file=-
--name=passwd --structure='login String, unused String, uid UInt16, gid
UInt16, comment String, home String, shell String' /bin/sh 20 /bin/false 5
/bin/bash 4 /usr/sbin/nologin 1 /bin/sync 1 %% When using the HTTP interface,
external data is passed in the multipart/form-data format. Each table is
transmitted as a separate file. The table name is taken from the file name.
The 'query_string' passes the parameters '_name__format', '_name__types', and
'_name__structure', where _name_ is the name of the table that these
parameters correspond to. The meaning of the parameters is the same as when
using the command-line client. Example:

    
    
    cat /etc/passwd | sed 's/:/\t/g' > passwd.tsv
    
    curl -F 'passwd=@passwd.tsv;' 'http://localhost:8123/?query=SELECT+shell,+count()+AS+c+FROM+passwd+GROUP+BY+shell+ORDER+BY+c+DESC&passwd_structure=login+String,+unused+String,+uid+UInt16,+gid+UInt16,+comment+String,+home+String,+shell+String'
    /bin/sh 20
    /bin/false      5
    /bin/bash       4
    /usr/sbin/nologin       1
    /bin/sync       1
    

For distributed query processing, the temporary tables are sent to all the
remote servers.

# Table engines

The table engine (type of table) determines: \- How and where data is stored -
where to write it to, and where to read it from. \- Which queries are
supported, and how. \- Concurrent data access. \- Use of indexes, if present.
\- Whether multithreaded request execution is possible. \- Data replication.
\- When reading data, the engine is only required to extract the necessary set
of columns. However, in some cases, the query may be partially processed
inside the table engine. Note that for most serious tasks, you should use
engines from the MergeTree family. ==TinyLog== The simplest table engine,
which stores data on a disk. Each column is stored in a separate compressed
file. When writing, data is appended to the end of files. Concurrent data
access is not restricted in any way: \- If you are simultaneously reading from
a table and writing to it in a different query, the read operation will
complete with an error. \- If you are writing to a table in multiple queries
simultaneously, the data will be broken. The typical way to use this table is
write-once: first just write the data one time, then read it as many times as
needed. Queries are executed in a single stream. In other words, this engine
is intended for relatively small tables (recommended up to 1,000,000 rows). It
makes sense to use this table engine if you have many small tables, since it
is simpler than the Log engine (fewer files need to be opened). The situation
when you have a large number of small tables guarantees poor productivity, but
may already be used when working with another DBMS, and you may find it easier
to switch to using TinyLog types of tables. Indexes are not supported. In
Yandex.Metrica, TinyLog tables are used for intermediary data that is
processed in small batches. ==Log== Log differs from TinyLog in that a small
file of "marks" resides with the column files. These marks are written on
every data block and contain offsets - where to start reading the file in
order to skip the specified number of rows. This makes it possible to read
table data in multiple threads. For concurrent data access, the read
operations can be performed simultaneously, while write operations block reads
and each other. The Log engine does not support indexes. Similarly, if writing
to a table failed, the table is broken, and reading from it returns an error.
The Log engine is appropriate for temporary data, write-once tables, and for
testing or demonstration purposes. ==Memory== The Memory engine stores data in
RAM, in uncompressed form. Data is stored in exactly the same form as it is
received when read. In other words, reading from this table is completely
free. Concurrent data access is synchronized. Locks are short: read and write
operations don't block each other. Indexes are not supported. Reading is
parallelized. Maximal productivity (over 10 GB/sec) is reached on simple
queries, because there is no reading from the disk, decompressing, or
deserializing data. (We should note that in many cases, the productivity of
the MergeTree engine is almost as high.) When restarting a server, data
disappears from the table and the table becomes empty. Normally, using this
table engine is not justified. However, it can be used for tests, and for
tasks where maximum speed is required on a relatively small number of rows (up
to approximately 100,000,000). The Memory engine is used by the system for
temporary tables with external query data (see the section "External data for
processing a query"), and for implementing GLOBAL IN (see the section "IN
operators"). ==Merge== The Merge engine (not to be confused with MergeTree)
does not store data itself, but allows reading from any number of other tables
simultaneously. Reading is automatically parallelized. Writing to a table is
not supported. When reading, the indexes of tables that are actually being
read are used, if they exist. The Merge engine accepts parameters: the
database name and a regular expression for tables. Example: %%Merge(hits,
'^WatchLog')%% \- Data will be read from the tables in the 'hits' database
with names that match the regex '^WatchLog'. Instead of the database name, you
can use a constant expression that returns a string. For example,
%%currentDatabase()%%. Regular expressions are re2 (similar to PCRE), case-
sensitive. See the notes about escaping symbols in regular expressions in the
"match" section. When selecting tables to read, the Merge table itself will
not be selected, even if it matches the regex. This is to avoid loops. It is
possible to create two Merge tables that will endlessly try to read each
others' data. But don't do this. The typical way to use the Merge engine is
for working with a large number of TinyLog tables as if with a single table.
===Virtual columns=== Virtual columns are columns that are provided by the
table engine, regardless of the table definition. In other words, these
columns are not specified in CREATE TABLE, but they are accessible for SELECT.
Virtual columns differ from normal columns in the following ways: \- They are
not specified in table definitions. \- Data can't be added to them with
INSERT. \- When using INSERT without specifying the list of columns, virtual
columns are ignored. \- They are not selected when using the asterisk (SELECT
*). \- Virtual columns are not shown in SHOW CREATE TABLE and DESC TABLE
queries. A Merge table contains the virtual column **_table** of the String
type. (If the table already has a '_table' column, the virtual column is named
'_table1', and if it already has '_table1', it is named '_table2', and so on.)
It contains the name of the table that data was read from. If the WHERE or
PREWHERE clause contains conditions for the '_table' column that do not depend
on other table columns (as one of the conjunction elements, or as an entire
expression), these conditions are used as an index. The conditions are
performed on a data set of table names to read data from, and the read
operation will be performed from only those tables that the condition was
triggered on. ==Distributed== The Distributed engine does not store data
itself, but allows distributed query processing on multiple servers. Reading
is automatically parallelized. During a read, the table indexes on remote
servers are used, if there are any. The Distributed engine accepts parameters:
the cluster name in the server's config file, the name of a remote database,
the name of a remote table, and (optionally) a sharding key. Example:
%%Distributed(calcs, default, hits[, sharding_key])%% \- Data will be read
from all servers in the 'calcs' cluster, from the 'default.hits' table located
on every server in the cluster. Data is not only read, but is partially
processed on the remote servers (to the extent that this is possible). For
example, for a query with GROUP BY, data will be aggregated on remote servers,
and the intermediate states of aggregate functions will be sent to the
requestor server. Then data will be further aggregated. Instead of the
database name, you can use a constant expression that returns a string. For
example, %%currentDatabase()%%. calcs - The cluster name in the server's
config file. Clusters are set like this: %% <remote_servers> <logs> <shard>
<!-- Optional. Shard weight when writing data. By default, 1. -->
<weight>1</weight> <!-- Optional. Whether to write data to just one of the
replicas. By default, false - write data to all of the replicas. -->
<internal_replication>false</internal_replication> <replica>
<host>example01-01-1</host> <port>9000</port> </replica> <replica>
<host>example01-01-2</host> <port>9000</port> </replica> </shard> <shard>
<weight>2</weight> <internal_replication>false</internal_replication>
<replica> <host>example01-02-1</host> <port>9000</port> </replica> <replica>
<host>example01-02-2</host> <port>9000</port> </replica> </shard> </logs>
</remote_servers> %% Here a cluster is defined with the name 'logs' that
consists of two shards, each of which contains two replicas. Shards refer to
the servers that contain different parts of the data (in order to read all the
data, you must access all the shards). Replicas are duplicating servers (in
order to read all the data, you can access the data on any one of the
replicas). For each server, there are several parameters: mandatory: 'host',
'port', and optional: 'user', 'password'. **host** \- address of remote
server. May be specified as domain name or IPv4 or IPv6 address. If you
specify domain, server will perform DNS lookup at startup, and result will be
cached till server shutdown. If DNS request is failed, server won't start. If
you are changing DNS records, restart the server for new records to take
effect. **port** \- TCP-port for interserver communication (tcp_port in
configuration file, usually 9000). Don't get confused with http_port. **user**
\- user name to connect to remote server. By default user is 'default'. This
user must have access rights to connect to remote server. Access rights are
managed in users.xml configuration file. For additional info, consider "Access
rights" section. **password** \- password to log in to remote server, in
plaintext. Default is empty string. When specifying replicas, one of the
available replicas will be selected for each of the shards when reading. You
can configure the algorithm for load balancing (the preference for which
replica to access) - see the 'load_balancing' setting. If the connection with
the server is not established, there will be an attempt to connect with a
short timeout. If the connection failed, the next replica will be selected,
and so on for all the replicas. If the connection attempt failed for all the
replicas, the attempt will be repeated the same way, several times. This works
in favor of resiliency, but does not provide complete fault tolerance: a
remote server might accept the connection, but might not work, or work poorly.
You can specify just one of the shards (in this case, query processing should
be called remote, rather than distributed) or up to any number of shards. In
each shard, you can specify from one to any number of replicas. You can
specify a different number of replicas for each shard. You can specify as many
clusters as you wish in the configuration. To view your clusters, use the
'system.clusters' table. The Distributed engine allows working with a cluster
like a local server. However, the cluster is inextensible: you must write its
configuration in the server config file (even better, for all the cluster's
servers). There is no support for Distributed tables that look at other
Distributed tables (except in cases when a Distributed table only has one
shard). As an alternative, make the Distributed table look at the "final"
tables. The Distributed engine requires writing clusters to the config file.
Adding clusters and servers requires restarting. If you need to send a query
to an unknown set of shards and replicas each time, you don't need to create a
Distributed table - use the 'remote' table function instead. See the section
"Table functions". There are two methods for writing data to a cluster: First,
you can define which servers to write which data to, and perform the write
directly on each shard. In other words, perform INSERT in the tables that the
distributed table "looks at". This is the most flexible solution - you can use
any sharding scheme, which could be non-trivial due to the requirements of the
subject area. This is also the most optimal solution, since data can be
written to different shards completely independently. Second, you can perform
INSERT in a Distributed table. In this case, the table will distribute the
inserted data across servers itself. In order to write to a Distributed table,
it must have a sharding key set (the last parameter). In addition, if there is
only one shard, the write operation works without specifying the sharding key,
since it doesn't have any meaning in this case. Each shard can have a weight
defined in the config file. By default, the weight is equal to one. Data is
distributed across shards in the amount proportional to the shard weight. For
example, if there are two shards and the first has a weight of 9 while the
second has a weight of 10, the first will be sent 9 / 19 parts of the rows,
and the second will be sent 10 / 19. Each shard can have the
'internal_replication' parameter defined in the config file. If this parameter
is set to 'true', the write operation selects the first healthy replica and
writes data to it. Use this alternative if the Distributed table "looks at"
replicated tables. In other words, if the table where data will be written is
going to replicate them itself. If it is set to 'false' (the default), data is
written to all replicas. In essence, this means that the Distributed table
replicates data itself. This is worse than using replicated tables, because
the consistency of replicas is not checked, and over time they will contain
slightly different data. To select the shard that a row of data is sent to,
the sharding expression is analyzed, and its remainder is taken from dividing
it by the total weight of the shards. The row is sent to the shard that
corresponds to the half-interval of the remainders from 'prev_weight' to
'prev_weights + weight', where 'prev_weights' is the total weight of the
shards with the smallest number, and 'weight' is the weight of this shard. For
example, if there are two shards, and the first has a weight of 9 while the
second has a weight of 10, the row will be sent to the first shard for the
remainders from the range [0, 9), and to the second for the remainders from
the range [10, 19). The sharding expression can be any expression from
constants and table columns that returns an integer. For example, you can use
the expression 'rand()' for random distribution of data, or 'UserID' for
distribution by the remainder from dividing the user's ID (then the data of a
single user will reside on a single shard, which simplifies running IN and
JOIN by users). If one of the columns is not distributed evenly enough, you
can wrap it in a hash function: intHash64(UserID). A simple remainder from
division is a limited solution for sharding and isn't always appropriate. It
works for medium and large volumes of data (dozens of servers), but not for
very large volumes of data (hundreds of servers or more). In the latter case,
use the sharding scheme required by the subject area, rather than using
entries in Distributed tables. When using Replicated tables, it is possible to
reshard data - look at "Resharding" section. But in many cases, better to do
without it. SELECT queries are sent to all the shards, and work regardless of
how data is distributed across the shards (they can be distributed completely
randomly). When you add a new shard, you don't have to transfer the old data
to it. You can write new data with a heavier weight - the data will be
distributed slightly unevenly, but queries will work correctly and
efficiently. You should be concerned about the sharding scheme in the
following cases: \- Queries are used that require joining data (IN or JOIN) by
a specific key. If data is sharded by this key, you can use local IN or JOIN
instead of GLOBAL IN or GLOBAL JOIN, which is much more efficient. \- A large
number of servers is used (hundreds or more) with a large number of small
queries (queries of individual clients - websites, advertisers, or partners).
In order for the small queries to not affect the entire cluster, it makes
sense to locate data for a single client on a single shard. Alternatively, as
we've done in Yandex.Metrica, you can set up bi-level sharding: divide the
entire cluster into "layers", where a layer may consist of multiple shards.
Data for a single client is located on a single layer, but shards can be added
to a layer as necessary, and data is randomly distributed within them.
Distributed tables are created for each layer, and a single shared distributed
table is created for global queries. Data is written asynchronously. For an
INSERT to a Distributed table, the data block is just written to the local
file system. The data is sent to the remote servers in the background as soon
as possible. You should check whether data is sent successfully by checking
the list of files (data waiting to be sent) in the table directory:
/opt/clickhouse/data/_database_/_table_/. If the server ceased to exist or had
a rough restart (for example, after a device failure) after an INSERT to a
Distributed table, the inserted data might be lost. If a damaged data part is
detected in the table directory, it is transferred to the 'broken'
subdirectory and no longer used. ==MergeTree== The MergeTree engine supports
an index by primary key and by date, and provides the possibility to update
data in real time. This is the most advanced table engine in ClickHouse. Don't
confuse it with the Merge engine. The engine accepts parameters: the name of a
Date type column containing the date, a sampling expression (optional), a
tuple that defines the table's primary key, and the index granularity.
Example: Example without sampling support: %%MergeTree(EventDate, (CounterID,
EventDate), 8192)%% Example with sampling support: %%MergeTree(EventDate,
intHash32(UserID), (CounterID, EventDate, intHash32(UserID)), 8192)%% A
MergeTree type table must have a separate column containing the date. In this
example, it is the 'EventDate' column. The type of the date column must be
'Date' (not 'DateTime'). The primary key may be a tuple from any expressions
(usually this is just a tuple of columns), or a single expression. The
sampling expression (optional) can be any expression. It must also be present
in the primary key. The example uses a hash of user IDs to pseudo-randomly
disperse data in the table for each CounterID and EventDate. In other words,
when using the SAMPLE clause in a query, you get an evenly pseudo-random
sample of data for a subset of users. The table is implemented as a set of
parts. Each part is sorted by the primary key. In addition, each part has the
minimum and maximum date assigned. When inserting in the table, a new sorted
part is created. The merge process is periodically initiated in the
background. When merging, several parts are selected, usually the smallest
ones, and then merged into one large sorted part. In other words, incremental
sorting occurs when inserting to the table. Merging is implemented so that the
table always consists of a small number of sorted parts, and the merge itself
doesn't do too much work. During insertion, data belonging to different months
is separated into different parts. The parts that correspond to different
months are never combined. The purpose of this is to provide local data
modification (for ease in backups). Parts are combined up to a certain size
threshold, so there aren't any merges that are too long. For each part, an
index file is also written. The index file contains the primary key value for
every 'index_granularity' row in the table. In other words, this is an
abbreviated index of sorted data. For columns, "marks" are also written to
each 'index_granularity' row so that data can be read in a specific range.
When reading from a table, the SELECT query is analyzed for whether indexes
can be used. An index can be used if the WHERE or PREWHERE clause has an
expression (as one of the conjunction elements, or entirely) that represents
an equality or inequality comparison operation, or if it has IN above columns
that are in the primary key or date, or Boolean operators over them. Thus, it
is possible to quickly run queries on one or many ranges of the primary key.
In the example given, queries will work quickly for a specific counter, for a
specific counter and range of dates, for a specific counter and date, for
multiple counters and a range of dates, and so on. %%SELECT count() FROM table
WHERE EventDate = toDate(now()) AND CounterID = 34%% %%SELECT count() FROM
table WHERE EventDate = toDate(now()) AND (CounterID = 34 OR CounterID = 42)%%
%%SELECT count() FROM table WHERE ((EventDate >= toDate('2014-01-01') AND
EventDate <= toDate('2014-01-31')) OR EventDate = toDate('2014-05-01')) AND
CounterID IN (101500, 731962, 160656) AND (CounterID = 101500 OR EventDate !=
toDate('2014-05-01'))%% All of these cases will use the index by date and by
primary key. The index is used even for complex expressions. Reading from the
table is organized so that using the index can't be slower than a full scan.
In this example, the index can't be used: %%SELECT count() FROM table WHERE
CounterID = 34 OR URL LIKE '%upyachka%'%% The index by date only allows
reading those parts that contain dates from the desired range. However, a data
part may contain data for many dates (up to an entire month), while within a
single part the data is ordered by the primary key, which might not contain
the date as the first column. Because of this, using a query with only a date
condition that does not specify the primary key prefix will cause more data to
be read than for a single date. For concurrent table access, we use multi-
versioning. In other words, when a table is simultaneously read and updated,
data is read from a set of parts that is current at the time of the query.
There are no lengthy locks. Inserts do not get in the way of read operations.
Reading from a table is automatically parallelized. The OPTIMIZE query is
supported, which calls an extra merge step. You can use a single large table
and continually add data to it in small chunks - this is what MergeTree is
intended for. Data replication is possible for all types of tables in the
MergeTree family (see the section "Data replication"). ==CollapsingMergeTree==
This engine differs from MergeTree in that it allows automatic deletion, or
"collapsing" certain pairs of rows when merging. Yandex.Metrica has normal
logs (such as hit logs) and change logs. Change logs are used for
incrementally calculating statistics on data that is constantly changing.
Examples are the log of session changes, or logs of changes to user histories.
Sessions are constantly changing in Yandex.Metrica. For example, the number of
hits per session increases. We refer to changes in any object as a pair (?old
values, ?new values). Old values may be missing if the object was created. New
values may be missing if the object was deleted. If the object was changed,
but existed previously and was not deleted, both values are present. In the
change log, one or two entries are made for each change. Each entry contains
all the attributes that the object has, plus a special attribute for
differentiating between the old and new values. When objects change, only the
new entries are added to the change log, and the existing ones are not
touched. The change log makes it possible to incrementally calculate almost
any statistics. To do this, we need to consider "new" rows with a plus sign,
and "old" rows with a minus sign. In other words, incremental calculation is
possible for all statistics whose algebraic structure contains an operation
for taking the inverse of an element. This is true of most statistics. We can
also calculate "idempotent" statistics, such as the number of unique visitors,
since the unique visitors are not deleted when making changes to sessions.
This is the main concept that allows Yandex.Metrica to work in real time.
CollapsingMergeTree accepts an additional parameter - the name of an Int8-type
column that contains the row's "sign". Example:
%%CollapsingMergeTree(EventDate, (CounterID, EventDate, intHash32(UniqID),
VisitID), 8192, Sign)%% Here, 'Sign' is a column containing -1 for "old"
values and 1 for "new" values. When merging, each group of consecutive
identical primary key values (columns for sorting data) is reduced to no more
than one row with the column value 'sign_column = -1' (the "negative row") and
no more than one row with the column value 'sign_column = 1' (the "positive
row"). In other words, entries from the change log are collapsed. If the
number of positive and negative rows matches, the first negative row and the
last positive row are written. If there is one more positive row than negative
rows, only the last positive row is written. If there is one more negative row
than positive rows, only the first negative row is written. Otherwise, there
will be a logical error and none of the rows will be written. (A logical error
can occur if the same section of the log was accidentally inserted more than
once. The error is just recorded in the server log, and the merge continues.)
Thus, collapsing should not change the results of calculating statistics.
Changes are gradually collapsed so that in the end only the last value of
almost every object is left. Compared to MergeTree, the CollapsingMergeTree
engine allows a multifold reduction of data volume. There are several ways to
get completely "collapsed" data from a CollapsingMergeTree table: 1\. Write a
query with GROUP BY and aggregate functions that accounts for the sign. For
example, to calculate quantity, write 'sum(Sign)' instead of 'count()'. To
calculate the sum of something, write 'sum(Sign * x)' instead of 'sum(x)', and
so on, and also add 'HAVING sum(Sign) > 0'. Not all amounts can be calculated
this way. For example, the aggregate functions 'min' and 'max' can't be
rewritten. 2\. If you must extract data without aggregation (for example, to
check whether rows are present whose newest values match certain conditions),
you can use the FINAL modifier for the FROM clause. This approach is
significantly less efficient. ==SummingMergeTree== This engine differs from
MergeTree in that it totals data while merging. %%SummingMergeTree(EventDate,
(OrderID, EventDate, BannerID, ...), 8192)%% The columns to total are
implicit. When merging, all rows with the same primary key value (in the
example, OrderId, EventDate, BannerID, ...) have their values totaled in
numeric columns that are not part of the primary key.
%%SummingMergeTree(EventDate, (OrderID, EventDate, BannerID, ...), 8192,
(Shows, Clicks, Cost, ...))%% The columns to total are set explicitly (the
last parameter - Shows, Clicks, Cost, ...). When merging, all rows with the
same primary key value have their values totaled in the specified columns. The
specified columns also must be numeric and must not be part of the primary
key. If the values were null in all of these columns, the row is deleted. (The
exception is cases when the data part would not have any rows left in it.) For
the other rows that are not part of the primary key, the first value that
occurs is selected when merging. Summation is not performed for a read
operation. If it is necessary, write the appropriate GROUP BY. In addition, a
table can have nested data structures that are processed in a special way. If
the name of a nested table ends in 'Map' and it contains at least two columns
that meet the following criteria: \- for the first table, numeric ((U)IntN,
Date, DateTime), we'll refer to it as 'key' \- for other tables, arithmetic
((U)IntN, Float32/64), we'll refer to it as '(values...)' then this nested
table is interpreted as a mapping of key => (values...), and when merging its
rows, the elements of two data sets are merged by 'key' with a summation of
the corresponding (values...). Examples: %% [(1, 100)] + [(2, 150)] -> [(1,
100), (2, 150)] [(1, 100)] + [(1, 150)] -> [(1, 250)] [(1, 100)] + [(1, 150),
(2, 150)] -> [(1, 250), (2, 150)] [(1, 100), (2, 150)] + [(1, -100)] -> [(2,
150)] %% For nested data structures, you don't need to specify the columns as
a list of columns for totaling. This table engine is not particularly useful.
Remember that when saving just pre-aggregated data, you lose some of the
system's advantages. ==AggregatingMergeTree== This engine differs from
MergeTree in that the merge combines the states of aggregate functions stored
in the table for rows with the same primary key value. In order for this to
work, it uses the AggregateFunction data type and the -State and -Merge
modifiers for aggregate functions. Let's examine it more closely. There is an
AggregateFunction data type, which is a parametric data type. As parameters,
the name of the aggregate function is passed, then the types of its arguments.
Examples: %%CREATE TABLE t ( column1 AggregateFunction(uniq, UInt64), column2
AggregateFunction(anyIf, String, UInt8), column3
AggregateFunction(quantiles(0.5, 0.9), UInt64) ) ENGINE = ... %% This type of
column stores the state of an aggregate function. To get this type of value,
use aggregate functions with the 'State' suffix. Example: uniqState(UserID),
quantilesState(0.5, 0.9)(SendTiming) - in contrast to the corresponding 'uniq'
and 'quantiles' functions, these functions return the state, rather than the
prepared value. In other words, they return an AggregateFunction type value.
An AggregateFunction type value can't be output in Pretty formats. In other
formats, these types of values are output as implementation-specific binary
data. The AggregateFunction type values are not intended for output or saving
in a dump. The only useful thing you can do with AggregateFunction type values
is combine the states and get a result, which essentially means to finish
aggregation. Aggregate functions with the 'Merge' suffix are used for this
purpose. Example: uniqMerge(UserIDState), where UserIDState has the
AggregateFunction type. In other words, an aggregate function with the 'Merge'
suffix takes a set of states, combines them, and returns the result. As an
example, these two queries return the same result: %%SELECT uniq(UserID) FROM
table%% %%SELECT uniqMerge(state) FROM (SELECT uniqState(UserID) AS state FROM
table GROUP BY RegionID)%% There is an AggregatingMergeTree engine. Its job
during a merge is to combine the states of aggregate functions from different
table rows with the same primary key value. You can't use a normal INSERT to
insert a row in a table containing AggregateFunction columns, because you
can't explicitly define the AggregateFunction value. Instead, use INSERT
SELECT with '-State' aggregate functions for inserting data. With SELECT from
an AggregatingMergeTree table, use GROUP BY and aggregate functions with the
'-Merge' modifier in order to complete data aggregation. You can use
AggregatingMergeTree tables for incremental data aggregation, including for
aggregated materialized views. Example: Creating a materialized
AggregatingMergeTree view that tracks the 'test.visits' table: %% CREATE
MATERIALIZED VIEW test.basic ENGINE = AggregatingMergeTree(StartDate,
(CounterID, StartDate), 8192) AS SELECT CounterID, StartDate, sumState(Sign)
AS Visits, uniqState(UserID) AS Users FROM test.visits GROUP BY CounterID,
StartDate; %% Inserting data in the 'test.visits' table. Data will also be
inserted in the view, where it will be aggregated: %% INSERT INTO test.visits
... %% Performing SELECT from the view using GROUP BY to finish data
aggregation: %% SELECT StartDate, sumMerge(Visits) AS Visits, uniqMerge(Users)
AS Users FROM test.basic GROUP BY StartDate ORDER BY StartDate; %% You can
create a materialized view like this and assign a normal view to it that
finishes data aggregation. Note that in most cases, using AggregatingMergeTree
is not justified, since queries can be run efficiently enough on non-
aggregated data. ==Null== When writing to a Null table, data is ignored. When
reading from a Null table, the response is empty. However, you can create a
materialized view on a Null table, so the data written to the table will end
up in the view. ==View== Used for implementing views (for more information,
see the CREATE VIEW query). It does not store data, but only stores the
specified SELECT query. When reading from a table, it runs this query (and
deletes all unnecessary columns from the query). ==MaterializedView== Used for
implementing materialized views (for more information, see CREATE MATERIALIZED
VIEW). For storing data, it uses a different engine that was specified when
creating the view. When reading from a table, it just uses this engine.
==Set== A data set that is always in RAM. It is intended for use on the right
side of the IN operator (see the section "IN operators"). You can use INSERT
to insert data in the table. New elements will be added to the data set, while
duplicates will be ignored. But you can't perform SELECT from the table. The
only way to retrieve data is by using it in the right half of the IN operator.
Data is always located in RAM. For INSERT, the blocks of inserted data are
also written to the directory of tables on the disk. When starting the server,
this data is loaded to RAM. In other words, after restarting, the data remains
in place. For a rough server restart, the block of data on the disk might be
lost or damaged. In the latter case, you may need to manually delete the file
with damaged data. ==Join== A prepared data structure for JOIN that is always
located in RAM. %%Join(ANY|ALL, LEFT|INNER, k1[, k2, ...])%% Engine
parameters: ANY|ALL - strictness, and LEFT|INNER - the type. These parameters
are set without quotes and must match the JOIN that the table will be used
for. k1, k2, ... are the key columns from the USING clause that the join will
be made on. The table can't be used for GLOBAL JOINs. You can use INSERT to
add data to the table, similar to the Set engine. For ANY, data for duplicated
keys will be ignored. For ALL, it will be counted. You can't perform SELECT
directly from the table. The only way to retrieve data is to use it as the
"right-hand" table for JOIN. Storing data on the disk is the same as for the
Set engine. ==Buffer== Buffers the data to write in RAM, periodically flushing
it to another table. During the read operation, data is read from the buffer
and the other table simultaneously. %%Buffer(database, table, num_layers,
min_time, max_time, min_rows, max_rows, min_bytes, max_bytes)%% Engine
parameters: database, table - The table to flush data to. Instead of the
database name, you can use a constant expression that returns a string.
num_layers - The level of parallelism. Physically, the table will be
represented as 'num_layers' of independent buffers. The recommended value is
16. min_time, max_time, min_rows, max_rows, min_bytes, and max_bytes are
conditions for flushing data from the buffer. Data is flushed from the buffer
and written to the destination table if all the 'min' conditions or at least
one 'max' condition are met. min_time, max_time - Condition for the time in
seconds from the moment of the first write to the buffer. min_rows, max_rows -
Condition for the number of rows in the buffer. min_bytes, max_bytes -
Condition for the number of bytes in the buffer. During the write operation,
data is inserted to a 'num_layers' number of random buffers. Or, if the data
part to insert is large enough (greater than 'max_rows' or 'max_bytes'), it is
written directly to the destination table, omitting the buffer. The conditions
for flushing the data are calculated separately for each of the 'num_layers'
buffers. For example, if num_layers = 16 and max_bytes = 100000000, the
maximum RAM consumption is 1.6 GB. Example: %%CREATE TABLE merge.hits_buffer
AS merge.hits ENGINE = Buffer(merge, hits, 16, 10, 100, 10000, 1000000,
10000000, 100000000)%% Creating a 'merge.hits_buffer' table with the same
structure as 'merge.hits' and using the Buffer engine. When writing to this
table, data is buffered in RAM and later written to the 'merge.hits' table. 16
buffers are created. The data in each of them is flushed if either 100 seconds
have passed, or one million rows have been written, or 100 MB of data have
been written; or if simultaneously 10 seconds have passed and 10,000 rows and
10 MB of data have been written. For example, if just one row has been
written, after 100 seconds it will be flushed, no matter what. But if many
rows have been written, the data will be flushed sooner. When the server is
stopped, with DROP TABLE or DETACH TABLE, buffer data is also flushed to the
destination table. You can set empty strings in single quotation marks for the
database and table name. This indicates the absence of a destination table. In
this case, when the data flush conditions are reached, the buffer is simply
cleared. This may be useful for keeping a window of data in memory. When
reading from a Buffer table, data is processed both from the buffer and from
the destination table (if there is one). Note that the Buffer tables does not
support an index. In other words, data in the buffer is fully scanned, which
might be slow for large buffers. (For data in a subordinate table, the index
it supports will be used.) If the set of columns in the Buffer table doesn't
match the set of columns in a subordinate table, a subset of columns that
exist in both tables is inserted. If the types don't match for one of the
columns in the Buffer table and a subordinate table, an error message is
entered in the server log and the buffer is cleared. The same thing happens if
the subordinate table doesn't exist when the buffer is flushed. If you need to
run ALTER for a subordinate table and the Buffer table, we recommend first
deleting the Buffer table, running ALTER for the subordinate table, then
creating the Buffer table again. If the server is restarted abnormally, the
data in the buffer is lost. PREWHERE, FINAL and SAMPLE do not work correctly
for Buffer tables. These conditions are passed to the destination table, but
are not used for processing data in the buffer. Because of this, we recommend
only using the Buffer table for writing, while reading from the destination
table. When adding data to a Buffer, one of the buffers is locked. This causes
delays if a read operation is simultaneously being performed from the table.
Data that is inserted to a Buffer table may end up in the subordinate table in
a different order and in different blocks. Because of this, a Buffer table is
difficult to use for writing to a CollapsingMergeTree correctly. To avoid
problems, you can set 'num_layers' to 1. If the destination table is
replicated, some expected characteristics of replicated tables are lost when
writing to a Buffer table. The random changes to the order of rows and sizes
of data parts cause data deduplication to quit working, which means it is not
possible to have a reliable 'exactly once' write to replicated tables. Due to
these disadvantages, we can only recommend using a Buffer table in rare cases.
A Buffer table is used when too many INSERTs are received from a large number
of servers over a unit of time and data can't be buffered before insertion,
which means the INSERTs can't run fast enough. Note that it doesn't make sense
to insert data one row at a time, even for Buffer tables. This will only
produce a speed of a few thousand rows per second, while inserting larger
blocks of data can produce over a million rows per second (see the section
"Performance"). ==Data replication== ===ReplicatedMergeTree===
===ReplicatedCollapsingMergeTree=== ===ReplicatedAggregatingMergeTree===
===ReplicatedSummingMergeTree=== Replication is only supported for tables in
the MergeTree family. Replication works at the level of an individual table,
not the entire server. A server can store both replicated and non-replicated
tables at the same time. INSERT and ALTER are replicated (for more
information, see ALTER). Compressed data is replicated, not query texts. The
CREATE, DROP, ATTACH, DETACH, and RENAME queries are not replicated. In other
words, they belong to a single server. The CREATE TABLE query creates a new
replicatable table on the server where the query is run. If this table already
exists on other servers, it adds a new replica. The DROP TABLE query deletes
the replica located on the server where the query is run. The RENAME query
renames the table on one of the replicas. In other words, replicated tables
can have different names on different replicas. Replication is not related to
sharding in any way. Replication works independently on each shard.
Replication is an optional feature. To use replication, set the addresses of
the ZooKeeper cluster in the config file. Example: %% <zookeeper> <node
index="1"> <host>example1</host> <port>2181</port> </node> <node index="2">
<host>example2</host> <port>2181</port> </node> <node index="3">
<host>example3</host> <port>2181</port> </node> </zookeeper> %% Use ZooKeeper
version 3.4.5 or later. For example, the version in the Ubuntu Precise package
is too old. You can specify any existing ZooKeeper cluster - the system will
use a directory on it for its own data (the directory is specified when
creating a replicatable table). If ZooKeeper isn't set in the config file, you
can't create replicated tables, and any existing replicated tables will be
read-only. ZooKeeper isn't used for SELECT queries. In other words,
replication doesn't affect the productivity of SELECT queries - they work just
as fast as for non-replicated tables. For each INSERT query (more precisely,
for each inserted block of data; the INSERT query contains a single block, or
per block for every max_insert_block_size = 1048576 rows), approximately ten
entries are made in ZooKeeper in several transactions. This leads to slightly
longer latencies for INSERT compared to non-replicated tables. But if you
follow the recommendations to insert data in batches of no more than one
INSERT per second, it doesn't create any problems. The entire ClickHouse
cluster used for coordinating one ZooKeeper cluster has a total of several
hundred INSERTs per second. The throughput on data inserts (the number of rows
per second) is just as high as for non-replicated data. For very large
clusters, you can use different ZooKeeper clusters for different shards.
However, this hasn't proven necessary on the Yandex.Metrica cluster
(approximately 300 servers). Replication is asynchronous and multi-master.
INSERT queries (as well as ALTER) can be sent to any available server. Data is
inserted on this server, then sent to the other servers. Because it is
asynchronous, recently inserted data appears on the other replicas with some
latency. If a part of the replicas is not available, the data on them is
written when they become available. If a replica is available, the latency is
the amount of time it takes to transfer the block of compressed data over the
network. There are no quorum writes. You can't write data with confirmation
that it was received by more than one replica. If you write a batch of data to
one replica and the server with this data ceases to exist before the data has
time to get to the other replicas, this data will be lost. Each block of data
is written atomically. The INSERT query is divided into blocks up to
max_insert_block_size = 1048576 rows. In other words, if the INSERT query has
less than 1048576 rows, it is made atomically. Blocks of data are duplicated.
For multiple writes of the same data block (data blocks of the same size
containing the same rows in the same order), the block is only written once.
The reason for this is in case of network failures when the client application
doesn't know if the data was written to the DB, so the INSERT query can simply
be repeated. It doesn't matter which replica INSERTs were sent to with
identical data - INSERTs are idempotent. This only works for the last 100
blocks inserted in a table. During replication, only the source data to insert
is transferred over the network. Further data transformation (merging) is
coordinated and performed on all the replicas in the same way. This minimizes
network usage, which means that replication works well when replicas reside in
different datacenters. (Note that duplicating data in different datacenters is
the main goal of replication.) You can have any number of replicas of the same
data. Yandex.Metrica uses double replication in production. Each server uses
RAID-5 or RAID-6, and RAID-10 in some cases. This is a relatively reliable and
convenient solution. The system monitors data synchronicity on replicas and is
able to recover after a failure. Failover is automatic (for small differences
in data) or semi-automatic (when data differs too much, which may indicate a
configuration error). ===Creating replicated tables=== The 'Replicated' prefix
is added to the table engine name. For example, ReplicatedMergeTree. Two
parameters are also added in the beginning of the parameters list - the path
to the table in ZooKeeper, and the replica name in ZooKeeper. Example:
ReplicatedMergeTree(**'/clickhouse/tables/{layer}-{shard}/hits'**,
**'{replica}'**, EventDate, intHash32(UserID), (CounterID, EventDate,
intHash32(UserID), EventTime), 8192) As the example shows, these parameters
can contain substitutions in curly brackets. The substituted values are taken
from the 'macros' section of the config file. Example: %% <macros>
<layer>05</layer> <shard>02</shard>
<replica>example05-02-1.yandex.ru</replica> </macros> %% The path to the table
in ZooKeeper should be unique for each replicated table. Tables on different
shards should have different paths. In this case, the path consists of the
following parts: %%/clickhouse/tables/%% is the common prefix. We recommend
using exactly this one. %%{layer}-{shard}%% is the shard identifier. In this
example it consists of two parts, since the Yandex.Metrica cluster uses bi-
level sharding. For most tasks, you can leave just the {shard} substitution,
which will be expanded to the shard identifier. %%hits%% is the name of the
node for the table in ZooKeeper. It is a good idea to make it the same as the
table name. It is defined explicitly, because in contrast to the table name,
it doesn't change after a RENAME query. The replica name identifies different
replicas of the same table. You can use the server name for this, as in the
example. The name only needs to be unique within each shard. You can define
everything explicitly instead of using substitutions. This might be convenient
for testing and for configuring small clusters, but it is inconvenient when
working with large clusters. Run CREATE TABLE on each replica. This query
creates a new replicated table, or adds a new replica to an existing one. If
you add a new replica after the table already contains some data on other
replicas, the data will be copied from the other replicas to the new one after
running the query. In other words, the new replica syncs itself with the
others. To delete a replica, run DROP TABLE. However, only one replica is
deleted - the one that resides on the server where you run the query.
===Recovery after failures=== If ZooKeeper is unavailable when a server
starts, replicated tables switch to read-only mode. The system periodically
attempts to connect to ZooKeeper. If ZooKeeper is unavailable during an
INSERT, or an error occurs when interacting with ZooKeeper, an exception is
thrown. After connecting to ZooKeeper, the system checks whether the set of
data in the local file system matches the expected set of data (ZooKeeper
stores this information). If there are minor inconsistencies, the system
resolves them by syncing data with the replicas. If the system detects broken
data parts (with the wrong size of files) or unrecognized parts (parts written
to the file system but not recorded in ZooKeeper), it moves them to the
'detached' subdirectory (they are not deleted). Any missing parts are copied
from the replicas. Note that ClickHouse does not perform any destructive
actions such as automatically deleting a large amount of data. When the server
starts (or establishes a new session with ZooKeeper), it only checks the
quantity and sizes of all files. If the file sizes match but bytes have been
changed somewhere in the middle, this is not detected immediately, but only
when attempting to read the data for a SELECT query. The query throws an
exception about a non-matching checksum or size of a compressed block. In this
case, data parts are added to the verification queue and copied from the
replicas if necessary. If the local set of data differs too much from the
expected one, a safety mechanism is triggered. The server enters this in the
log and refuses to launch. The reason for this is that this case may indicate
a configuration error, such as if a replica on a shard was accidentally
configured like a replica on a different shard. However, the thresholds for
this mechanism are set fairly low, and this situation might occur during
normal failure recovery. In this case, data is restored semi-automatically -
by "pushing a button". To start recovery, create the node
/_path_to_table_/_replica_name_/flags/force_restore_data in ZooKeeper with any
content, and launch the server. On start, the server deletes this flag and
starts recovery. ===Recovery after complete data loss=== If all data and
metadata disappeared from one of the servers, follow these steps for recovery:
1\. Install ClickHouse on the server. Define substitutions correctly in the
config file that contains the shard identifier and replicas, if you use them.
2\. If you had unreplicated tables that must be manually duplicated on the
servers, copy their data from a replica (in the directory
/opt/clickhouse/data/_db_name_/_table_name_/). If replicated tables had
unreplicated parts, synchronize them as well. Note that these exist only if
you migrated from MergeTree to ReplicatedMergeTree (see "Converting from
MergeTree to ReplicatedMergeTree"). 3\. Copy table definitions located in
%%/opt/clickhouse/metadata/%% from a replica. If a shard or replica identifier
is defined explicitly in the table definitions, correct it so that it
corresponds to this replica. (Alternatively, launch the server and make all
the ATTACH TABLE queries that should have been in the .sql files in
%%/opt/clickhouse/metadata/%%.) 4\. Create the
/_path_to_table_/_replica_name_/flags/force_restore_data node in ZooKeeper
with any content, and launch the server (restart it if it is already running).
Data will be downloaded from replicas. An alternative recovery option is to
delete information about the lost replica from ZooKeeper (
/_path_to_table_/_replica_name_), then create the replica again as described
in "Creating replicated tables". There is no restriction on network bandwidth
during recovery. Keep this in mind if you are restoring many replicas at once.
===Converting from MergeTree to ReplicatedMergeTree=== From here on, we use
"MergeTree" to refer to all the table engines in the MergeTree family,
including ReplicatedMergeTree. If you had a MergeTree table that was manually
replicated, you can convert it to a replicatable table. You might need to do
this if you have already collected a large amount of data in a MergeTree table
and now you want to enable replication. There are two ways to do this: 1\.
Leave the old data "as is" without syncing it. To do this, rename the existing
MergeTree table, then create a ReplicatedMergeTree table with the old name. In
the directory with data for the new table
(/opt/clickhouse/data/_db_name_/_table_name_/), create the 'unreplicated'
subdirectory and move the data from the old table to it. Then restart the
server. For read requests, the replicated table will also read from the
'unreplicated' data set. The integrity of this data is not monitored in any
way. 2\. Add the old data to the set of replicatable data. If the data differs
on various replicas, first sync it, or delete this data on all the replicas
except one. Rename the existing MergeTree table, then create a
ReplicatedMergeTree table with the old name. Move the data from the old table
to the 'detached' subdirectory inside the directory with the new table data
(/opt/clickhouse/data/_db_name_/_table_name_/). Then run ALTER TABLE ATTACH
PART on one of the replicas to add these data parts to the working set. If
exactly the same parts exist on the other replicas, they are added to the
working set on them. If not, the parts are downloaded from the replica that
has them. ===Converting from ReplicatedMergeTree to MergeTree=== Create a
MergeTree table with a different name. Move all the data from the directory
with the ReplicatedMergeTree table data to the new table's data directory.
Then delete the ReplicatedMergeTree table and restart the server. If you want
to get rid of a ReplicatedMergeTree table without launching the server: \-
Delete the corresponding .sql file in the metadata directory
(%%/opt/clickhouse/metadata/%%). \- Delete the corresponding path in ZooKeeper
(/_path_to_table_/_replica_name_). After this, you can launch the server,
create a MergeTree table, move the data to its directory, and then restart the
server. ===Recovery when metadata in the ZooKeeper cluster is lost or
damaged=== If you lost ZooKeeper, you can save data by moving it to an
unreplicated table as described above. ==Resharding== %%ALTER TABLE t RESHARD
[COPY] [PARTITION partition] TO _cluster description_ USING _sharding key_%%
Query works only for Replicated tables and for Distributed tables that are
looking at Replicated tables. When executing, query first checks correctness
of query, sufficiency of free space on nodes and writes to ZooKeeper at some
path a task to to. Next work is done asynchronously. For using resharding, you
must specify path in ZooKeeper for task queue in configuration file: %%
<resharding> <task_queue_path>/clickhouse/task_queue</task_queue_path>
</resharding> %% When running %%ALTER TABLE t RESHARD%% query, node in
ZooKeeper is created if not exists. Cluster description is list of shards with
weights to distribute the data. Shard is specified as address of table in
ZooKeeper. Example: %%/clickhouse/tables/01-03/hits%% Relative weight of shard
(optional, default is 1) could be specified after %%WEIGHT%% keyword. Example:
%% ALTER TABLE merge.hits RESHARD PARTITION 201501 TO
'/clickhouse/tables/01-01/hits' WEIGHT 1, '/clickhouse/tables/01-02/hits'
WEIGHT 2, '/clickhouse/tables/01-03/hits' WEIGHT 1,
'/clickhouse/tables/01-04/hits' WEIGHT 1 USING UserID %% Sharding key
(%%UserID%% in example) has same semantic as for Distributed tables. You could
specify %%rand()%% as sharding key for random distribution of data. When query
is run, it checks: \- identity of table structure on all shards. \-
availability of free space on local node in amount of partition size in bytes,
with additional 10% reserve. \- availability of free space on all replicas of
all specified shards, except local replica, if exists, in amount of patition
size times ratio of shard weight to total weight of all shards, with
additional 10% reserve. Next, asynchronous processing of query is of following
steps: 1\. Split patition to parts on local node. It merges all parts forming
a partition and in the same time, splits them to several, according sharding
key. Result is placed to /reshard directory in table data directory. Source
parts doesn't modified and all process doesn't intervent table working data
set. 2\. Copying all parts to remote nodes (to each replica of corresponding
shard). 3\. Execution of queries %%ALTER TABLE t DROP PARTITION%% on local
node and %%ALTER TABLE t ATTACH PARTITION%% on all shards. Note: this
operation is not atomic. There are time point when user could see absence of
data. When %%COPY%% keyword is specified, source data is not removed. It is
suitable for copying data from one cluster to another with changing sharding
scheme in same time. 4\. Removing temporary data from local node. When having
multiple resharding queries, their tasks will be done sequentially. Query in
example is to reshard single partition. If you don't specify partition in
query, then tasks to reshard all partitions will be created. Example: %% ALTER
TABLE merge.hits RESHARD TO ... %% When resharding Distributed tables, each
shard will be resharded (corresponding query is sent to each shard). You could
reshard Distributed table to itself or to another table. Resharding is
intended for "old" data: in case when during job, resharded partition was
modified, task for that partition will be cancelled. On each server,
resharding is done in single thread. It is doing that way to not disturb
normal query processing. As of June 2016, resharding is in "beta" state: it
was tested only for small data sets - up to 5 TB.

# System tables

System tables are used for implementing part of the system's functionality,
and for providing access to information about how the system is working. You
can't delete a system table (but you can perform DETACH). System tables don't
have files with data on the disk or files with metadata. The server creates
all the system tables when it starts. System tables are read-only. System
tables are located in the 'system' database. ==system.one== This table
contains a single row with a single 'dummy' UInt8 column containing the value
0. This table is used if a SELECT query doesn't specify the FROM clause. This
is similar to the DUAL table found in other DBMSs. ==system.numbers== This
table contains a single UInt64 column named 'number' that contains almost all
the natural numbers starting from zero. You can use this table for tests, or
if you need to do a brute force search. Reads from this table are not
parallelized. ==system.numbers_mt== The same as 'system.numbers' but reads are
parallelized. The numbers can be returned in any order. Used for tests.
==system.tables== This table contains the String columns 'database', 'name',
and 'engine' and DateTime column metadata_modification_time. Each table that
the server knows about is entered in the 'system.tables' table. There is an
issue: table engines are specified without parameters. This system table is
used for implementing SHOW TABLES queries. ==system.databases== This table
contains a single String column called 'name' \- the name of a database. Each
database that the server knows about has a corresponding entry in the table.
This system table is used for implementing the SHOW DATABASES query.
==system.processes== This system table is used for implementing the SHOW
PROCESSLIST query. Columns: %% user String - Name of the user who made the
request. For distributed query processing, this is the user who helped the
requestor server send the query to this server, not the user who made the
distributed request on the requestor server. address String - The IP address
the request was made from. The same for distributed processing. elapsed
Float64 - The time in seconds since request execution started. rows_read
UInt64 - The number of rows read from the table. For distributed processing,
on the requestor server, this is the total for all remote servers. bytes_read
UInt64 - The number of uncompressed bytes read from the table. For distributed
processing, on the requestor server, this is the total for all remote servers.
total_rows_approx UInt64 - The approximation of the total number of rows that
should be read. For distributed processing, on the requestor server, this is
the total for all remote servers. It can be updated during request processing,
when new sources to process become known. memory_usage UInt64 - How much
memory the request uses. It might not include some types of dedicated memory.
query String - The query text. For INSERT, it doesn't include the data to
insert. query_id String - Query ID, if defined. %% ==system.events== Contains
information about the number of events that have occurred in the system. This
is used for profiling and monitoring purposes. Example: The number of
processed SELECT queries. Columns: 'event String' \- the event name, and
'value UInt64' \- the quantity. ==system.clusters== Contains information about
clusters available in the config file and the servers in them. Columns: %%
cluster String - Cluster name. shard_num UInt32 - Number of a shard in the
cluster, starting from 1. shard_weight UInt32 - Relative weight of a shard
when writing data. replica_num UInt32 - Number of a replica in the shard,
starting from 1. host_name String - Host name as specified in the config.
host_address String - Host's IP address obtained from DNS. port UInt16 - The
port used to access the server. user String - The username to use for
connecting to the server. %% ==system.columns== Contains information about the
columns in all tables. You can use this table to get information similar to
DESCRIBE TABLE, but for multiple tables at once. %% database String - Name of
the database the table is located in. table String - Table name. name String -
Column name. type String - Column type. default_type String - Expression type
(DEFAULT, MATERIALIZED, ALIAS) for the default value, or an empty string if it
is not defined. default_expression String - Expression for the default value,
or an empty string if it is not defined. %% ==system.dictionaries== Contains
information about external dictionaries. Columns: %% name String - Dictionary
name. type String - Dictionary type: Flat, Hashed, Cache. origin String - Path
to the config file where the dictionary is described. attribute.names
Array(String) - Array of attribute names provided by the dictionary.
attribute.types Array(String) - Corresponding array of attribute types
provided by the dictionary. has_hierarchy UInt8 - Whether the dictionary is
hierarchical. bytes_allocated UInt64 - The amount of RAM used by the
dictionary. hit_rate Float64 - For cache dictionaries, the percent of usage
for which the value was in the cache. element_count UInt64 - The number of
items stored in the dictionary. load_factor Float64 - The filled percentage of
the dictionary (for a hashed dictionary, it is the filled percentage of the
hash table). creation_time DateTime - Time spent for the creation or last
successful reload of the dictionary. last_exception String - Text of an error
that occurred when creating or reloading the dictionary, if the dictionary
couldn't be created. source String - Text describing the data source for the
dictionary. %% Note that the amount of memory used by the dictionary is not
proportional to the number of items stored in it. So for flat and cached
dictionaries, all the memory cells are pre-assigned, regardless of how full
the dictionary actually is. ==system.functions== Contains information about
normal and aggregate functions. Columns: %% name String - Function name.
is_aggregate UInt8 - Whether it is an aggregate function. %% ==system.merges==
Contains information about merges currently in process for tables in the
MergeTree family. Columns: %% database String - Name of the database the table
is located in. table String - Name of the table. elapsed Float64 - Time in
seconds since the merge started. progress Float64 - Percent of progress made,
from 0 to 1. num_parts UInt64 - Number of parts to merge. result_part_name
String - Name of the part that will be formed as the result of the merge.
total_size_bytes_compressed UInt64 - Total size of compressed data in the
parts being merged. total_size_marks UInt64 - Total number of marks in the
parts being merged. bytes_read_uncompressed UInt64 - Amount of bytes read,
decompressed. rows_read UInt64 - Number of rows read.
bytes_written_uncompressed UInt64 - Amount of bytes written, uncompressed.
rows_written UInt64 - Number of rows written. %% ==system.parts== Contains
information about parts of a table in the MergeTree family. Columns: %%
database String - Name of the database where the table that this part belongs
to is located. table String - Name of the table that this part belongs to.
engine String - Name of the table engine, without parameters. partition String
- Name of the partition, in the format YYYYMM. name String - Name of the part.
replicated UInt8 - Whether the part belongs to replicated data. active UInt8 -
Whether the part is used in a table, or is no longer needed and will be
deleted soon. Inactive parts remain after merging. marks UInt64 - Number of
marks - multiply by the index granularity (usually 8192) to get the
approximate number of rows in the part. bytes UInt64 - Number of bytes when
compressed. modification_time DateTime - Time the directory with the part was
modified. Usually corresponds to the part's creation time. remove_time
DateTime - For inactive parts only - the time when the part became inactive.
refcount UInt32 - The number of places where the part is used. A value greater
than 2 indicates that this part participates in queries or merges. %%
==system.replicas== Contains information and status for replicated tables
residing on the local server. This table can be used for monitoring. The table
contains a row for every Replicated* table. Example: %% SELECT * FROM
system.replicas WHERE table = 'visits' FORMAT Vertical Row 1: ────── database:
merge table: visits engine: ReplicatedCollapsingMergeTree is_leader: 1
is_readonly: 0 is_session_expired: 0 future_parts: 1 parts_to_check: 0
zookeeper_path: /clickhouse/tables/01-06/visits replica_name:
example01-06-1.yandex.ru replica_path:
/clickhouse/tables/01-06/visits/replicas/example01-06-1.yandex.ru
columns_version: 9 queue_size: 1 inserts_in_queue: 0 merges_in_queue: 1
log_max_index: 596273 log_pointer: 596274 total_replicas: 2 active_replicas: 2
%% Columns: %% database: Database name. table: Table name. engine: Table
engine name. is_leader: Whether the replica is the leader. Only one replica
can be the leader at a time. The leader is responsible for selecting
background merges to perform. Note that writes can be performed to any replica
that is available and has a session in ZK, regardless of whether it is a
leader. is_readonly: Whether the replica is in read-only mode. This mode is
turned on if the config doesn't have sections with ZK, if an unknown error
occurred when reinitializing sessions in ZK, and during session
reinitialization in ZK. is_session_expired: Whether the session with ZK has
expired. Basically the same as 'is_readonly'. future_parts: The number of data
parts that will appear as the result of INSERTs or merges that haven't been
done yet. parts_to_check: The number of data parts in the queue for
verification. A part is put in the verification queue if there is suspicion
that it might be damaged. zookeeper_path: Path to table data in ZK.
replica_name: Replica name in ZK. Different replicas of the same table have
different names. replica_path: Path to replica data in ZK. The same as
concatenating 'zookeeper_path/replicas/replica_path'. columns_version: Version
number of the table structure. Indicates how many times ALTER was performed.
If replicas have different versions, it means some replicas haven't made all
of the ALTERs yet. queue_size: Size of the queue for operations waiting to be
performed. Operations include inserting blocks of data, merges, and certain
other actions. It usually coincides with 'future_parts'. inserts_in_queue:
Number of inserts of blocks of data that need to be made. Insertions are
usually replicated fairly quickly. If this number is large, it means something
is wrong. merges_in_queue: The number of merges waiting to be made. Sometimes
merges are lengthy, so this value may be greater than one for a long time. The
next 4 columns have a non-zero value only where there is an active session
with ZK. log_max_index: Maximum entry number in the log of general activity.
log_pointer: Maximum entry number from the log of general activity that the
replica copied to its queue for execution, plus one. If log_pointer is much
smaller than log_max_index, something is wrong. total_replicas: The total
number of known replicas of this table. active_replicas: The number of
replicas of this table that have a session in ZK (i.e., the number of
functioning replicas). %% If you request all the columns, the table may work a
bit slowly, since several reads from ZK are made for each row. If you don't
request the last 4 columns (log_max_index, log_pointer, total_replicas,
active_replicas), the table works quickly. For example, you can check that
everything is working correctly like this: %% SELECT database, table,
is_leader, is_readonly, is_session_expired, future_parts, parts_to_check,
columns_version, queue_size, inserts_in_queue, merges_in_queue, log_max_index,
log_pointer, total_replicas, active_replicas FROM system.replicas WHERE
is_readonly OR is_session_expired OR future_parts > 20 OR parts_to_check > 10
OR queue_size > 20 OR inserts_in_queue > 10 OR log_max_index - log_pointer >
10 OR total_replicas < 2 OR active_replicas < total_replicas %% If this query
doesn't return anything, it means that everything is fine. ==system.settings==
Contains information about settings that are currently in use (i.e. used for
executing the query you are using to read from the system.settings table).
Columns: %% name String - Setting name. value String - Setting value. changed
UInt8 - Whether the setting was explicitly defined in the config or explicitly
changed. %% Example: %% SELECT * FROM system.settings WHERE changed
┌─name───────────────────┬─value───────┬─changed─┐ │ max_threads │ 8 │ 1 │ │
use_uncompressed_cache │ 0 │ 1 │ │ load_balancing │ random │ 1 │ │
max_memory_usage │ 10000000000 │ 1 │
└────────────────────────┴─────────────┴─────────┘ %% ==system.zookeeper==
Allows reading data from the ZooKeeper cluster defined in the config. The
query must have a 'path' equality condition in the WHERE clause. This is the
path in ZooKeeper for the children that you want to get data for. Query SELECT
* FROM system.zookeeper WHERE path = '/clickhouse' outputs data for all
children on the /clickhouse node. To output data for all root nodes, write
path = '/'. If the path specified in 'path' doesn't exist, an exception will
be thrown. Columns: %% name String - Name of the node. path String - Path to
the node. value String - Value of the node. dataLength Int32 - Size of the
value. numChildren Int32 - Number of children. czxid Int64 - ID of the
transaction that created the node. mzxid Int64 - ID of the transaction that
last changed the node. pzxid Int64 - ID of the transaction that last added or
removed children. ctime DateTime - Time of node creation. mtime DateTime -
Time of the last node modification. version Int32 - Node version - the number
of times the node was changed. cversion Int32 - Number of added or removed
children. aversion Int32 - Number of changes to ACL. ephemeralOwner Int64 -
For ephemeral nodes, the ID of the session that owns this node. %% Example: %%
SELECT * FROM system.zookeeper WHERE path =
'/clickhouse/tables/01-08/visits/replicas' FORMAT Vertical Row 1: ────── name:
example01-08-1.yandex.ru value: czxid: 932998691229 mzxid: 932998691229 ctime:
2015-03-27 16:49:51 mtime: 2015-03-27 16:49:51 version: 0 cversion: 47
aversion: 0 ephemeralOwner: 0 dataLength: 0 numChildren: 7 pzxid: 987021031383
path: /clickhouse/tables/01-08/visits/replicas Row 2: ────── name:
example01-08-2.yandex.ru value: czxid: 933002738135 mzxid: 933002738135 ctime:
2015-03-27 16:57:01 mtime: 2015-03-27 16:57:01 version: 0 cversion: 37
aversion: 0 ephemeralOwner: 0 dataLength: 0 numChildren: 7 pzxid: 987021252247
path: /clickhouse/tables/01-08/visits/replicas %%

# Table functions

Table functions can be specified in the FROM clause instead of the database
and table names. Table functions can only be used if 'readonly' is not set.
Table functions aren't related to other functions. ==merge== %%merge(db_name,
'tables_regexp')%% creates a temporary Merge table. For more information, see
the section "Table engines, Merge". The table structure is taken from the
first table encountered that matches the regular expression. ==remote==
%%remote('addresses_expr', db, table[, 'user'[, 'password']])%% or
%%remote('addresses_expr', db.table[, 'user'[, 'password']])%% \- Allows
accessing a remote server without creating a Distributed table.
%%addresses_expr%% - An expression that generates addresses of remote servers.
This may be just one server address. The server address is host:port, or just
the host. The host can be specified as the server name, or as the IPv4 or IPv6
address. An IPv6 address is specified in square brackets. The port is the TCP
port on the remote server. If the port is omitted, it uses %%tcp_port%% from
the server's config file (by default, 9000). Note: As an exception, when
specifying an IPv6 address, the port is required. Examples: %% example01-01-1
example01-01-1:9000 localhost 127.0.0.1 [::]:9000 [2a02:6b8:0:1111::11]:9000%%
Multiple addresses can be comma-separated. In this case, the query goes to all
the specified addresses (like to shards with different data) and uses
distributed processing. Example: %%example01-01-1,example01-02-1%% Part of the
expression can be specified in curly brackets. The previous example can be
written as follows: %%example01-0{1,2}-1%% Curly brackets can contain a range
of numbers separated by two dots (non-negative integers). In this case, the
range is expanded to a set of values that generate shard addresses. If the
first number starts with zero, the values are formed with the same zero
alignment. The previous example can be written as follows:
%%example01-{01..02}-1%% If you have multiple pairs of curly brackets, it
generates the direct product of the corresponding sets. Addresses and
fragments in curly brackets can be separated by the pipe (|) symbol. In this
case, the corresponding sets of addresses are interpreted as replicas, and the
query will be sent to the first healthy replica. The replicas are evaluated in
the order currently set in the 'load_balancing' setting. Example:
%%example01-{01..02}-{1|2}%% This example specifies two shards that each have
two replicas. The number of addresses generated is limited by a constant.
Right now this is 1000 addresses. Using the 'remote' table function is less
optimal than creating a Distributed table, because in this case, the server
connection is re-established for every request. In addition, if host names are
set, the names are resolved, and errors are not counted when working with
various replicas. When processing a large number of queries, always create the
Distributed table ahead of time, and don't use the 'remote' table function.
The 'remote' table function can be useful in the following cases: \- Accessing
a specific server for data comparison, debugging, and testing. \- Queries
between various ClickHouse clusters for research purposes. \- Infrequent
distributed requests that are made manually. \- Distributed requests where the
set of servers is re-defined each time. The username can be omitted. In this
case, the 'default' username is used. The password can be omitted. In this
case, an empty password is used.

# Formats

The format determines how data is given (written by server as output) to you
after SELECTs, and how it is accepted (read by server as input) for INSERTs.
==Native== The most efficient format. Data is written and read by blocks in
binary format. For each block, the number of rows, number of columns, column
names and types, and parts of columns in this block are recorded one after
another. In other words, this format is "columnar" \- it doesn't convert
columns to rows. This is the format used in the native interface for
interaction between servers, for using the command-line client, and for C++
clients. You can use this format to quickly generate dumps that can only be
read by the ClickHouse DBMS. It doesn't make sense to work with this format
yourself. ==TabSeparated== In TabSeparated format, data is written by row.
Each row contains values separated by tabs. Each value is follow by a tab,
except the last value in the row, which is followed by a line break. Strictly
Unix line breaks are assumed everywhere. The last row also must contain a line
break at the end. Values are written in text format, without enclosing
quotation marks, and with special characters escaped. Numbers are written in
decimal form. Numbers may contain an extra "+" symbol at the beginning (but it
is not recorded during an output). Non-negative numbers can't contain the
negative sign. When parsing, it is allowed to parse an empty string as a zero,
or (for signed types) a string consisting of just a minus sign as a zero.
Numbers that do not fit into the corresponding data type may be parsed as a
different number, without an error message. Floating-point numbers are
formatted in decimal form. The dot is used as the decimal separator.
Exponential entries are supported, as are 'inf', '+inf', '-inf', and 'nan'. An
entry of floating-point numbers may begin or end with a decimal point. During
formatting, accuracy may be lost on floating-point numbers. During parsing, a
result is not necessarily the nearest machine-representable number. Dates are
formatted in YYYY-MM-DD format and parsed in the same format, but with any
characters as separators. DateTimes are formatted in the format YYYY-MM-DD
hh:mm:ss and parsed in the same format, but with any characters as separators.
This all occurs in the system time zone at the time the client or server
starts (depending on which one formats data). For DateTimes, daylight saving
time is not specified. So if a dump has times during daylight saving time, the
dump does not unequivocally match the data, and parsing will select one of the
two times. During a parsing operation, incorrect dates and dates with times
can be parsed with natural overflow or as null dates and times, without an
error message. As an exception, parsing DateTime is also supported in Unix
timestamp format, if it consists of exactly 10 decimal digits. The result is
not time zone-dependent. The formats YYYY-MM-DD hh:mm:ss and NNNNNNNNNN are
differentiated automatically. Strings are parsed and formatted with backslash-
escaped special characters. The following escape sequences are used while
formatting: %%\b%%, %%\f%%, %%\r,%% %%\n%%, %%\t%%, %%\0%%, %%\'%%, and
%%\\\%%. For parsing, also supported %%\a%%, %%\v%% and \x_HH_ (hex escape
sequence) and any sequences of the type \_c_ where _c_ is any character (these
sequences are converted to _c_). This means that parsing supports formats
where a line break can be written as %%\n%% or as %%\%% and a line break. For
example, the string 'Hello world' with a line break between the words instead
of a space can be retrieved in any of the following variations:
%%Hello\nworld%% %%Hello\ world%% The second variant is supported because
MySQL uses it when writing tab-separated dumps. Only a small set of symbols
are escaped. You can easily stumble onto a string value that your terminal
will ruin in output. Minimum set of symbols that you must escape in
TabSeparated format is tab, newline (LF) and backslash. Arrays are formatted
as a list of comma-separated values in square brackets. Number items in the
array are formatted as normally, but dates, dates with times, and strings are
formatted in single quotes with the same escaping rules as above. The
TabSeparated format is convenient for processing data using custom programs
and scripts. It is used by default in the HTTP interface, and in the command-
line client's batch mode. This format also allows transferring data between
different DBMSs. For example, you can get a dump from MySQL and upload it to
ClickHouse, or vice versa. The TabSeparated format supports outputting total
values (when using WITH TOTALS) and extreme values (when 'extremes' is set to
1). In these cases, the total values and extremes are output after the main
data. The main result, total values, and extremes are separated from each
other by an empty line. Example: %%SELECT EventDate, count() AS c FROM
test.hits GROUP BY EventDate WITH TOTALS ORDER BY EventDate FORMAT
TabSeparated%% %% 2014-03-17 1406958 2014-03-18 1383658 2014-03-19 1405797
2014-03-20 1353623 2014-03-21 1245779 2014-03-22 1031592 2014-03-23 1046491
0000-00-00 8873898 2014-03-17 1031592 2014-03-23 1406958 %%
==TabSeparatedWithNames== Differs from the TabSeparated format in that the
column names are output in the first row. For parsing, the first row is
completely ignored. You can't use column names to determine their position or
to check their correctness. (Support for using header while parsing could be
added in future.) ==TabSeparatedWithNamesAndTypes== Differs from the
TabSeparated format in that the column names are output to the first row,
while the column types are in the second row. For parsing, the first and
second rows are completely ignored. ==TabSeparatedRaw== Differs from the
TabSeparated format in that the rows are formatted without escaping. This
format is only appropriate for outputting a query result, but not for parsing
data to insert into a table. ==BlockTabSeparated== Data is not written by row,
but by column and block. Each block consists of parts of columns, each of
which is written on a separate line. The values are tab-separated. The last
value in a column part is followed by a line break instead of a tab. Blocks
are separated by a double line break. The rest of the rules are the same as in
the TabSeparated format. This format is only appropriate for outputting a
query result, not for parsing. ==CSV== Comma separated values
([RFC](https://tools.ietf.org/html/rfc4180)). String values are output in
double quotes. Double quote inside a string is output as two consecutive
double quotes. That's all escaping rules. Date and DateTime values are output
in double quotes. Numbers are output without quotes. Fields are delimited by
commas. Rows are delimited by unix newlines (LF). Arrays are output in
following way: first, array are serialized to String (as in TabSeparated or
Values formats), and then the String value are output in double quotes. Tuples
are narrowed and serialized as separate columns. During parsing, values could
be enclosed or not enclosed in quotes. Supported both single and double
quotes. In particular, Strings could be represented without quotes - in that
case, they are parsed up to comma or newline (CR or LF). Contrary to RFC, in
case of parsing strings without quotes, leading and trailing spaces and tabs
are ignored. As line delimiter, both Unix (LF), Windows (CR LF) or Mac OS
Classic (LF CR) variants are supported. CSV format supports output of totals
and extremes similar to TabSeparated format. ==CSVWithNames== Also contains
header, similar to TabSeparatedWithNames. ==RowBinary== Writes data by row in
binary format. Rows and values are listed consecutively, without separators.
This format is less efficient than the Native format, since it is row-based.
Numbers is written in little endian, fixed width. For example, UInt64 takes 8
bytes. DateTime is written as UInt32 with unix timestamp value. Date is
written as UInt16 with number of days since 1970-01-01 in value. String is
written as length in varint (unsigned
[LEB128](https://en.wikipedia.org/wiki/LEB128)) format and then bytes of
string. FixedString is written as just its bytes. Array is written as length
in varint (unsigned [LEB128](https://en.wikipedia.org/wiki/LEB128)) format and
then all elements, contiguously. ==Pretty== Writes data as Unicode-art tables,
also using ANSI-escape sequences for setting colors in the terminal. A full
grid of the table is drawn, and each row occupies two lines in the terminal.
Each result block is output as a separate table. This is necessary so that
blocks can be output without buffering results (buffering would be necessary
in order to pre-calculate the visible width of all the values). To avoid
dumping too much data to the terminal, only the first 10,000 rows are printed.
If the number of rows is greater than or equal to 10,000, the message "Showed
first 10,000" is printed. This format is only appropriate for outputting a
query result, not for parsing. The Pretty format supports outputting total
values (when using WITH TOTALS) and extremes (when 'extremes' is set to 1). In
these cases, total values and extreme values are output after the main data,
in separate tables. Example (shown for the PrettyCompact format): %%SELECT
EventDate, count() AS c FROM test.hits GROUP BY EventDate WITH TOTALS ORDER BY
EventDate FORMAT PrettyCompact%% %% ┌──EventDate─┬───────c─┐ │ 2014-03-17 │
1406958 │ │ 2014-03-18 │ 1383658 │ │ 2014-03-19 │ 1405797 │ │ 2014-03-20 │
1353623 │ │ 2014-03-21 │ 1245779 │ │ 2014-03-22 │ 1031592 │ │ 2014-03-23 │
1046491 │ └────────────┴─────────┘ Totals: ┌──EventDate─┬───────c─┐ │
0000-00-00 │ 8873898 │ └────────────┴─────────┘ Extremes:
┌──EventDate─┬───────c─┐ │ 2014-03-17 │ 1031592 │ │ 2014-03-23 │ 1406958 │
└────────────┴─────────┘ %% ==PrettyCompact== Differs from Pretty in that the
grid is drawn between rows and the result is more compact. This format is used
by default in the command-line client in interactive mode.
==PrettyCompactMonoBlock== Differs from PrettyCompact in that up to 10,000
rows are buffered, then output as a single table, not by blocks.
==PrettySpace== Differs from PrettyCompact in that whitespace (space
characters) is used instead of the grid. ==PrettyNoEscapes== Differs from
Pretty in that ANSI-escape sequences aren't used. This is necessary for
displaying this format in a browser, as well as for using the 'watch' command-
line utility. Example: %%watch -n1 "clickhouse-client --query='SELECT * FROM
system.events FORMAT PrettyCompactNoEscapes'"%% You can use the HTTP interface
for displaying in the browser. ==PrettyCompactNoEscapes== The same.
==PrettySpaceNoEscapes== The same. ==Vertical== Prints each value on a
separate line with the column name specified. This format is convenient for
printing just one or a few rows, if each row consists of a large number of
columns. This format is only appropriate for outputting a query result, not
for parsing. ==Values== Prints every row in parentheses. Rows are separated by
commas. There is no comma after the last row. The values inside the
parentheses are also comma-separated. Numbers are output in decimal format
without quotes. Arrays are output in square brackets. Strings, dates, and
dates with times are output in quotes. Escaping rules and parsing are same as
in the TabSeparated format. During formatting, extra spaces aren't inserted,
but during parsing, they are allowed and skipped (except for spaces inside
array values, which are not allowed). Minimum set of symbols that you must
escape in Values format is single quote and backslash. This is the format that
is used in INSERT INTO t VALUES ... But you can also use it for query result.
==JSON== Outputs data in JSON format. Besides data tables, it also outputs
column names and types, along with some additional information - the total
number of output rows, and the number of rows that could have been output if
there weren't a LIMIT. Example: %%SELECT SearchPhrase, count() AS c FROM
test.hits GROUP BY SearchPhrase WITH TOTALS ORDER BY c DESC LIMIT 5 FORMAT
JSON%% %% { "meta": [ { "name": "SearchPhrase", "type": "String" }, { "name":
"c", "type": "UInt64" } ], "data": [ { "SearchPhrase": "", "c": "8267016" }, {
"SearchPhrase": "bath interiors", "c": "2166" }, { "SearchPhrase": "yandex",
"c": "1655" }, { "SearchPhrase": "spring 2014 fashion", "c": "1549" }, {
"SearchPhrase": "freeform photo", "c": "1480" } ], "totals": { "SearchPhrase":
"", "c": "8873898" }, "extremes": { "min": { "SearchPhrase": "", "c": "1480"
}, "max": { "SearchPhrase": "", "c": "8267016" } }, "rows": 5,
"rows_before_limit_at_least": 141137 } %% JSON is compatible with JavaScript.
For this purpose, certain symbols are additionally escaped: the forward slash
%%/%% is escaped as %%\/%%; alternative line breaks %%U+2028%% and %%U+2029%%,
which don't work in some browsers, are escaped as \u_XXXX_-sequences. ASCII
control characters are escaped: backspace, form feed, line feed, carriage
return, and horizontal tab as %%\b%%, %%\f%%, %%\n%%, %%\r%%, and %%\t%%
respectively, along with the rest of the bytes from the range 00-1F using
\u_XXXX_-sequences. Invalid UTF-8 sequences are changed to the replacement
character %%�%% and, thus, the output text will consist of valid UTF-8
sequences. UInt64 and Int64 numbers are output in double quotes for
compatibility with JavaScript. %%rows%% - The total number of output rows.
%%rows_before_limit_at_least%% - The minimal number of rows there would have
been without a LIMIT. Output only if the query contains LIMIT. If the query
contains GROUP BY, %%rows_before_limit_at_least%% is the exact number of rows
there would have been without a LIMIT. %%totals%% - Total values (when using
%%WITH TOTALS%%). %%extremes%% - Extreme values (when %%extremes%% is set to
1). This format is only appropriate for outputting a query result, not for
parsing. See JSONEachRow format for INSERT queries. ==JSONCompact== Differs
from JSON only in that data rows are output in arrays, not in objects.
Example: %% { "meta": [ { "name": "SearchPhrase", "type": "String" }, {
"name": "c", "type": "UInt64" } ], "data": [ ["", "8267016"], ["bath
interiors", "2166"], ["yandex", "1655"], ["spring 2014 fashion", "1549"],
["freeform photo", "1480"] ], "totals": ["","8873898"], "extremes": { "min":
["","1480"], "max": ["","8267016"] }, "rows": 5, "rows_before_limit_at_least":
141137 } %% This format is only appropriate for outputting a query result, not
for parsing. See JSONEachRow format for INSERT queries. ==JSONEachRow== If put
in SELECT query, displays data in newline delimited JSON (JSON objects
separated by \\\n character) format. If put in INSERT query, expects this kind
of data as input. %% {"SearchPhrase":"","count()":"8267016"}
{"SearchPhrase":"bathroom interior","count()":"2166"}
{"SearchPhrase":"yandex","count()":"1655"} {"SearchPhrase":"spring 2014
fashion","count()":"1549"} {"SearchPhrase":"free-form photo","count()":"1480"}
{"SearchPhrase":"Angelina Jolie","count()":"1245"}
{"SearchPhrase":"omsk","count()":"1112"} {"SearchPhrase":"photos of dog
breeds","count()":"1091"} {"SearchPhrase":"curtain design","count()":"1064"}
{"SearchPhrase":"baku","count()":"1000"} %% Unlike JSON format, there are no
replacements of invalid UTF-8 sequences. There can be arbitrary amount of
bytes in a line. This is done in order to avoid data loss during formatting.
Values are displayed analogous to JSON format. In INSERT queries JSON data can
be supplied with arbitrary order of columns (JSON key-value pairs). It is also
possible to omit values in which case the default value of the column is
inserted. N.B. when using JSONEachRow format, complex default values are not
supported, so when omitting a column its value will be zeros or empty string
depending on its type. Space characters between JSON objects are skipped.
Between objects there can be a comma which is ignored. Newline character is
not a mandatory separator for objects. ==TSKV== Similar to TabSeparated, but
displays data in %%name=value%% format. Names are displayed just as in
TabSeparated. Additionally, a %%=%% symbol is displayed. %% SearchPhrase=
count()=8267016 SearchPhrase=bathroom interior count()=2166
SearchPhrase=yandex count()=1655 SearchPhrase=spring 2014 fashion count()=1549
SearchPhrase=free-form photo count()=1480 SearchPhrase=Angelina Jolie
count()=1245 SearchPhrase=omsk count()=1112 SearchPhrase=photos of dog breeds
count()=1091 SearchPhrase=curtain design count()=1064 SearchPhrase=baku
count()=1000 %% In case of many small columns this format is obviously not
effective and there usually is no reason to use it. This format is supported
because it is used for some cases in Yandex. Format is supported both for
input and output. In INSERT queries data can be supplied with arbitrary order
of columns. It is also possible to omit values in which case the default value
of the column is inserted. N.B. when using TSKV format, complex default values
are not supported, so when omitting a column its value will be zeros or empty
string depending on its type. ==XML== XML format is supported only for
displaying data, not for INSERTS. Example: %% <?xml version='1.0'
encoding='UTF-8' ?> <result> <meta> <columns> <column>
<name>SearchPhrase</name> <type>String</type> </column> <column>
<name>count()</name> <type>UInt64</type> </column> </columns> </meta> <data>
<row> <SearchPhrase></SearchPhrase> <field>8267016</field> </row> <row>
<SearchPhrase>bathroom interior</SearchPhrase> <field>2166</field> </row>
<row> <SearchPhrase>yandex> <field>1655</field> </row> <row>
<SearchPhrase>spring 2014 fashion</SearchPhrase> <field>1549</field> </row>
<row> <SearchPhrase>free-form photo</SearchPhrase> <field>1480</field> </row>
<row> <SearchPhrase>Angelina Jolie</SearchPhrase> <field>1245</field> </row>
<row> <SearchPhrase>omsk</SearchPhrase> <field>1112</field> </row> <row>
<SearchPhrase>photos of dog breeds</SearchPhrase> <field>1091</field> </row>
<row> <SearchPhrase>curtain design</SearchPhrase> <field>1064</field> </row>
<row> <SearchPhrase>baku</SearchPhrase> <field>1000</field> </row> </data>
<rows>10</rows>
<rows_before_limit_at_least>141137</rows_before_limit_at_least> </result> %%
If name of a column contains some unacceptable character, field is used as a
name. In other aspects XML uses JSON structure. As in case of JSON, invalid
UTF-8 sequences are replaced by replacement character � so displayed text will
only contain valid UTF-8 sequences. In string values %%<%% and %%&%% are
displayed as %%&lt;%% and %%&amp;%%. Arrays are displayed as
%%<array><elem>Hello</elem><elem>World</elem>...</array>%%, and tuples as
%%<tuple><elem>Hello</elem><elem>World</elem>...</tuple>%%. ==Null== Nothing
is output. However, the query is processed, and when using the command-line
client, data is transmitted to the client. This is used for tests, including
productivity testing. Obviously, this format is only appropriate for
outputting a query result, not for parsing.

# Data types

==UInt8, UInt16, UInt32, UInt64, Int8, Int16, Int32, Int64== Fixed-length
integers, with or without a sign. ==Float32, Float64== Floating-point numbers
are just like 'float' and 'double' in the C language. In contrast to standard
SQL, floating-point numbers support 'inf', '-inf', and even 'nan's. See the
notes on sorting nans in "ORDER BY clause". We do not recommend storing
floating-point numbers in tables. ==String== Strings of an arbitrary length.
The length is not limited. The value can contain an arbitrary set of bytes,
including null bytes. The String type replaces the types VARCHAR, BLOB, CLOB,
and others from other DBMSs. ===Encodings=== ClickHouse doesn't have the
concept of encodings. Strings can contain an arbitrary set of bytes, which are
stored and output as-is. If you need to store texts, we recommend using UTF-8
encoding. At the very least, if your terminal uses UTF-8 (as recommended), you
can read and write your values without making conversions. Similarly, certain
functions for working with strings have separate variations that work under
the assumption that the string contains a set of bytes representing a UTF-8
encoded text. For example, the 'length' function calculates the string length
in bytes, while the 'lengthUTF8' function calculates the string length in
Unicode code points, assuming that the value is UTF-8 encoded.
==FixedString(N)== A fixed-length string of N bytes (not characters or code
points). N must be a strictly positive natural number. When server reads a
string (as an input passed in INSERT query, for example) that contains fewer
bytes, the string is padded to N bytes by appending null bytes at the right.
When server reads a string that contains more bytes, an error message is
returned. When server writes a string (as an output of SELECT query, for
example), null bytes are not trimmed off of the end of the string, but are
output. Note that this behavior differs from MySQL behavior for the CHAR type
(where strings are padded with spaces, and the spaces are removed for output).
Fewer functions can work with the FixedString(N) type than with String, so it
is less convenient to use. ==Date== A date. Stored in two bytes as the number
of days since 1970-01-01 (unsigned). Allows storing values from just after the
beginning of the Unix Epoch to the upper threshold defined by a constant at
the compilation stage (currently, this is until the year 2038, but it may be
expanded to 2106). The minimum value is output as 0000-00-00. The date is
stored without the time zone. ==DateTime== Date with time. Stored in four
bytes as a Unix timestamp (unsigned). Allows storing values in the same range
as for the Date type. The minimal value is output as 0000-00-00 00:00:00. The
time is stored with accuracy up to one second (without leap seconds). ===Time
zones=== The date with time is converted from text (divided into component
parts) to binary and back, using the system's time zone at the time the client
or server starts. In text format, information about daylight savings is lost.
Supports only those time zones that never had the time differ from UTC for a
partial number of hours (without leap seconds) over the entire time range you
will be working with. So when working with a textual date (for example, when
saving text dumps), keep in mind that there may be ambiguity during changes
for daylight savings time, and there may be problems matching data if the time
zone changed. ==Enum== Enum8 or Enum16. A set of enumerated string values that
are stored as Int8 or Int16. Example: %%Enum8('hello' = 1, 'world' = 2)%% \-
This data type has two possible values - 'hello' and 'world'. The numeric
values must be within -128..127 for %%Enum8%% and -32768..32767 for
%%Enum16%%. Every member of the enum must also have different numbers. The
empty string is a valid value. The numbers do not need to be sequential and
can be in any order. The order does not matter. In memory, the data is stored
in the same way as the numeric types %%Int8%% and %%Int16%%. When reading in
text format, the string is read and the corresponding numeric value is looked
up. An exception will be thrown if it is not found. When writing in text
format, the stored number is looked up and the corresponding string is written
out. An exception will be thrown if the number does not correspond to a known
value. In binary format, the information is saved in the same way as %%Int8%%
and %%Int16%%. The implicit default value for an Enum is the value having the
smallest numeric value. In %%ORDER BY%%, %%GROUP BY%%, %%IN%%, %%DISTINCT%%,
etc. Enums behave like the numeric value. e.g. they will be sorted by the
numeric value in an %%ORDER BY%%. Equality and comparison operators behave
like they do on the underlying numeric value. Enum values cannot be compared
to numbers, they must be compared to a string. If the string compared to is
not a valid value for the Enum, an exception will be thrown. The %%IN%%
operator is supported with the Enum on the left hand side and a set of strings
on the right hand side. Most numeric and string operations are not defined for
Enum values, e.g. adding a number to an Enum or concatenating a string to an
Enum. However, the %%toString%% function can be used to convert the Enum to
its string value. Enum values are also convertible to numeric types using the
%%to_T_%% function where _T_ is a numeric type. When T corresponds to the
enum's underlying numeric type, this conversion is zero-cost. It is possible
to add new members to the Enum using ALTER. If the only change is to the set
of values, the operation will be almost instant. It is also possible to remove
members of the Enum using ALTER. Removing members is only safe if the removed
value has never been used in the table. As a safeguard, changing the numeric
value of a previously defined Enum member will throw an exception. Using
ALTER, it is possible to change an %%Enum8%% to an %%Enum16%% or vice versa -
just like changing an %%Int8%% to %%Int16%%. ==Array(T)== Array of T-type
items. The T type can be any type, including an array. We don't recommend
using multidimensional arrays, because they are not well supported (for
example, you can't store multidimensional arrays in any tables except Memory
tables). ==Tuple(T1, T2, ...)== Tuples can't be written to tables (other than
Memory tables). They are used for temporary column grouping. Columns can be
grouped when an IN expression is used in a query, and for specifying certain
formal parameters of lambda functions. For more information, see "IN
operators" and "Higher order functions". Tuples can be output as the result of
running a query. In this case, for text formats other than JSON*, values are
comma-separated in brackets. In JSON* formats, tuples are output as arrays (in
square brackets). ==Nested data structures== ==Nested(Name1 Type1, Name2
Type2, ...)== A nested data structure is like a nested table. The parameters
of a nested data structure - the column names and types - are specified the
same way as in a CREATE query. Each table row can correspond to any number of
rows in a nested data structure. Example: %% CREATE TABLE test.visits (
CounterID UInt32, StartDate Date, Sign Int8, IsNew UInt8, VisitID UInt64,
UserID UInt64, ... Goals Nested ( ID UInt32, Serial UInt32, EventTime
DateTime, Price Int64, OrderID String, CurrencyID UInt32 ), ... ) ENGINE =
CollapsingMergeTree(StartDate, intHash32(UserID), (CounterID, StartDate,
intHash32(UserID), VisitID), 8192, Sign) %% This example declares the 'Goals'
nested data structure, which contains data about conversions (goals reached).
Each row in the 'visits' table can correspond to zero or any number of
conversions. Only a single nesting level is supported. In most cases, when
working with a nested data structure, its individual columns are specified. To
do this, the column names are separated by a dot. These columns make up an
array of matching types. All the column arrays of a single nested data
structure have the same length. Example: %% SELECT Goals.ID, Goals.EventTime
FROM test.visits WHERE CounterID = 101500 AND length(Goals.ID) < 5 LIMIT 10 ┌─
Goals.ID───────────────────────┬─Goals.EventTime──────────────────────────────
─────────────────────────────────────────────┐ │ [1073752,591325,591325] │
['2014-03-17 16:38:10','2014-03-17 16:38:48','2014-03-17 16:42:27'] │ │
[1073752] │ ['2014-03-17 00:28:25'] │ │ [1073752] │ ['2014-03-17 10:46:20'] │
│ [1073752,591325,591325,591325] │ ['2014-03-17 13:59:20','2014-03-17
22:17:55','2014-03-17 22:18:07','2014-03-17 22:18:51'] │ │ [] │ [] │ │
[1073752,591325,591325] │ ['2014-03-17 11:37:06','2014-03-17
14:07:47','2014-03-17 14:36:21'] │ │ [] │ [] │ │ [] │ [] │ │ [591325,1073752]
│ ['2014-03-17 00:46:05','2014-03-17 00:46:05'] │ │
[1073752,591325,591325,591325] │ ['2014-03-17 13:28:33','2014-03-17
13:30:26','2014-03-17 18:51:21','2014-03-17 18:51:45'] │ └────────────────────
────────────┴─────────────────────────────────────────────────────────────────
──────────────────────────┘ %% It is easiest to think of a nested data
structure as a set of multiple column arrays of the same length. The only
place where a SELECT query can specify the name of an entire nested data
structure instead of individual columns is the ARRAY JOIN clause. For more
information, see "ARRAY JOIN clause". Example: %% SELECT Goal.ID,
Goal.EventTime FROM test.visits ARRAY JOIN Goals AS Goal WHERE CounterID =
101500 AND length(Goals.ID) < 5 LIMIT 10 ┌─Goal.ID─┬──────Goal.EventTime─┐ │
1073752 │ 2014-03-17 16:38:10 │ │ 591325 │ 2014-03-17 16:38:48 │ │ 591325 │
2014-03-17 16:42:27 │ │ 1073752 │ 2014-03-17 00:28:25 │ │ 1073752 │ 2014-03-17
10:46:20 │ │ 1073752 │ 2014-03-17 13:59:20 │ │ 591325 │ 2014-03-17 22:17:55 │
│ 591325 │ 2014-03-17 22:18:07 │ │ 591325 │ 2014-03-17 22:18:51 │ │ 1073752 │
2014-03-17 11:37:06 │ └─────────┴─────────────────────┘ %% You can't perform
SELECT for an entire nested data structure. You can only explicitly list
individual columns that are part of it. For an INSERT query, you should pass
all the component column arrays of a nested data structure separately (as if
they were individual column arrays). During insertion, the system checks that
they have the same length. For a DESCRIBE query, the columns in a nested data
structure are listed separately in the same way. The ALTER query is very
limited for elements in a nested data structure. ==AggregateFunction(name,
types_of_arguments...)== The intermediate state of an aggregate function. To
get it, use aggregate functions with the '-State' suffix. For more
information, see "AggregatingMergeTree". ==Special data types== Special data
type values can't be saved to a table or output in results, but are used as
the intermediate result of running a query. ===Set=== Used for the right half
of an IN expression. ===Expression=== Used for representing lambda expressions
in high-order functions. ==Boolean values== There isn't a separate type for
boolean values. They use the UInt8 type, restricted to the values 0 or 1.

# Operators

All operators are transformed to the corresponding functions at the query
parsing stage, in accordance with their precedence and associativity. ==Access
operators== %%a[N]%% - Access to an array element, arrayElement(a, N)
function. %%a.N%% - Access to a tuple element, tupleElement(a, N) function.
==Numeric negation operator== %%-a%% - negate(a) function ==Multiplication and
division operators== %%a * b%% - multiply(a, b) function %%a / b%% - divide(a,
b) function %%a % b%% - modulo(a, b) function ==Addition and subtraction
operators== %%a + b%% - plus(a, b) function %%a - b%% - minus(a, b) function
==Comparison operators== %%a = b%% - equals(a, b) function %%a == b%% -
equals(a, b) function %%a != b%% - notEquals(a, b) function a <> b \-
notEquals(a, b) function %%a <= b%% - lessOrEquals(a, b) function a >= b \-
greaterOrEquals(a, b) function %%a < b%% - less(a, b) function a > b \-
greater(a, b) function %%a LIKE s%% - like(a, b) function %%a NOT LIKE s%% -
notLike(a, b) function ==Operators for working with data sets== See the
section "IN operators". %%a IN ...%% - in(a, b) function %%a NOT IN ...%% -
notIn(a, b) function %%a GLOBAL IN ...%% - globalIn(a, b) function %%a GLOBAL
NOT IN ...%% - globalNotIn(a, b) function ==Logical negation operator== %%NOT
a%% - not(a) function ==Logical "AND" operator== %%a AND b%% - and(a, b)
function ==Logical "OR" operator== %%a OR b%% - or(a, b) function
==Conditional operator== %%a ? b : c%% - if(a, b, c) function ==Lambda
creation operator== x -> expr \- lambda(x, expr) function The following
operators do not have a priority, since they are brackets: ==Array creation
operator== %%[x1, ...]%% - array(x1, ...) function ==Tuple creation operator==
%%(x1, x2, ...)%% - tuple(x2, x2, ...) function ==Associativity== All binary
operators have left associativity. For example, '1 + 2 + 3' is transformed to
'plus(plus(1, 2), 3)'. Sometimes this doesn't work the way you expect. For
example, 'SELECT 4 > 3 > 2' results in 0. For efficiency, the 'and' and 'or'
functions accept any number of arguments. The corresponding chains of AND and
OR operators are transformed to a single call of these functions.

# Functions

There are at least* two types of functions - regular functions (they are just
called "functions") and aggregate functions. These are completely different
concepts. Regular functions work as if they are applied to each row separately
(for each row, the result of the function doesn't depend on the other rows).
Aggregate functions accumulate a set of values from various rows (i.e. they
depend on the entire set of rows). In this section we discuss regular
functions. For aggregate functions, see the section "Aggregate functions". * -
There is a third type of function that the 'arrayJoin' function belongs to;
table functions can also be mentioned separately. ===Strong typing=== In
contrast to standard SQL, ClickHouse has strong typing. In other words, it
doesn't make implicit conversions between types. Each function works for a
specific set of types. This means that sometimes you need to use type
conversion functions. ===Сommon subexpression elimination=== All expressions
in a query that have the same AST (the same record or same result of syntactic
parsing) are considered to have identical values. Such expressions are
concatenated and executed once. Identical subqueries are also eliminated this
way. ===Types of results=== All functions return a single return as the result
(not several values, and not zero values). The type of result is usually
defined only by the types of arguments, not by the values. Exceptions are the
tupleElement function (the a.N operator), and the toFixedString function.
===Constants=== For simplicity, certain functions can only work with constants
for some arguments. For example, the right argument of the LIKE operator must
be a constant. Almost all functions return a constant for constant arguments.
The exception is functions that generate random numbers. The 'now' function
returns different values for queries that were run at different times, but the
result is considered a constant, since constancy is only important within a
single query. A constant expression is also considered a constant (for
example, the right half of the LIKE operator can be constructed from multiple
constants). Functions can be implemented in different ways for constant and
non-constant arguments (different code is executed). But the results for a
constant and for a true column containing only the same value should match
each other. ===Immutability=== Functions can't change the values of their
arguments - any changes are returned as the result. Thus, the result of
calculating separate functions does not depend on the order in which the
functions are written in the query. ===Error handling=== Some functions might
throw an exception if the data is invalid. In this case, the query is canceled
and an error text is returned to the client. For distributed processing, when
an exception occurs on one of the servers, the other servers also attempt to
abort the query. ===Evaluation of argument expressions=== In almost all
programming languages, one of the arguments might not be evaluated for certain
operators. This is usually for the operators &&, ||, ?:. But in ClickHouse,
arguments of functions (operators) are always evaluated. This is because
entire parts of columns are evaluated at once, instead of calculating each row
separately. ===Performing functions for distributed query processing=== For
distributed query processing, as many stages of query processing as possible
are performed on remote servers, and the rest of the stages (merging
intermediate results and everything after that) are performed on the requestor
server. This means that functions can be performed on different servers. For
example, in the query SELECT **f**(sum(**g**(x))) FROM distributed_table GROUP
BY **h**(y), \- if %%distributed_table%% has at least two shards, the
functions %%g%% and %%h%% are performed on remote servers, and the function
%%f%% - is performed on the requestor server. \- if %%distributed_table%% has
only one shard, all the functions %%f%%, %%g%%, and %%h%% are performed on
this shard's server. The result of a function usually doesn't depend on which
server it is performed on. However, sometimes this is important. For example,
functions that work with dictionaries use the dictionary that exists on the
server they are running on. Another example is the %%hostName%% function,
which returns the name of the server it is running on in order to make GROUP
BY by servers in a SELECT query. If a function in a query is performed on the
requestor server, but you need to perform it on remote servers, you can wrap
it in an 'any' aggregate function or add it to a key in GROUP BY. ==Arithmetic
functions== For all arithmetic functions, the result type is calculated as the
smallest number type that the result fits in, if there is such a type. The
minimum is taken simultaneously based on the number of bits, whether it is
signed, and whether it floats. If there are not enough bits, the highest bit
type is taken. Example:

    
    
    :) SELECT toTypeName(0), toTypeName(0 + 0), toTypeName(0 + 0 + 0), toTypeName(0 + 0 + 0 + 0)
    
    ┌─_toTypeName(0)_─┬─_toTypeName(plus(0, 0))_─┬─_toTypeName(plus(plus(0, 0), 0))_─┬─_toTypeName(plus(plus(plus(0, 0), 0), 0))_─┐
    │ UInt8         │ UInt16                 │ UInt32                          │ UInt64                                   │
    └───────────────┴────────────────────────┴─────────────────────────────────┴──────────────────────────────────────────┘
    

Arithmetic functions work for any pair of types from UInt8, UInt16, UInt32,
UInt64, Int8, Int16, Int32, Int64, Float32, or Float64. Overflow is produced
the same way as in C++. ===plus(a, b), a + b operator=== Calculates the sum of
the numbers. You can also add whole numbers with a date or date and time. In
the case of a date, adding a whole number means adding the corresponding
number of days. For a date with time, it means adding the corresponding number
of seconds. ===minus(a, b), a - b operator=== Calculates the difference. The
result is always signed. You can also calculate whole numbers from a date or
date with time. The idea is the same - see above for 'plus'. ===multiply(a,
b), a * b operator=== Calculates the product of the numbers. ===divide(a, b),
a / b operator=== Calculates the quotient of the numbers. The result type is
always a floating-point type. It is not integer division. For integer
division, use the 'intDiv' function. When dividing by zero you get 'inf',
'-inf', or 'nan'. ===intDiv(a, b)=== Calculates the quotient of the numbers.
Divides into integers, rounding down (by the absolute value). When dividing by
zero or when dividing a minimal negative number by minus one, an exception is
thrown. ===intDivOrZero(a, b)=== Differs from 'intDiv' in that it returns zero
when dividing by zero or when dividing a minimal negative number by minus one.
===modulo(a, b), a % b operator=== Calculates the remainder after division. If
arguments are floating-point numbers, they are pre-converted to integers by
dropping the decimal portion. The remainder is taken in the same sense as in
C++. Truncated division is used for negative numbers. An exception is thrown
when dividing by zero or when dividing a minimal negative number by minus one.
===negate(a), -a operator=== Calculates a number with the reverse sign. The
result is always signed. ===abs(a)=== Calculates the absolute value of the
number 'a'. That is, if a< 0, it returns -a. For unsigned types, it doesn't do
anything. For signed integer types, it returns an unsigned number. ==Bit
functions== Bit functions work for any pair of types from UInt8, UInt16,
UInt32, UInt64, Int8, Int16, Int32, Int64, Float32, or Float64. The result
type is an integer with bits equal to the maximum bits of its arguments. If at
least one of the arguments is signed, the result is a signed number. If an
argument is a floating-point number, it is cast to Int64. ===bitAnd(a, b)===
===bitOr(a, b)=== ===bitXor(a, b)=== ===bitNot(a)=== ===bitShiftLeft(a, b)===
===bitShiftRight(a, b)=== ==Comparison functions== Comparison functions always
return 0 or 1 (Uint8). The following types can be compared: \- numbers \-
strings and fixed strings \- dates \- dates with times within each group, but
not between different groups. For example, you can't compare a date with a
string. You have to use a function to convert the string to a date, or vice
versa. Strings are compared by bytes. A shorter string is smaller than all
strings that start with it and that contain at least one more character.
Signed and unsigned numbers are compared the same way as in C++. That is, you
can get an incorrect result in some cases. Example: SELECT 9223372036854775807
> -1 ===equals, a = b and a == b operator===

### notEquals, a != b and a <> b operator

===less, < operator===

### greater, > operator

===lessOrEquals, <= operator===

### greaterOrEquals, >= operator

==Logical functions== Logical functions accept any numeric types, but return a
UInt8 number equal to 0 or 1. Zero as an argument is considered "false," while
any non-zero value is considered "true". ===and, AND operator=== ===or, OR
operator=== ===not, NOT operator=== ===xor=== ==Type conversion functions==
===toUInt8, toUInt16, toUInt32, toUInt64=== ===toInt8, toInt16, toInt32,
toInt64=== ===toFloat32, toFloat64=== ===toDate, toDateTime=== ===toString===
Functions for converting between numbers, strings (but not fixed strings),
dates, and dates with times. All these functions accept one argument. When
converting to or from a string, the value is formatted or parsed using the
same rules as for the TabSeparated format (and almost all other text formats).
If the string can't be parsed, an exception is thrown and the request is
canceled. When converting dates to numbers or vice versa, the date corresponds
to the number of days since the beginning of the Unix epoch. When converting
dates with times to numbers or vice versa, the date with time corresponds to
the number of seconds since the beginning of the Unix epoch. Formats of date
and date with time for toDate/toDateTime functions are defined as follows: %%
YYYY-MM-DD YYYY-MM-DD hh:mm:ss %% As an exception, if converting from UInt32,
Int32, UInt64, or Int64 type numbers to Date, and if the number is greater
than or equal to 65536, the number is interpreted as a Unix timestamp (and not
as the number of days) and is rounded to the date. This allows support for the
common occurrence of writing 'toDate(unix_timestamp)', which otherwise would
be an error and would require writing the more cumbersome
'toDate(toDateTime(unix_timestamp))'. Conversion between a date and date with
time is performed the natural way: by adding a null time or dropping the time.
Conversion between numeric types uses the same rules as assignments between
different numeric types in C++. To do transformations on DateTime in given
time zone, pass second argument with time zone name: %% SELECT
toDateTime('2016-06-15 23:00:00') AS time, toDate(time) AS date_local,
toDate(time, 'Asia/Yekaterinburg') AS date_yekat, toString(time, 'US/Samoa')
AS time_samoa
┌────────────────time─┬─date_local─┬─date_yekat─┬─time_samoa──────────┐ │
2016-06-15 23:00:00 │ 2016-06-15 │ 2016-06-16 │ 2016-06-15 09:00:00 │
└─────────────────────┴────────────┴────────────┴─────────────────────┘ %% To
format DateTime in given time zone: %% toString(now(), 'Asia/Yekaterinburg')
%% To get unix timestamp for string with datetime in specified time zone: %%
toUnixTimestamp('2000-01-01 00:00:00', 'Asia/Yekaterinburg') %%
===toFixedString(s, N)=== Converts a String type argument to a FixedString(N)
type (a string with fixed length N). N must be a constant. If the string has
fewer bytes than N, it is passed with null bytes to the right. If the string
has more bytes than N, an exception is thrown. ===toStringCutToZero(s)===
Accepts a String or FixedString argument. Returns a String and removes the
null bytes from the end of the string. ===reinterpretAsUInt8,
reinterpretAsUInt16, reinterpretAsUInt32, reinterpretAsUInt64===
===reinterpretAsInt8, reinterpretAsInt16, reinterpretAsInt32,
reinterpretAsInt64=== ===reinterpretAsFloat32, reinterpretAsFloat64===
===reinterpretAsDate, reinterpretAsDateTime=== These functions accept a string
and interpret the bytes placed at the beginning of the string as a number in
host order (little endian). If the string isn't long enough, the functions
work as if the string is padded with the necessary number of null bytes. If
the string is longer than needed, the extra bytes are ignored. A date is
interpreted as the number of days since the beginning of the Unix Epoch, and a
date with time is interpreted as the number of seconds since the beginning of
the Unix Epoch. ===reinterpretAsString=== This function accepts a number or
date or date with time, and returns a string containing bytes representing the
corresponding value in host order (little endian). Null bytes are dropped from
the end. For example, a UInt32 type value of 255 is a string that is one byte
long. ==Functions for working with dates and times== ===toYear=== \- Converts
a date or date with time to a UInt16 number containing the year number (AD).
===toMonth=== \- Converts a date or date with time to a UInt8 number
containing the month number (1-12). ===toDayOfMonth=== \- Converts a date or
date with time to a UInt8 number containing the number of the day of the month
(1-31). ===toDayOfWeek=== \- Converts a date or date with time to a UInt8
number containing the number of the day of the week (Monday is 1, and Sunday
is 7). ===toHour=== \- Converts a date with time to a UInt8 number containing
the number of the hour in 24-hour time (0-23). This function assumes that if
clocks are moved ahead, it is by one hour and occurs at 2 a.m., and if clocks
are moved back, it is by one hour and occurs at 3 a.m. (which is not always
true - even in Moscow the clocks were once changed at a different time).
===toMinute=== \- Converts a date with time to a UInt8 number containing the
number of the minute of the hour (0-59). ===toSecond=== \- Converts a date
with time to a UInt8 number containing the number of the second in the minute
(0-59). Leap seconds are not accounted for. ===toMonday=== \- Rounds down a
date or date with time to the nearest Monday. Returns the date.
===toStartOfMonth=== \- Rounds down a date or date with time to the first day
of the month. Returns the date. ===toStartOfQuarter=== \- Rounds down a date
or date with time to the first day of the quarter. The first day of the
quarter is either 1 January, 1 April, 1 July, or 1 October. Returns the date.
===toStartOfYear=== \- Rounds down a date or date with time to the first day
of the year. Returns the date. ===toStartOfMinute=== \- Rounds down a date
with time to the start of the minute. ===toStartOfHour=== \- Rounds down a
date with time to the start of the hour. ===toTime=== \- Converts a date with
time to the date of the start of the Unix Epoch, while preserving the time.
===toRelativeYearNum=== \- Converts a date with time or date to the number of
the year, starting from a certain fixed point in the past.
===toRelativeMonthNum=== \- Converts a date with time or date to the number of
the month, starting from a certain fixed point in the past.
===toRelativeWeekNum=== \- Converts a date with time or date to the number of
the week, starting from a certain fixed point in the past.
===toRelativeDayNum=== \- Converts a date with time or date to the number of
the day, starting from a certain fixed point in the past.
===toRelativeHourNum=== \- Converts a date with time or date to the number of
the hour, starting from a certain fixed point in the past.
===toRelativeMinuteNum=== \- Converts a date with time or date to the number
of the minute, starting from a certain fixed point in the past.
===toRelativeSecondNum=== \- Converts a date with time or date to the number
of the second, starting from a certain fixed point in the past. ===now===
Accepts zero arguments and returns the current time at one of the moments of
request execution. This function returns a constant, even if the request took
a long time to complete. ===today=== Accepts zero arguments and returns the
current date at one of the moments of request execution. The same as
'toDate(now())'. ===yesterday=== Accepts zero arguments and returns
yesterday's date at one of the moments of request execution. The same as
'today() - 1'. ===timeSlot=== \- Rounds the time to the half hour. This
function is specific to Yandex.Metrica, since half an hour is the minimum
amount of time for breaking a session into two sessions if a counter shows a
single user's consecutive pageviews that differ in time by strictly more than
this amount. This means that tuples (the counter number, user ID, and time
slot) can be used to search for pageviews that are included in the
corresponding session. ===timeSlots(StartTime, Duration)=== \- For a time
interval starting at 'StartTime' and continuing for 'Duration' seconds, it
returns an array of moments in time, consisting of points from this interval
rounded down to the half hour. For example, %%timeSlots(toDateTime('2012-01-01
12:20:00'), toUInt32(600)) = [toDateTime('2012-01-01 12:00:00'),
toDateTime('2012-01-01 12:30:00')]%%. This is necessary for searching for
pageviews in the corresponding session. ==Functions for working with strings==
===empty=== \- Returns 1 for an empty string or 0 for a non-empty string. The
result type is UInt8. A string is considered non-empty if it contains at least
one byte, even if this is a space or a null byte. The function also works for
arrays. ===notEmpty=== \- Returns 0 for an empty string or 1 for a non-empty
string. The result type is UInt8. The function also works for arrays.
===length=== \- Returns the length of a string in bytes (not in characters,
and not in code points). The result type is UInt64. The function also works
for arrays. ===lengthUTF8=== \- Returns the length of a string in Unicode code
points (not in characters), assuming that the string contains a set of bytes
that make up UTF-8 encoded text. If this assumption is not met, it returns
some result (it doesn't throw an exception). The result type is UInt64.
===lower=== \- Converts ASCII Latin symbols in a string to lowercase.
===upper=== \- Converts ASCII Latin symbols in a string to uppercase.
===lowerUTF8=== \- Converts a string to lowercase, assuming the string
contains a set of bytes that make up a UTF-8 encoded text. It doesn't detect
the language. So for Turkish the result might not be exactly correct. If
length of UTF-8 sequence is different for upper and lower case of code point,
then result for that code point could be incorrect. If value contains invalid
UTF-8, the behavior is unspecified. ===upperUTF8=== \- Converts a string to
uppercase, assuming the string contains a set of bytes that make up a UTF-8
encoded text. It doesn't detect the language. So for Turkish the result might
not be exactly correct. If length of UTF-8 sequence is different for upper and
lower case of code point, then result for that code point could be incorrect.
If value contains invalid UTF-8, the behavior is unspecified. ===reverse=== \-
Reverses the string (as a sequence of bytes). ===reverseUTF8=== \- Reverses a
sequence of Unicode code points, assuming that the string contains a set of
bytes representing a UTF-8 text. Otherwise, it does something else (it doesn't
throw an exception). ===concat(s1, s2)=== \- Concatenates two strings, without
a separator. If you need to concatenate more than two strings, write 'concat'
multiple times. ===substring(s, offset, length)=== \- Returns a substring
starting with the byte from the 'offset' index that is 'length' bytes long.
Character indexing starts from one (as in standard SQL). The 'offset' and
'length' arguments must be constants. ===substringUTF8(s, offset, length)===
The same as 'substring', but for Unicode code points. Works under the
assumption that the string contains a set of bytes representing a UTF-8
encoded text. If this assumption is not met, it returns some result (it
doesn't throw an exception). ===appendTrailingCharIfAbsent(s, c)=== If the
%%s%% string is non-empty and does not contain the %%c%% character at the end,
it appends the %%c%% character to the end. ==Functions for searching strings==
The search is case-sensitive in all these functions. The search substring or
regular expression must be a constant in all these functions.
===position(haystack, needle)=== \- Search for the 'needle' substring in the
'haystack' string. Returns the position (in bytes) of the found substring,
starting from 1, or returns 0 if the substring was not found.
===positionUTF8(haystack, needle)=== The same as 'position', but the position
is returned in Unicode code points. Works under the assumption that the string
contains a set of bytes representing a UTF-8 encoded text. If this assumption
is not met, it returns some result (it doesn't throw an exception).
===match(haystack, pattern)=== \- Checks whether the string matches the
'pattern' regular expression. The regular expression is re2. Returns 0 if it
doesn't match, or 1 if it matches. Note that the backslash symbol (%%\%%) is
used for escaping in the regular expression. The same symbol is used for
escaping in string literals. So in order to escape the symbol in a regular
expression, you must write two backslashes (%%\\\%%) in a string literal. The
regular expression works with the string as if it is a set of bytes. The
regular expression can't contain null bytes. For patterns to search for
substrings in a string, it is better to use LIKE or 'position', since they
work much faster. ===extract(haystack, pattern)=== Extracts a fragment of a
string using a regular expression. If 'haystack' doesn't match the 'pattern'
regex, an empty string is returned. If the regex doesn't contain subpatterns,
it takes the fragment that matches the entire regex. Otherwise, it takes the
fragment that matches the first subpattern. ===extractAll(haystack,
pattern)=== Extracts all the fragments of a string using a regular expression.
If 'haystack' doesn't match the 'pattern' regex, an empty string is returned.
Returns an array of strings consisting of all matches to the regex. In
general, the behavior is the same as the 'extract' function (it takes the
first subpattern, or the entire expression if there isn't a subpattern).
===like(haystack, pattern), haystack LIKE pattern operator=== \- Checks
whether a string matches a simple regular expression. The regular expression
can contain the metasymbols %%%%% and %%_%%. %%%%% indicates any quantity of
any bytes (including zero characters). %%_%% indicates any one byte. Use the
backslash (%%\%%) for escaping metasymbols. See the note on escaping in the
description of the 'match' function. For regular expressions like%%%needle%%%,
the code is more optimal and works as fast as the 'position' function. For
other regular expressions, the code is the same as for the 'match' function.
===notLike(haystack, pattern), haystack NOT LIKE pattern operator=== The same
thing as 'like', but negative. ==Functions for searching and replacing in
strings== ===replaceOne(haystack, pattern, replacement)=== Replaces the first
occurrence, if it exists, of the 'pattern' substring in 'haystack' with the
'replacement' substring. Hereafter, 'pattern' and 'replacement' must be
constants. ===replaceAll(haystack, pattern, replacement)=== Replaces all
occurrences of the 'pattern' substring in 'haystack' with the 'replacement'
substring. ===replaceRegexpOne(haystack, pattern, replacement)=== Replacement
using the 'pattern' regular expression. A re2 regular expression. Replaces
only the first occurrence, if it exists. A pattern can be specified as
'replacement'. This pattern can include substitutions \0-\9\\. The
substitution \0 includes the entire regular expression. The substitutions
\1-\9 include the subpattern corresponding to the number. In order to specify
the \ symbol in a pattern, you must use a \ symbol to escape it. Also keep in
mind that a string literal requires an extra escape. Example 1. Converting the
date to American format: %% SELECT DISTINCT EventDate,
replaceRegexpOne(toString(EventDate), '(\\\d{4})-(\\\d{2})-(\\\d{2})',
'\\\2/\\\3/\\\1') AS res FROM test.hits LIMIT 7 FORMAT TabSeparated 2014-03-17
03/17/2014 2014-03-18 03/18/2014 2014-03-19 03/19/2014 2014-03-20 03/20/2014
2014-03-21 03/21/2014 2014-03-22 03/22/2014 2014-03-23 03/23/2014 %% Example
2. Copy the string ten times: %% SELECT replaceRegexpOne('Hello, World!',
'.*', '\\\0\\\0\\\0\\\0\\\0\\\0\\\0\\\0\\\0\\\0') AS res ┌─res────────────────
──────────────────────────────────────────────────────────────────────────────
──────────────────────────────────┐ │ Hello, World!Hello, World!Hello,
World!Hello, World!Hello, World!Hello, World!Hello, World!Hello, World!Hello,
World!Hello, World! │ └───────────────────────────────────────────────────────
─────────────────────────────────────────────────────────────────────────────┘
%% ===replaceRegexpAll(haystack, pattern, replacement)=== This does the same
thing, but replaces all the occurrences. Example: %% SELECT
replaceRegexpAll('Hello, World!', '.', '\\\0\\\0') AS res
┌─res────────────────────────┐ │ HHeelllloo,, WWoorrlldd!! │
└────────────────────────────┘ %% As an exception, if a regular expression
worked on an empty substring, the replacement is not made more than once.
Example: %% SELECT replaceRegexpAll('Hello, World!', '^', 'here: ') AS res
┌─res─────────────────┐ │ here: Hello, World! │ └─────────────────────┘ %%
==Functions for working with arrays== ===empty=== \- Returns 1 for an empty
array, or 0 for a non-empty array. The result type is UInt8. The function also
works for strings. ===notEmpty=== \- Returns 0 for an empty array, or 1 for a
non-empty array. The result type is UInt8. The function also works for
strings. ===length=== \- Returns the number of items in the array. The result
type is UInt64. The function also works for strings. ===emptyArrayUInt8,
emptyArrayUInt16, emptyArrayUInt32, emptyArrayUInt64=== ===emptyArrayInt8,
emptyArrayInt16, emptyArrayInt32, emptyArrayInt64=== ===emptyArrayFloat32,
emptyArrayFloat64=== ===emptyArrayDate, emptyArrayDateTime===
===emptyArrayString=== Accepts zero arguments and returns an empty array of
the appropriate type. ===range(N)=== \- Returns an array of numbers from 0 to
N-1. Just in case, an exception is thrown if arrays with a total length of
more than 100,000,000 elements are created in a data block. ===array(x1, ...),
[x1, ...] operator=== \- Creates an array from the function arguments. The
arguments must be constants and have types that have the smallest common type.
At least one argument must be passed, because otherwise it isn't clear which
type of array to create. That is, you can't use this function to create an
empty array (to do that, use the 'emptyArray*' function described above).
Returns an 'Array(T)' type result, where 'T' is the smallest common type out
of the passed arguments. ===arrayElement(arr, n), arr[n] operator=== \- Get
the element with the index 'n' from the array 'arr'. 'n' should be any integer
type. Indexes in an array begin from one. Negative indexes are supported - in
this case, it selects the corresponding element numbered from the end. For
example, 'arr[-1]' is the last item in the array. If the index goes beyond the
array bounds: \- if both arguments are constants, an exception is thrown. \-
otherwise, a default value is returned (0 for numbers, an empty string for
strings, etc.). ===has(arr, elem)=== \- Checking whether the 'arr' array has
the 'elem' element. Returns 0 if the the element is not in the array, or 1 if
it is. 'elem' must be a constant. ===indexOf(arr, x)=== \- Returns the index
of the 'x' element (starting from 1) if it is in the array, or 0 if it is not.
===countEqual(arr, x)=== \- Returns the number of elements in the array equal
to 'x'. Equivalent to arrayCount(elem -> elem = x, arr).
===arrayEnumerate(arr)=== \- Returns the array %%[1, 2, 3, ..., length(arr)]%%
This function is normally used together with ARRAY JOIN. It allows counting
something just once for each array after applying ARRAY JOIN. Example: %%
SELECT count() AS Reaches, countIf(num = 1) AS Hits FROM test.hits ARRAY JOIN
GoalsReached, arrayEnumerate(GoalsReached) AS num WHERE CounterID = 160656
LIMIT 10 ┌─Reaches─┬──Hits─┐ │ 95606 │ 31406 │ └─────────┴───────┘ %% In this
example, Reaches is the number of conversions (the strings received after
applying ARRAY JOIN), and Hits is the number of pageviews (strings before
ARRAY JOIN). In this particular case, you can get the same result in an easier
way: %% SELECT sum(length(GoalsReached)) AS Reaches, count() AS Hits FROM
test.hits WHERE (CounterID = 160656) AND notEmpty(GoalsReached)
┌─Reaches─┬──Hits─┐ │ 95606 │ 31406 │ └─────────┴───────┘ %% This function can
also be used in higher-order functions. For example, you can use it to get
array indexes for elements that match a condition. ===arrayEnumerateUniq(arr,
...)=== \- Returns an array the same size as the source array, indicating for
each element what its position is among elements with the same value. For
example: %%arrayEnumerateUniq([10, 20, 10, 30]) = [1, 1, 2, 1]%%. This
function is useful when using ARRAY JOIN and aggregation of array elements.
Example: %% SELECT Goals.ID AS GoalID, sum(Sign) AS Reaches, sumIf(Sign, num =
1) AS Visits FROM test.visits ARRAY JOIN Goals, arrayEnumerateUniq(Goals.ID)
AS num WHERE CounterID = 160656 GROUP BY GoalID ORDER BY Reaches DESC LIMIT 10
┌──GoalID─┬─Reaches─┬─Visits─┐ │ 53225 │ 3214 │ 1097 │ │ 2825062 │ 3188 │ 1097
│ │ 56600 │ 2803 │ 488 │ │ 1989037 │ 2401 │ 365 │ │ 2830064 │ 2396 │ 910 │ │
1113562 │ 2372 │ 373 │ │ 3270895 │ 2262 │ 812 │ │ 1084657 │ 2262 │ 345 │ │
56599 │ 2260 │ 799 │ │ 3271094 │ 2256 │ 812 │ └─────────┴─────────┴────────┘
%% In this example, each goal ID has a calculation of the number of
conversions (each element in the Goals nested data structure is a goal that
was reached, which we refer to as a conversion) and the number of sessions.
Without ARRAY JOIN, we would have counted the number of sessions as
%%sum(Sign)%%. But in this particular case, the rows were multiplied by the
nested Goals structure, so in order to count each session one time after this,
we apply a condition to the value of the %%arrayEnumerateUniq(Goals.ID)%%
function. The arrayEnumerateUniq function can take multiple arrays of the same
size as arguments. In this case, uniqueness is considered for tuples of
elements in the same positions in all the arrays. %% SELECT
arrayEnumerateUniq([1, 1, 1, 2, 2, 2], [1, 1, 2, 1, 1, 2]) AS res
┌─res───────────┐ │ [1,2,1,1,2,1] │ └───────────────┘ %% This is necessary
when using ARRAY JOIN with a nested data structure and further aggregation
across multiple elements in this structure. ===arrayJoin(arr)=== \- A special
function. See the section "arrayJoin function". ==Higher-order functions==

### -> operator, lambda(params, expr) function

Allows describing a lambda function for passing to a higher-order function.
The left side of the arrow has a formal parameter - any ID, or multiple formal
parameters - any IDs in a tuple. The right side of the arrow has an expression
that can use these formal parameters, as well as any table columns. Examples:
x -> 2 * x, str -> str != Referer. Higher-order functions can only accept
lambda functions as their functional argument. A lambda function that accepts
multiple arguments can be passed to a higher-order function. In this case, the
higher-order function is passed several arrays of identical length that these
arguments will correspond to. For all functions other than 'arrayMap' and
'arrayFilter', the first argument (the lambda function) can be omitted. In
this case, identical mapping is assumed. ===arrayMap(func, arr1, ...)===
Returns an array obtained from the original application of the 'func' function
to each element in the 'arr' array. ===arrayFilter(func, arr1, ...)=== Returns
an array containing only the elements in 'arr1' for which 'func' returns
something other than 0. Examples: %% SELECT arrayFilter(x -> x LIKE '%World%',
['Hello', 'abc World']) AS res ┌─res───────────┐ │ ['abc World'] │
└───────────────┘ SELECT arrayFilter( (i, x) -> x LIKE '%World%',
arrayEnumerate(arr), ['Hello', 'abc World'] AS arr) AS res ┌─res─┐ │ [2] │
└─────┘ %% ===arrayCount([func,] arr1, ...)=== Returns the number of elements
in 'arr' for which 'func' returns something other than 0. If 'func' is not
specified, it returns the number of non-zero items in the array.
===arrayExists([func,] arr1, ...)=== Returns 1 if there is at least one
element in 'arr' for which 'func' returns something other than 0. Otherwise,
it returns 0. ===arrayAll([func,] arr1, ...)=== Returns 1 if 'func' returns
something other than 0 for all the elements in 'arr'. Otherwise, it returns 0.
===arraySum([func,] arr1, ...)=== Returns the sum of the 'func' values. If the
function is omitted, it just returns the sum of the array elements.
===arrayFirst(func, arr1, ...)=== Returns the first element in the 'arr1'
array for which 'func' returns something other than 0.
===arrayFirstIndex(func, arr1, ...)=== Returns the index of the first element
in the 'arr1' array for which 'func' returns something other than 0.
==Functions for splitting and merging strings and arrays==
===splitByChar(separator, s)=== \- Splits a string into substrings, using
'separator' as the separator. 'separator' must be a string constant consisting
of exactly one character. Returns an array of selected substrings. Empty
substrings may be selected if the separator occurs at the beginning or end of
the string, or if there are multiple consecutive separators.
===splitByString(separator, s)=== \- The same as above, but it uses a string
of multiple characters as the separator. The string must be non-empty.
===alphaTokens(s)=== \- Selects substrings of consecutive bytes from the range
a-z and A-Z. Returns an array of selected substrings. ==Functions for working
with URLs== All these functions don't follow the RFC. They are maximally
simplified for improved performance. ===Functions that extract part of a
URL=== If there isn't anything similar in a URL, an empty string is returned.

#### protocol

\- Selects the protocol. Examples: http, ftp, mailto, magnet...

#### domain

\- Selects the domain.

#### domainWithoutWWW

\- Selects the domain and removes no more than one 'www.' from the beginning
of it, if present.

#### topLevelDomain

\- Selects the top-level domain. Example: .ru.

#### firstSignificantSubdomain

\- Selects the "first significant subdomain". This is a non-standard concept
specific to Yandex.Metrica. The first significant subdomain is a second-level
domain if it is 'com', 'net', 'org', or 'co'. Otherwise, it is a third-level
domain. For example, firstSignificantSubdomain('https://news.yandex.ru/') =
'yandex', firstSignificantSubdomain('https://news.yandex.com.tr/') = 'yandex'.
The list of "insignificant" second-level domains and other implementation
details may change in the future.

#### cutToFirstSignificantSubdomain

\- Selects the part of the domain that includes top-level subdomains up to the
"first significant subdomain" (see the explanation above). For example,
cutToFirstSignificantSubdomain('https://news.yandex.com.tr/') =
'yandex.com.tr'.

#### path

\- Selects the path. Example: /top/news.html The path does not include the
query-string.

#### pathFull

\- The same as above, but including query-string and fragment. Example:
/top/news.html?page=2#comments

#### queryString

\- Selects the query-string. Example: page=1&lr=213. query-string does not
include the first question mark, or # and everything that comes after #.

#### fragment

\- Selects the fragment identifier. fragment does not include the first number
sign (#).

#### queryStringAndFragment

\- Selects the query-string and fragment identifier. Example: page=1#29390.

#### extractURLParameter(URL, name)

\- Selects the value of the 'name' parameter in the URL, if present.
Otherwise, selects an empty string. If there are many parameters with this
name, it returns the first occurrence. This function works under the
assumption that the parameter name is encoded in the URL in exactly the same
way as in the argument passed.

#### extractURLParameters(URL)

\- Gets an array of name=value strings corresponding to the URL parameters.
The values are not decoded in any way.

#### extractURLParameterNames(URL)

\- Gets an array of name=value strings corresponding to the names of URL
parameters. The values are not decoded in any way.

#### URLHierarchy(URL)

\- Gets an array containing the URL trimmed to the %%/%%, %%?%% characters in
the path and query-string. Consecutive separator characters are counted as
one. The cut is made in the position after all the consecutive separator
characters. Example:

#### URLPathHierarchy(URL)

\- The same thing, but without the protocol and host in the result. The /
element (root) is not included. Example: This function is used for
implementing tree-view reports by URL in Yandex.Metrica. %%
URLPathHierarchy('https://example.com/browse/CONV-6788') = [ '/browse/',
'/browse/CONV-6788' ] %% ===Functions that remove part of a URL.=== If the URL
doesn't have anything similar, the URL remains unchanged.

#### cutWWW

\- Removes no more than one 'www.' from the beginning of the URL's domain, if
present.

#### cutQueryString

\- Removes the query-string. The question mark is also removed.

#### cutFragment

\- Removes the fragment identifier. The number sign is also removed.

#### cutQueryStringAndFragment

\- Removes the query-string and fragment identifier. The question mark and
number sign are also removed.

#### cutURLParameter(URL, name)

\- Removes the URL parameter named 'name', if present. This function works
under the assumption that the parameter name is encoded in the URL exactly the
same way as in the passed argument. ==Functions for working with IP
addresses== ===IPv4NumToString(num)=== Takes a UInt32 number. Interprets it as
an IPv4 address in big endian. Returns a string containing the corresponding
IPv4 address in the format A.B.C.d (dot-separated numbers in decimal form).
===IPv4StringToNum(s)=== The reverse function of IPv4NumToString. If the IPv4
address has an invalid format, it returns 0. ===IPv4NumToStringClassC(num)===
Similar to IPv4NumToString, but using %%xxx%% instead of the last octet.
Example: %% SELECT IPv4NumToStringClassC(ClientIP) AS k, count() AS c FROM
test.hits GROUP BY k ORDER BY c DESC LIMIT 10 ┌─k──────────────┬─────c─┐ │
83.149.9.xxx │ 26238 │ │ 217.118.81.xxx │ 26074 │ │ 213.87.129.xxx │ 25481 │ │
83.149.8.xxx │ 24984 │ │ 217.118.83.xxx │ 22797 │ │ 78.25.120.xxx │ 22354 │ │
213.87.131.xxx │ 21285 │ │ 78.25.121.xxx │ 20887 │ │ 188.162.65.xxx │ 19694 │
│ 83.149.48.xxx │ 17406 │ └────────────────┴───────┘ %% Since using 'xxx' is
highly unusual, this may be changed in the future. We recommend that you don't
rely on the exact format of this fragment. ===IPv6NumToString(x)=== Accepts a
FixedString(16) value containing the IPv6 address in binary format. Returns a
string containing this address in text format. IPv6-mapped IPv4 addresses are
output in the format %%::ffff:111.222.33.44%%. Examples: %% SELECT
IPv6NumToString(toFixedString(unhex('2A0206B8000000000000000000000011'), 16))
AS addr ┌─addr─────────┐ │ 2a02:6b8::11 │ └──────────────┘ %% %% SELECT
IPv6NumToString(ClientIP6 AS k), count() AS c FROM hits_all WHERE EventDate =
today() AND substring(ClientIP6, 1, 12) != unhex('00000000000000000000FFFF')
GROUP BY k ORDER BY c DESC LIMIT 10
┌─IPv6NumToString(ClientIP6)──────────────┬─────c─┐ │ 2a02:2168:aaa:bbbb::2 │
24695 │ │ 2a02:2698:abcd:abcd:abcd:abcd:8888:5555 │ 22408 │ │
2a02:6b8:0:fff::ff │ 16389 │ │ 2a01:4f8:111:6666::2 │ 16016 │ │
2a02:2168:888:222::1 │ 15896 │ │ 2a01:7e00::ffff:ffff:ffff:222 │ 14774 │ │
2a02:8109:eee:ee:eeee:eeee:eeee:eeee │ 14443 │ │
2a02:810b:8888:888:8888:8888:8888:8888 │ 14345 │ │
2a02:6b8:0:444:4444:4444:4444:4444 │ 14279 │ │ 2a01:7e00::ffff:ffff:ffff:ffff
│ 13880 │ └─────────────────────────────────────────┴───────┘ %% %% SELECT
IPv6NumToString(ClientIP6 AS k), count() AS c FROM hits_all WHERE EventDate =
today() GROUP BY k ORDER BY c DESC LIMIT 10
┌─IPv6NumToString(ClientIP6)─┬──────c─┐ │ ::ffff:94.26.111.111 │ 747440 │ │
::ffff:37.143.222.4 │ 529483 │ │ ::ffff:5.166.111.99 │ 317707 │ │
::ffff:46.38.11.77 │ 263086 │ │ ::ffff:79.105.111.111 │ 186611 │ │
::ffff:93.92.111.88 │ 176773 │ │ ::ffff:84.53.111.33 │ 158709 │ │
::ffff:217.118.11.22 │ 154004 │ │ ::ffff:217.118.11.33 │ 148449 │ │
::ffff:217.118.11.44 │ 148243 │ └────────────────────────────┴────────┘ %%
===IPv6StringToNum(s)=== The reverse function of IPv6NumToString. If the IPv6
address has an invalid format, it returns a string of null bytes. HEX can be
uppercase or lowercase. ==Functions for generating pseudo-random numbers==
Non-cryptographic generators of pseudo-random numbers are used. All the
functions accept zero arguments or one argument. If an argument is passed, it
can be any type, and its value is not used for anything. The only purpose of
this argument is to prevent common subexpression elimination, so that two
different instances of the same function return different columns with
different random numbers. ===rand=== \- Returns a pseudo-random UInt32 number,
evenly distributed among all UInt32-type numbers. Uses a linear congruential
generator. ===rand64=== \- Returns a pseudo-random UInt64 number, evenly
distributed among all UInt64-type numbers. Uses a linear congruential
generator. ==Hash functions== Hash functions can be used for deterministic
pseudo-random shuffling of elements. ===halfMD5=== \- Calculates the MD5 from
a string. Then it takes the first 8 bytes of the hash and interprets them as
UInt64 in big endian. Accepts a String-type argument. Returns UInt64. This
function works fairly slowly (5 million short strings per second per processor
core). If you don't need MD5 in particular, use the 'sipHash64' function
instead. ===MD5=== \- Calculates the MD5 from a string and returns the
resulting set of bytes as FixedString(16). If you don't need MD5 in
particular, but you need a decent cryptographic 128-bit hash, use the
'sipHash128' function instead. If you need the same result as gives 'md5sum'
utility, write %%lower(hex(MD5(s)))%%. ===sipHash64=== \- Calculates SipHash
from a string. Accepts a String-type argument. Returns UInt64. SipHash is a
cryptographic hash function. It works at least three times faster than MD5.
For more information, see <https://131002.net/siphash/> ===sipHash128=== \-
Calculates SipHash from a string. Accepts a String-type argument. Returns
FixedString(16). Differs from sipHash64 in that the final xor-folding state is
only done up to 128 bytes. ===cityHash64=== \- Calculates CityHash64 from a
string or a similar hash function for any number of any type of arguments. For
String-type arguments, CityHash is used. This is a fast non-cryptographic hash
function for strings with decent quality. For other types of arguments, a
decent implementation-specific fast non-cryptographic hash function is used.
If multiple arguments are passed, the function is calculated using the same
rules and chain combinations using the CityHash combinator. For example, you
can compute the checksum of an entire table with accuracy up to the row order:
%%SELECT sum(cityHash64(*)) FROM table%%. ===intHash32=== \- Calculates a
32-bit hash code from any type of integer. This is a relatively fast non-
cryptographic hash function of average quality for numbers. ===intHash64=== \-
Calculates a 64-bit hash code from any type of integer. It works faster than
intHash32. Average quality. ===SHA1=== ===SHA224=== ===SHA256=== \- Calculates
SHA-1, SHA-224, or SHA-256 from a string and returns the resulting set of
bytes as FixedString(20), FixedString(28), or FixedString(32). The function
works fairly slowly (SHA-1 processes about 5 million short strings per second
per processor core, while SHA-224 and SHA-256 process about 2.2 million). We
recommend using this function only in cases when you need a specific hash
function and you can't select it. Even in these cases, we recommend applying
the function offline and pre-calculating values when inserting them into the
table, instead of applying it in SELECTS. ===URLHash(url[, N])=== A fast,
decent-quality non-cryptographic hash function for a string obtained from a
URL using some type of normalization. URLHash(s) - Calculates a hash from a
string without one of the trailing symbols /,? or # at the end, if present.
URL Hash(s, N) - Calculates a hash from a string up to the N level in the URL
hierarchy, without one of the trailing symbols /,? or # at the end, if
present. Levels are the same as in URLHierarchy. This function is specific to
Yandex.Metrica. ==Encoding functions== ===hex=== Accepts a string, number,
date, or date with time. Returns a string containing the argument's
hexadecimal representation. Uses uppercase letters A-F. Doesn't use %%0x%%
prefixes or %%h%% suffixes. For strings, all bytes are simply encoded as two
hexadecimal numbers. Numbers are converted to big endian ("human readable")
format. For numbers, older zeros are trimmed, but only by entire bytes. For
example, %%hex(1) = '01'%%. Dates are encoded as the number of days since the
beginning of the Unix Epoch. Dates with times are encoded as the number of
seconds since the beginning of the Unix Epoch. ===unhex(str)=== Accepts a
string containing any number of hexadecimal digits, and returns a string
containing the corresponding bytes. Supports both uppercase and lowercase
letters A-F. The number of hexadecimal digits doesn't have to be even. If it
is odd, the last digit is interpreted as the younger half of the 00-0F byte.
If the argument string contains anything other than hexadecimal digits, some
implementation-defined result is returned (an exception isn't thrown). If you
want to convert the result to a number, you can use the functions 'reverse'
and 'reinterpretAs_Type_'. ===bitmaskToList(num)=== Accepts an integer.
Returns a string containing the list of powers of two that total the source
number when summed. They are comma-separated without spaces in text format, in
ascending order. ===bitmaskToArray(num)=== Accepts an integer. Returns an
array of UInt64 numbers containing the list of powers of two that total the
source number when summed. Numbers in the array are in ascending order.
==Rounding functions== ===floor(x[, N])=== Returns a rounder number that is
less than or equal to 'x'. A round number is a multiple of 1 / 10N, or the
nearest number of the appropriate data type if 1 / 10N isn't exact. 'N' is an
integer constant, optional parameter. By default it is zero, which means to
round to an integer. 'N' may be negative. Examples: %%floor(123.45, 1) =
123.4%%, %%floor(123.45, -1) = 120%%. 'x' is any numeric type. The result is a
number of the same type. For integer arguments, it makes sense to round with a
negative 'N' value (for non-negative 'N', the function doesn't do anything).
If rounding causes overflow (for example, %%floor(-128, -1)%%), an
implementation-specific result is returned. ===ceil(x[, N])=== Returns the
smallest round number that is greater than or equal to 'x'. In every other
way, it is the same as the 'floor' function (see above). ===round(x[, N])===
Returns the round number nearest to 'num', which may be less than, greater
than, or equal to 'x'. If 'x' is exactly in the middle between the nearest
round numbers, one of them is returned (implementation-specific). The number
'-0.' may or may not be considered round (implementation-specific). In every
other way, this function is the same as 'floor' and 'ceil' described above.
===roundToExp2(num)=== Accepts a number. If the number is less than one, it
returns 0. Otherwise, it rounds the number down to the nearest (whole non-
negative) degree of two. ===roundDuration(num)=== Accepts a number. If the
number is less than one, it returns 0. Otherwise, it rounds the number down to
numbers from the set: 1, 10, 30, 60, 120, 180, 240, 300, 600, 1200, 1800,
3600, 7200, 18000, 36000. This function is specific to Yandex.Metrica and used
for implementing the report on session length. ===roundAge(num)=== Accepts a
number. If the number is less than 18, it returns 0. Otherwise, it rounds the
number down to numbers from the set: 18, 25, 35, 45. This function is specific
to Yandex.Metrica and used for implementing the report on user age.
==Conditional functions== ===if(cond, then, else), cond ? then : else
operator=== Returns 'then' if 'cond != 0', or 'else' if 'cond = 0'. 'cond'
must be UInt 8, and 'then' and 'else' must be a type that has the smallest
common type. ==Mathematical functions== All the functions return a Float64
number. The accuracy of the result is close to the maximum precision possible,
but the result might not coincide with the machine representable number
nearest to the corresponding real number. ===e()=== Accepts zero arguments and
returns a Float64 number close to the _e_ number. ===pi()=== Accepts zero
arguments and returns a Float64 number close to _π_. ===exp(x)=== Accepts a
numeric argument and returns a Float64 number close to the exponent of the
argument. ===log(x)=== Accepts a numeric argument and returns a Float64 number
close to the natural logarithm of the argument. ===exp2(x)=== Accepts a
numeric argument and returns a Float64 number close to 2x. ===log2(x)===
Accepts a numeric argument and returns a Float64 number close to the binary
logarithm of the argument. ===exp10(x)=== Accepts a numeric argument and
returns a Float64 number close to 10x. ===log10(x)=== Accepts a numeric
argument and returns a Float64 number close to the decimal logarithm of the
argument. ===sqrt(x)=== Accepts a numeric argument and returns a Float64
number close to the square root of the argument. ===cbrt(x)=== Accepts a
numeric argument and returns a Float64 number close to the cubic root of the
argument. ===erf(x)=== ![](https://upload.wikimedia.org/math/3/4/4/3443265ce8c
b884d9c894401ab15fa71.png) If 'x' is non-negative, then %%erf(x / σ√2)%% is
the probability that a random variable having a normal distribution with
standard deviation 'σ' takes the value that is separated from the expected
value by more than 'x'. Example (three sigma rule): %% SELECT erf(3 / sqrt(2))
┌─erf(divide(3, sqrt(2)))─┐ │ 0.9973002039367398 │ └─────────────────────────┘
%% ===erfc(x)=== Accepts a numeric argument and returns a Float64 number close
to 1 - erf(x), but without loss of precision for large 'x' values.
===lgamma(x)=== The logarithm of the gamma function. ===tgamma(x)=== Gamma
function. ===sin(x)=== The sine. ===cos(x)=== The cosine. ===tan(x)=== The
tangent. ===asin(x)=== The arc sine. ===acos(x)=== The arc cosine.
===atan(x)=== The arc tangent. ===pow(x, y)=== xy. ==Functions for working
with Yandex.Metrica dictionaries== In order for the functions below to work,
the server config must specify the paths and addresses for getting all the
Yandex.Metrica dictionaries. The dictionaries are loaded at the first call of
any of these functions. If the reference lists can't be loaded, an exception
is thrown. For information about creating reference lists, see the section
"Dictionaries". ===Multiple geobases=== ClickHouse supports working with
multiple alternative geobases (regional hierarchies) simultaneously, in order
to support various perspectives on which countries certain regions belong to.
The 'clickhouse-server' config specifies the file with the regional hierarchy:
<path_to_regions_hierarchy_file>/opt/geo/regions_hierarchy.txt</path_to_region
s_hierarchy_file> Besides this file, it also searches for files nearby that
have the _ symbol and any suffix appended to the name (before the file
extension). For example, it will also find the file
%%/opt/geo/regions_hierarchy_ua.txt%%, if present. %%ua%% is called the
dictionary key. For a dictionary without a suffix, the key is an empty string.
All the dictionaries are re-loaded in runtime (once every certain number of
seconds, as defined in the builtin_dictionaries_reload_interval config
parameter, or once an hour by default). However, the list of available
dictionaries is defined one time, when the server starts. All functions for
working with regions have an optional argument at the end - the dictionary
key. It is indicated as the _geobase_. Example: %% regionToCountry(RegionID) -
Uses the default dictionary: /opt/geo/regions_hierarchy.txt
regionToCountry(RegionID, '') - Uses the default dictionary:
/opt/geo/regions_hierarchy.txt regionToCountry(RegionID, 'ua') - Uses the
dictionary for the 'ua' key: /opt/geo/regions_hierarchy_ua.txt %%
===regionToCity(id[, geobase])=== Accepts a UInt32 number - the region ID from
the Yandex geobase. If this region is a city or part of a city, it returns the
region ID for the appropriate city. Otherwise, returns 0. ===regionToArea(id[,
geobase])=== Converts a region to an area (type 5 in the geobase). In every
other way, this function is the same as 'regionToCity'. %% SELECT DISTINCT
regionToName(regionToArea(toUInt32(number), 'ua'), 'en') FROM system.numbers
LIMIT 15 ┌─regionToName(regionToArea(toUInt32(number), \'ua\'), \'en\')─┐ │ │
│ Moscow and Moscow region │ │ Saint-Petersburg and Leningradskaya oblast │ │
Belgorod District │ │ Ivanovo district │ │ Kaluga District │ │ Kostroma
District │ │ Kursk District │ │ Lipetsk District │ │ Orel District │ │ Ryazhan
District │ │ Smolensk District │ │ Tambov District │ │ Tver District │ │ Tula
District │ └──────────────────────────────────────────────────────────────┘ %%
===regionToDistrict(id[, geobase])=== Converts a region to a federal district
(type 4 in the geobase). In every other way, this function is the same as
'regionToCity'. %% SELECT DISTINCT
regionToName(regionToDistrict(toUInt32(number), 'ua'), 'en') FROM
system.numbers LIMIT 15 ┌─regionToName(regionToDistrict(toUInt32(number),
\'ua\'), \'en\')─┐ │ │ │ Central │ │ Northwest │ │ South │ │ North Kavkaz │ │
Volga Region │ │ Ural │ │ Siberian │ │ Far East │ │ Scotland │ │ Faroe Islands
│ │ Flemish Region │ │ Brussels-Capital Region │ │ Wallonia │ │ Federation of
Bosnia and Herzegovina │
└──────────────────────────────────────────────────────────────────┘ %%
===regionToCountry(id[, geobase])=== Converts a region to a country. In every
other way, this function is the same as 'regionToCity'. Example:
%%regionToCountry(toUInt32(213)) = 225%% converts Moscow (213) to Russia
(225). ===regionToContinent(id[, geobase])=== Converts a region to a
continent. In every other way, this function is the same as 'regionToCity'.
Example: %%regionToContinent(toUInt32(213)) = 10001%% converts Moscow (213) to
Eurasia (10001). ===regionToPopulation(id[, geobase])=== Gets the population
for a region. The population can be recorded in files with the geobase. See
the section "External dictionaries". If the population is not recorded for the
region, it returns 0. In the Yandex geobase, the population might be recorded
for child regions, but not for parent regions. ===regionIn(lhs, rhs[,
geobase])=== Checks whether a 'lhs' region belongs to a 'rhs' region. Returns
a UInt8 number equal to 1 if it belongs, or 0 if it doesn't belong. The
relationship is reflexive - any region also belongs to itself.
===regionHierarchy(id[, geobase])=== Accepts a UInt32 number - the region ID
from the Yandex geobase. Returns an array of region IDs consisting of the
passed region and all parents along the chain. Example:
%%regionHierarchy(toUInt32(213)) = [213,1,3,225,10001,10000]%%.
===regionToName(id[, lang])=== Accepts a UInt32 number - the region ID from
the Yandex geobase. A string with the name of the language can be passed as a
second argument. Supported languages are: ru, en, ua, uk, by, kz, tr. If the
second argument is omitted, the language 'ru' is used. If the language is not
supported, an exception is thrown. Returns a string - the name of the region
in the corresponding language. If the region with the specified ID doesn't
exist, an empty string is returned. 'ua' and 'uk' mean the same thing -
Ukrainian. ===OSToRoot=== Accepts a UInt8 number - the ID of the operating
system from the Yandex.Metrica dictionary. If any OS matches the passed
number, it returns a UInt8 number - the ID of the corresponding root OS (for
example, it converts Windows Vista to Windows). Otherwise, returns 0.
===OSIn(lhs, rhs)=== Checks whether the 'lhs' operating system belongs to the
'rhs' operating system. ===OSHierarchy=== Accepts a UInt8 number - the ID of
the operating system from the Yandex.Metrica dictionary. Returns an array with
a hierarchy of operating systems. Similar to the 'regionHierarchy' function.
===SEToRoot=== Accepts a UInt8 number - the ID of the search engine from the
Yandex.Metrica dictionary. If any search engine matches the passed number, it
returns a UInt8 number - the ID of the corresponding root search engine (for
example, it converts Yandex.Images to Yandex). Otherwise, returns 0.
===SEIn(lhs, rhs)=== Checks whether the 'lhs' search engine belongs to the
'rhs' search engine. ===SEHierarchy=== Accepts a UInt8 number - the ID of the
search engine from the Yandex.Metrica dictionary. Returns an array with a
hierarchy of search engines. Similar to the 'regionHierarchy' function.
==Functions for working with external dictionaries== For more information, see
the section "External dictionaries". ===dictGetUInt8, dictGetUInt16,
dictGetUInt32, dictGetUInt64=== ===dictGetInt8, dictGetInt16, dictGetInt32,
dictGetInt64=== ===dictGetFloat32, dictGetFloat64=== ===dictGetDate,
dictGetDateTime=== ===dictGetString=== dictGet_T_('dict_name', 'attr_name',
id) \- Gets the value of the 'attr_name' attribute from the 'dict_name'
dictionary by the 'id' key. 'dict_name' and 'attr_name' are constant strings.
'id' must be UInt64. If the 'id' key is not in the dictionary, it returns the
default value set in the dictionary definition. ===dictIsIn===
%%dictIsIn('dict_name', child_id, ancestor_id)%% \- For the 'dict_name'
hierarchical dictionary, finds out whether the 'child_id' key is located
inside 'ancestor_id' (or matches 'ancestor_id'). Returns UInt8.
===dictGetHierarchy=== %%dictGetHierarchy('dict_name', id)%% \- For the
'dict_name' hierarchical dictionary, returns an array of dictionary keys
starting from 'id' and continuing along the chain of parent elements. Returns
Array(UInt64). ==Functions for working with JSON.== In Yandex.Metrica, JSON is
passed by users as _session parameters_. There are several functions for
working with this JSON. (Although in most of the cases, the JSONs are
additionally pre-processed, and the resulting values are put in separate
columns in their processed format.) All these functions are based on strong
assumptions about what the JSON can be, but they try not to do anything. The
following assumptions are made: 1\. The field name (function argument) must be
a constant. 2\. The field name is somehow canonically encoded in JSON. For
example, %%visitParamHas('{"abc":"def"}', 'abc') = 1%% , but
%%visitParamHas('{"\\\u0061\\\u0062\\\u0063":"def"}', 'abc') = 0%% 3\. Fields
are searched for on any nesting level, indiscriminately. If there are multiple
matching fields, the first occurrence is used. 4\. JSON doesn't have space
characters outside of string literals. ===visitParamHas(params, name)===
Checks whether there is a field with the 'name' name.
===visitParamExtractUInt(params, name)=== Parses UInt64 from the value of the
field named 'name'. If this is a string field, it tries to parse a number from
the beginning of the string. If the field doesn't exist, or it exists but
doesn't contain a number, it returns 0. ===visitParamExtractInt(params,
name)=== The same as for Int64. ===visitParamExtractFloat(params, name)=== The
same as for Float64. ===visitParamExtractBool(params, name)=== Parses a
true/false value. The result is UInt8. ===visitParamExtractRaw(params,
name)=== Returns the value of a field, including separators. Examples:
%%visitParamExtractRaw('{"abc":"\\\n\\\u0000"}', 'abc') = '"\\\n\\\u0000"'%%
%%visitParamExtractRaw('{"abc":{"def":[1,2,3]}}', 'abc') = '{"def":[1,2,3]}'%%
===visitParamExtractString(params, name)=== Parses the string in double
quotes. The value is unescaped. If unescaping failed, it returns an empty
string. Examples: %%visitParamExtractString('{"abc":"\\\n\\\u0000"}', 'abc') =
'\n\0'%% %%visitParamExtractString('{"abc":"\\\u263a"}', 'abc') = '☺'%%
%%visitParamExtractString('{"abc":"\\\u263"}', 'abc') = ''%%
%%visitParamExtractString('{"abc":"hello}', 'abc') = ''%% Currently, there is
no support for code points not from the basic multilingual plane written in
the format \uXXXX\uYYYY (they are converted to CESU-8 instead of UTF-8).
==Functions for implementing the IN operator== ===in, notIn, globalIn,
globalNotIn=== See the section "IN operators". ===tuple(x, y, ...), operator
(x, y, ...)=== \- A function that allows grouping multiple columns. For
columns with the types T1, T2, ..., it returns a Tuple(T1, T2, ...) type tuple
containing these columns. There is no cost to execute the function. Tuples are
normally used as intermediate values for an argument of IN operators, or for
creating a list of formal parameters of lambda functions. Tuples can't be
written to a table. ===tupleElement(tuple, n), operator x.N=== \- A function
that allows getting columns from a tuple. 'N' is the column index, starting
from 1. 'N' must be a constant. 'N' must be a strict postive integer no
greater than the size of the tuple. There is no cost to execute the function.
==Other functions== ===hostName()=== \- Returns a string with the name of the
host that this function was performed on. For distributed processing, this is
the name of the remote server host, if the function is performed on a remote
server. ===visibleWidth(x)=== \- Calculates the approximate width when
outputting values to the console in text format (tab-separated). This function
is used by the system for implementing Pretty formats. ===toTypeName(x)=== \-
Gets the type name. Returns a string containing the type name of the passed
argument. ===blockSize()=== \- Gets the size of the block. In ClickHouse,
queries are always run on blocks (sets of column parts). This function allows
getting the size of the block that you called it for. ===materialize(x)=== \-
Turns a constant into a full column containing just one value. In ClickHouse,
full columns and constants are represented differently in memory. Functions
work differently for constant arguments and normal arguments (different code
is executed), although the result is almost always the same. This function is
for debugging this behavior. ===ignore(...)=== \- A function that accepts any
arguments and always returns 0. However, the argument is still calculated.
This can be used for benchmarks. ===sleep(seconds)=== Sleeps 'seconds' seconds
on each data block. You can specify an integer or a floating-point number.
===currentDatabase()=== Returns the name of the current database. You can use
this function in table engine parameters in a CREATE TABLE query where you
need to specify the database. ===isFinite(x)=== Accepts Float32 and Float64
and returns UInt8 equal to 1 if the argument is not infinite and not a NaN,
otherwise 0. ===isInfinite(x)=== Accepts Float32 and Float64 and returns UInt8
equal to 1 if the argument is infinite, otherwise 0. Note that 0 is returned
for a NaN. ===isNaN(x)=== Accepts Float32 and Float64 and returns UInt8 equal
to 1 if the argument is a NaN, otherwise 0. ===bar=== Allows building a
unicode-art diagram. bar(x, min, max, width) - Draws a band with a width
proportional to (x - min) and equal to 'width' characters when x == max. min,
max - Integer constants. The value must fit in Int64. width - Constant,
positive number, may be a fraction. The band is drawn with accuracy to one
eighth of a symbol. Example: %% SELECT toHour(EventTime) AS h, count() AS c,
bar(c, 0, 600000, 20) AS bar FROM test.hits GROUP BY h ORDER BY h ASC
┌──h─┬──────c─┬─bar────────────────┐ │ 0 │ 292907 │ █████████▋ │ │ 1 │ 180563
│ ██████ │ │ 2 │ 114861 │ ███▋ │ │ 3 │ 85069 │ ██▋ │ │ 4 │ 68543 │ ██▎ │ │ 5 │
78116 │ ██▌ │ │ 6 │ 113474 │ ███▋ │ │ 7 │ 170678 │ █████▋ │ │ 8 │ 278380 │
█████████▎ │ │ 9 │ 391053 │ █████████████ │ │ 10 │ 457681 │ ███████████████▎ │
│ 11 │ 493667 │ ████████████████▍ │ │ 12 │ 509641 │ ████████████████▊ │ │ 13 │
522947 │ █████████████████▍ │ │ 14 │ 539954 │ █████████████████▊ │ │ 15 │
528460 │ █████████████████▌ │ │ 16 │ 539201 │ █████████████████▊ │ │ 17 │
523539 │ █████████████████▍ │ │ 18 │ 506467 │ ████████████████▊ │ │ 19 │
520915 │ █████████████████▎ │ │ 20 │ 521665 │ █████████████████▍ │ │ 21 │
542078 │ ██████████████████ │ │ 22 │ 493642 │ ████████████████▍ │ │ 23 │
400397 │ █████████████▎ │ └────┴────────┴────────────────────┘ %%
===transform=== Transforms a value according to the explicitly defined mapping
of some elements to other ones. There are two variations of this function: 1\.
%%transform(x, array_from, array_to, default)%% %%x%% - What to transform.
%%array_from%% - Constant array of values for converting. %%array_to%% -
Constant array of values to convert the values in 'from' to. %%default%% -
Constant. Which value to use if 'x' is not equal to one of the values in
'from'. 'array_from' and 'array_to' are arrays of the same size. Types:
transform(T, Array(T), Array(U), U) -> U 'T' and 'U' can be numeric, string,
or Date or DateTime types. Where the same letter is indicated (T or U), for
numeric types these might not be matching types, but types that have a common
type. For example, the first argument can have the Int64 type, while the
second has the Array(Uint16) type. If the 'x' value is equal to one of the
elements in the 'array_from' array, it returns the existing element (that is
numbered the same) from the 'array_to' array. Otherwise, it returns 'default'.
If there are multiple matching elements in 'array_from', it returns one of the
matches. Example: %% SELECT transform(SearchEngineID, [2, 3], ['Yandex',
'Google'], 'Others') AS title, count() AS c FROM test.hits WHERE
SearchEngineID != 0 GROUP BY title ORDER BY c DESC ┌─title──┬──────c─┐ │
Yandex │ 498635 │ │ Google │ 229872 │ │ Others │ 104472 │ └────────┴────────┘
%% 2\. %%transform(x, array_from, array_to)%% Differs from the first variation
in that the 'default' argument is omitted. If the 'x' value is equal to one of
the elements in the 'array_from' array, it returns the matching element (that
is numbered the same) from the 'array_to' array. Otherwise, it returns 'x'.
Types: transform(T, Array(T), Array(T)) -> T Example: %% SELECT
transform(domain(Referer), ['yandex.ru', 'google.ru', 'vk.com'],
['www.yandex', 'ввв.яндекс.рф', 'example.com']) AS s, count() AS c FROM
test.hits GROUP BY domain(Referer) ORDER BY count() DESC LIMIT 10
┌─s──────────────┬───────c─┐ │ │ 2906259 │ │ www.yandex │ 867767 │ │
███████.ru │ 313599 │ │ mail.yandex.ru │ 107147 │ │ ввв.яндекс.рф │ 105668 │ │
██████.ru │ 100355 │ │ █████████.ru │ 65040 │ │ news.yandex.ru │ 64515 │ │
██████.net │ 59141 │ │ example.com │ 57316 │ └────────────────┴─────────┘ %%
==arrayJoin function== This is a very unusual function. Normal functions don't
change a set of rows, but just change the values in each row (map). Aggregate
functions compress a set of rows (fold or reduce). The 'arrayJoin' function
takes each row and generates a set of rows (unfold). This function takes an
array as an argument, and propagates the source row to multiple rows for the
number of elements in the array. All the values in columns are simply copied,
except the values in the column where this function is applied - it is
replaced with the corresponding array value. A query can use multiple
'arrayJoin' functions. In this case, the transformation is performed multiple
times. Note the ARRAY JOIN syntax in the SELECT query, which provides broader
possibilities. Example: %% :) SELECT arrayJoin([1, 2, 3] AS src) AS dst,
'Hello', src SELECT arrayJoin([1, 2, 3] AS src) AS dst, 'Hello', src
┌─dst─┬─\'Hello\'─┬─src─────┐ │ 1 │ Hello │ [1,2,3] │ │ 2 │ Hello │ [1,2,3] │
│ 3 │ Hello │ [1,2,3] │ └─────┴───────────┴─────────┘ %%

# Aggregate functions

==count()== Counts the number of rows. Accepts zero arguments and returns
UInt64. The syntax COUNT(DISTINCT x) is not supported. The separate 'uniq'
aggregate function exists for this purpose. A 'SELECT count() FROM table'
query is not optimized, because the number of entries in the table is not
stored separately. It will select some small column from the table and count
the number of values in it. ==any(x)== Selects the first encountered value.
The query can be executed in any order and even in a different order each
time, so the result of this function is indeterminate. To get a determinate
result, you can use the 'min' or 'max' function instead of 'any'. In some
cases, you can rely on the order of execution. This applies to cases when
SELECT comes from a subquery that uses ORDER BY. When a SELECT query has the
GROUP BY clause or at least one aggregate function, ClickHouse (in contrast to
MySQL) requires that all expressions in the SELECT, HAVING, and ORDER BY
clauses be calculated from keys or from aggregate functions. That is, each
column selected from the table must be used either in keys, or inside
aggregate functions. To get behavior like in MySQL, you can put the other
columns in the 'any' aggregate function. ==anyLast(x)== Selects the last value
encountered. The result is just as indeterminate as for the 'any' function.
==min(x)== Calculates the minimum. ==max(x)== Calculates the maximum.
==argMin(arg, val)== Calculates the 'arg' value for a minimal 'val' value. If
there are several different values of 'arg' for minimal values of 'val', the
first of these values encountered is output. ==argMax(arg, val)== Calculates
the 'arg' value for a maximum 'val' value. If there are several different
values of 'arg' for maximum values of 'val', the first of these values
encountered is output. ==sum(x)== Calculates the sum. Only works for numbers.
==avg(x)== Calculates the average. Only works for numbers. The result is
always Float64. ==uniq(x)== Calculates the approximate number of different
values of the argument. Works for numbers, strings, dates, and dates with
times. Uses an adaptive sampling algorithm: for the calculation state, it uses
a sample of element hash values with a size up to 65535. Compared with the
widely known HyperLogLog algorithm, this algorithm is less effective in terms
of accuracy and memory consumption (even up to proportionality), but it is
adaptive. This means that with fairly high accuracy, it consumes less memory
during simultaneous computation of cardinality for a large number of data sets
whose cardinality has power law distribution (i.e. in cases when most of the
data sets are small). This algorithm is also very accurate for data sets with
small cardinality (up to 65536) and very efficient on CPU (when computing not
too many of these functions, using 'uniq' is almost as fast as using other
aggregate functions). There is no compensation for the bias of an estimate, so
for large data sets the results are systematically deflated. This function is
normally used for computing the number of unique visitors in Yandex.Metrica,
so this bias does not play a role. The result is determinate (it doesn't
depend on the order of query execution). ==uniqHLL12(x)== Uses the HyperLogLog
algorithm to approximate the number of different values of the argument. It
uses 212 5-bit cells. The size of the state is slightly more than 2.5 KB. The
result is determinate (it doesn't depend on the order of query execution). In
most cases, use the 'uniq' function. You should only use this function if you
understand its advantages well. ==uniqExact(x)== Calculates the number of
different values of the argument, exactly. There is no reason to fear
approximations, so it's better to use the 'uniq' function. You should use the
'uniqExact' function if you definitely need an exact result. The 'uniqExact'
function uses more memory than the 'uniq' function, because the size of the
state has unbounded growth as the number of different values increases.
==groupArray(x)== Creates an array of argument values. Values can be added to
the array in any (indeterminate) order. In some cases, you can rely on the
order of execution. This applies to cases when SELECT comes from a subquery
that uses ORDER BY. ==groupUniqArray(x)== Creates an array from different
argument values. Memory consumption is the same as for the 'uniqExact'
function. ==median(x)== Approximates the median. Also see the similar
'quantile' function. Works for numbers, dates, and dates with times. For
numbers it returns Float64, for dates - a date, and for dates with times - a
date with time. Uses reservoir sampling with a reservoir size up to 8192. If
necessary, the result is output with linear approximation from the two
neighboring values. This algorithm proved to be more practical than another
well-known algorithm - QDigest. The result depends on the order of running the
query, and is nondeterministic. ==medianTiming(x)== Calculates the median with
fixed accuracy. Works for numbers. Intended for calculating medians of page
loading time in milliseconds. Also see the similar 'quantileTiming' function.
If the value is greater than 30,000 (a page loading time of more than 30
seconds), the result is equated to 30,000. If the value is less than 1024, the
calculation is exact. If the value is from 1025 to 29,000, the calculation is
rounded to a multiple of 16. In addition, if the total number of values passed
to the aggregate function was less than 32, the calculation is exact. When
passing negative values to the function, the behavior is undefined. The
returned value has the Float32 type. If no values were passed to the function
(when using 'medianTimingIf' or 'quantileTimingIf'), 'nan' is returned. The
purpose of this is to differentiate these instances from zeros. See the note
on sorting NaNs in "ORDER BY clause". The result is determinate (it doesn't
depend on the order of query execution). For its purpose (calculating
quantiles of page loading times), using this function is more effective and
the result is more accurate than for the 'median/quantile' function.
==medianDeterministic(x, determinator)== This function works similarly to the
'median' function - it approximates the median. However, in contrast to
'median', the result is deterministic and does not depend on the order of
query execution. To achieve this, the function takes a second argument - the
"determinator". This is a number whose hash is used instead of a random number
generator in the reservoir sampling algorithm. For the function to work
correctly, the same determinator value should not occur too often. For the
determinator, you can use an event ID, user ID, and so on. Don't use this
function for calculating timings. The 'medianTiming', 'quantileTiming', and
'quantilesTiming' functions are better suited to this purpose.
==medianTimingWeighted(x, weight)== Differs from the 'medianTiming' function
in that it has a second argument - "weights". Weight is a non-negative
integer. The result is calculated as if the 'x' value were passed 'weight'
number of times to the 'medianTiming' function. ==varSamp(x)== Calculates the
amount Σ((x - x̅)2) / (n - 1), where 'n' is the sample size and 'x̅' is the
average value of 'x'. It represents an unbiased estimate of the variance of a
random variable, if the values passed to the function are a sample of this
random amount. Returns Float64. If n <= 1, it returns +∞. ==varPop(x)==
Calculates the amount Σ((x - x̅)2) / n, where 'n' is the sample size and 'x̅'
is the average value of 'x'. In other words, dispersion for a set of values.
Returns Float64. ==stddevSamp(x)== The result is equal to the square root of
'varSamp(x)'. ==stddevPop(x)== The result is equal to the square root of
'varPop(x)'. ==covarSamp(x, y)== Calculates the value of %%Σ((x - x̅)(y - y̅))
/ (n - 1)%%. Returns Float64. If n <= 1, it returns +∞. ==covarPop(x, y)==
Calculates the value of %%Σ((x - x̅)(y - y̅)) / n%%. ==corr(x, y)== Calculates
the Pearson correlation coefficient: Σ((x - x̅)(y - y̅)) / sqrt(Σ((x - x̅)2) *
Σ((y - y̅)2)). ==Parametric aggregate functions== Some aggregate functions can
accept not only argument columns (used for compression), but a set of
parameters - constants for initialization. The syntax is two pairs of brackets
instead of one. The first is for parameters, and the second is for arguments.
==quantile(level)(x)== Approximates the 'level' quantile. 'level' is a
constant, a floating-point number from 0 to 1. We recommend using a 'level'
value in the range of 0.01 .. 0.99. Don't use a 'level' value equal to 0 or 1
- use the 'min' and 'max' functions for these cases. The algorithm is the same
as for the 'median' function. Actually, 'quantile' and 'median' are internally
the same function. You can use the 'quantile' function without parameters - in
this case, it calculates the median, and you can use the 'median' function
with parameters - in this case, it calculates the quantile of the set level.
When using multiple 'quantile' and 'median' functions with different levels in
a query, the internal states are not combined (that is, the query works less
efficiently than it could). In this case, use the 'quantiles' function.
==quantiles(level1, level2, ...)(x)== Approximates quantiles of all specified
levels. The result is an array containing the corresponding number of values.
==quantileTiming(level)(x)== Calculates the quantile of 'level' using the same
algorithm as the 'medianTiming' function. ==quantilesTiming(level1, level2,
...)(x)== Calculates the quantiles of all specified levels using the same
algorithm as the 'medianTiming' function. ==quantileTimingWeighted(level)(x,
weight)== Calculates the quantile of 'level' using the same algorithm as the
'medianTimingWeighted' function. ==quantilesTimingWeighted(level1, level2,
...)(x, weight)== Calculates the quantiles of all specified levels using the
same algorithm as the 'medianTimingWeighted' function.
==quantileDeterministic(level)(x, determinator)== Calculates the quantile of
'level' using the same algorithm as the 'medianDeterministic' function.
==quantilesDeterministic(level1, level2, ...)(x, determinator)== Calculates
the quantiles of all specified levels using the same algorithm as the
'medianDeterministic' function. ==sequenceMatch(pattern)(time, cond1, cond2,
...)== Pattern matching for event chains. 'pattern' is a string containing a
pattern to match. The pattern is similar to a regular expression. 'time' is
the event time of the DateTime type. 'cond1, cond2 ...' are from one to 32
arguments of the UInt8 type that indicate whether an event condition was met.
The function collects a sequence of events in RAM. Then it checks whether this
sequence matches the pattern. It returns UInt8 - 0 if the pattern isn't
matched, or 1 if it matches. Example: %%sequenceMatch('(?1).*(?2)')(EventTime,
URL LIKE '%company%', URL LIKE '%cart%')%% \- whether there was a chain of
events in which pages with the address in %%company%% were visited earlier
than pages with the address in %%cart%%. This is a degenerate example. You
could write it using other aggregate functions: %%minIf(EventTime, URL LIKE
'%company%') < maxIf(EventTime, URL LIKE '%cart%')%%. However, there is no
such solution for more complex situations. Pattern syntax: %%(?1)%% -
Reference to a condition (any number in place of 1). %%.*%% - Any number of
events. (?t>=1800) \- Time condition. Any quantity of any type of events is
allowed over the specified time. The operators <, >, <= may be used instead of
>=. Any number may be specified in place of 1800. Events that occur during the
same second may be put in the chain in any order. This may affect the result
of the function. ==uniqUpTo(N)(x)== Calculates the number of different
argument values, if it is less than or equal to N. If the number of different
argument values is greater than N, it returns N + 1. Recommended for use with
small Ns, up to 10. The maximum N value is 100. For the state of an aggregate
function, it uses the amount of memory equal to 1 + N * the size of one value
of bytes. For strings, it stores a non-cryptographic hash of 8 bytes. That is,
the calculation is approximated for strings. It works as fast as possible,
except for cases when a large N value is used and the number of unique values
is slightly less than N. Usage example: Problem: Generate a report that shows
only keywords that produced at least 5 unique users. Solution: Write in the
query GROUP BY SearchPhrase HAVING uniqUpTo(4)(UserID) >= 5 ==Aggregate
function combinators== The name of an aggregate function can have a suffix
appended to it. This changes the way the aggregate function works. There are
%%If%% and %%Array%% combinators. See the sections below. ==-If combinator.
Conditional aggregate functions== The suffix -%%If%% can be appended to the
name of any aggregate function. In this case, the aggregate function accepts
an extra argument - a condition (Uint8 type). The aggregate function processes
only the rows that trigger the condition. If the condition was not triggered
even once, it returns a default value (usually zeros or empty strings).
Examples: %%countIf(cond)%%, %%avgIf(x, cond)%%, %%quantilesTimingIf(level1,
level2)(x, cond)%%, %%argMinIf(arg, val, cond)%% and so on. You can use
aggregate functions to calculate aggregates for multiple conditions at once,
without using subqueries and JOINs. For example, in Yandex.Metrica, we use
conditional aggregate functions for implementing segment comparison
functionality. ==-Array combinator. Aggregate functions for array arguments==
The -%%Array%% suffix can be appended to any aggregate function. In this case,
the aggregate function takes arguments of the 'Array(T)' type (arrays) instead
of 'T' type arguments. If the aggregate function accepts multiple arguments,
this must be arrays of equal lengths. When processing arrays, the aggregate
function works like the original aggregate function across all array elements.
Example 1: %%sumArray(arr)%% - Totals all the elements of all 'arr' arrays. In
this example, it could have been written more simply: %%sum(arraySum(arr))%%.
Example 2: %%uniqArray(arr)%% - Count the number of unique elements in all
'arr' arrays. This could be done an easier way: %%uniq(arrayJoin(arr))%%, but
it's not always possible to add 'arrayJoin' to a query. The -%%If%% and
-%%Array%% combinators can be used together. However, 'Array' must come first,
then 'If'. Examples: %%uniqArrayIf(arr, cond)%%,
%%quantilesTimingArrayIf(level1, level2)(arr, cond)%%. Due to this order, the
'cond' argument can't be an array. ==-State combinator== ==-Merge combinator==

# Dictionaries

A dictionary is a mapping (key -> attributes) that can be used in a query as
functions. You can think of this as a more convenient and efficient type of
JOIN with dimension tables. There are built-in (internal) and add-on
(external) dictionaries. ==Internal dictionaries== ClickHouse contains a
built-in feature for working with a geobase. This allows you to: \- Use a
region's ID to get its name in the desired language. \- Use a region's ID to
get the ID of a city, area, federal district, country, or continent. \- Check
whether a region is part of another region. \- Get a chain of parent regions.
All the functions support "translocality," the ability to simultaneously use
different perspectives on region ownership. For more information, see the
section "Functions for working with Yandex.Metrica dictionaries". The internal
dictionaries are disabled in the default package. To enable them, uncomment
the parameters 'path_to_regions_hierarchy_file' and
'path_to_regions_names_files' in the server config file. The geobase is loaded
from text files. If you are Yandex employee, to create them, use the following
instructions: https://github.yandex-team.ru/raw/Metrika/ClickHouse_private/mas
ter/doc/create_embedded_geobase_dictionaries.txt Put the
regions_hierarchy*.txt files in the path_to_regions_hierarchy_file directory.
This configuration parameter must contain the path to the
regions_hierarchy.txt file (the default regional hierarchy), and the other
files (regions_hierarchy_ua.txt) must be located in the same directory. Put
the regions_names_*.txt files in the path_to_regions_names_files directory.
You can also create these files yourself. The file format is as follows:
regions_hierarchy*.txt: TabSeparated (no header), columns: \- Region ID
(UInt32) \- Parent region ID (UInt32) \- Region type (UInt8): 1 - continent, 3
- country, 4 - federal district, 5 - region, 6 - city; other types don't have
values. \- Population (UInt32) - Optional column. regions_names_*.txt:
TabSeparated (no header), columns: \- Region ID (UInt32) \- Region name
(String) - Can't contain tabs or line breaks, even escaped ones. A flat array
is used for storing in RAM. For this reason, IDs shouldn't be more than a
million. Dictionaries can be updated without the server restart. However, the
set of available dictionaries is not updated. For updates, the file
modification times are checked. If a file has changed, the dictionary is
updated. The interval to check for changes is configured in the
'builtin_dictionaries_reload_interval' parameter. Dictionary updates (other
than loading at first use) do not block queries. During updates, queries use
the old versions of dictionaries. If an error occurs during an update, the
error is written to the server log, while queries continue using the old
version of dictionaries. We recommend periodically updating the dictionaries
with the geobase. During an update, generate new files and write them to a
separate location. When everything is ready, rename them to the files used by
the server. There are also functions for working with OS identifiers and
Yandex.Metrica search engines, but they shouldn't be used. ==External
dictionaries== It is possible to add your own dictionaries from various data
sources. The data source for a dictionary can be a file in the local file
system, the ClickHouse server, or a MySQL server. A dictionary can be stored
completely in RAM and updated regularly, or it can be partially cached in RAM
and dynamically load missing values. The configuration of external
dictionaries is in a separate file or files specified in the
'dictionaries_config' configuration parameter. This parameter contains the
absolute or relative path to the file with the dictionary configuration. A
relative path is relative to the directory with the server config file. The
path can contain wildcards * and ?, in which case all matching files are
found. Example: dictionaries/*.xml. The dictionary configuration, as well as
the set of files with the configuration, can be updated without restarting the
server. The server checks updates every 5 seconds. This means that
dictionaries can be enabled dynamically. Dictionaries can be created when
starting the server, or at first use. This is defined by the
'dictionaries_lazy_load' parameter in the main server config file. This
parameter is optional, 'true' by default. If set to 'true', each dictionary is
created at first use. If dictionary creation failed, the function that was
using the dictionary throws an exception. If 'false', all dictionaries are
created when the server starts, and if there is an error, the server shuts
down. The dictionary config file has the following format: %% <dictionaries>
<comment>Optional element with any content; completely ignored.</comment> <!--
You can set any number of different dictionaries. --> <dictionary> <!--
Dictionary name. The dictionary will be accessed for use by this name. -->
<name>os</name> <!-- Data source. --> <source> <!-- Source is a file in the
local file system. --> <file> <!-- Path on the local file system. -->
<path>/opt/dictionaries/os.tsv</path> <!-- Which format to use for reading the
file. --> <format>TabSeparated</format> </file> <!-- or the source is a table
on a MySQL server. <mysql> <!- - These parameters can be specified outside
(common for all replicas) or inside a specific replica - -> <port>3306</port>
<user>metrika</user> <password>qwerty</password> <!- - Specify from one to any
number of replicas for fault tolerance. - -> <replica>
<host>example01-1</host> <priority>1</priority> <!- - The lower the value, the
higher the priority. - -> </replica> <replica> <host>example01-2</host>
<priority>1</priority> </replica> <db>conv_main</db> <table>counters</table>
</mysql> \--> <!-- or the source is a table on the ClickHouse server.
<clickhouse> <host>example01-01-1</host> <port>9000</port>
<user>default</user> <password></password> <db>default</db>
<table>counters</table> </clickhouse> <!- - If the address is similar to
localhost, the request is made without network interaction. For fault
tolerance, you can create a Distributed table on localhost and enter it. - ->
\--> </source> <!-- Update interval for fully loaded dictionaries. 0 - never
update. --> <lifetime> <min>300</min> <max>360</max> <!-- The update interval
is selected uniformly randomly between min and max, in order to spread out the
load when updating dictionaries on a large number of servers. --> </lifetime>
<!-- or <!- - The update interval for fully loaded dictionaries or
invalidation time for cached dictionaries. 0 - never update. - ->
<lifetime>300</lifetime> \--> <layout> <!-- Method for storing in memory. -->
<flat /> <!-- or <hashed /> or <cache> <!- - Cache size in number of cells;
rounded up to a degree of two. - -> <size_in_cells>1000000000</size_in_cells>
</cache> --> </layout> <!-- Structure. --> <structure> <!-- Description of the
column that serves as the dictionary identifier (key). --> <id> <!-- Column
name with ID. --> <name>Id</name> </id> <attribute> <!-- Column name. -->
<name>Name</name> <!-- Column type. (How the column is understood when
loading. For MySQL, a table can have TEXT, VARCHAR, and BLOB, but these are
all loaded as String) --> <type>String</type> <!-- Value to use for a non-
existing element. In the example, an empty string. -->
<null_value></null_value> </attribute> <!-- Any number of attributes can be
specified. --> <attribute> <name>ParentID</name> <type>UInt64</type>
<null_value>0</null_value> <!-- Whether it defines a hierarchy - mapping to
the parent ID (by default, false). --> <hierarchical>true</hierarchical> <!--
The mapping id -> attribute can be considered injective, in order to optimize
GROUP BY. (by default, false) --> <injective>true</injective> </attribute>
</structure> </dictionary> </dictionaries> %% The dictionary identifier (key
attribute) must be a number that fits into UInt64. Compound and string keys
are not supported. However, if your dictionary has a complex key, you can hash
it and use the hash as the key. You can use View for this purpose (in both
ClickHouse and MySQL). There are three ways to store dictionaries in memory.
1\. %%flat%% - As flat arrays. This is the most effective method. It works if
all keys are smaller than 500,000. If a larger key is discovered when creating
the dictionary, an exception is thrown and the dictionary is not created. The
dictionary is loaded to RAM in its entirety. The dictionary uses the amount of
memory proportional to maximum key value. With the limit of 500,000, memory
consumption is not likely to be high. All types of sources are supported. When
updating, data (from a file or from a table) is read in its entirety. 2\.
%%hashed%% - As hash tables. This method is slightly less effective than the
first one. The dictionary is also loaded to RAM in its entirety, and can
contain any number of items with any identifiers. In practice, it makes sense
to use up to tens of millions of items, while there is enough RAM. All types
of sources are supported. When updating, data (from a file or from a table) is
read in its entirety. 3\. %%cache%% - This is the least effective method. It
is appropriate if the dictionary doesn't fit in RAM. It is a cache of a fixed
number of cells, where frequently-used data can be located. MySQL and
ClickHouse sources are supported, but file sources are not supported. When
searching a dictionary, the cache is searched first. For each data block, all
keys not found in the cache (or expired keys) are collected in a package,
which is sent to the source with the query %%SELECT attrs... FROM db.table
WHERE id IN (k1, k2, ...)%%. The received data is then written to the cache.
We recommend using the flat method when possible, or hashed. The speed of the
dictionaries is impeccable with this type of memory storage. Use the cache
method only in cases when it is unavoidable. The speed of the cache depends
strongly on correct settings and the usage scenario. A cache type dictionary
only works normally for high enough hit rates (recommended 99% and higher).
You can view the average hit rate in the system.dictionaries table. Set a
large enough cache size. You will need to experiment to find the right number
of cells - select a value, use a query to get the cache completely full, look
at the memory consumption (this information is in the system.dictionaries
table), then proportionally increase the number of cells so that a reasonable
amount of memory is consumed. We recommend MySQL as the source for the cache,
because ClickHouse doesn't handle requests with random reads very well. In all
cases, performance is better if you call the function for working with a
dictionary after GROUP BY, and if the attribute being fetched is marked as
injective. For a dictionary cache, performance improves if you call the
function after LIMIT. To do this, you can use a subquery with LIMIT, and call
the function with the dictionary from the outside. An attribute is called
injective if different attribute values correspond to different keys. So when
GROUP BY uses a function that fetches an attribute value by the key, this
function is automatically taken out of GROUP BY. When updating dictionaries
from a file, first the file modification time is checked, and it is loaded
only if the file has changed. When updating from MySQL, for flat and hashed
dictionaries, first a SHOW TABLE STATUS query is made, and the table update
time is checked. If it is not NULL, it is compared to the stored time. This
works for MyISAM tables, but for InnoDB tables the update time is unknown, so
loading from InnoDB is performed on each update. For cache dictionaries, the
expiration (lifetime) of data in the cache can be set. If more time than
'lifetime' has passed since loading the data in a cell, the cell's value is
not used, and it is re-requested the next time it needs to be used. If a
dictionary couldn't be loaded even once, an attempt to use it throws an
exception. If an error occurred during a request to a cached source, an
exception is thrown. Dictionary updates (other than loading for first use) do
not block queries. During updates, the old version of a dictionary is used. If
an error occurs during an update, the error is written to the server log, and
queries continue using the old version of dictionaries. You can view the list
of external dictionaries and their status in the system.dictionaries table. To
use external dictionaries, see the section "Functions for working with
external dictionaries". Note that you can convert values for a small
dictionary by specifying all the contents of the dictionary directly in a
SELECT query (see the section "transform function"). This functionality is not
related to external dictionaries.

# Settings

In this section, we review settings that you can make using a SET query or in
a config file. Remember that these settings can be set for a session or
globally. Settings that can only be made in the server config file are not
covered here. ==max_block_size== In ClickHouse, data is processed by blocks
(sets of column parts). The internal processing cycles for a single block are
efficient enough, but there are noticeable expenditures on each block.
'max_block_size' is a recommendation for what size of block (in number of
rows) to load from tables. The block size shouldn't be too small, so that the
expenditures on each block are still noticeable, but not too large, so that
the query with LIMIT that is completed after the first block is processed
quickly, so that too much memory isn't consumed when extracting a large number
of columns in multiple threads, and so that at least some cache locality is
preserved. By default, it is 65,536. Blocks the size of 'max_block_size' are
not always loaded from the table. If it is obvious that less data needs to be
retrieved, a smaller block is processed. ==max_insert_block_size== The size of
blocks to form for insertion into a table. This setting only applies in cases
when the server forms the blocks. For example, for an INSERT via the HTTP
interface, the server parses the data format and forms blocks of the specified
size. But when using clickhouse-client, the client parses the data itself, and
the 'max_insert_block_size' setting on the server doesn't affect the size of
the inserted blocks. The setting also doesn't have a purpose when using INSERT
SELECT, since data is inserted in the same blocks that are formed after
SELECT. By default, it is 1,048,576. This is slightly more than
'max_block_size'. The reason for this is because certain table engines
(*MergeTree) form a data part on the disk for each inserted block, which is a
fairly large entity. Similarly, *MergeTree tables sort data during insertion,
and a large enough block size allows sorting more data in RAM. ==max_threads==
The maximum number of query processing threads \- excluding threads for
retrieving data from remote servers (see the 'max_distributed_connections'
parameter). This parameter applies to threads that perform the same stages of
the query execution pipeline in parallel. For example, if reading from a
table, evaluating expressions with functions, filtering with WHERE and pre-
aggregating for GROUP BY can all be done in parallel using at least
'max_threads' number of threads, then 'max_threads' are used. By default, 8.
If less than one SELECT query is normally run on a server at a time, set this
parameter to a value slightly less than the actual number of processor cores.
For queries that are completed quickly because of a LIMIT, you can set a lower
'max_threads'. For example, if the necessary number of entries are located in
every block and max_threads = 8, 8 blocks are retrieved, although it would
have been enough to read just one. The smaller the 'max_threads' value, the
less memory is consumed. ==max_compress_block_size== The maximum size of
blocks of uncompressed data before compressing for writing to a table. By
default, 1,048,576 (1 MiB). If the size is reduced, the compression rate is
significantly reduced, the compression and decompression speed increases
slightly due to cache locality, and memory consumption is reduced. There
usually isn't any reason to change this setting. Don't confuse blocks for
compression (a chunk of memory consisting of bytes) and blocks for query
processing (a set of rows from a table). ==min_compress_block_size== For
*MergeTree tables. In order to reduce latency when processing queries, a block
is compressed when writing the next mark if its size is at least
'min_compress_block_size'. By default, 65,536. The actual size of the block,
if the uncompressed data less than 'max_compress_block_size' is no less than
this value and no less than the volume of data for one mark. Let's look at an
example. Assume that 'index_granularity' was set to 8192 during table
creation. We are writing a UInt32-type column (4 bytes per value). When
writing 8192 rows, the total will be 32 KB of data. Since
min_compress_block_size = 65,536, a compressed block will be formed for every
two marks. We are writing a URL column with the String type (average size of
60 bytes per value). When writing 8192 rows, the average will be slightly less
than 500 KB of data. Since this is more than 65,536, a compressed block will
be formed for each mark. In this case, when reading data from the disk in the
range of a single mark, extra data won't be decompressed. There usually isn't
any reason to change this setting. ==max_query_size== The maximum part of a
query that can be taken to RAM for parsing with the SQL parser. The INSERT
query also contains data for INSERT that is processed by a separate stream
parser (that consumes O(1) RAM), which is not included in this restriction. By
default, 64 KiB. ==interactive_delay== The interval in microseconds for
checking whether request execution has been canceled and sending the progress.
By default, 100,000 (check for canceling and send progress ten times per
second). ==connect_timeout== ==receive_timeout== ==send_timeout== Timeouts in
seconds on the socket used for communicating with the client. By default, 10,
300, 300. ==poll_interval== Lock in a wait loop for the specified number of
seconds. By default, 10. ==max_distributed_connections== The maximum number of
simultaneous connections with remote servers for distributed processing of a
single query to a single Distributed table. We recommend setting a value no
less than the number of servers in the cluster. By default, 100. The following
parameters are only used when creating Distributed tables (and when launching
a server), so there is no reason to change them at runtime.
==distributed_connections_pool_size== The maximum number of simultaneous
connections with remote servers for distributed processing of all queries to a
single Distributed table. We recommend setting a value no less than the number
of servers in the cluster. By default, 128.
==connect_timeout_with_failover_ms== The timeout in milliseconds for
connecting to a remote server for a Distributed table engine, if the 'shard'
and 'replica' sections are used in the cluster definition. If unsuccessful,
several attempts are made to connect to various replicas. By default, 50.
==connections_with_failover_max_tries== The maximum number of connection
attempts with each replica, for the Distributed table engine. By default, 3.
==extremes== Whether to count extreme values (the minimums and maximums in
columns of a query result). Accepts 0 or 1. By default, 0 (disabled). For more
information, see the section "Extreme values". ==use_uncompressed_cache==
Whether to use a cache of uncompressed blocks. Accepts 0 or 1. By default, 0
(disabled). The uncompressed cache (only for tables in the MergeTree family)
allows significantly reducing latency and increasing throughput when working
with a large number of short queries. Enable this setting for users who send
frequent short requests. Also pay attention to the 'uncompressed_cache_size'
configuration parameter (only set in the config file) - the size of
uncompressed cache blocks. By default, it is 8 GiB. The uncompressed cache is
filled in as needed; the least-used data is automatically deleted. For queries
that read at least a somewhat large volume of data (one million rows or more),
the uncompressed cache is disabled automatically in order to save space for
truly small queries. So you can keep the 'use_uncompressed_cache' setting
always set to 1. ==replace_running_query== When using the HTTP interface, the
'query_id' parameter can be passed. This is any string that serves as the
query identifier. If a query from the same user with the same 'query_id'
already exists at this time, the behavior depends on the
'replace_running_query' parameter. 0 (default) - Throw an exception (don't
allow the query to run if a query with the same 'query_id' is already
running). 1 - Cancel the old query and start running the new one.
Yandex.Metrica uses this parameter set to 1 for implementing suggestions for
segmentation conditions. After entering the next character, if the old query
hasn't finished yet, it should be canceled. ==load_balancing== Which replicas
(among healthy replicas) to preferably send a query to (on the first attempt)
for distributed processing. **random** (default) The number of errors is
counted for each replica. The query is sent to the replica with the fewest
errors, and if there are several of these, to any one of them. Disadvantages:
Server proximity is not accounted for; if the replicas have different data,
you will also get different data. **nearest_hostname** The number of errors is
counted for each replica. Every 5 minutes, the number of errors is integrally
divided by 2. Thus, the number of errors is calculated for a recent time with
exponential smoothing. If there is one replica with a minimal number of errors
(i.e. errors occurred recently on the other replicas), the query is sent to
it. If there are multiple replicas with the same minimal number of errors, the
query is sent to the replica with a host name that is most similar to the
server's host name in the config file (for the number of different characters
in identical positions, up to the minimum length of both host names). As an
example, example01-01-1 and example01-01-2.yandex.ru are different in one
position, while example01-01-1 and example01-02-2 differ in two places. This
method might seem a little stupid, but it doesn't use external data about
network topology, and it doesn't compare IP addresses, which would be
complicated for our IPv6 addresses. Thus, if there are equivalent replicas,
the closest one by name is preferred. We can also assume that when sending a
query to the same server, in the absence of failures, a distributed query will
also go to the same servers. So even if different data is placed on the
replicas, the query will return mostly the same results. **in_order** Replicas
are accessed in the same order as they are specified. The number of errors
does not matter. This method is appropriate when you know exactly which
replica is preferable. ==totals_mode== How to calculate TOTALS when HAVING is
present, as well as when max_rows_to_group_by and group_by_overflow_mode =
'any' are present. See the section "WITH TOTALS modifier".
==totals_auto_threshold== The threshold for totals_mode = 'auto'. See the
section "WITH TOTALS modifier". ==default_sample== A floating-point number
from 0 to 1. By default, 1. Allows setting a default sampling coefficient for
all SELECT queries. (For tables that don't support sampling, an exception will
be thrown.) If set to 1, default sampling is not performed.
====input_format_skip_unknown_fields== If the parameter is true, INSERT
operation will skip columns with unknown names from input. Otherwise, an
exception will be generated. The parameter works only for JSONEachRow and TSKV
input formats. ==Restrictions on query complexity== Restrictions on query
complexity are part of the settings. They are used in order to provide safer
execution from the user interface. Almost all the restrictions only apply to
SELECTs. For distributed query processing, restrictions are applied on each
server separately. Restrictions on the "maximum amount of something" can take
the value 0, which means "unrestricted". Most restrictions also have an
'overflow_mode' setting, meaning what to do when the limit is exceeded. It can
take one of two values: 'throw' or 'break'. Restrictions on aggregation
(group_by_overflow_mode) also have the value 'any'. throw - Throw an exception
(default). break - Stop executing the query and return the partial result, as
if the source data ran out. any (only for group_by_overflow_mode) - Continuing
aggregation for the keys that got into the set, but don't add new keys to the
set. ===readonly=== If set to 1, run only queries that don't change data or
settings. As an example, SELECT and SHOW queries are allowed, but INSERT and
SET are forbidden. After you write %%SET readonly = 1%%, you can't disable
readonly mode in the current session. When using the GET method in the HTTP
interface, 'readonly = 1' is set automatically. In other words, for queries
that modify data, you can only use the POST method. You can send the query
itself either in the POST body, or in the URL parameter.
===max_memory_usage=== The maximum amount of memory consumption when running a
query on a single server. By default, 10 GB. The setting doesn't consider the
volume of available memory or the total volume of memory on the machine. The
restriction applies to a single query within a single server. You can use SHOW
PROCESSLIST to see the current memory consumption for each query. In addition,
the peak memory consumption is tracked for each query and written to the log.
Certain cases of memory consumption are not tracked: \- Large constants (for
example, a very long string constant). \- The states of 'groupArray' aggregate
functions, and also 'quantile' (it is tracked for 'quantileTiming'). Memory
consumption is not fully considered for aggregate function states 'min',
'max', 'any', 'anyLast', 'argMin', and 'argMax' from String and Array
arguments. ===max_rows_to_read=== The following restrictions can be checked on
each block (instead of on each row). That is, the restrictions can be broken a
little. When running a query in multiple threads, the following restrictions
apply to each thread separately. Maximum number of rows that can be read from
a table when running a query. ===max_bytes_to_read=== Maximum number of bytes
(uncompressed data) that can be read from a table when running a query.
===read_overflow_mode=== What to do when the volume of data read exceeds one
of the limits: 'throw' or 'break'. By default, throw.
===max_rows_to_group_by=== Maximum number of unique keys received from
aggregation. This setting lets you limit memory consumption when aggregating.
===group_by_overflow_mode=== What to do when the number of unique keys for
aggregation exceeds the limit: 'throw', 'break', or 'any'. By default, throw.
Using the 'any' value lets you run an approximation of GROUP BY. The quality
of this approximation depends on the statistical nature of the data.
===max_rows_to_sort=== Maximum number of rows before sorting. This allows you
to limit memory consumption when sorting. ===max_bytes_to_sort=== Maximum
number of bytes before sorting. ===sort_overflow_mode=== What to do if the
number of rows received before sorting exceeds one of the limits: 'throw' or
'break'. By default, throw. ===max_result_rows=== Limit on the number of rows
in the result. Also checked for subqueries, and on remote servers when running
parts of a distributed query. ===max_result_bytes=== Limit on the number of
bytes in the result. The same as the previous setting.
===result_overflow_mode=== What to do if the volume of the result exceeds one
of the limits: 'throw' or 'break'. By default, throw. Using 'break' is similar
to using LIMIT. ===max_execution_time=== Maximum query execution time in
seconds. At this time, it is not checked for one of the sorting stages, or
when merging and finalizing aggregate functions. ===timeout_overflow_mode===
What to do if the query is run longer than 'max_execution_time': 'throw' or
'break'. By default, throw. ===min_execution_speed=== Minimal execution speed
in rows per second. Checked on every data block when
'timeout_before_checking_execution_speed' expires. If the execution speed is
lower, an exception is thrown. ===timeout_before_checking_execution_speed===
Checks that execution speed is not too slow (no less than
'min_execution_speed'), after the specified time in seconds has expired.
===max_columns_to_read=== Maximum number of columns that can be read from a
table in a single query. If a query requires reading a greater number of
columns, it throws an exception. ===max_temporary_columns=== Maximum number of
temporary columns that must be kept in RAM at the same time when running a
query, including constant columns. If there are more temporary columns than
this, it throws an exception. ===max_temporary_non_const_columns=== The same
thing as 'max_temporary_columns', but without counting constant columns. Note
that constant columns are formed fairly often when running a query, but they
require approximately zero computing resources. ===max_subquery_depth===
Maximum nesting depth of subqueries. If subqueries are deeper, an exception is
thrown. By default, 100. ===max_pipeline_depth=== Maximum pipeline depth.
Corresponds to the number of transformations that each data block goes through
during query processing. Counted within the limits of a single server. If the
pipeline depth is greater, an exception is thrown. By default, 1000.
===max_ast_depth=== Maximum nesting depth of a query syntactic tree. If
exceeded, an exception is thrown. At this time, it isn't checked during
parsing, but only after parsing the query. That is, a syntactic tree that is
too deep can be created during parsing, but the query will fail. By default,
1000. ===max_ast_elements=== Maximum number of elements in a query syntactic
tree. If exceeded, an exception is thrown. In the same way as the previous
setting, it is checked only after parsing the query. By default, 10,000.
===max_rows_in_set=== Maximum number of rows for a data set in the IN clause
created from a subquery. ===max_bytes_in_set=== Maximum number of bytes
(uncompressed data) used by a set in the IN clause created from a subquery.
===set_overflow_mode=== What to do when the amount of data exceeds one of the
limits: 'throw' or 'break'. By default, throw. ===max_rows_in_distinct===
Maximum number of different rows when using DISTINCT.
===max_bytes_in_distinct=== Maximum number of bytes used by a hash table when
using DISTINCT. ===distinct_overflow_mode=== What to do when the amount of
data exceeds one of the limits: 'throw' or 'break'. By default, throw.
===max_rows_to_transfer=== Maximum number of rows that can be passed to a
remote server or saved in a temporary table when using GLOBAL IN.
===max_bytes_to_transfer=== Maximum number of bytes (uncompressed data) that
can be passed to a remote server or saved in a temporary table when using
GLOBAL IN. ===transfer_overflow_mode=== What to do when the amount of data
exceeds one of the limits: 'throw' or 'break'. By default, throw. ==Settings
profiles== A settings profile is a collection of settings grouped under the
same name. Each ClickHouse user has a profile. To apply all the settings in a
profile, set 'profile'. Example: %% SET profile = 'web' %% \- Load the 'web'
profile. That is, set all the options belonging to the 'web' profile. Settings
profiles are declared in the user config file. This is normally 'users.xml'.
Example: %% <!-- Settings profiles. --> <profiles> <!-- Default settings -->
<default> <!-- Maximum number of threads for executing a single query. -->
<max_threads>8</max_threads> </default> <!-- Settings for queries from the
user interface --> <web> <max_rows_to_read>1000000000</max_rows_to_read>
<max_bytes_to_read>100000000000</max_bytes_to_read>
<max_rows_to_group_by>1000000</max_rows_to_group_by>
<group_by_overflow_mode>any</group_by_overflow_mode>
<max_rows_to_sort>1000000</max_rows_to_sort>
<max_bytes_to_sort>1000000000</max_bytes_to_sort>
<max_result_rows>100000</max_result_rows>
<max_result_bytes>100000000</max_result_bytes>
<result_overflow_mode>break</result_overflow_mode>
<max_execution_time>600</max_execution_time>
<min_execution_speed>1000000</min_execution_speed> <timeout_before_checking_ex
ecution_speed>15</timeout_before_checking_execution_speed>
<max_columns_to_read>25</max_columns_to_read>
<max_temporary_columns>100</max_temporary_columns>
<max_temporary_non_const_columns>50</max_temporary_non_const_columns>
<max_subquery_depth>2</max_subquery_depth>
<max_pipeline_depth>25</max_pipeline_depth> <max_ast_depth>50</max_ast_depth>
<max_ast_elements>100</max_ast_elements> <readonly>1</readonly> </web>
</profiles> %% In the example, two profiles are set: 'default' and 'web'. The
'default' profile has a special purpose - it must always be present and is
applied when starting the server. In other words, the 'default' profile
contains default settings. The 'web' profile is a regular profile that can be
set using the SET query or using a URL parameter in an HTTP query. Settings
profiles can inherit from each other. To use inheritance, indicate the
'profile' setting before the other settings that are listed in the profile.

# Configuration files

The main server config file is 'config.xml'. It resides in the /etc
/clickhouse-server/ directory. Certain settings can be overridden in the *.xml
and *.conf files from the 'conf.d' and 'config.d' directories next to the
config. The 'replace' and 'remove' attributes can be specified for the
elements of these config files. If neither is specified, it combines the
contents of elements recursively, replacing values of duplicate children. If
'replace' is specified, it replaces the entire element with the specified one.
If 'remove' is specified, it deletes the element. The config can also define
"substitutions". If an element has the 'incl' attribute, the corresponding
substitution from the file will be used as the value. By default, the path to
the file with substitutions is '/etc/metrika.xml'. This can be changed in the
config in the 'include_from' element. The substitution values are specified in
'/yandex/_substitution_name_' elements of this file. The 'config.xml' file can
specify a separate config with user settings, profiles, and quotas. The
relative path to this config is set in the 'users_config' element. By default,
it is 'users.xml'. If 'users_config' is omitted, the user settings, profiles,
and quotas are specified directly in 'config.xml'. The server tracks changes
to 'users_config' and reloads it in runtime. That is, you can add or change
users and their settings without relaunching the server. For 'users.config',
overrides and substitutions may also exist in files from the
'_users_config_.d' directory (for example, 'users.d'). Note that the server
tracks updates only directly in the 'users.xml' file, so all the possible
overrides are not updated in runtime. For each config file, the server also
generates _file_-preprocessed.xml files on launch. These files contain all the
completed substitutions and overrides, and they are intended for informational
use. The server itself does not use these files, and you do not need to edit
them.

# Access rights

Users and access rights are set up in the user config. This is usually
'users.xml'. Users are recorded in the 'users' section. Let's look at part of
the 'users.xml' file: %% <!-- Users and ACL. --> <users> <!-- If the username
is not specified, the default user is used. --> <default> <!-- Password (in
plaintext). May be empty. --> <password></password> <!-- List of networks that
access is allowed from. Each list item has one of the following forms: <ip> IP
address or subnet mask. For example, 222.111.222.3 or 10.0.0.1/8 or
2a02:6b8::3 or 2a02:6b8::3/64. <host> Host name. Example: example01. A DNS
query is made for verification, and all received address are compared to the
client address. <host_regexp> Regex for host names. For example,
^example\d\d-\d\d-\d\\.yandex\\.ru$ A DNS PTR query is made to verify the
client address and the regex is applied to the result. Then another DNS query
is made for the result of the PTR query, and all received address are compared
to the client address. We strongly recommend that the regex ends with
\\.yandex\\.ru$. If you are installing ClickHouse independently, here you
should specify: <networks> <ip>::/0</ip> </networks> --> <networks
incl="networks" /> <!-- Settings profile for the user. -->
<profile>default</profile> <!-- Quota for the user. --> <quota>default</quota>
</default> <!-- For queries from the user interface. --> <web>
<password></password> <networks incl="networks" /> <profile>web</profile>
<quota>default</quota> </web> %% Here we can see that two users are declared:
'default' and 'web'. We added the 'web' user ourselves. The 'default' user is
chosen in cases when the username is not passed, so this user must be present
in the config file. The 'default' user is also used for distributed query
processing - the system accesses remote servers under this username. So the
'default' user must have an empty password and must not have substantial
restrictions or quotas - otherwise, distributed queries will fail. The
password is specified in plain text directly in the config. In this regard,
you should not consider these passwords as providing security against
potential malicious attacks. Rather, they are necessary for protection from
Yandex employees. A list of networks is specified that access is allowed from.
In this example, the list of networks for both users is loaded from a separate
file (/etc/metrika.xml) containing the 'networks' substitution. Here is a
fragment of it: %% <yandex> ... <networks> <ip>::/64</ip>
<ip>93.158.111.111/26</ip> <ip>2a02:6b8:0:1::/64</ip> ... </networks>
</yandex> %% We could have defined this list of networks directly in
'users.xml', or in a file in the 'users.d' directory (for more information,
see the section "Configuration files"). The config includes comments
explaining how to open access from everywhere. For use in production, only
specify IP elements (IP addresses and their masks), since using 'host' and
'hoost_regexp' might cause extra latency. Next the user settings profile is
specified (see the section "Settings profiles"). You can specify the default
profile, 'default'. The profile can have any name. You can specify the same
profile for different users. The most important thing you can write in the
settings profile is 'readonly' set to 1, which provides read-only access.
After this, the quota is defined (see the section "Quotas"). You can specify
the default quota, 'default'. It is set in the config by default so that it
only counts resource usage, but does not restrict it. The quota can have any
name. You can specify the same quota for different users - in this case,
resource usage is calculated for each user individually.

# Quotas

Quotas allow you to limit resource usage over a period of time, or simply
track the use of resources. Quotas are set up in the user config. This is
usually 'users.xml'. The system also has a feature for limiting the complexity
of a single query (see the section "Restrictions on query complexity"). In
contrast to query complexity restrictions, quotas: \- place restrictions on a
set of queries that can be run over a period of time, instead of limiting a
single query. \- account for resources spent on all remote servers for
distributed query processing. Let's look at the section of the 'users.xml'
file that defines quotas. %% <!-- Quotas. --> <quotas> <!-- Quota name. -->
<default> <!-- Restrictions for a time period. You can set multiple time
intervals with various restrictions. --> <interval> <!-- Length of time. -->
<duration>3600</duration> <!-- No restrictions. Just collect data for the
specified time interval. --> <queries>0</queries> <errors>0</errors>
<result_rows>0</result_rows> <read_rows>0</read_rows>
<execution_time>0</execution_time> </interval> </default> %% By default, the
quota just tracks resource consumption for each hour, without limiting usage.
%% <statbox> <!-- Restrictions for a time period. You can set multiple time
intervals with various restrictions. --> <interval> <!-- Length of time.-->
<duration>3600</duration> <queries>1000</queries> <errors>100</errors>
<result_rows>1000000000</result_rows> <read_rows>100000000000</read_rows>
<execution_time>900</execution_time> </interval> <interval>
<duration>86400</duration> <queries>10000</queries> <errors>1000</errors>
<result_rows>5000000000</result_rows> <read_rows>500000000000</read_rows>
<execution_time>7200</execution_time> </interval> </statbox> %% For the
'statbox' quota, restrictions are set for every hour and for every 24 hours
(86,400 seconds). The time interval is counted starting from an
implementation-defined fixed moment in time. In other words, the 24-hour
interval doesn't necessarily begin at midnight. When the interval ends, all
collected values are cleared. For the next hour, the quota calculation starts
over. Let's examine the amounts that can be restricted: **queries** \- The
overall number of queries. **errors** \- The number of queries that threw
exceptions. **result_rows** \- The total number of rows output in results.
**read_rows** \- The total number of source rows retrieved from tables for
running a query, on all remote servers. **execution_time** \- The total time
of query execution, in seconds (wall time). If the limit is exceeded for at
least one time interval, an exception is thrown with a text about which
restriction was exceeded, for which interval, and when the new interval begins
(when queries can be sent again). Quotas can use the "quota key" feature in
order to report on resources for multiple keys independently. Here is an
example of this: %% <!-- For the global report builder. --> <web_global> <!--
keyed - the quota_key "key" is passed in the query parameter, and the quota is
tracked separately for each key value. For example, you can pass a Metrica
username as the key, so the quota will be counted separately for each
username. Using keys makes sense only if quota_key is transmitted by the
program, not by a user. You can also write <keyed_by_ip /> so the IP address
is used as the quota key. (But keep in mind that users can change the IPv6
address fairly easily.) --> <keyed /> %% The quota is assigned to users in the
'users' section of the config. See the section "Access rights". For
distributed query processing, the accumulated amounts are stored on the
requestor server. So if the user goes to another server, the quota there will
"start over". When the server is restarted, quotas are reset.

![Yandex.Metrica](https://bs.yandex.ru/informer/18343495/2_1_FFFFFFFF_EFEFEFFF
_0_pageviews)

