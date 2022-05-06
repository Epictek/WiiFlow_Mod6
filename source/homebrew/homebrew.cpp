#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <string.h>
#include <ogc/machine/processor.h>
#include <ogc/lwp_threads.h>
#include <vector>
#include <string>
#include "homebrew.h"
#include "defines.h"
#include "loader/sys.h"
#include "banner/AnimatedBanner.h"
#include "fileOps/fileOps.h"
#include "gecko/gecko.hpp"

static u8 *EXECUTE_ADDR = (u8*)0x92000000;

static u8 *BOOTER_ADDR = (u8*)0x93300000;
static entry BOOTER_ENTRY = (entry)BOOTER_ADDR;

static __argv *ARGS_ADDR = (__argv*)0x93300800; // more than twice as much as the appbooter, just for safety
static char *CMD_ADDR = (char*)ARGS_ADDR + sizeof(struct __argv);

extern "C" { void __exception_closeall(); }

char *homebrew_ptr = NULL;
u32 homebrew_size = 0;

u8 *appbooter_ptr = NULL;
u32 appbooter_size = 0;

using std::string;
using std::vector;

#ifdef APP_WIIFLOW_LITE
extern const u8 stub_bin[];
extern const u32 stub_bin_size;
#else
extern const u8 wfstub_bin[];
extern const u32 wfstub_bin_size;
#endif

u8 valid = 0;

static vector<string> Arguments;

static bool IsDollZ(u8 *buf)
{
	u8 cmp1[] = {0x3C};
	return memcmp(&buf[0x100], cmp1, sizeof(cmp1)) == 0;
}

static bool IsSpecialELF(u8 *buf)
{
	u32 cmp1[] = {0x7F454C46};
	u8 cmp2[] = {0x00};
	return memcmp(buf, cmp1, sizeof(cmp1)) == 0 && memcmp(&buf[0x24], cmp2, sizeof(cmp2)) == 0;
}

void AddBootArgument(const char *argv)
{
	string arg(argv);
	Arguments.push_back(arg);
}

void AddBootArgument(const char *argv, unsigned int size)
{
	string arg(argv, size);
	Arguments.push_back(arg);
}
/**
bool LoadAppBooter(const char *filepath)
{
	u8 *tmp_ptr = fsop_ReadFile(filepath, &appbooter_size);
	if(appbooter_size == 0 || tmp_ptr == NULL)
		return false;
	appbooter_ptr = (u8*)MEM2_lo_alloc(appbooter_size); // safety because upper mem2 is dol
	if(appbooter_ptr == NULL)
	{
		free(tmp_ptr);
		return false;
	}
	memcpy(appbooter_ptr, tmp_ptr, appbooter_size);
	DCFlushRange(appbooter_ptr, appbooter_size);
	free(tmp_ptr);
	return true;
}
**/
/**/
extern const u8 app_booter_bin[];
extern const u32 app_booter_bin_size;

bool LoadAppBooter(void)
{
	appbooter_ptr = DecompressCopy(app_booter_bin, app_booter_bin_size, &appbooter_size);
	if(appbooter_size == 0 || appbooter_ptr == NULL)
		return false;
	return true;
}
/**/
bool LoadHomebrew(const char *filepath)
{
	if(filepath == NULL)
		return false;

	u32 filesize = 0;
	fsop_GetFileSizeBytes(filepath, &filesize);
	if(filesize == 0)
		return false;

	if(filesize <= ((u32)BOOTER_ADDR - (u32)EXECUTE_ADDR))
	{
		fsop_ReadFileLoc(filepath, filesize, EXECUTE_ADDR);
		DCFlushRange(EXECUTE_ADDR, filesize);
		homebrew_ptr = (char*)EXECUTE_ADDR;
		homebrew_size = filesize;
		return true;
	}
	return false;
}

char *GetHomebrew(unsigned int *size)
{
	*size = homebrew_size;
	return homebrew_ptr;
}

