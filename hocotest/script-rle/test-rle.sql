DROP TABLE dna_seq;
drop table baseline;
drop table baseline_plain;

DROP EXTENSION hocotext;
CREATE EXTENSION hocotext;

CREATE TABLE dna_seq(seq_id SERIAL PRIMARY KEY,
exp_date DATE NOT NULL,
seq_data TEXT COMPRESSION RLE,
quality_score DECIMAL(5,2));


CREATE TABLE baseline(seq_id SERIAL PRIMARY KEY,
exp_date DATE NOT NULL,
seq_data TEXT COMPRESSION PGLZ,
quality_score DECIMAL(5,2));


CREATE TABLE baseline_plain(seq_id SERIAL PRIMARY KEY,
exp_date DATE NOT NULL,
seq_data TEXT STORAGE External,
quality_score DECIMAL(5,2));

-- insert into dna_seq values(1,'2023-03-25', hoco_rle(repeat('aaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbabcdefghigk',200)),95.5); 
-- insert into baseline values(1,'2023-03-25', repeat('aaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbabcdefghigk',200),95.5); 
-- insert into baseline_plain values(1,'2023-03-25', repeat('aaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbabcdefghigk',200),95.5); 

insert into dna_seq values(1,'2023-03-25', hoco_rle(pg_read_file('/data/seqs/seq_0328')),95.5); 
insert into baseline values(1,'2023-03-25', pg_read_file('/data/seqs/seq_0328'),95.5); 
insert into baseline_plain values(1,'2023-03-25', pg_read_file('/data/seqs/seq_0328'),95.5); 


SELECT
relname AS "Table",
pg_size_pretty(pg_total_relation_size(relid)) as "Size",
pg_size_pretty(pg_total_relation_size(relid)-pg_relation_size(relid)) as "External Size"
FROM pg_catalog.pg_statio_user_tables ORDER BY pg_total_relation_size(relid) DESC;