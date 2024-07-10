DROP TABLE test;

DROP EXTENSION hocotext;

CREATE EXTENSION hocotext;

CREATE TABLE test (
        id SERIAL PRIMARY KEY,
        content text
    );

INSERT INTO test (content)
    VALUES (hoco_tadoc(pg_read_file('/home/yeweitang/postgres/dataset/HDFS-medium.txt')));

SELECT pg_backend_pid();

-- SELECT to_tsvector(content) FROM test WHERE id = 1;

DO $$
BEGIN
    PERFORM to_tsvector(content) FROM test WHERE id = 1;
END 
$$;