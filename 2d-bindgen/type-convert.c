#include <girepository.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

typedef struct _rt rt;

typedef enum
{
	rt_param_type,
	rt_param_lifetime,
}
rt_param_kind;

typedef struct
{
	char* name;
}
rt_lifetime;

typedef struct
{
	rt_param_kind kind;
	union {
		rt_lifetime lifetime;
		rt* type;
	};
}
rt_param;

typedef struct
{
	char* name;
	rt_param* params;
	unsigned int n;
}
rt_prim;

typedef struct
{
}
rt_owned_str;

typedef struct
{
	rt_lifetime* lifetime;
}
rt_str_slice;

typedef struct
{
	rt* contents;
}
rt_owned_vec;

typedef struct
{
	rt* contents;
	rt_lifetime* lifetime;
}
rt_vec_slice;

typedef struct
{
	bool mut;
	rt* contents;
	rt_lifetime* lifetime;
}
rt_borrowed_ptr;

typedef struct
{
	rt* contents;
}
rt_raw_ptr;

typedef struct
{
	rt* contents;
}
rt_owned_ptr;

typedef struct
{
	rt* contents;
	unsigned int n;
}
rt_tuple;

typedef struct
{
	rt_tuple args;
	rt* result;
}
rt_func;

typedef struct
{
	rt_lifetime* lifetime;
	rt_tuple args;
	rt* result;
}
rt_closure;

typedef enum
{
	rt_kind_prim,
	rt_kind_owned_str,
	rt_kind_str_slice,
	rt_kind_owned_vec,
	rt_kind_vec_slice,
	rt_kind_borrowed_ptr,
	rt_kind_raw_ptr,
	rt_kind_owned_ptr,
	rt_kind_tuple,
	rt_kind_func,
	rt_kind_closure,
}
rt_kind;

struct _rt
{
	rt_kind kind;
	union
	{
		rt_prim prim;
		rt_owned_str owned_str;
		rt_str_slice str_slice;
		rt_owned_vec owned_vec;
		rt_vec_slice vec_slice;
		rt_borrowed_ptr borrowed_ptr;
		rt_raw_ptr raw_ptr;
		rt_owned_ptr owned_ptr;
		rt_tuple tuple;
		rt_func func;
		rt_closure closure;
	};
};

void bare_rust_lifetime(FILE* f, rt_lifetime* lifetime)
{
	if(!lifetime)
		return;
	fprintf(f, "'%s ", lifetime->name?lifetime->name:"static");
}

void bare_rust_type(FILE* f, rt* type);

void bare_rust_params(FILE* f, rt_param* params, int n)
{
	int i;
	for(i=0; i<n; i++)
	{
		if(params[i].kind==rt_param_type)
			bare_rust_type(f, params[i].type);
		else /*rt_param_lifetime*/
			bare_rust_lifetime(f, &params[i].lifetime);
		if(i<n-1)
			fprintf(f, ", ");
	}
}

void bare_rust_types(FILE* f, rt* types, int n)
{
	int i;
	for(i=0; i<n; i++)
	{
		bare_rust_type(f, &types[i]);
		if(i<n-1)
			fprintf(f, ", ");
	}
}

void bare_rust_type(FILE* f, rt* type)
{
	switch(type->kind)
	{

		case rt_kind_prim:
			fprintf(f, type->prim.name);
			if(type->prim.n)
			{
				fprintf(f, "<");
				bare_rust_params(f, type->prim.params, type->prim.n);
				fprintf(f, ">");
			}
			break;
		case rt_kind_owned_str:
			fprintf(f, "~str");
			break;
		case rt_kind_str_slice:
			fprintf(f, "&");
			bare_rust_lifetime(f, type->str_slice.lifetime);
			fprintf(f, "str");
			break;
		case rt_kind_owned_vec:
			fprintf(f, "~[");
			bare_rust_type(f, type->owned_vec.contents);
			fprintf(f, "]");
			break;
		case rt_kind_vec_slice:
			fprintf(f, "&");
			bare_rust_lifetime(f, type->vec_slice.lifetime);
			fprintf(f, "[");
			bare_rust_type(f, type->vec_slice.contents);
			fprintf(f, "]");
			break;
		case rt_kind_borrowed_ptr:
			fprintf(f, "&");
			bare_rust_lifetime(f, type->borrowed_ptr.lifetime);
			bare_rust_type(f, type->borrowed_ptr.contents);
			break;
		case rt_kind_raw_ptr:
			fprintf(f, "*");
			bare_rust_type(f, type->raw_ptr.contents);
			break;
		case rt_kind_owned_ptr:
			fprintf(f, "~");
			bare_rust_type(f, type->owned_ptr.contents);
			break;
		case rt_kind_tuple:
			fprintf(f, "(");
			bare_rust_types(f, type->tuple.contents, type->tuple.n);
			fprintf(f, ")");
			break;
		case rt_kind_func:
			fprintf(f, "proc(");
			bare_rust_types(f, type->func.args.contents, type->func.args.n);
			fprintf(f, ") -> ");
			bare_rust_type(f, type->func.result);
			break;
		case rt_kind_closure:
			if(type->closure.lifetime)
			{
				fprintf(f, "&");
				bare_rust_lifetime(f, type->closure.lifetime);
			}
			fprintf(f, "|");
			bare_rust_types(f, type->closure.args.contents, type->closure.args.n);
			fprintf(f, "| -> ");
			bare_rust_type(f, type->closure.result);
			break;
	}
}

