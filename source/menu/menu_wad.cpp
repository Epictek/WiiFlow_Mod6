/****************************************************************************
 * Copyright (C) 2013 FIX94
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "menu/menu.hpp"
#include "channel/nand.hpp"
// #include "libwbfs/wiidisc.h"

extern "C" {
#include "hw/sha1.h"
#include "hw/aes.h"
};

#define WAD_BUF 0x10000

struct _hdr {
	u32 header_len;
	u16 type;
	u16 padding;
	u32 certs_len;
	u32 crl_len;
	u32 tik_len;
	u32 tmd_len;
	u32 data_len;
	u32 footer_len;
} ATTRIBUTE_PACKED hdr;

void skip_align(FILE *f, u32 size)
{
	size_t align_missing = (ALIGN(64, size) - size);
	if(align_missing == 0)
		return;
	fseek(f, align_missing, SEEK_CUR);
}

int installWad(const char *path, bool install)
{
	u32 size = 0;
	fsop_GetFileSizeBytes(path, &size);

	FILE *wad_file = fopen(path, "rb");
	fread(&hdr, sizeof(hdr), 1, wad_file);
	skip_align(wad_file, sizeof(hdr));

	u64 totalLen = (hdr.certs_len + hdr.crl_len + hdr.tik_len + hdr.tmd_len + hdr.data_len + hdr.footer_len);
	if(!install)
	{
		mainMenu.m_thrdTotal += totalLen;
		fclose(wad_file);
		return 0;
	}
	gprintf("Installing %s\n", path);
	if(size < totalLen || hdr.tik_len == 0 || hdr.tmd_len == 0 || hdr.data_len == 0)
	{
		fclose(wad_file);
		return -3;
	}
	fseek(wad_file, ALIGN(64, hdr.certs_len), SEEK_CUR);
	fseek(wad_file, ALIGN(64, hdr.crl_len), SEEK_CUR);
	// gprintf("Reading tik\n");
	u8 *tik_buf = (u8*)MEM2_alloc(hdr.tik_len);
	fread(tik_buf, hdr.tik_len, 1, wad_file);
	skip_align(wad_file, hdr.tik_len);
	// gprintf("Decrypting key\n");
	u8 tik_key[16];
	_decrypt_title_key(tik_buf, tik_key); // was "decrypt_title_key" before modifying install game feature
	// gprintf("Reading tmd\n");
	signed_blob *tmd_buf = (signed_blob*)MEM2_alloc(hdr.tmd_len);
	fread(tmd_buf, hdr.tmd_len, 1, wad_file);
	skip_align(wad_file, hdr.tmd_len);

	const tmd *tmd_ptr = (const tmd*)SIGNATURE_PAYLOAD(tmd_buf);
	u64 tid = tmd_ptr->title_id;
	const char *EmuNAND = NULL;

	EmuNAND = NandHandle.GetPath();
	u32 uid_size = 0;
	u8 *uid_buf = fsop_ReadFile(fmt("%s/sys/uid.sys", EmuNAND), &uid_size);
	if(uid_buf == NULL)
	{
		gprintf("No uid.sys found!\n");
		MEM2_free(tmd_buf);
		MEM2_free(tik_buf);
		return -5;
	}
	else if(uid_size % 0xC != 0)
	{
		gprintf("uid.sys size is invalid!\n");
		MEM2_free(tmd_buf);
		MEM2_free(tik_buf);
		MEM2_free(uid_buf);
		return -6;
	}

	bool chan_exist = false;
	uid *uid_file = (uid*)uid_buf;
	u32 chans = uid_size/sizeof(uid);
	for(u32 i = 0; i < chans; ++i)
	{
		if(uid_file[i].TitleID == tid)
			chan_exist = true;
	}
	if(chan_exist == false)
	{
		gprintf("Updating uid.sys\n");
		u32 new_uid_size = (chans+1)*sizeof(uid);
		u8 *new_uid_buf = (u8*)MEM2_alloc(new_uid_size);
		memset(new_uid_buf, 0, new_uid_size);
		/* Copy over old uid */
		memcpy(new_uid_buf, uid_buf, chans*sizeof(uid));
		uid *new_uid_file = (uid*)new_uid_buf;
		new_uid_file[chans].TitleID = tid;
		new_uid_file[chans].uid = 0x1000+chans;
		fsop_WriteFile(fmt("%s/sys/uid.sys", EmuNAND), new_uid_file, new_uid_size);
		MEM2_free(new_uid_buf);
	}
	/* Clear old tik */
	fsop_deleteFile(fmt("%s/ticket/%08x/%08x.tik", EmuNAND, (u32)(tid>>32), (u32)tid&0xFFFFFFFF));
	/* Clear old content */
	fsop_deleteFolder(fmt("%s/title/%08x/%08x/content", EmuNAND, (u32)(tid>>32), (u32)tid&0xFFFFFFFF));

	/* (Re)create folder structure */
	fsop_MakeFolder(fmt("%s/ticket", EmuNAND));
	fsop_MakeFolder(fmt("%s/ticket/%08x", EmuNAND, (u32)(tid>>32)));

	fsop_MakeFolder(fmt("%s/title", EmuNAND));
	fsop_MakeFolder(fmt("%s/title/%08x", EmuNAND, (u32)(tid>>32)));
	fsop_MakeFolder(fmt("%s/title/%08x/%08x", EmuNAND, (u32)(tid>>32), (u32)tid&0xFFFFFFFF));
	fsop_MakeFolder(fmt("%s/title/%08x/%08x/content", EmuNAND, (u32)(tid>>32), (u32)tid&0xFFFFFFFF));
	fsop_MakeFolder(fmt("%s/title/%08x/%08x/data", EmuNAND, (u32)(tid>>32), (u32)tid&0xFFFFFFFF));

	int hash_errors = 0;
	u8 *AES_WAD_Buf = (u8*)MEM2_alloc(WAD_BUF);
	/* Decrypt and write app files */
	for(u16 cnt = 0; cnt < tmd_ptr->num_contents; cnt++)
	{
		u8 aes_iv[16];
		memset(aes_iv, 0, 16);
		const tmd_content *content = &tmd_ptr->contents[cnt];
		u16 content_index = content->index;
		memcpy(aes_iv, &content_index, 2);
		/* Longass filename */
		FILE *app_file = NULL;

		const char *app_name = fmt("%s/title/%08x/%08x/content/%08x.app", EmuNAND,
			(u32)(tid>>32), (u32)tid&0xFFFFFFFF, content->cid);
		app_file = fopen(app_name, "wb");
		gprintf("Writing Emu NAND File %s\n", app_name);
		
		u64 read = 0;

		SHA1_CTX ctx;
		SHA1Init(&ctx);
		AES_ResetEngine();
		u32 size_enc_full = ALIGN(16, content->size);
		while(read < size_enc_full)
		{
			u64 size_enc = (size_enc_full - read);
			if (size_enc > WAD_BUF)
				size_enc = WAD_BUF;

			u16 num_blocks = (size_enc / 16);
			fread(AES_WAD_Buf, size_enc, 1, wad_file);
			AES_EnableDecrypt(tik_key, aes_iv); // ISFS seems to reset it?
			memcpy(aes_iv, AES_WAD_Buf+(size_enc-16), 16); // last block for cbc
			AES_Decrypt(AES_WAD_Buf, AES_WAD_Buf, num_blocks);

			u64 size_dec = (content->size - read);
			if(size_dec > WAD_BUF)
				size_dec = WAD_BUF;
			SHA1Update(&ctx, AES_WAD_Buf, size_dec);
			fwrite(AES_WAD_Buf, size_dec, 1, app_file);
			
			/* Don't forget to increase the read size */
			read += size_enc;
			mainMenu.update_pThread(size_enc);
		}
		sha1 app_sha1;
		SHA1Final(app_sha1, &ctx);
		skip_align(wad_file, size_enc_full);

		fclose(app_file);

		if(memcmp(app_sha1, content->hash, sizeof(sha1)) != 0)
		{
			gprintf("sha1 mismatch on %08x.app!\n", content->cid);
			hash_errors++;
		}
	}
	MEM2_free(AES_WAD_Buf);

	fsop_WriteFile(fmt("%s/ticket/%08x/%08x.tik", EmuNAND, (u32)(tid>>32), (u32)tid&0xFFFFFFFF), tik_buf, hdr.tik_len);
	fsop_WriteFile(fmt("%s/title/%08x/%08x/content/title.tmd", EmuNAND, (u32)(tid>>32), (u32)tid&0xFFFFFFFF), tmd_buf, hdr.tmd_len);

	MEM2_free(tik_buf);
	MEM2_free(tmd_buf);

	return hash_errors;
}

