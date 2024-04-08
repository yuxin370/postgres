source env-debug

make distclean

#  --enable-profiling      build with profiling enabled
# need to install profiling tools.
#  --enable-coverage       build with coverage testing instrumentation
#  Needs to install lcov
#  --enable-tap-tests      enable TAP tests (requires Perl and IPC::Run)
#  Needs to install perl tools. IPC::Run and Test::More
#

# 调整预配置，并设置编译选项O0，不开启任何的编译优化，这是为了后面更好的看到完整的调试信息
./configure --prefix=/usr \
    --enable-debug \
    --enable-depend \
    CCFLAG=-O3 \
    CC='gcc'

make -j 4

sudo make install

echo 'Run Sucessful.'