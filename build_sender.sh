/usr/bin/c++ -o sender.o -c sender.cpp

/usr/bin/c++ -rdynamic sender.o  -o sender.exe -lamqpcpp -lev -lpthread -ldl -lssl
./sender.exe
