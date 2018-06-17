CXX= g++
CXXFLAGS= -c -g -Wall -std=c++11
TARFILES= ex5.tar whatsappServer.cpp whatsappClient.cpp Makefile README

# All
all: Server Client

# make Executable file
Server: whatsappServer.o
	$(CXX) whatsappServer.o -o whatsappServer

Client: whatsappClient.o
	$(CXX) whatsappClient.o -o whatsappClient

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $<

# tar
tar:
	tar -cvf $(TARFILES)

# Other Targets

clean:
	-rm -vf *.o *.a whatsappClient whatsappServer
