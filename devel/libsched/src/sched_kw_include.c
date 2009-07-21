#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <list.h>
#include <util.h>
#include <sched_util.h>
#include <sched_kw_include.h>
#include <sched_macros.h>
#include <tokenizer.h>

/**
   This file implemtents support for the INCLUDE keyword in the
   SCHEDULE files. Observe that the implementation is extremely
   minimal, all it does is:

    1. Recognize the INCLUDE keyword.
    2. Internalize the string representing the included file.

   It does NOT descent into any recursive parsing. The main reason to
   have this support is to let the untyped parser "safely" read up to
   the next "/" (that approach ill be completely fooled by path
   separators in INCLUDE statements).
*/

#define SCHED_KW_INCLUDE_ID 1085006


struct sched_kw_include_struct {
  UTIL_TYPE_ID_DECLARATION;
  char * include_file;      /* The file to include ... */
};


static sched_kw_include_type * sched_kw_include_alloc_empty() {
  sched_kw_include_type * kw = util_malloc( sizeof * kw , __func__);
  UTIL_TYPE_ID_INIT(kw , SCHED_KW_INCLUDE_ID);
  kw->include_file           = NULL;
  return kw;
}


static void sched_kw_include_set_file( sched_kw_include_type * kw , const char * file) {
  kw->include_file = util_alloc_string_copy( file );
}




sched_kw_include_type * sched_kw_include_fscanf_alloc(FILE * stream , bool * at_eof , const char * kw_name) {
  sched_kw_include_type * kw = sched_kw_include_alloc_empty();
  char * line = util_fscanf_alloc_upto(stream , "\n");
  if (line == NULL)
    util_abort("%s: something fishy ... \n",__func__);
  
  /**
     line will now typically be a string containing a (possibly) ' or
     " quoted filename, terminated with /. We remove the quotes, and
     the terminating / before we internalize the filename.
  */
 
 {
    tokenizer_type  * tokenizer = tokenizer_alloc(" \t\r\n" , "\"\'" , NULL , "--" , "\n");
    stringlist_type * tokens    = tokenize_buffer( tokenizer , line , true );
    sched_kw_include_set_file( kw , stringlist_iget( tokens , 0) );
    tokenizer_free( tokenizer );
    stringlist_free( tokens );
  }
  free(line);

  return kw;
}



void sched_kw_include_free( sched_kw_include_type * kw ) {
  util_safe_free( kw->include_file );
  free( kw );
}


void sched_kw_include_fprintf( const sched_kw_include_type * kw , FILE * stream ) {
  fprintf(stream , "INCLUDE\n");
  fprintf(stream , "   \'%s\' /\n\n" , kw->include_file);
}


void sched_kw_include_fwrite( const sched_kw_include_type * kw , FILE * stream ) {
  util_abort("%s: Not implemented ... \n",__func__);
}


sched_kw_include_type * sched_kw_include_fread_alloc( FILE * stream ) {
  util_abort("%s: Not implemented ... \n",__func__);
  return NULL;
}


sched_kw_include_type * sched_kw_include_alloc_copy( const sched_kw_include_type * kw ) {
  sched_kw_include_type * copy = sched_kw_include_alloc_empty();
  sched_kw_include_set_file( copy , kw->include_file );
  return copy;
}


KW_IMPL(include)