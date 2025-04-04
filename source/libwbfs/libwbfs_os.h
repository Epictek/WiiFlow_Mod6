
#ifndef LIBWBFS_GLUE_H
#define LIBWBFS_GLUE_H

#include <gctypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gccore.h>
#include <malloc.h>

#include "gecko/gecko.hpp"
#include "loader/disc.h"
#include "loader/utils.h"
#include "memory/mem2.hpp"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int wd_last_error;
static inline void wbfs_fatal(const char *x)
{
	gprintf(x);
	wd_last_error = 1;
}

static inline void wbfs_error(const char *x)
{
	gprintf(x);
	wd_last_error = 2;
}

static inline void *wbfs_malloc(size_t size)
{
	void *p = MEM2_lo_alloc(size);
	if(p) memset(p, 0, size);
	return p;
}

static inline void wbfs_free(void *ptr)
{
	MEM2_lo_free(ptr);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define wbfs_be16(x)		(*((u16*)(x)))
#define wbfs_be32(x)		(*((u32*)(x)))
#define wbfs_ntohl(x)		(x)
#define wbfs_htonl(x)		(x)
#define wbfs_ntohs(x)		(x)
#define wbfs_htons(x)		(x)

#define wbfs_memcmp(x,y,z)	memcmp(x,y,z)
#define wbfs_memcpy(x,y,z)	memcpy(x,y,z)
#define wbfs_memset(x,y,z)	memset(x,y,z)

#define wbfs_fatal(x)	   do { printf("\nwbfs panic: %s\n\n",x); return; } while(0)
#define wbfs_error(x)	   do { printf("\nwbfs error: %s\n\n",x); } while(0)
#define wbfs_malloc(x)	  MEM2_alloc(x)
#define wbfs_free(x)		free(x)
#define wbfs_ioalloc(x)	 MEM2_alloc(((x) + 31) & ~31)
#define wbfs_iofree(x)	  free(x)

#endif
