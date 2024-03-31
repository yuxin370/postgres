# 调整预配置，并设置编译选项O0，不开启任何的编译优化，这是为了后面更好的看到完整的调试信息
cd ./contrib/hocotext/

make -j4

sudo make install

cd ../..

echo 'Compile End'

# rm $PGHOME/data/postmaster.pid

# pg_ctl -D $PGHOME/data stop -m immediate

# pg_ctl -D $PGHOME/data start
pg_ctl -D /opt/pgsql/postgresql/data/ start

# running this script should ensure there is a dbbase called test 
psql test