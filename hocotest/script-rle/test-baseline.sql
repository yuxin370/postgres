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




INSERT INTO baseline values (1,'2023-03-25',pg_read_file('/data/seqs/seq_0327'),95.5,pg_read_file('/data/logs/log_0329.txt'));
INSERT INTO baseline_plain values (1,'2023-03-25',pg_read_file('/data/seqs/seq_0327'),95.5,pg_read_file('/data/logs/log_0329.txt'));


-- 3. Substring replacement
EXPLAIN ANALYZE UPDATE baseline SET seq_data = OVERLAY(seq_data placing 'AAGTAGGGTTTTTNNNNNNNNN'from 100000000 for 300);
EXPLAIN ANALYZE UPDATE baseline_plain SET seq_data = OVERLAY(seq_data placing 'AAGTAGGGTTTTTNNNNNNNNN'from 100000000 for 300);

--4.Full-text search
EXPLAIN ANALYZE SELECT to_tsvector(exp_log) FROM baseline WHERE seq_id=1;
EXPLAIN ANALYZE SELECT to_tsvector(exp_log) FROM baseline_plain WHERE seq_id=1;

