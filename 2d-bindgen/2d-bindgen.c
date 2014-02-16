#include <girepository.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>


bool is_glib=false;
void set_is_glib(const char* namespace)
{
	is_glib=!strcmp(namespace, "GLib");
}
const char* glib_prefix()
{
	return is_glib?"":"GLib::";
}

const gchar* shared_lib_path = NULL;
void process_info(GIBaseInfo* info);
FILE* raw = NULL;
FILE* hl = NULL;
int raw_indent = 0;
int hl_indent = 0;
#define raw_line(...) do{fprintf(raw, "\n% *s", 4*raw_indent, ""); fprintf(raw, __VA_ARGS__);}while(0)
#define hl_line(...) do{fprintf(hl, "\n% *s", 4*hl_indent, ""); fprintf(hl, __VA_ARGS__);}while(0)

#define fmt_line(f, ...) do{fprintf(f, "\n% *s", 4*raw_indent, ""); fprintf(f, __VA_ARGS__);}while(0)

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
			return do_constant(info);
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

char* progname = NULL;

void process_namespace(GIRepository* repo, const char* outdir, const gchar* namespace)
{
	//generate a file for that namespace's raw contents
	char* filename=g_strdup_printf("%s/gen-%s.rs", outdir, namespace);
	raw = fopen(filename, "w");
	assert(raw);
	g_free(filename);
	
	set_is_glib(namespace);
	raw_line("extern mod grust;");
	raw_line("use std::libc;");
	if(!is_glib)
		raw_line("use GLib;");
	
	raw_line("use grust::gboolean;");
	raw_line("pub type interface = ();");
	
	char* hl_filename=g_strdup_printf("%s/lib.rs", outdir);
	hl = fopen(hl_filename, "w");
	assert(hl);
	g_free(hl_filename);
	
	fprintf(hl, "/* rust binding generated by %s; not for manual modification */", progname);
	hl_line("#[crate_id = \"grust-%s-%s#2.0\"];", namespace, progname);
	hl_line("#[desc = \"Rust bindings for %s, generated by 2d-bindgen\"];", namespace);
	hl_line("#[crate_type = \"lib\"];");
	hl_line("");

	hl_line("extern mod grust;");
	hl_line("#[path=\"gen-%s.rs\"]", namespace);
	hl_line("mod raw;");
	hl_line("");
	
	hl_line("pub mod %s {", namespace);
	hl_indent++;
	hl_line("use raw;");
	hl_line("use grust;");
	hl_line("use std::libc;");
	hl_line("use grust::gboolean;");/*TODO: fix gboolean*/
		//TODO: output high-level wrappers...
	
	shared_lib_path = g_irepository_get_shared_library(repo, namespace);
	
	int n = g_irepository_get_n_infos (repo, namespace);
	int i;
	for(i=0; i<n; i++)
	{
		process_info(g_irepository_get_info(repo, namespace, i));
	}
	fclose(raw);
	
	hl_indent--;
	hl_line("}\n");
}

int main(int argc, char** argv)
{
	GError* err = NULL;
	GIRepository* repo = g_irepository_get_default();
	
	progname = basename(argv[0]);
	
	GITypelib* lib = NULL;
	if(argc>1)
	{
		const char* libname = argv[1];
		lib = g_irepository_require(repo, libname, NULL, 0, &err);
		if(err)
		{
			fprintf(stderr, "%s\n", err->message);
			exit(1);
		}
		
		char* dirname=g_strdup_printf("gen-%s", libname);
		if(mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
		{
			if(errno != EEXIST)
			{
				fprintf(stderr, "could not create directory %s: %s", dirname, strerror(errno));
				return;
			}
		}

		/*TODO: recursive dependency resolution CLI option*/
		int recurse=0;
		if(recurse)
		{
			gchar** namespaces = g_irepository_get_loaded_namespaces(repo);
			int i;
			for(i=0; namespaces[i]; i++)
			{
				process_namespace(repo, dirname, namespaces[i]);
			}
			g_strfreev(namespaces);
		}
		else
		{
			process_namespace(repo, dirname, libname);
		}
		g_free(dirname);
	}
	return 0;
}
