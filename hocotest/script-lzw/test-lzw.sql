DROP TABLE dna_seq;
drop table baseline;
drop table baseline_plain;

DROP EXTENSION hocotext;
CREATE EXTENSION hocotext;

CREATE TABLE dna_seq(seq_id SERIAL PRIMARY KEY,
exp_date DATE NOT NULL,
seq_data TEXT COMPRESSION LZW,
quality_score DECIMAL(5,2));

CREATE TABLE baseline(seq_id SERIAL PRIMARY KEY,
exp_date DATE NOT NULL,
seq_data TEXT COMPRESSION PGLZ,
quality_score DECIMAL(5,2));

CREATE TABLE baseline_plain(seq_id SERIAL PRIMARY KEY,
exp_date DATE NOT NULL,
seq_data TEXT STORAGE External,
quality_score DECIMAL(5,2));

explain analyze insert into dna_seq values(2,'2023-03-25', (repeat('| a b c d e f g h i j k l m n o p q r s t u v w x y z ',40)),95.5); 
explain analyze insert into baseline values(2,'2023-03-25', (repeat('| a b c d e f g h i j k l m n dedwewfe88877 da*dada o p q r s t u v w x y z ',1)),95.5); 
explain analyze insert into baseline values(4,'2023-03-25', (repeat('| you knows what , %^&$ 9839 anbd45. ',80)),95.5); 
explain analyze insert into dna_seq values(3,'2023-03-25', (repeat('| you knows what , %^&$ 9839 anbd45. ',80)),95.5); 

explain analyze insert into baseline values(2,'2023-03-25', (repeat('| you knows what , is loves, nothing happens',80)),95.5); 
explain analyze insert into baseline values(3,'2023-03-25', (repeat('hello world gcc -Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation',10)),95.5); 
explain analyze insert into baseline_plain values(4,'2023-03-25', (repeat('hello world gcc -Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation',50)),95.5); 


insert into dna_seq values(4,'2023-03-25', (repeat('hello world my lord ',200)),95.5); 

delete from dna_seq;
delete from baseline;
insert into dna_seq values(3,'2023-03-25', (repeat('Whispers of the stars weave stories in the light. A moonbeams caress, soft on the cheek ',300)),95.5); 
insert into baseline values(4,'2023-03-25', (repeat('Whispers of the stars weave stories in the light. A moonbeams caress, soft on the cheek ',300)),95.5); 

select hoco_de_lzw(seq_data) from baseline;
select * from baseline_plain;

SELECT seq_data FROM dna_seq;
SELECT seq_data FROM baseline_plain;
select to_tsvector(seq_data) FROM dna_seq;

select to_tsvector(seq_data) FROM baseline;

select pg_backend_pid();



SELECT
relname AS "Table",
pg_size_pretty(pg_total_relation_size(relid)) as "Size",
pg_size_pretty(pg_total_relation_size(relid)-pg_relation_size(relid)) as "External Size"
FROM pg_catalog.pg_statio_user_tables ORDER BY pg_total_relation_size(relid) DESC;


-- explain analyze select seq_id from dna_seq where seq_data LIKE '%babc%k';
-- explain analyze select seq_id from baseline where seq_data LIKE '%babc%k';
-- explain analyze select seq_id from baseline_plain where seq_data LIKE '%babc%k';


-- insert into dna_seq values(1,'2023-03-25', hoco_lzw(repeat('hello world',200)),95.5); 
-- insert into baseline values(1,'2023-03-25', repeat('hello world',200),95.5); 
-- insert into baseline_plain values(1,'2023-03-25', repeat('hello world',200),95.5); 

-- insert into dna_seq values(1,'2023-03-25', hoco_lzw(pg_read_file('/data/seqs/seq_0328')),95.5); 
-- insert into baseline values(1,'2023-03-25', pg_read_file('/data/seqs/seq_0328'),95.5); 
-- insert into baseline_plain values(1,'2023-03-25', pg_read_file('/data/seqs/seq_0328'),95.5); 

