CREATE TABLE test (
    id SERIAL PRIMARY KEY,
    content hocotext compression pglz
);

INSERT INTO test (content)
VALUES (hocotext_compress_tadoc(pg_read_file('/home/yeweitang/postgres/dataset/simple.txt')));

DROP TABLE test;