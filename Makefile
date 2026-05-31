HEADERS = $(wildcard src/*.h)
SOURCES = $(wildcard src/*.cpp)
FORMAT = clang-format --style=llvm -i

all: format build

format: ${SOURCES} ${HEADERS}
	$(FORMAT) $(HEADERS) $(SOURCES)

clean:
	cd build && make clean && cd ..

build: ${SOURCES} ${HEADERS}
	cd build && cmake .. && make cylang && cd ..