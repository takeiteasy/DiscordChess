TARGET=ffi_img_test
LIB=ffi_img_lib
CC=clang
DEPS=-lpng

all: $(TARGET)

$(TARGET): $(TARGET).o $(LIB).dylib
	$(CC) $^ -o $@

$(TARGET).o: tests/$(TARGET).c
	$(CC) -c $< -o $@

$(LIB).dylib: $(LIB).o
	clang -shared -fpic -o $@ $^ $(DEPS)

$(LIB).o: $(LIB).c $(LIB).h
	$(CC) -c -o $@ $<

clean:
	rm -f *.o *.dylib $(TARGET)