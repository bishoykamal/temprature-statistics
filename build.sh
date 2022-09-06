# g++ -g -Wall -std=c++11 -o main -static main.cpp -lamqpcpp -lpthread -luv -lev -ldl -pthread
# ./main




/usr/bin/c++ -o main.o -c main.cpp

/usr/bin/c++ -rdynamic main.o  -o main.exe -lamqpcpp -lev -lpthread -ldl -lssl
./main.exe