int getTID(const char *path, u64 *tid)
{
	if(!fsop_FileExist(path))
		return -1;

	u32 size = 0;
	fsop_GetFileSizeBytes(path, &size);
	if(size < sizeof(hdr))
		return -2;

	FILE *wad_file = fopen(path, "rb");
	fread(&hdr, sizeof(hdr), 1, wad_file);

	/* Skip to tmd */
	skip_align(wad_file, sizeof(hdr));
	fseek(wad_file, ALIGN(64, hdr.certs_len), SEEK_CUR);
	fseek(wad_file, ALIGN(64, hdr.crl_len), SEEK_CUR);
	fseek(wad_file, ALIGN(64, hdr.tik_len), SEEK_CUR);

	/* Read tmd and close wad */
	signed_blob *tmd_buf = (signed_blob*)MEM2_alloc(hdr.tmd_len);
	fread(tmd_buf, hdr.tmd_len, 1, wad_file);
	fclose(wad_file);

	/* Get its tid, return and free mem */
	const tmd *tmd_ptr = (const tmd*)SIGNATURE_PAYLOAD(tmd_buf);
	(*tid) = tmd_ptr->title_id;
	MEM2_free(tmd_buf);

	return 0;
}

/*******************************************************************************************************/

void CMenu::_showWad()
{
	m_btnMgr.setText(m_configLblTitle, _t("wad1", L"Install Wii channel"));
	m_btnMgr.show(m_configLblTitle);
	m_btnMgr.setText(m_configBtnCenter, _t("cfgne6", L"Start"));
	m_btnMgr.show(m_configBtnCenter);
	m_btnMgr.show(m_configLblDialog);
	m_btnMgr.show(m_configBtnBack);
}

