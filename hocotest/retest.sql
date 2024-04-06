DROP TABLE test;

DROP EXTENSION hocotext;

CREATE EXTENSION hocotext;

CREATE TABLE test (
        id SERIAL PRIMARY KEY,
        content hocotext compression pglz
    );

INSERT INTO test (content)
    VALUES (hocotext_compress_tadoc(pg_read_file('/home/yeweitang/postgres/dataset/Horspool-tiny.txt')));

SELECT pg_backend_pid();

-- SELECT pg_backend_pid();

SELECT hocotext_decompress_tadoc(content) FROM test WHERE id = 1;