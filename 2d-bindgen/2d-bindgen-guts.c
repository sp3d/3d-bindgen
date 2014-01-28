#include <assert.h>
#include <ctype.h>

gboolean name_is_valid(const gchar* name)
{
	if(isdigit(name[0]))
		return false;
	
	const char* keywords[]={"match", "loop", "fn", "extern", "priv", "pub", "mod", "ref", "in", "type", "static", "true", "false", "break", "struct", "enum"};
	int i;
	for(i=0; i<sizeof(keywords)/sizeof(keywords[0]); i++)
	{
		if(!strcmp(name, keywords[i]))
		{
			return false;
		}
	}
	return true;
}

const gchar* escaped_name(GIBaseInfo* info)
{
	static gchar buf[256];
	const gchar* name = g_base_info_get_name(info);
	if(!name)
	{
		return name;
	}
	if(!name_is_valid(name))
	{
		buf[0]='_';
		strncpy(buf+1, name, sizeof(buf)-2);
		return buf;
	}
	return name;
}

/*TODO: allocation*/
void bare_allocation(FILE* f)
{
	fprintf(f, "*");
}

void bare_type(FILE* f, GITypeInfo* type);
void do_function(GICallableInfo* info);

void bare_type_named(FILE* f, GITypeInfo* type, const gchar* name)
{
	fprintf(f, "%s: ", name);
	bare_type(f, type);
}

enum ArgShow
{
	ARGS_NAMES=1<<0,
	ARGS_TYPES=2<<0,
	ARGS_FULL=ARGS_NAMES|ARGS_TYPES,
};

void bare_arg(FILE* f, GIArgInfo* info, enum ArgShow show)
{
	if(show == ARGS_FULL)
		bare_type_named(f, g_arg_info_get_type(info), escaped_name(info));
	else if(show == ARGS_TYPES)
		bare_type(f, g_arg_info_get_type(info));
	else if(show == ARGS_NAMES)
		fprintf(f, "%s", escaped_name(info));
}

void bare_args(FILE* f, GICallableInfo* info, enum ArgShow show, bool skip_first)
{
	int n = g_callable_info_get_n_args(info);
	int i;
	for(i=skip_first; i<n; i++)
	{
		GIArgInfo* arg = g_callable_info_get_arg(info, i);
		bare_arg(f, arg, show);
		g_base_info_unref(arg);
		if(i+1<n)
		{
			fprintf(f, ", ");
		}
	}
}

const gchar* type_tag(GITypeTag tag)
{
	assert(tag != GI_TYPE_TAG_ARRAY);
	switch(tag)
	{
		case GI_TYPE_TAG_VOID:
			return "()";
		case GI_TYPE_TAG_INT8:
			return "i8";
		case GI_TYPE_TAG_UINT8:
			return "u8";
		case GI_TYPE_TAG_INT16:
			return "i16";
		case GI_TYPE_TAG_UINT16:
			return "u16";
		case GI_TYPE_TAG_INT32:
			return "i32";
		case GI_TYPE_TAG_UINT32:
			return "u32";
		case GI_TYPE_TAG_INT64:
			return "i64";
		case GI_TYPE_TAG_UINT64:
			return "u64";
		case GI_TYPE_TAG_FLOAT:
			return "f32";
		case GI_TYPE_TAG_DOUBLE:
			return "f64";
		case GI_TYPE_TAG_GTYPE:
			return "grust::object::GType";
		case GI_TYPE_TAG_UTF8:
			return "grust::utf8";
		case GI_TYPE_TAG_FILENAME:
			return "*Path";
		case GI_TYPE_TAG_UNICHAR:
			return "char";
		default:
			return g_type_tag_to_string(tag);
	}
}

