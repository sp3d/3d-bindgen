build: 2d-bindgen

2d-bindgen: 2d-bindgen.c 2d-bindgen-guts.c
	gcc -g `pkg-config --cflags --libs gobject-introspection-1.0` 2d-bindgen.c -o 2d-bindgen

GRUST_LIBS=-L ../../grust/build/grust/.libs/

TEST_LIB=GLib
#TEST_LIB=GIRepository

generate: gen-$(TEST_LIB)/lib.rs

gen-$(TEST_LIB)/lib.rs: 2d-bindgen
	./2d-bindgen $(TEST_LIB)
	mv $(TEST_LIB) gen-$(TEST_LIB)

generate-compile: gen-$(TEST_LIB)/lib.rs
	rustc --lib gen-$(TEST_LIB)/lib.rs $(GRUST_LIBS)

clean:
	$(RM) 2d-bindgen
	$(RM) -r gen-*
