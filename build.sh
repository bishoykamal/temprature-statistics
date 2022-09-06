/usr/bin/c++ -o main.o -c main.cpp

/usr/bin/c++ -rdynamic main.o  -o main.exe -lamqpcpp -lev -lpthread -ldl -lssl
./main.exe