rt* owned(rt* type)
{
	rt* owned=malloc(sizeof(rt));
	owned->kind=rt_kind_owned_ptr;
	owned->owned_ptr.contents=type;
	
	return owned;
}

rt* rawptr(rt* type, const char* lifetime)
{
	rt* raw=malloc(sizeof(rt));
	raw->kind=rt_kind_raw_ptr;
	raw->raw_ptr.contents=type;
	
	return raw;
}


rt* borrow(rt* type, bool mut, const char* lifetime)
{
	rt* borrowed=malloc(sizeof(rt));
	borrowed->kind=rt_kind_borrowed_ptr;
	borrowed->borrowed_ptr.contents=type;
	borrowed->borrowed_ptr.lifetime->name=lifetime?strdup(lifetime):NULL;
	borrowed->borrowed_ptr.mut=mut;
	
	return borrowed;
}

rt* option(rt* type)
{
	rt* option=malloc(sizeof(rt));
	option->kind=rt_kind_prim;
	option->prim.name=strdup("Option");
	option->prim.params=malloc(sizeof(rt_param));
	option->prim.params[0].kind=rt_param_type;
	option->prim.params[0].type=type;
	option->prim.n=1;
	
	return option;
}

rt* bind_unsafe_func_arg(GIArgInfo* arg)
{
	GITypeInfo* type=g_arg_info_get_type(arg);
	if(g_type_info_is_pointer(type))
	{
		
	}
	
	switch(g_type_info_get_tag(type))
	{
		case GI_TYPE_TAG_INTERFACE:/*object or trait object*/
			break;
	}
	g_base_info_unref(type);
}

void bind_unsafe_func(GICallableInfo* info)
{
	if(g_base_info_get_type(info) == GI_INFO_TYPE_FUNCTION && g_callable_info_is_method(info))
	{
		//fprintf(stderr, "method should be in an impl!\n");
		/*
		let imp=find_or_make_impl(args[0]);
		add to impl
		print impls at the end of binding
		*/
	}
	
	if(g_callable_info_can_throw_gerror(info))
	{
		//fprintf(stderr, "throws gerror!\n");
	}
	
	typedef struct
	{
		int c_idx;
		int rs_idx;
		rt* rust_type;
	}
	arg_attrs;
	
	/*get args from info*/
	gint n_args=g_callable_info_get_n_args(info);
	assert(n_args>=0);
	
	arg_attrs args[n_args];
	int i;
	for(i=0; i<n_args; i++)
	{
		args[i].c_idx=i;
		args[i].rs_idx=-1;
		args[i].rust_type=NULL;
	}
	GIArgInfo arg_info;
	for(i=0; i<n_args; i++)
	{
		/* an rs_idx of -1 indicates it's been processed and should be skipped
		in Rust signature generation (having been subsumed by another argument) */
		if(args[i].rs_idx != -1)
			continue;
		else
			args[i].rs_idx = i;
		
		g_callable_info_load_arg(info, i, &arg_info);
		GITransfer transfer=g_arg_info_get_ownership_transfer(&arg_info);
		/*
		GI_TRANSFER_NOTHING -> T/&[T]/&glib::Array<T> for T=Foo:Pod/&Foo
		GI_TRANSFER_CONTAINER -> ~[T]/glib::Array<T> for T=Foo:Pod/&Foo
		GI_TRANSFER_EVERYTHING -> ~[T]/glib::Array<T> for T=Foo/~Foo
		*/
		GIScopeType scope=g_arg_info_get_scope(&arg_info);
		if(scope != GI_SCOPE_TYPE_INVALID) /*arg is a callback*/
		{
			assert(transfer==GI_TRANSFER_NOTHING && "unknown transfer for a closure");
			gint destroy_index=g_arg_info_get_destroy(&arg_info);
			if(destroy_index > -1)
			{
				assert(destroy_index < n_args);
//				args[destroy_index].rs_idx=;
			}
			gint data_index=g_arg_info_get_closure(&arg_info);
			if(data_index > -1)
			{
				assert(data_index < n_args);
//				args[data_index].rs_idx=;
			}
		}
		else
		{
			/*for an arg: 
				if g_arg_info_is_caller_allocates (GIArgInfo *info)
					outer pointer is &mut or &
				else
					outer pointer is &mut 
			*/
			GITypeInfo arg_type;
			g_arg_info_load_type(&arg_info, &arg_type);
			
			if(g_arg_info_is_optional(&arg_info))
			{
				fprintf(stderr, "optional argument encountered! call the cops! (I've never seen one of these)\n");
				assert(g_arg_info_is_optional(&arg_info) == g_arg_info_may_be_null(&arg_info));
			}
		}
		g_callable_info_load_arg(info, i, &arg_info);
	}
	
	/*get return type from info*/
	GITypeInfo* c_ret_type=g_callable_info_get_return_type(info);
	rt* ret_type=NULL;
	if(g_callable_info_may_return_null(info))
	{
		if(ret_type->kind==rt_kind_owned_ptr || ret_type->kind==rt_kind_borrowed_ptr)
		{
			ret_type=option(ret_type);
		}
		else if(ret_type->kind==rt_kind_raw_ptr)
		{
			//raw pointers may be NULL implicitly
		}
		else
		{
			assert(0 && "attempt to optionize a non-pointer C return type");
		}
	}
	
	
	g_base_info_unref(c_ret_type);
}
