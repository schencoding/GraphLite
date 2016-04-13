------------------------------------------------------------
Requirements
------------------------------------------------------------
1. JDK 1.7.x
2. Hadoop 2.6.x
3. protocol buffers
   $ apt-get install protobuf-c-compiler libprotobuf-c0 libprotobuf-c0-dev

------------------------------------------------------------
Directory Structure
------------------------------------------------------------
bin/         scripts and graphlite executable
engine/      graphlite engine source code     
example/     PageRank example
include/     header that represents programming API

Input/       a number of small example graphs
Output/      empty, will contain the output of a run

Makefile     this can make both engine and example

LICENSE.txt  Apache License, Version 2.0

README.txt   this file

------------------------------------------------------------
Build graphlite
------------------------------------------------------------
1. source bin/setenv

   (1) edit bin/setenv, set the following paths:
       JAVA_HOME, HADOOP_HOME, GRAPHLITE_HOME

   (2) $ . bin/setenv

2. build graphlite

   $ cd engine
   $ make

   check if bin/graphlite is successfully generated.

------------------------------------------------------------
Compile and Run Vertex Program
------------------------------------------------------------

1. build example

   $ cd example
   $ make

   check if example/PageRankVertex.so is successfully generated.
   
2. run example

   $ start-graphlite example/PageRankVertex.so Input/facebookcombined_4w Output/out

   PageRankVertex.cc declares 5 processes, including 1 master and 4 workers.
   So the input graph file is prepared as four files: Input/facebookcombined_4w_[1-4]

   The output of PageRank will be in: Output/out_[1-4]

   Workers generate log files in WorkOut/worker*.out

------------------------------------------------------------
Write Vertex Program
------------------------------------------------------------
Please refer to PageRankVertex.cc

1. change VERTEX_CLASS_NAME(name) definition to use a different class name

2. VERTEX_CLASS_NAME(InputFormatter) can be kept as is

3. VERTEX_CLASS_NAME(OutputFormatter): this is where the output is generated

4. VERTEX_CLASS_NAME(Aggregator): you can implement other types of aggregation

5. VERTEX_CLASS_NAME(): the main vertex program with compute()
   
6. VERTEX_CLASS_NAME(Graph): set the running configuration here

7. Modify Makefile:
   EXAMPLE_ALGOS=PageRankVertex

   if your program is your_program.cc, then 
   EXAMPLE_ALGOS=your_program

   make will produce your_program.so

------------------------------------------------------------
Use Hash Partitioner
------------------------------------------------------------

 bin/hash-partitioner.pl can be used to divide a graph input
 file into multiple partitions.

  $ hash-partitioner.pl Input/facebookcombined 4

  will generate: Input/facebookcombined_4w_[1-4]

