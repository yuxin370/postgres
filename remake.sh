pg_ctl stop -m immediate

cd ./contrib/hocotext/

make -j4

sudo make install

cd ../..

echo 'Compile End'

# rm $PGHOME/data/postmaster.pid

# pg_ctl -D $PGHOME/data stop -m immediate


# pg_ctl -D $PGHOME/data start
pg_ctl start

# stupid method always works
sudo cp -r /usr/local/pgsql/share/extension/* /usr/share/postgresql/extension/ 

# running this script should ensure there is a dbbase called test 
psql test