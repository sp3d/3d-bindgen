build: 2d-bindgen

2d-bindgen: 2d-bindgen.c 2d-bindgen-guts.c
	gcc -g `pkg-config --cflags --libs gobject-introspection-1.0` 2d-bindgen.c -o 2d-bindgen

GRUST_LIBS=-L ../../grust/build/grust/.libs/

TEST_LIB=GLib
#TEST_LIB=GIRepository

generate: gen-$(TEST_LIB).rs

gen-$(TEST_LIB).rs: 2d-bindgen
	./2d-bindgen $(TEST_LIB) > gen-$(TEST_LIB).rs

generate-compile: gen-$(TEST_LIB).rs
	rustc gen-$(TEST_LIB).rs $(GRUST_LIBS)

clean:
	$(RM) 2d-bindgen gen-*.rs libgrust-*-2d-bindgen-*.so
