DO $$
DECLARE
    execution_times NUMERIC[] := ARRAY[]::NUMERIC[];
    start_time TIMESTAMP;
    end_time TIMESTAMP;
    elapsed_time NUMERIC;
    total_time NUMERIC := 0;
    average_time NUMERIC;
    variance NUMERIC := 0;
    i INT;
BEGIN
    
    -- clear
    DROP TABLE test;

  -- create a new table to test insert operation
    CREATE TABLE test (
        id SERIAL PRIMARY KEY,
        content hocotext compression pglz
    );

    -- insert first to preparing for decompression
    INSERT INTO test (content)
    VALUES (pg_read_file('/home/yeweitang/postgres/dataset/Android-medium.txt'));

    -- 执行 SELECT 语句十次
    RAISE NOTICE 'PERFORM pglz decompress;';
    FOR i IN 1..10 LOOP
        start_time := clock_timestamp();
    
        -- PERFORM char_length(c1) from baseline;
        -- test tadoc_decompress
        PERFORM content FROM test WHERE id = 1;

        end_time := clock_timestamp();
        elapsed_time := EXTRACT(EPOCH FROM (end_time - start_time));
        execution_times := array_append(execution_times, elapsed_time);
        total_time := total_time + elapsed_time;
    END LOOP;
  
    -- 计算平均时间
    average_time := total_time / 10;
    
    -- 计算方差
    FOR i IN 1..10 LOOP
        variance := variance + POWER(execution_times[i] - average_time, 2);
    END LOOP;
    variance := variance / 10;
    
    -- 输出结果
    RAISE NOTICE 'Total Time: % ms', total_time * 1000;
    RAISE NOTICE 'Average Time: % ms', average_time * 1000;
    RAISE NOTICE 'Variance: %', variance;
  
END $$;
