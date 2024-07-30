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

explain analyze insert into dna_seq values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',400)),95.5); 
explain analyze insert into baseline values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',400)),95.5); 
explain analyze insert into baseline_plain values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',400)),95.5); 

/*
test=# explain analyze insert into dna_seq values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',400)),95.5); 
                                             QUERY PLAN                                              
-----------------------------------------------------------------------------------------------------
 Insert on dna_seq  (cost=0.00..0.01 rows=0 width=0) (actual time=3531.566..3531.567 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.002..0.004 rows=1 loops=1)
 Planning Time: 0.062 ms
 Execution Time: 3541.167 ms
(4 rows)

test=# explain analyze insert into baseline values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',400)),95.5); 
                                           QUERY PLAN                                           
------------------------------------------------------------------------------------------------
 Insert on baseline  (cost=0.00..0.01 rows=0 width=0) (actual time=0.191..0.192 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.002..0.004 rows=1 loops=1)
 Planning Time: 0.064 ms
 Execution Time: 0.212 ms
(4 rows)

test=# explain analyze insert into baseline_plain values(2,'2023-03-25', (repeat('afrfrg b c d e f g h i j k l m n o p q r s t u v w x y z',400)),95.5); 
                                              QUERY PLAN                                              
------------------------------------------------------------------------------------------------------
 Insert on baseline_plain  (cost=0.00..0.01 rows=0 width=0) (actual time=2.583..2.585 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.006..0.008 rows=1 loops=1)
 Planning Time: 0.085 ms
 Execution Time: 2.615 ms
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
 Seq Scan on dna_seq  (cost=0.00..275.20 rows=1020 width=32) (actual time=22.039..22.049 rows=1 loops=1)
 Planning Time: 0.087 ms
 Execution Time: 22.078 ms
(3 rows)

test=# explain analyze select to_tsvector(seq_data) FROM baseline;
                                                QUERY PLAN                                                
----------------------------------------------------------------------------------------------------------
 Seq Scan on baseline  (cost=0.00..275.20 rows=1020 width=32) (actual time=44.707..44.715 rows=1 loops=1)
 Planning Time: 0.079 ms
 Execution Time: 44.739 ms
(3 rows)

test=# explain analyze select to_tsvector(seq_data) FROM baseline_plain;
                                                   QUERY PLAN                                                   
----------------------------------------------------------------------------------------------------------------
 Seq Scan on baseline_plain  (cost=0.00..275.20 rows=1020 width=32) (actual time=38.904..38.912 rows=1 loops=1)
 Planning Time: 0.084 ms
 Execution Time: 38.936 ms
(3 rows)
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
---------------------------------------------------------------------------------------------------------
 Insert on dna_seq  (cost=0.00..0.01 rows=0 width=0) (actual time=114995.755..114995.757 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.004..0.006 rows=1 loops=1)
 Planning Time: 0.872 ms
 Execution Time: 115184.405 ms
(4 rows)

test=# explain analyze insert into baseline values(3,'2023-03-25', (repeat('Whispers of the stars weave stories in the light. A moonbeams caress, soft on the cheek ',3000)),95.5); 
                                            QUERY PLAN                                            
--------------------------------------------------------------------------------------------------
 Insert on baseline  (cost=0.00..0.01 rows=0 width=0) (actual time=34.300..34.303 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.003..0.005 rows=1 loops=1)
 Planning Time: 49.406 ms
 Execution Time: 34.342 ms
(4 rows)

test=# explain analyze insert into baseline_plain values(3,'2023-03-25', (repeat('Whispers of the stars weave stories in the light. A moonbeams caress, soft on the cheek ',3000)),95.5); 
                                              QUERY PLAN                                              
------------------------------------------------------------------------------------------------------
 Insert on baseline_plain  (cost=0.00..0.01 rows=0 width=0) (actual time=1.961..1.963 rows=0 loops=1)
   ->  Result  (cost=0.00..0.01 rows=1 width=52) (actual time=0.002..0.004 rows=1 loops=1)
 Planning Time: 0.400 ms
 Execution Time: 1.982 ms
(4 rows)
*/


--SELECT seq_data FROM dna_seq;
--SELECT seq_data FROM baseline;
--SELECT seq_data FROM baseline_plain;
explain (analyze,buffers) select to_tsvector(seq_data) FROM dna_seq;
explain (analyze,buffers) select to_tsvector(seq_data) FROM baseline;
explain (analyze,buffers) select to_tsvector(seq_data) FROM baseline_plain;




/*
test=# explain (analyze,buffers) select to_tsvector(seq_data) FROM dna_seq;
                                               QUERY PLAN                                                
---------------------------------------------------------------------------------------------------------
 Seq Scan on dna_seq  (cost=0.00..275.20 rows=1020 width=32) (actual time=90.438..90.444 rows=1 loops=1)
   Buffers: shared hit=4
 Planning Time: 0.082 ms
 Execution Time: 90.468 ms
(4 rows)

test=# explain (analyze,buffers) select to_tsvector(seq_data) FROM baseline;
                                                 QUERY PLAN                                                 
------------------------------------------------------------------------------------------------------------
 Seq Scan on baseline  (cost=0.00..275.20 rows=1020 width=32) (actual time=197.143..197.150 rows=1 loops=1)
   Buffers: shared hit=3
 Planning Time: 0.070 ms
 Execution Time: 197.172 ms
(4 rows)

test=# explain (analyze,buffers) select to_tsvector(seq_data) FROM baseline_plain;
                                                    QUERY PLAN                                                    
------------------------------------------------------------------------------------------------------------------
 Seq Scan on baseline_plain  (cost=0.00..275.20 rows=1020 width=32) (actual time=199.041..199.050 rows=1 loops=1)
   Buffers: shared hit=36
 Planning Time: 0.141 ms
 Execution Time: 199.086 ms
(4 rows)
*/

SELECT
relname AS "Table",
pg_size_pretty(pg_total_relation_size(relid)) as "Size",
pg_size_pretty(pg_total_relation_size(relid)-pg_relation_size(relid)) as "External Size"
FROM pg_catalog.pg_statio_user_tables ORDER BY pg_total_relation_size(relid) DESC;

/*

     Table      |  Size  | External Size 
----------------+--------+---------------
 baseline_plain | 344 kB | 336 kB
 dna_seq        | 64 kB  | 56 kB
 baseline       | 32 kB  | 24 kB

 */