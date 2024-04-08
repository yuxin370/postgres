CREATE TABLE dna_seq(seq_id SERIAL PRIMARY KEY,
exp_date DATE NOT NULL,
seq_data TEXT COMPRESSION RLE,
quality_score DECIMAL(5,2),
exp_log TEXT COMPRESSION TADOC
);



INSERT INTO dna_seq values (1,'2023-03-25',hoco_rle(pg_read_file('/data/seqs/seq_0327')),95.5,hoco_tadoc(pg_read_file('/data/logs/log_0329.txt')));
