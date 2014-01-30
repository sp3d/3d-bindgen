build: 2d-bindgen

2d-bindgen: 2d-bindgen.c 2d-bindgen-guts.c type-convert.c
	gcc -g `pkg-config --cflags --libs gobject-introspection-1.0` 2d-bindgen.c type-convert.c -o 2d-bindgen

GRUST_LIBS=-L ../../grust/build/grust/.libs/ -L ../../grust/build/na/.libs/

TEST_LIB=GLib
#TEST_LIB=GIRepository

g-hello-world: gen-GLib/libgrust-GLib-*.so
	rustc -L gen-GLib/ -L ../../grust/build/grust/.libs/ -L ../../grust/build/na/.libs/ g-hello-world.rs 

bin-test: g-hello-world
	LD_LIBRARY_PATH=../../grust/build/na/.libs/ ./g-hello-world

generate: gen-$(TEST_LIB)/lib.rs

gen-$(TEST_LIB)/lib.rs: 2d-bindgen
	./2d-bindgen $(TEST_LIB)

generate-compile: gen-GLib/libgrust-$(TEST_LIB)-*.so

gen-$(TEST_LIB)/libgrust-$(TEST_LIB)-*.so: gen-$(TEST_LIB)/lib.rs
	rustc --lib gen-$(TEST_LIB)/lib.rs $(GRUST_LIBS)

clean:
	$(RM) 2d-bindgen
	$(RM) -r gen-*
	$(RM) -r g-hello-world
