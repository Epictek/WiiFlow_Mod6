
#ifndef __CHANHANDLE_HPP_
#define __CHANHANDLE_HPP_

typedef struct _dolheader
{
	u32 section_pos[18];
	u32 section_start[18];
	u32 section_size[18];
	u32 bss_start;
	u32 bss_size;
	u32 entry_point;
	u32 padding[7];
} ATTRIBUTE_PACKED dolheader;

void PatchChannel(u8 vidMode, GXRModeObj *vmode, bool vipatch, bool countryString, u8 patchVidModes, int aspectRatio, u32 returnTo, u8 private_server, const char *server_addr, u8 deflicker, bool patchFix480p, u8 bootType);
u32 LoadChannel(u64 title, bool dol, u32 *IOS);

extern bool isForwarder;

#endif /* __CHANHANDLE_HPP_ */
