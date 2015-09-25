#######  Compiler, tools and options
CXX           = g++
DEFINES       = -DQT_WEBKIT -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED
CXXFLAGS        = -pipe -O2 -Wall -W -D_REENTRANT $(DEFINES)
INCPATH       = -I/home/pi/lepton/common -I/home/pi/lepton/leptonSDKEmb32PUB -I.
LINK          = g++
LFLAGS        = -Wl,-O1
LIBS          = -L/home/pi/lepton/leptonSDKEmb32PUB/Debug
DEL_FILE		  = rm -f

####### Output directory
OBJECTS_DIR   = ./

####### Files
SOURCES = main.cpp \
					Palettes.cpp \
					SPI.cpp 

OBJECTS = main.o \
					Palettes.o \
					SPI.o 

TARGET = lepdump

####### Implicit rules
.SUFFIXES: .cpp

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

####### Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

clean:
	-$(DEL_FILE) $(OBJECTS) $(TARGET)

####### Compile
main.o: main.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o main.o main.cpp

Palettes.o: ../common/Palettes.cpp ../common/Palettes.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o Palettes.o ../common/Palettes.cpp

SPI.o: ../common/SPI.cpp ../common/SPI.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o SPI.o ../common/SPI.cpp
