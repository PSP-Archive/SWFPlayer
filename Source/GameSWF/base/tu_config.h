// tu_config.h	-- by Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Generic include file for configuring tu-testbed.


#ifndef TU_CONFIG_H
#define TU_CONFIG_H


// In case you need some unusual stuff to be predefined.  For example,
// maybe you #define new/delete to be something special.
#include "compatibility_include.h"

#include "base/dlmalloc.h"

// #define these in compatibility_include.h if you want something different.
#ifndef tu_malloc
#define tu_malloc(size) dlmalloc(size)
#endif
#ifndef tu_realloc
#define tu_realloc(old_ptr, new_size, old_size) dlrealloc(old_ptr, new_size)
#endif
#ifndef tu_free
#define tu_free(old_ptr, old_size) dlfree(old_ptr)
#endif

// tu_error_exit() is for fatal errors; it should not return!
// You can #define it to something else in compatibility_include.h; e.g. you could
// throw an exception, halt, whatever.
#ifndef tu_error_exit
#include <stdlib.h>	// for exit()
#define tu_error_exit(error_code, error_message) { fprintf(stderr, error_message); exit(error_code); }
#endif


// define TU_CONFIG_LINK_TO_JPEGLIB to 0 to exclude jpeg code from
// your build.  Be aware of what you're doing -- it may break
// features!
#ifndef TU_CONFIG_LINK_TO_JPEGLIB
#define TU_CONFIG_LINK_TO_JPEGLIB 1
#endif

// define TU_CONFIG_LINK_TO_ZLIB to 0 to exclude zlib code from your
// build.  Be aware of what you're doing -- it may break features that
// you need!
#ifndef TU_CONFIG_LINK_TO_ZLIB
#define TU_CONFIG_LINK_TO_ZLIB 1
#endif

// define TU_CONFIG_LINK_TO_LIBPNG to 0 to exclude libpng code from
// your build.  Be aware of what you're doing -- it may break
// features!
#ifndef TU_CONFIG_LINK_TO_LIBPNG
#define TU_CONFIG_LINK_TO_LIBPNG 1
#endif

// define TU_CONFIG_LINK_TO_LIBXML to 1 to include XML support in
// gameswf, depending on the GNOME libxml library.
#ifndef TU_CONFIG_LINK_TO_LIBXML
#define TU_CONFIG_LINK_TO_LIBXML 1
#endif


#endif // TU_CONFIG_H
