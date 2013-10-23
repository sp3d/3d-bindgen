/* 3d-bindgen: a GIR binding generator for Rust, in Rust, under the WTFPL. */
extern mod xml;

/* usage: 3d-bindgen < /usr/share/gir-1.0/Gtk-2.0.gir */

fn main()
{
	let mut p = xml::Parser::new();
//	let mut e = xml::ElementBuilder::new();
//	let mut level = 0;

	while !std::io::stdin().eof()
	{
		let input = std::io::stdin().read_line();
		do p.parse_str(input + "\n") |event| {
			match event {
				Ok(xml::PI(cont)) => printfln!("<?%s?>", cont),
				Ok(xml::StartTag(name, attrs)) => {
					printf!("<%s", name);
					for attr in attrs.iter() {
						printf!(" %s='%s'", attr.name, attr.value);
					}
					println(">");
				}
				Ok(xml::EndTag(name)) => printfln!("</%s>", name),
				Ok(xml::Characters(chars)) => if chars.iter().any(|x| !std::char::is_whitespace(x)) {println(chars)},
				Ok(xml::CDATA(chars)) => printfln!("<![CDATA[%s]]>", chars),
				Ok(xml::Comment(cont)) => printfln!("<!--%s-->", cont),
				Err(e) => printfln!("%?", e)
			}
			/*
			match event {
				Ok(event) => match e.push_event(event) {
					Ok(Some(e)) => printfln!(("%?", e)),
					Ok(None) => (),
					Err(e) => printfln!(("%?", e)),
				},
				Err(e) => printfln!(("%?", e))
			}
		//*/
		}
	}
}

/* the indentation of this enum is intended to summarize the GIR XML format
and therefore dictates the state transitions implemented in next_expectation*/
enum Expectation
{
	XmlHeader,
	Repository,
		Includes,
		Namespaces,
			RecordsFunctionsAliasesConstantsClass,
				Return,
					ReturnType,
				Parameters,
					ParameterTypes,
	Eof,
}

fn next_expectation(current: Expectation) -> &'static [Expectation]
{
	match current
	{
		XmlHeader => &[Repository],
		Repository => &[Includes],
		Includes => &[Namespaces],
		Namespaces => &[RecordsFunctions, Eof],
		RecordsFunctions => &[Return, Parameters],
		Return => &[ReturnType],
		ReturnType => &[],
		Parameters => &[ParameterTypes],
		ParameterTypes => &[],
		Eof => fail!("unexpected content after end of repository tag!"),
	}
}

//an empty next_expectation means that the current expectation can be popped off of the stack.
//an empty next_expectation means that the current expectation can be popped off of the stack.

struct State
{
	e: ~[Expectation];
	repository: Option<~str>;
	namespace: Option<~str>;
}

fn process_module(state: &mut State, e: &xml::Event)
{
	match sta
	let include_names = parse_includes(n);
	let namespaces = parse_namespaces(n);
}

/*parse_foo functions return whether they received an acceptable XML node*/

fn parse_repository(s: &mut State, e: &xml::Event) -> bool
{
	match event {
		Ok(xml::StartTag("repository", attrs)) => {
			for attr in attrs.iter() {
				printf!(" %s='%s'", attr.name, attr.value);
			}; true
		}
		_ => false
		Ok(xml::EndTag(name)) => printfln!("</%s>", name),
		Ok(xml::Characters(chars)) => if chars.iter().any(|x| !std::char::is_whitespace(x)) {println(chars)},
		Ok(xml::CDATA(chars)) => printfln!("<![CDATA[%s]]>", chars),
		Ok(xml::Comment(cont)) => printfln!("<!--%s-->", cont),
		Err(e) => printfln!("%?", e)
	}
	if "repository" != n.name {fail!()};
}

fn parse_includes(s: &mut State, e: &xml::Event) -> bool
{
	
}

fn parse_namespaces(_n: &xml::Element) -> bool
{

}
