extern crate glib = "grust-GLib-2d-bindgen";
use glib::GLib;

fn main()
{
	println!("{}", GLib::strdup(unsafe {std::cast::transmute(~"hello world")}));
}