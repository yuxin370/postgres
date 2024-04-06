gcc -o tadoc -O3 tadoc.c

vtune -c hotspots -r ./hotspots/ ./tadoc