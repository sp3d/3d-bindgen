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
void raw_allocation(void)
{
	fprintf(out, "*");
}

void raw_type(GITypeInfo* type);
void do_function(GICallableInfo* info);

void raw_type_named(GITypeInfo* type, const gchar* name)
{
	fprintf(out, "%s: ", name);
	raw_type(type);
}

void do_arg(GIArgInfo* info)
{
	raw_type_named(g_arg_info_get_type(info), escaped_name(info));
}

void do_args(GICallableInfo* info, bool skip_first)
{
	int n = g_callable_info_get_n_args(info);
	int i;
	for(i=skip_first; i<n; i++)
	{
		GIArgInfo* arg = g_callable_info_get_arg(info, i);
		do_arg(arg);
		g_base_info_unref(arg);
		if(i+1<n)
		{
			fprintf(out, ", ");
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

void raw_type_params(GITypeInfo* type, int n)
{
	GITypeInfo* param = g_type_info_get_param_type(type, 0);
	int i=1;
	if(param)
	{
		fprintf(out, "<");
		raw_type(param);
		while(param && i<n)
		{
			g_base_info_unref(param);
			param = g_type_info_get_param_type(type, i++);
			if(param)
			{
				fprintf(out,", ");
				raw_type(param);
			}
		}
		g_base_info_unref(param);
		fprintf(out,">");
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

void raw_fn_type(GICallableInfo* type)
{
	fprintf(out, "extern \"C\" fn(");
	do_args(type, false);
	fprintf(out, ")");
}

void raw_union_type(GIUnionInfo* info)
{
	fprintf(out, "[u8, ..%zd]", g_union_info_get_size(info));
}

void raw_type(GITypeInfo* type)
{
	GITypeTag tag = g_type_info_get_tag(type);
	bool is_pointer = g_type_info_is_pointer(type);
	if(G_TYPE_TAG_IS_BASIC(tag))
	{
		if(is_pointer)
		{
			if(tag == GI_TYPE_TAG_VOID)
			{
				fprintf(out, "*libc::c_void");
				return;
			}
			else
			{
				raw_allocation();
			}
		}
		fprintf(out, "%s", type_tag(tag));
	}
	else
	{
		if(tag == GI_TYPE_TAG_ARRAY)
		{
			GIArrayType arr_type = g_type_info_get_array_type(type);
			GITypeInfo* param = g_type_info_get_param_type(type, 0);
			switch(arr_type)
			{
				case GI_ARRAY_TYPE_C:/*&[T] or ~[T]*/
					/*TODO: allocation*/
					fprintf(out, "&'static [");
					raw_type(param);
					fprintf(out, "]");
					break;
				case GI_ARRAY_TYPE_ARRAY:/* &/~ GLib::Array<T> */
					raw_allocation();
					fprintf(out, "%sArray<", glib_prefix());
					raw_type(param);
					fprintf(out, ">");
					break;
				case GI_ARRAY_TYPE_PTR_ARRAY:/* &/~ GLib::PtrArray<T> */
					raw_allocation();
					fprintf(out, "%sPtrArray<", glib_prefix());
					raw_type(param);
					fprintf(out, ">");
					break;
				case GI_ARRAY_TYPE_BYTE_ARRAY:/* &/~ GLib::ByteArray*/
					raw_allocation();
					fprintf(out, "%sByteArray", glib_prefix());
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
				raw_allocation();
			}
			GIBaseInfo* iface = g_type_info_get_interface(type);
			switch(g_base_info_get_type(iface))
			{
				case GI_INFO_TYPE_CALLBACK:
					raw_fn_type(iface);
					break;
				case GI_INFO_TYPE_STRUCT:
					//TODO: struct namespacing?
					fprintf(out, "%s", g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_BOXED:
					fprintf(out, "GLib::Boxed<%s>", g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_ENUM:
					fprintf(out, "%s::%s", g_base_info_get_name(iface), g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_FLAGS:
					fprintf(out, "u64/*(%s enum)*/", g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_OBJECT:
					fprintf(out, "grust::Object<%s>", g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_INTERFACE:
					fprintf(out, "grust::Interface<%s>", g_base_info_get_name(iface));
					break;
				case GI_INFO_TYPE_UNION:
					raw_union_type(iface);
					break;
				case GI_INFO_TYPE_UNRESOLVED:
					fprintf(stderr, "unresolved type %s! any bindings using this type will be garbage! (let's call it a void* for now)\n", g_base_info_get_name(iface));
					fprintf(out, "*libc::c_void");
					break;
				default:
					fprintf(stderr, "[iface type %d %s]\n", g_base_info_get_type(iface), g_base_info_get_name(iface));
					break;
			}
			g_base_info_unref(iface);
		}
		else if(tag == GI_TYPE_TAG_GLIST)
		{
			raw_allocation();
			fprintf(out, "%sList", glib_prefix());
			raw_type_params(type, 1);
		}
		else if(tag == GI_TYPE_TAG_GSLIST)
		{
			raw_allocation();
			fprintf(out, "%sSList", glib_prefix());
			raw_type_params(type, 1);
		}
		else if(tag == GI_TYPE_TAG_GHASH)
		{
			raw_allocation();
			fprintf(out, "%sHashTable", glib_prefix());
			raw_type_params(type, 2);
		}
		else if(tag == GI_TYPE_TAG_ERROR)
		{
			/*TODO: allocation*/
			fprintf(out, "&mut &mut %sError", glib_prefix());
		}
		else
		{
			fprintf(stderr, "(type %s)", type_tag(tag));
		}
	}
}

/* first output a FFI definition for the C function,
then output a Rust-flavored wrapper fn/method */
void do_function(GICallableInfo* info)
{
	const gchar* function_name = escaped_name(info);
	int n = g_callable_info_get_n_args(info);
	bool option = g_callable_info_may_return_null(info);
	
	bool is_callback = g_base_info_get_type(info) == GI_INFO_TYPE_CALLBACK;
	bool is_method = false;
	if(!is_callback)
		is_method = !!(g_function_info_get_flags(info) & GI_FUNCTION_IS_METHOD);

	//bool getter maps to (&A) -> (B)
	//bool setter maps to (&A or ~A) -> ()
	//bool has_error maps to Result<T>
	
	if(is_callback)
	{
		fmt_line("type %s = ", function_name);
		raw_fn_type(info);
	}
	else if(is_method)
	{
		GITypeInfo* object_type = g_callable_info_get_arg(info, 0);
		const gchar* type_name = escaped_name(object_type);
/*
impl Foo {
fn bar(&self) -> uint {4}
}
*/
		fmt_line("impl %s {", type_name);
		indent++;
		fmt_line("fn %s(", function_name);
		/*do self arg*/
		fprintf(out, "?DUMMY_SELF, ");
		do_args(info, true);
		fprintf(out, ")");
		indent--;
		fmt_line("}");
		g_base_info_unref(object_type);
	}
	else
	{
		bool constructor = !!(g_function_info_get_flags(info) & GI_FUNCTION_IS_CONSTRUCTOR);
		const gchar* extern_name = g_function_info_get_symbol(info);
		fmt_line("extern \"C\" { fn %s(", function_name);
		do_args(info, false);
		fprintf(out, ")");
	}
	GITypeInfo* ret_type = g_callable_info_get_return_type(info);
	if(g_type_info_get_tag(ret_type) != GI_TYPE_TAG_VOID)
	{
		fprintf(out, " -> ");
		raw_type(ret_type);
	}
	g_base_info_unref(ret_type);
	if(is_callback)
	{
		fprintf(out, ";");
	}
	else if(!is_method)
	{
		fprintf(out, ";}");
	}
}

void do_field(GIFieldInfo* info)
{
	bool writable = !!(g_field_info_get_flags(info) & GI_FIELD_IS_WRITABLE);
	fmt_line("");
	raw_type_named(g_field_info_get_type(info), escaped_name(info));
	printf(",");
}

void do_struct(GIStructInfo* info)
{
	const char* name = escaped_name(info);
	fmt_line("struct %s", name);
	if(is_glib)
	{
		if(!strcmp(name, "List") || !strcmp(name, "SList") || !strcmp(name, "Array") || !strcmp(name, "PtrArray"))
		{
			fprintf(out, "<T>");
		}
		else if(!strcmp(name, "HashTable"))
		{
			fprintf(out, "<K, V>");
		}
	}
	
	int n = g_struct_info_get_n_fields(info);
	int i;
	if(n>0)
	{
		fprintf(out, " {");
		indent++;
		for(i=0; i<n; i++)
		{
			GITypeInfo* field = g_struct_info_get_field(info, i);
			do_field(field);
			g_base_info_unref(field);
		}
		indent--;
		fmt_line("}");
	}
	else
	{
		fprintf(out, ";");
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
	fmt_line("mod %s { %spub enum %s {", escaped_name(info), repr, escaped_name(info));
	indent++;
	
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
	indent--;
	/* TODO: use max and/or min to determine signedness/range */
	fmt_line("} }");
}

/*do_flags*/

void do_object(GIObjectInfo* info)
{
	/* TODO: more complete object support */
	const char* name = escaped_name(info);
	fmt_line("struct %s", name);
	fprintf(out, ";");
}

/*this may be possible to turn into a trait!*/
void do_interface(GIInterfaceInfo* info)
{
	"trait %s : %s {"/*, traitname, pre+req+uisites*/;
	"fn %s(";
	")";
	"}";
}

void do_constant(GIConstantInfo* info)
{
	//TODO: this
}

//...

/*a possibly-untagged enum*/
void do_union(GIUnionInfo* info)
{
	//fmt_line("%s: ", escaped_name(info));
	fmt_line("type %s = ", escaped_name(info));
	raw_union_type(info);
	fprintf(out, ";");
}
//...

/*a integral enum variant*/
void do_value(GIValueInfo* info)
{
	gint64 value = g_value_info_get_value(info);
	fmt_line("%s = %d,", escaped_name(info), value);
}

void do_type(GITypeInfo* info)
{
	fmt_line("type %s = ", escaped_name(info));
	raw_type(info);
	fprintf(out, ";");
}


//...


