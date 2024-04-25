
default: demo

BINFILE := subnet-subtractor
SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:%.cpp=%.o)

demo: $(BINFILE)
	./$< 192.168.0.0/17 +192.168.128.0/17 -192.168.178.0/24 -192.168.179.0/24

clean:
	rm -f $(BINFILE) $(OBJECTS)

$(BINFILE): $(OBJECTS)
	g++ -o $@ $^

%.o: %.cpp %.h Makefile
	g++ -o $@ -c $<
