#ifndef __NK_H__
#define __NK_H__

enum ExtNANDCfg
{
	NCON_EXT_DI_PATH		= (1<<0),
	NCON_EXT_NAND_PATH		= (1<<1),
	NCON_HIDE_EXT_PATH		= (1<<2),
	NCON_EXT_RETURN_TO		= (1<<3),
};

typedef struct _memcfg
{
	u32 magic;
	u64 titleid;
	u32 config;
	u64 returnto;
	u32 paddinga;
	u32 paddingb;
	char dipath[256];
	char nandpath[256];
} memcfg;

/** code from Cyan - USB Loader GX **/
#define NANDCONFIG_MAXNAND 8
#define NANDCONFIG_HEADER_SIZE 0x10
#define NANDCONFIG_NANDINFO_SIZE 0x100

typedef struct
{
	char	Path[128];
	char	Name[64];
	char	DiPath[64];
} NandInfo;

typedef struct _NandConfig
{
	u32			NandCnt;
	u32			NandSel;
	u32			Padding1;
	u32			Padding2;
	NandInfo	Nands[];
} NandConfig;
/**/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

s32 Launch_nk(u64 TitleID, const char *nandpath, u64 ReturnTo);
bool Load_Neek2o_Kernel(int part);
void check_neek2o(void);
bool neek2o(void);

/** code from Cyan - USB Loader GX **/
int neek2oSetNAND(const char* nandpath, int part);
int neekPathFormat(char* nandpath_out, const char* nandpath_in, u32 len);
/**/

/*void NKKeyCreate(u8 *TIK);
void NKAESDecryptBlock(u8 *in, u8 *out);*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //__NK_H__