void bare_type_params(FILE* f, GITypeInfo* type, int n)
{
	GITypeInfo* param = g_type_info_get_param_type(type, 0);
	int i=1;
	if(param)
	{
		fprintf(f, "<");
		bare_type(f, param);
		while(param && i<n)
		{
			g_base_info_unref(param);
			param = g_type_info_get_param_type(type, i++);
			if(param)
			{
				fprintf(f,", ");
				bare_type(f, param);
			}
		}
		g_base_info_unref(param);
		fprintf(f,">");
	}
}

/* used to determine what sort of pointers these functions/structs mean */
enum TypeContext
{
	TC_NONE=0,
	TC_ARG_IN=1<<0,
	TC_ARG_OUT=1<<1,
	TC_ARG_IN_OUT=TC_ARG_IN&TC_ARG_OUT,
	TC_RETURN_VAL=1<<2,
	TC_INSIDE_ARRAY=1<<3,
	TC_IS_OBJECT=1<<4,
	TC_IS_INTERFACE=1<<5,
	TC_IS_CONSTANT=1<<6,
	TC_TRANSFER_NONE=1<<7,
	TC_TRANSFER_CONTAINER=1<<8,
	TC_TRANSFER_FULL=1<<9,
};

void bare_fn_type(FILE* f, GICallableInfo* type)
{
	fprintf(f, "extern \"C\" fn(");
	bare_args(f, type, ARGS_FULL, false);
	fprintf(f, ")");
}

void bare_union_type(FILE* f, GIUnionInfo* info)
{
	fprintf(f, "[u8, ..%zd]", g_union_info_get_size(info));
}

