select pg_backend_pid();

CREATE TABLE dna_seq(seq_id SERIAL ,
exp_date DATE NOT NULL,
seq_data TEXT COMPRESSION LZW,
quality_score DECIMAL(5,2));

CREATE TABLE baseline(seq_id SERIAL ,
exp_date DATE NOT NULL,
seq_data TEXT COMPRESSION PGLZ,
quality_score DECIMAL(5,2));

CREATE TABLE baseline_plain(seq_id SERIAL,
exp_date DATE NOT NULL,
seq_data TEXT STORAGE External,
quality_score DECIMAL(5,2));


delete from dna_seq;
delete from baseline;
delete from baseline_plain;

explain analyze insert into dna_seq values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',40)),95.5); 
explain analyze insert into baseline values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',40)),95.5); 
explain analyze insert into baseline_plain values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',400)),95.5); 

/*
test=# explain analyze insert into dna_seq values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',400)),95.5); 
                                             QUERY PLAN                                           
-----------------------------------------------------------------------------------------------------
 Insert on dna_seq  (cost=0.00..0.01 rows=0 width=0) (actual time=2871.873..287
1.874 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.007..0.009 rows
=1 loops=1)
 Planning Time: 0.256 ms
 Execution Time: 2968.063 ms
(4 rows)

test=# explain analyze insert into baseline values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',400)),95.5); 
                                           QUERY PLAN                          
------------------------------------------------------------------------------------------------
 Insert on baseline  (cost=0.00..0.01 rows=0 width=0) (actual time=1.234..1.236
 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.004..0.006 rows
=1 loops=1)
 Planning Time: 0.081 ms
 Execution Time: 1.264 ms
(4 rows)

test=# explain analyze insert into baseline_plain values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',400)),95.5); 
                                              QUERY PLAN                       
------------------------------------------------------------------------------------------------------
 Insert on baseline_plain  (cost=0.00..0.01 rows=0 width=0) (actual time=3.373.
.3.376 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.002..0.005 rows
=1 loops=1)
 Planning Time: 0.041 ms
 Execution Time: 3.413 ms
(4 rows)
*/

--SELECT seq_data FROM dna_seq;
--SELECT seq_data FROM baseline;
--SELECT seq_data FROM baseline_plain;
explain analyze select to_tsvector(seq_data) FROM dna_seq;
explain analyze select to_tsvector(seq_data) FROM baseline;
explain analyze select to_tsvector(seq_data) FROM baseline_plain;

--res
/*
test=# explain analyze select to_tsvector(seq_data) FROM dna_seq;
                                               QUERY PLAN                                                
---------------------------------------------------------------------------------------------------------
 Seq Scan on dna_seq  (cost=0.00..275.20 rows=1020 width=32) (actual time=15.410..15.415 rows=1 loops=1)
 Planning Time: 0.052 ms
 Execution Time: 15.429 ms
(3 rows)

test=# explain analyze select to_tsvector(seq_data) FROM dna_seq;   -- 解压再计算
                                               QUERY PLAN                                           
---------------------------------------------------------------------------------------------------------
 Seq Scan on dna_seq  (cost=0.00..275.20 rows=1020 width=32) (actual time=13.534..13.550 rows=1 loop
s=1)
 Planning Time: 0.056 ms
 Execution Time: 13.576 ms
(3 rows)


test=# explain analyze select to_tsvector(seq_data) FROM baseline;
                                                QUERY PLAN                                                
----------------------------------------------------------------------------------------------------------
 Seq Scan on baseline  (cost=0.00..275.20 rows=1020 width=32) (actual time=10.405..10.411 rows=1 loops=1)
 Planning Time: 0.068 ms
 Execution Time: 10.432 ms
(3 rows)

test=# explain analyze select to_tsvector(seq_data) FROM baseline_plain;
                                                   QUERY PLAN                                                   
----------------------------------------------------------------------------------------------------------------
 Seq Scan on baseline_plain  (cost=0.00..275.20 rows=1020 width=32) (actual time=11.016..11.022 rows=1 loops=1)
 Planning Time: 0.067 ms
 Execution Time: 11.043 ms
*/

delete from dna_seq;
delete from baseline;
delete from baseline_plain;

