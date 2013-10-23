#include <girepository.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool is_glib=false;
void set_is_glib(const char* namespace)
{
	is_glib=!strcmp(namespace, "GLib");
}
const char* glib_prefix()
{
	return is_glib?"":"GLib::";
}

void process_info(GIBaseInfo* info);
FILE* out = NULL;
int indent = 0;
#define fmt_line(...) do{fprintf(out, "\n% *s", 4*indent, ""); fprintf(out, __VA_ARGS__);}while(0)

#include "2d-bindgen-guts.c"


void process_info(GIBaseInfo* info)
{
	switch(g_base_info_get_type(info))
	{
		case GI_INFO_TYPE_FUNCTION:
		case GI_INFO_TYPE_CALLBACK:
			return do_function(info);
			break;
		case GI_INFO_TYPE_STRUCT:
			return do_struct(info);
			break;
		case GI_INFO_TYPE_BOXED:
			break;
		case GI_INFO_TYPE_ENUM:
			return do_enum(info);
			break;
		case GI_INFO_TYPE_FLAGS:
			break;
		case GI_INFO_TYPE_OBJECT:
			return do_object(info);
			break;
		case GI_INFO_TYPE_INTERFACE:
			return do_interface(info);
			break;
		case GI_INFO_TYPE_CONSTANT:
//TODO:			return do_constant(info);
			break;
		case GI_INFO_TYPE_UNION:
			return do_union(info);
			break;
		case GI_INFO_TYPE_VALUE:
			return do_value(info);
			break;
		case GI_INFO_TYPE_SIGNAL:
		case GI_INFO_TYPE_VFUNC:
		case GI_INFO_TYPE_PROPERTY:
		case GI_INFO_TYPE_FIELD:
			break;
		case GI_INFO_TYPE_ARG:
			break;
		case GI_INFO_TYPE_TYPE:
			return do_type(info);
			break;
		default:
			fprintf(stderr, "invalid info type '%s' for %s::%s\n",
				g_info_type_to_string(g_base_info_get_type(info)), 
				g_base_info_get_namespace(info), g_base_info_get_name(info));
			break;
	}
	fprintf(stderr, "trace: %s is a %s\n", g_base_info_get_name(info),
		g_info_type_to_string(g_base_info_get_type(info)));
}

void process_namespace(GIRepository* repo, const gchar* namespace)
{
	set_is_glib(namespace);
	fmt_line("pub mod %s {", namespace);
	indent++;
	fmt_line("#[link_name=\"%s\"]", basename(g_strdup(g_irepository_get_shared_library(repo, namespace))));
	fmt_line("extern mod grust;");
	fmt_line("use std::libc;");
	if(!is_glib)
		fmt_line("use GLib;");
	//fmt_line("use grust::utf8;");
	fmt_line("type gboolean = u32;");
	fmt_line("type interface = ();");
	int n = g_irepository_get_n_infos (repo, namespace);
	int i;
	for(i=0; i<n; i++)
	{
		process_info(g_irepository_get_info(repo, namespace, i));
	}
	indent--;
	fmt_line("}\n");
}

int main(int argc, char** argv)
{
	out = stdout;
	GError* err = NULL;
	GIRepository* repo = g_irepository_get_default();
	
	GITypelib* lib = NULL;
	if(argc>1)
		lib = g_irepository_require(repo, argv[1], NULL, 0, &err);
	if(err)
	{
		fprintf(stderr, "%s\n", err->message);
		exit(1);
	}
	
	fmt_line("#[crate_type = \"lib\"];");
	
	gchar** namespaces = g_irepository_get_loaded_namespaces(repo);
	int i;
	for(i=0; namespaces[i]; i++)
	{
		process_namespace(repo, namespaces[i]);
	}
	g_strfreev(namespaces);
}
