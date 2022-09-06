/usr/bin/c++ -o receiver.o -c receiver.cpp

/usr/bin/c++ -rdynamic receiver.o  -o receiver.exe -lamqpcpp -lev -lpthread -ldl -lssl
./receiver.exe
