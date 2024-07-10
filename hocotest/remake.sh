pg_ctl stop -m immediate

cd ../contrib/hocotext/

make clean

make -j4

sudo make install

cd ../..

echo 'Compile End'

# rm $PGHOME/data/postmaster.pid

# pg_ctl -D $PGHOME/data stop -m immediate

# pg_ctl -D $PGHOME/data start
pg_ctl start

cd ./hocotest/

# psql -U yeweitang -d test -f retest.sql

# running this script should ensure there is a dbbase called test 
psql test