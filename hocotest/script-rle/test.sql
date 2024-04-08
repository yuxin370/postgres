SET client_min_messages TO WARNING;

DROP TABLE dna_seq;
drop table baseline;
drop table baseline_plain;

DROP EXTENSION hocotext;
CREATE EXTENSION hocotext;

CREATE TABLE dna_seq(seq_id SERIAL PRIMARY KEY,
exp_date DATE NOT NULL,
seq_data TEXT COMPRESSION RLE,
quality_score DECIMAL(5,2),
exp_log TEXT COMPRESSION TADOC
);


CREATE TABLE baseline(seq_id SERIAL PRIMARY KEY,
exp_date DATE NOT NULL,
seq_data TEXT COMPRESSION PGLZ,
quality_score DECIMAL(5,2),
exp_log TEXT COMPRESSION PGLZ
);


CREATE TABLE baseline_plain(seq_id SERIAL PRIMARY KEY,
exp_date DATE NOT NULL,
seq_data TEXT STORAGE External,
quality_score DECIMAL(5,2),
exp_log TEXT STORAGE External
);

INSERT INTO dna_seq values (1,'2023-03-25',pg_read_file('/data/seqs/seq_0327'),95.5,pg_read_file('/data/logs/log_0329.txt'));
INSERT INTO dna_seq values (1,'2023-03-25',hoco_rle(pg_read_file('/data/seqs/seq_0327')),95.5,hoco_tadoc(pg_read_file('/data/logs/log_0329.txt')));

INSERT INTO baseline values (1,'2023-03-25',pg_read_file('/data/seqs/seq_0327'),95.5,pg_read_file('/data/logs/log_0329.txt'));
INSERT INTO baseline_plain values (1,'2023-03-25',pg_read_file('/data/seqs/seq_0327'),95.5,pg_read_file('/data/logs/log_0329.txt'));

SELECT
relname AS "Table",
pg_size_pretty(pg_total_relation_size(relid)) as "Size",
pg_size_pretty(pg_total_relation_size(relid)-pg_relation_size(relid)) as "External Size"
FROM pg_catalog.pg_statio_user_tables ORDER BY pg_total_relation_size(relid) DESC;


-- 3. Substring replacement
EXPLAIN ANALYZE UPDATE dna_seq SET seq_data = OVERLAY(seq_data placing 'AAGTAGGGTTTTTNNNNNNNNN'from 100000000 for 300);
EXPLAIN ANALYZE UPDATE baseline SET seq_data = OVERLAY(seq_data placing 'AAGTAGGGTTTTTNNNNNNNNN'from 100000000 for 300);
EXPLAIN ANALYZE UPDATE baseline_plain SET seq_data = OVERLAY(seq_data placing 'AAGTAGGGTTTTTNNNNNNNNN'from 100000000 for 300);

--4.Full-text search
EXPLAIN ANALYZE SELECT to_tsvector(exp_log) FROM dna_seq WHERE seq_id=1;
EXPLAIN ANALYZE SELECT to_tsvector(exp_log) FROM baseline WHERE seq_id=1;
EXPLAIN ANALYZE SELECT to_tsvector(exp_log) FROM baseline_plain WHERE seq_id=1;

