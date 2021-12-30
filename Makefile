TARGET	= canonvr
SOURCES	= $(wildcard *.cpp) $(wildcard lib/*.cpp) lib/linux/nfd_gtk.cpp
HEADERS	= $(wildcard *.h) $(wildcard lib/*.h)
OBJECTS	= $(patsubst %.cpp,%.o,$(SOURCES))
CXXFLAGS	= --std=c++17 -g -Wall -DDEBUG -DX11 -pthread `pkg-config opencv4 --cflags` `pkg-config gtk+-3.0 --cflags` -Iinclude
LDLIBS	= `pkg-config opencv4 --libs` `pkg-config gtk+-3.0 --libs` `pkg-config glfw3 --libs` -ldl

.PHONY: clean

$(TARGET): $(OBJECTS)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(TARGET).dep: $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -MM $(SOURCES) > $@

clean:
	-$(RM) $(TARGET) *.o lib/*.o lib/linux/nfd_gtk.o *~ .*~ *.bak *.dep imgui.ini a.out core

-include $(TARGET).dep
