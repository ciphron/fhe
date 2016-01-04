CXX		= 	g++
CXXFLAGS 	= 	

# replace with location of HElib on your system
HELIB		= 	/opt/HElib
HELIB_LIB	=	$(HELIB)/src/fhe.a
HELIB_SRC	=	$(HELIB)/src
LDFLAGS 	= 	$(HELIB_LIB) /usr/local/lib/libntl.a libscdl.a -ljson
SOURCES 	= 	fhe.cpp
EXEC		= 	fhe

all: $(SOURCES) $(EXEC)

$(EXEC): $(SOURCES)
	$(CXX) $(CXXFLAGS) -I$(HELIB_SRC) $(SOURCES) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(EXEC)