explain analyze insert into dna_seq values(3,'2023-03-25', (repeat('Whispers of the stars weave stories in the light. A moonbeams caress, soft on the cheek ',3000)),95.5); 
explain analyze insert into baseline values(3,'2023-03-25', (repeat('Whispers of the stars weave stories in the light. A moonbeams caress, soft on the cheek ',3000)),95.5); 
explain analyze insert into baseline_plain values(3,'2023-03-25', (repeat('Whispers of the stars weave stories in the light. A moonbeams caress, soft on the cheek ',3000)),95.5); 
--res

/*
test=# explain analyze insert into dna_seq values(3,'2023-03-25', (repeat('Whispers of the stars weave stories in the light. A moonbeams caress, soft on the cheek ',3000)),95.5); 
                                              QUERY PLAN                                               
-------------------------------------------------------------------------------------------------------
 Insert on dna_seq  (cost=0.00..0.01 rows=0 width=0) (actual time=83438.052..83438.054 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.002..0.004 rows=1 loops=1)
 Planning Time: 1.599 ms
 Execution Time: 83490.272 ms
(4 rows)

test=# explain analyze insert into baseline values(3,'2023-03-25', (repeat('Whispers of the stars weave stories in the light. A moonbeams caress, soft on the cheek ',3000)),95.5); 
                                            QUERY PLAN                                            
--------------------------------------------------------------------------------------------------
 Insert on baseline  (cost=0.00..0.01 rows=0 width=0) (actual time=23.846..23.848 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.005..0.007 rows=1 loops=1)
 Planning Time: 1523.935 ms
 Execution Time: 23.890 ms
(4 rows)

test=# explain analyze insert into baseline_plain values(3,'2023-03-25', (repeat('Whispers of the stars weave stories in the light. A moonbeams caress, soft on the cheek ',3000)),95.5); 
                                              QUERY PLAN                                              
------------------------------------------------------------------------------------------------------
 Insert on baseline_plain  (cost=0.00..0.01 rows=0 width=0) (actual time=5.421..5.423 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.002..0.006 rows=1 loops=1)
 Planning Time: 0.804 ms
 Execution Time: 5.450 ms
(4 rows)
*/


--SELECT seq_data FROM dna_seq;
--SELECT seq_data FROM baseline;
--SELECT seq_data FROM baseline_plain;
explain analyze select to_tsvector(seq_data) FROM dna_seq;
explain analyze select to_tsvector(seq_data) FROM baseline;
explain analyze select to_tsvector(seq_data) FROM baseline_plain;




/*

test=# explain analyze select to_tsvector(seq_data) FROM dna_seq;
                                               QUERY PLAN                                                
---------------------------------------------------------------------------------------------------------
 Seq Scan on dna_seq  (cost=0.00..275.20 rows=1020 width=32) (actual time=89.071..89.078 rows=1 loops=1)
 Planning Time: 0.068 ms
 Execution Time: 89.094 ms
(3 rows)

test=# explain analyze select to_tsvector(seq_data) FROM dna_seq;   --解压后直接计算
                                               QUERY PLAN                                           
---------------------------------------------------------------------------------------------------------
 Seq Scan on dna_seq  (cost=0.00..275.20 rows=1020 width=32) (actual time=17.695..68.042 rows=2 loop
s=1)
 Planning Time: 0.131 ms
 Execution Time: 68.072 ms
(3 rows)

test=# explain analyze select to_tsvector(seq_data) FROM baseline;
                                                QUERY PLAN                                                
----------------------------------------------------------------------------------------------------------
 Seq Scan on baseline  (cost=0.00..275.20 rows=1020 width=32) (actual time=52.632..52.639 rows=1 loops=1)
 Planning Time: 0.086 ms
 Execution Time: 52.661 ms
(3 rows)

test=# explain analyze select to_tsvector(seq_data) FROM baseline_plain;
                                                   QUERY PLAN                                                   
----------------------------------------------------------------------------------------------------------------
 Seq Scan on baseline_plain  (cost=0.00..275.20 rows=1020 width=32) (actual time=65.091..65.099 rows=1 loops=1)
 Planning Time: 0.058 ms
 Execution Time: 65.120 ms
(3 rows)
*/