void bare_type(FILE* f, GITypeInfo* type)
{
	GITypeTag tag = g_type_info_get_tag(type);
	bool is_pointer = g_type_info_is_pointer(type);
	if(G_TYPE_TAG_IS_BASIC(tag))
	{
		if(is_pointer)
		{
			if(tag == GI_TYPE_TAG_VOID)
			{
				fprintf(f, "*libc::c_void");
				return;
			}
			else
			{
				bare_allocation(f);
			}
		}
		fprintf(f, "%s", type_tag(tag));
	}
	else
	{
		if(tag == GI_TYPE_TAG_ARRAY)
		{
			GIArrayType arr_type = g_type_info_get_array_type(type);
			GITypeInfo* param = g_type_info_get_param_type(type, 0);
			switch(arr_type)
			{
				case GI_ARRAY_TYPE_C:/* raw pointer */
					fprintf(f, "*");
					bare_type(f, param);
					break;
				case GI_ARRAY_TYPE_ARRAY:/* &/~ GLib::Array<T> */
					bare_allocation(f);
					fprintf(f, "%sArray<", glib_prefix());
					bare_type(f, param);
					fprintf(f, ">");
					break;
				case GI_ARRAY_TYPE_PTR_ARRAY:/* &/~ GLib::PtrArray<T> */
					bare_allocation(f);
					fprintf(f, "%sPtrArray<", glib_prefix());
					bare_type(f, param);
					fprintf(f, ">");
					break;
				case GI_ARRAY_TYPE_BYTE_ARRAY:/* &/~ GLib::ByteArray*/
					bare_allocation(f);
					fprintf(f, "%sByteArray", glib_prefix());
					break;
				default:
					assert(0);
			}
			g_base_info_unref(param);
		}
		else if(tag == GI_TYPE_TAG_INTERFACE)
		{
			if(is_pointer)
			{
				bare_allocation(f);
			}
			GIBaseInfo* iface = g_type_info_get_interface(type);
			switch(g_base_info_get_type(iface))
			{
				case GI_INFO_TYPE_CALLBACK:
					bare_fn_type(f, iface);
					break;
				case GI_INFO_TYPE_STRUCT:
					//TODO: struct namespacing?
					fprintf(f, "%s", g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_BOXED:
					fprintf(f, "GLib::Boxed<%s>", g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_ENUM:
					fprintf(f, "%s::%s", g_base_info_get_name(iface), g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_FLAGS:
					fprintf(f, "u64/*(%s enum)*/", g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_OBJECT:
					fprintf(f, "grust::Object<%s>", g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_INTERFACE:
					fprintf(f, "grust::Interface<%s>", g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_UNION:
					bare_union_type(f, iface);
					break;
				case GI_INFO_TYPE_UNRESOLVED:
					fprintf(stderr, "unresolved type %s! any bindings using this type will be garbage! (let's call it a void* for now)\n", g_base_info_get_name(iface));
					fprintf(f, "*libc::c_void");
					break;
				default:
					fprintf(stderr, "[iface type %d %s]\n", g_base_info_get_type(iface), g_base_info_get_name(iface));
					break;
			}
			g_base_info_unref(iface);
		}
		else if(tag == GI_TYPE_TAG_GLIST)
		{
			bare_allocation(f);
			fprintf(f, "%sList", glib_prefix());
			bare_type_params(f, type, 1);
		}
		else if(tag == GI_TYPE_TAG_GSLIST)
		{
			bare_allocation(f);
			fprintf(f, "%sSList", glib_prefix());
			bare_type_params(f, type, 1);
		}
		else if(tag == GI_TYPE_TAG_GHASH)
		{
			bare_allocation(f);
			fprintf(f, "%sHashTable", glib_prefix());
			bare_type_params(f, type, 2);
		}
		else if(tag == GI_TYPE_TAG_ERROR)
		{
			/*TODO: allocation*/
			fprintf(f, "&mut &mut %sError", glib_prefix());
		}
		else
		{
			fprintf(stderr, "(type %s)", type_tag(tag));
		}
	}
}

/* output a return type for a function, or nothing if it returns void */
void bare_return_type(FILE* f, GICallableInfo* info)
{
	GITypeInfo* ret_type = g_callable_info_get_return_type(info);
	if(g_type_info_get_tag(ret_type) != GI_TYPE_TAG_VOID)
	{
		fprintf(f, " -> ");
		bare_type(f, ret_type);
	}
	g_base_info_unref(ret_type);
}

/* first output a FFI definition for the C function,
then output a Rust-flavored wrapper fn/method */
void do_function(GICallableInfo* info)
{
	const gchar* function_name = escaped_name(info);
	int n = g_callable_info_get_n_args(info);
	bool option = g_callable_info_may_return_null(info);
	
	bool is_callback = g_base_info_get_type(info) == GI_INFO_TYPE_CALLBACK;
	bool is_constructor = !is_callback && !!(g_function_info_get_flags(info) & GI_FUNCTION_IS_CONSTRUCTOR);
	bool is_method = false;
	if(!is_callback)
		is_method = !!(g_function_info_get_flags(info) & GI_FUNCTION_IS_METHOD);
	
	//bool getter maps to (&A) -> (B)
	//bool setter maps to (&A or ~A) -> ()
	//bool has_error maps to Result<T>
	
	if(is_callback)
	{
		/* simply output the type */
		raw_line("pub type %s = ", function_name);
		bare_fn_type(raw, info);
		fprintf(raw, ";");
		
		/* ...and expose it at the top-level */
		hl_line("pub type %s = raw::%s;", function_name, function_name);
		return;
	}
	
	/* output raw binding */
	const gchar* extern_name = g_function_info_get_symbol(info);
	raw_line("extern \"C\" { pub fn %s(", extern_name);
	bare_args(raw, info, ARGS_FULL, false);
	fprintf(raw, ")");
	bare_return_type(raw, info);
	fprintf(raw, ";}");
	
	/* output high-level binding */
	if(is_method)
	{
		/* output high-level binding as method */
		GITypeInfo* object_type = g_callable_info_get_arg(info, 0);
		const gchar* type_name = escaped_name(object_type);
/*
impl Foo {
fn bar(&self) -> uint {4}
}
*/
		raw_line("impl %s {", type_name);
		raw_indent++;
		raw_line("fn %s(", function_name);
		/*do self arg*/
		fprintf(raw, "&mut self, ");
		bare_args(raw, info, ARGS_FULL, true);
		fprintf(raw, ")");
		raw_indent--;
		raw_line("}");
		g_base_info_unref(object_type);
	}
/*	else if(is_constructor)
	{
		//TODO: output proper constructor function
	}*/
	else
	{
		/* output regular fn wrapper */
		hl_line("pub fn %s(", function_name);
		bare_args(hl, info, ARGS_FULL, false);
		fprintf(hl, ")");
		bare_return_type(hl, info);
		fprintf(hl, " { unsafe {");
		const gchar* extern_name = g_function_info_get_symbol(info);
		fprintf(hl, "raw::%s(", extern_name);
		bare_args(hl, info, ARGS_NAMES, false);
		fprintf(hl, ") } }");
	}
}

void do_field(GIFieldInfo* info)
{
	bool writable = !!(g_field_info_get_flags(info) & GI_FIELD_IS_WRITABLE);
	fmt_line(raw, "");
	bare_type_named(raw, g_field_info_get_type(info), escaped_name(info));
	fprintf(raw, ",");
}

void bare_type_generic_params(FILE* f, GITypeInfo* type)
{
	const char* name = escaped_name(type);
	if(1/*TODO: only emit for these types from the glib namespace*/)
	{
		if(!strcmp(name, "List") || !strcmp(name, "SList") || !strcmp(name, "Array") || !strcmp(name, "PtrArray"))
		{
			fprintf(f, "<T>");
		}
		else if(!strcmp(name, "HashTable"))
		{
			fprintf(f, "<K, V>");
		}
	}
}

void do_struct(GIStructInfo* info)
{
	const char* name = escaped_name(info);
	raw_line("pub struct %s", name);
	bare_type_generic_params(raw, info);
	hl_line("pub type %s", name);
	bare_type_generic_params(hl, info);
	fprintf(hl, " = raw::%s", name);
	bare_type_generic_params(hl, info);
	fprintf(hl, ";");
	
	int n = g_struct_info_get_n_fields(info);
	int i;
	if(n>0)
	{
		fprintf(raw, " {");
		raw_indent++;
		for(i=0; i<n; i++)
		{
			GITypeInfo* field = g_struct_info_get_field(info, i);
			do_field(field);
			g_base_info_unref(field);
		}
		raw_indent--;
		raw_line("}");
	}
	else
	{
		fprintf(raw, ";");
	}
}

/*do_boxed: a Drop impl and a DeepClone impl*/

void do_value(GIValueInfo* info);

/*a rust enum, with only integral variants*/
void do_enum(GIEnumInfo* info)
{
	int n = g_enum_info_get_n_values(info);
	int i;
	gint64 min = G_MAXINT64;
	gint64 max = G_MININT64;
	
	/*TODO: single-variant enums *do* need to have a specified repr but this is impossible currently*/
	const char* repr = (n>1) ? "#[repr(C)] " : "";
	raw_line("pub mod %s { %spub enum %s {", escaped_name(info), repr, escaped_name(info));
	raw_indent++;
	
	hl_line("pub mod %s { use raw; pub type %s = raw::%s::%s; }", escaped_name(info), escaped_name(info), escaped_name(info), escaped_name(info));
	
	GITypeInfo* first_variant = g_enum_info_get_value(info, 0);
	gint64 last_value = g_value_info_get_value(first_variant)+1;
	g_base_info_unref(first_variant);
	
	for(i=0; i<n; i++)
	{
		GITypeInfo* variant = g_enum_info_get_value(info, i);
		gint64 value = g_value_info_get_value(variant);
		min = MIN(min, value);
		max = MAX(max, value);
		if(value != last_value)
		{
			do_value(variant);
		}
		else
		{
			fprintf(stderr, "duplicate value %d for %s::", value, escaped_name(info));
			fprintf(stderr, "%s\n", escaped_name(variant));
		}
		last_value = value;
		g_base_info_unref(variant);
	}
	raw_indent--;
	/* TODO: use max and/or min to determine signedness/range */
	raw_line("} }");
}

/*do_flags*/

void do_object(GIObjectInfo* info)
{
	/* TODO: more complete object support */
	const char* name = escaped_name(info);
	raw_line("/*object*/pub struct %s", name);
	fprintf(raw, ";");
}

/*this may be possible to turn into a trait!*/
void do_interface(GIInterfaceInfo* info)
{
	"trait %s : %s {"/*, traitname, pre+req+uisites*/;
	"fn %s(";
	")";
	"}";
}

/*TODO: this is WRONG. g_strescape escapes \b, \f, \n, \r, \t, \v, ', and ", but turns 0x01-0x1f,0x7f-0xff into "\octal" which Rust does not support. */
gchar* rust_str_escape(const gchar* str)
{
	return g_strescape(str, NULL);
/*	size_t n = strlen(str);
	escaped=malloc(n*4);*/ /*worst-case we escape every character*/
}

void bare_value(FILE* f, GIArgument value, GITypeInfo* type)
{
	GITypeTag tag = g_type_info_get_tag(type);
	assert(tag != GI_TYPE_TAG_ARRAY);
	switch(tag)
	{
		case GI_TYPE_TAG_VOID:
			fprintf(f, "()");
			break;
		case GI_TYPE_TAG_INT8:
			fprintf(f, "%d", value.v_int8);
			break;
		case GI_TYPE_TAG_UINT8:
			fprintf(f, "%d", value.v_uint8);
			break;
		case GI_TYPE_TAG_INT16:
			fprintf(f, "%d", value.v_int16);
			break;
		case GI_TYPE_TAG_UINT16:
			fprintf(f, "%d", value.v_uint16);
			break;
		case GI_TYPE_TAG_INT32:
			fprintf(f, "%d", value.v_int32);
			break;
		case GI_TYPE_TAG_UINT32:
			fprintf(f, "%d", value.v_uint32);
			break;
		case GI_TYPE_TAG_INT64:
			fprintf(f, "%d", value.v_int64);
			break;
		case GI_TYPE_TAG_UINT64:
			fprintf(f, "%d", value.v_uint64);
			break;
		case GI_TYPE_TAG_FLOAT:/*TODO: unambiguous float notation?*/
			fprintf(f, "%f", value.v_float);
			break;
		case GI_TYPE_TAG_DOUBLE:/*TODO: unambiguous float notation?*/
			fprintf(f, "%lf", value.v_double);
			break;
		case GI_TYPE_TAG_UTF8:
			{
				gchar* str=rust_str_escape(value.v_string);
				fprintf(f, "\"%s\"", str);
				free(str);
			}
			break;
		default:
			assert(0 && "unknown type tag when expressing value");
	}
}

void do_constant(GIConstantInfo* info)
{
	GIArgument value;/*union of v_foo fields for various types. /great/.*/
	g_constant_info_get_value(info, &value);

	hl_line("pub static %s: ", escaped_name(info));
	GITypeInfo* type = g_constant_info_get_type(info);
	/*TODO: string types are a mess, generate constants w/ bytes!?*/
	if(g_type_info_get_tag(type)==GI_TYPE_TAG_UTF8)
		fprintf(hl, "&'static str");
	else
		bare_type(hl, type);
	fprintf(hl, " = ");
	bare_value(hl, value, type);
	g_base_info_unref(type);
	fprintf(hl, ";");
}

//...

/*a possibly-untagged enum*/
void do_union(GIUnionInfo* info)
{
	//raw_line("%s: ", escaped_name(info));
	raw_line("pub type %s = ", escaped_name(info));
	bare_union_type(raw, info);
	fprintf(raw, ";");
}
//...

/*a integral enum variant*/
void do_value(GIValueInfo* info)
{
	gint64 value = g_value_info_get_value(info);
	raw_line("%s = %d,", escaped_name(info), value);
}

void do_type(GITypeInfo* info)
{
	raw_line("pub type %s = ", escaped_name(info));
	bare_type(raw, info);
	fprintf(raw, ";");
}


//...