void CMenu::_Wad(const char *wad_path, bool folder)
{
	if(wad_path == NULL)
		return;
	
	dirent *pent = NULL;
	DIR *pdir = NULL;
	u64 tid = 0;
	u16 totalWad = 0;
	int result = -1;

	if(_FindEmuPart(EMU_NAND, false) < 0)
	{
		error(_t("cfgne8", L"No valid FAT partition found for nand emulation!"));
		return;
	}

	m_btnMgr.setText(m_configLblDialog, _t("wad99", L"Click Start to begin installation"));
	
	_showWad();

	while(!m_exit)
	{
		_mainLoopCommon();
		if(BTN_HOME_PRESSED || BTN_B_OR_1_PRESSED)
			break;
		else if(BTN_LEFT_REV_PRESSED || BTN_UP_PRESSED)
			m_btnMgr.up();
		else if(BTN_RIGHT_REV_PRESSED || BTN_DOWN_PRESSED)
			m_btnMgr.down();
		else if(BTN_A_OR_2_PRESSED)
		{
			if(m_btnMgr.selected(m_configBtnBack))
				break;
			else if(m_btnMgr.selected(m_configBtnCenter)) // start install
			{
				_hideConfig();
				_showProgress();
				
				if(!folder) // file
				{
					_start_pThread();
					m_thrdMessage = wfmt(_fmt("wad4", L"Installing %s..."), (strrchr(wad_path, '/') + 1));
					m_thrdMessageAdded = true;
					installWad(wad_path, false); // update m_thrdTotal
					if(!((getTID(wad_path, &tid) < 0) || ((u32)(tid>>32) != 0x00010001)))
						result = installWad(wad_path, true); // actually install
					_stop_pThread();
				}
				else // folder
				{
					pdir = opendir(wad_path);
					if(pdir != NULL)
					{
						_start_pThread();
						//! count files and update m_thrdTotal
						while((pent = readdir(pdir)) != NULL) 
						{
							const char *fileType = strrchr(pent->d_name, '.');
							if(pent->d_type == DT_REG && strcasecmp(fileType, ".wad") == 0)
							{
								const char *tmp_wad_path = NULL;
								tmp_wad_path = fmt("%s/%s", wad_path, pent->d_name);
								installWad(tmp_wad_path, false);
								totalWad++;
							}
						}
						//! now install files
						rewinddir(pdir);
						u16 i = 0;
						while((pent = readdir(pdir)) != NULL) 
						{
							const char *fileType = strrchr(pent->d_name, '.');
							if(pent->d_type == DT_REG && strcasecmp(fileType, ".wad") == 0)
							{
								i++;
								const char *tmp_wad_path = NULL;
								tmp_wad_path = fmt("%s/%s", wad_path, pent->d_name);
								m_thrdMessage = wfmt(_fmt("wad97", L"Installing wad %d / %d - %s..."), i, totalWad, pent->d_name);
								m_thrdMessageAdded = true;
								if(!((getTID(tmp_wad_path, &tid) < 0) || ((u32)(tid>>32) != 0x00010001)))
									result = installWad(tmp_wad_path, true);
							}
						}
						_stop_pThread();
						closedir(pdir);
					}
				}
				if(result >= 0)
					m_btnMgr.setText(m_configLblDialog, _t("wad6", L"Installation finished."));
				else if(result == -5) // no uid.sys found
					m_btnMgr.setText(m_configLblDialog, _t("wad95", L"No valid emunand found. Extract nand to emunand first."));
				else
					m_btnMgr.setText(m_configLblDialog, _t("wad5", L"Installation failed."));
				m_btnMgr.show(m_configBtnBack);
			}
		}
	}
	//! the game has been installed with no hash fail, display it in coverflow
	if(result == 0) 
	{
		m_cfg.setInt(channel_domain, "channels_type", CHANNELS_EMU);
		u32 channelTitle = (u32)tid&0xFFFFFFFF; // will be the last wad installed if folder
		char cfPos[5];
		cfPos[4] = '\0';
		memcpy(cfPos, &channelTitle, 4); // convert to ID4
		m_cfg.setString(channel_domain, "current_item", cfPos);
		m_current_view = COVERFLOW_CHANNEL;
		m_cfg.setUInt(general_domain, "sources", m_current_view);
		m_refreshGameList = true;
		m_cfg.setBool(channel_domain, "update_cache", true);
		_srcTierBack(true);
		_getCustomBgTex();
	}

	_hideConfigFull(true);
}