static int SetupARGV()
{
	__argv *args = ARGS_ADDR;
	memset(args, 0, sizeof(struct __argv));
	args->argvMagic = ARGV_MAGIC;

	u32 position = 0;

	/* Count Arguments Size */
	u32 stringlength = 1;
	for(u32 i = 0; i < Arguments.size(); i++)
		stringlength += Arguments[i].size()+1;
	args->length = stringlength;

	/* Append Arguments */
	args->argc = Arguments.size();
	args->commandLine = CMD_ADDR;
	for(int i = 0; i < args->argc; i++)
	{
		memcpy(&args->commandLine[position], Arguments[i].c_str(), Arguments[i].size() + 1);
		position += Arguments[i].size() + 1;
	}
	args->commandLine[args->length - 1] = '\0';
	args->argv = &args->commandLine;
	args->endARGV = args->argv + 1;
	Arguments.clear();

	return 0;
}

void writeStub()
{
	/* Clear potential homebrew channel stub */
	memset((void*)0x80001800, 0, 0x1800);

	/* Extract our stub */
	u32 StubSize = 0;
#ifdef APP_WIIFLOW_LITE
	//! stub.bin loads WiiFlow Lite channel WFLA
	u8 *Stub = DecompressCopy(stub_bin, stub_bin_size, &StubSize); 
#else
	//! wfstub.bin loads Wiiflow channel DWFA (no more WIIH)
	u8 *Stub = DecompressCopy(wfstub_bin, wfstub_bin_size, &StubSize); 
#endif

	/* Copy our own stub into memory */
	memcpy((void*)0x80001800, Stub, StubSize);
	DCFlushRange((void*)0x80001800, StubSize);

	/* And free the memory again */
#ifdef APP_WIIFLOW_LITE
	if(Stub != stub_bin)
#else
	if(Stub != wfstub_bin)
#endif
		free(Stub);
}
/**
void BootHomebrew()
{
	if(!IsDollZ(EXECUTE_ADDR) && !IsSpecialELF(EXECUTE_ADDR))
		SetupARGV();
	else
		gprintf("Homebrew Boot Arguments disabled\n");
	memcpy(BOOTER_ADDR, appbooter_ptr, appbooter_size);
	DCFlushRange(BOOTER_ADDR, appbooter_size);
	free(appbooter_ptr);
	JumpToEntry(BOOTER_ENTRY);
}
**/
/**/
void BootHomebrew()
{
	if(!IsDollZ(EXECUTE_ADDR) && !IsSpecialELF(EXECUTE_ADDR))
		SetupARGV();
	else
		gprintf("Homebrew Boot Arguments disabled\n");

	u32 cpu_isr;
	memcpy(BOOTER_ADDR, appbooter_ptr, appbooter_size);
	DCFlushRange(BOOTER_ADDR, appbooter_size);
	ICInvalidateRange(BOOTER_ADDR, appbooter_size);
	// free(appbooter_ptr);
	SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
	_CPU_ISR_Disable( cpu_isr );
	__exception_closeall();
	BOOTER_ENTRY();
	_CPU_ISR_Restore( cpu_isr );
}
/**/
extern "C" { extern void __exception_closeall(); }
u32 AppEntrypoint = 0;
void JumpToEntry(entry EntryPoint)
{
	AppEntrypoint = (u32)EntryPoint;
	gprintf("Jumping to %08x\n", AppEntrypoint);
	u32 level = IRQ_Disable();
	__IOS_ShutdownSubsystems();
	__exception_closeall();
	__lwp_thread_closeall(); //dont like it but whatever
	asm volatile (
		"lis %r3, AppEntrypoint@h\n"
		"ori %r3, %r3, AppEntrypoint@l\n"
		"lwz %r3, 0(%r3)\n"
		"mtlr %r3\n"
		"blr\n"
	);
	IRQ_Restore(level);
}