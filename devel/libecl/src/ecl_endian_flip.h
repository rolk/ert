#ifndef __ECL_ENDIAN_FLIP_H__
#define __ECL_ENDIAN_FLIP_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
   This header file checks is the ECLIPSE endianness and the hardware
   endianness are equal, and defines the macro ECL_ENDIAN_FLIP
   accordingly. 
   
   All the ecl_xxx functions will use the ECL_ENDIAN_FLIP macro to
   determine whether the endian flip should be performed. When opening
   a fortio instance explicitly you can use the ECL_ENDIAN_FLIP macro
   to get the endianness correct (for ECLIPSE usage that is).
*/

#define ECLIPSE_BYTE_ORDER  __BIG_ENDIAN   // Alternatively: __LITTLE_ENDIAN

#ifdef __BYTE_ORDER
#if  __BYTE_ORDER == ECLIPSE_BYTE_ORDER
#define ECL_ENDIAN_FLIP false
#else
#define ECL_ENDIAN_FLIP true
#endif               
#else
#error: The macro __BYTE_ORDER is not defined?
#endif 

#undef ECLIPSE_BYTE_ORDER


#ifdef __cplusplus
}
#endif
#endif