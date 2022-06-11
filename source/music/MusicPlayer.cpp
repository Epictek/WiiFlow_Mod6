/****************************************************************************
 * Copyright (C) 2012 FIX94
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
#include <cstdio>
#include <algorithm>
#include <fstream>

#include "MusicPlayer.hpp"
#include "SoundHandler.hpp"
#include "devicemounter/DeviceHandler.hpp"
#include "list/ListGenerator.hpp"
#include "gui/text.hpp"
#include "gecko/gecko.hpp"

#define	MUSIC_DEPTH 10
Musicplayer MusicPlayer;

static vector<string> FileNames;
static vector<string>::const_iterator CurrentFileName;

void Musicplayer::Cleanup()
{
	Stop();
	DisplayTime = 0;
	CurrentPosition = 0;
	MusicChanged = false;
	MusicStopped = true;
	FileNames.clear();
}

static inline void FileNameAdder(char *Path)
{
	/* No need for more checks */
	FileNames.push_back(Path);
}

void Musicplayer::Init(Config &cfg, const string& musicDir, const string& themeMusicDir) 
{
	if(usingPlaylist)
	{
		InitPlaylist(cfg, curPlaylist, pl_device);
		return;
	}
	Cleanup();
	// FadeRate = cfg.getInt("GENERAL", "music_fade_rate", 8);
	FadeRate = 8;
	Volume = cfg.getInt("GENERAL", "sound_volume_music", 255);

	MusicFile.SetVoice(0);
	SetVolume(0);

	vector<string> Types = stringToVector(".mp3|.ogg", '|');
	GetFiles(musicDir.c_str(), Types, FileNameAdder, false, MUSIC_DEPTH);
	GetFiles(themeMusicDir.c_str(), Types, FileNameAdder, false, MUSIC_DEPTH);
	if(cfg.getBool("GENERAL", "randomize_music", true) && FileNames.size() > 0)
	{
		srand(unsigned(time(NULL)));
		random_shuffle(FileNames.begin(), FileNames.end());
	}
	usingPlaylist = false;
	OneSong = (FileNames.size() == 1);
	CurrentFileName = FileNames.begin();
}

int Musicplayer::InitPlaylist(Config &cfg, const char *playlist, u8 device)
{
	std::ifstream filestr;
	filestr.open(playlist);
	
	if(filestr.fail())
		return 0;
		
	filestr.seekg(0, std::ios_base::end);
	int size = filestr.tellg();
	if(size <= 0)
		return -1;
	filestr.seekg(0, std::ios_base::beg);
	
	string song;
	FileNames.clear();
	while(!filestr.eof()) 
	{
		getline(filestr, song, '\r');
		if(song.find(".mp3") == string::npos && song.find(".ogg") == string::npos)// if not song path continue to next line
			continue;
		while(song.find("\\") != string::npos)// convert all '\' to '/'
			song.replace(song.find("\\"), 1, "/");
		string::size_type p = song.find("/");// remove drive letter and anything else before first /
		song.erase(0, p);
		const char *songPath = fmt("%s:%s", DeviceName[device], song.c_str());
		FileNames.push_back(songPath);
	}
	filestr.close();
	
	curPlaylist = playlist;
	usingPlaylist = true;
	pl_device = device;
	if(cfg.getBool("GENERAL", "randomize_music", true) && FileNames.size() > 0)
	{
		srand(unsigned(time(NULL)));
		random_shuffle(FileNames.begin(), FileNames.end());
	}
	OneSong = (FileNames.size() == 1);
	CurrentFileName = FileNames.begin();
	LoadCurrentFile();
	return 1;
}

void Musicplayer::SetFadeRate(u8 faderate)
{
	FadeRate = faderate;
}

void Musicplayer::SetMaxVolume(u8 volume)
{
	Volume = volume;
	SetVolume(volume);
}

void Musicplayer::SetVolume(u8 volume)
{
	CurrentVolume = volume;
	MusicFile.SetVolume(CurrentVolume);
}

void Musicplayer::SetResampleSetting(bool resample)
{
	ResampleSetting = resample;
}

void Musicplayer::Previous()
{
	if(FileNames.empty() || PosFromPrevFile())
		return;

	if(CurrentFileName == FileNames.begin())
		CurrentFileName = FileNames.end();
	--CurrentFileName;
	LoadCurrentFile();
}

void Musicplayer::Next()
{
	if(FileNames.empty() || PosFromPrevFile())
		return;

	++CurrentFileName;
	if(CurrentFileName == FileNames.end())
		CurrentFileName = FileNames.begin();
	LoadCurrentFile();
}

bool Musicplayer::PosFromPrevFile()
{
	if(!CurrentPosition)
		return false;

	MusicFile.Load((*CurrentFileName).c_str());
	SoundHandle.Decoder(MusicFile.GetVoice())->Seek(CurrentPosition);
	SetVolume(CurrentVolume);
	MusicFile.Play();
	CurrentPosition = 0;
	MusicStopped = false;
	MusicChanged = false;
	return true;
}

void Musicplayer::Stop()
{
	if(!MusicFile.IsPlaying())
		return;
	MusicFile.Pause();// pause for now
	CurrentPosition = SoundHandle.Decoder(MusicFile.GetVoice())->Tell();
	MusicFile.FreeMemory();// does MusicFile.Stop() then frees mem
	MusicStopped = true;
}

void Musicplayer::Pause()
{
	if(!MusicFile.IsPlaying())
		return;
	MusicFile.Pause();
}

void Musicplayer::Resume()
{
	MusicFile.Resume();
}

void Musicplayer::Tick(bool attenuate)// attenuate means fade to zero volume
{
	if(FileNames.empty())
		return;
	if(!attenuate && CurrentVolume < Volume)
		SetVolume(CurrentVolume + FadeRate > Volume ? Volume : CurrentVolume + FadeRate);
	else if(attenuate && CurrentVolume > 0)
		SetVolume(CurrentVolume - FadeRate < 0 ? 0 : CurrentVolume - FadeRate);
	if(!attenuate && !MusicFile.IsPlaying())
		Next();
}

void Musicplayer::LoadCurrentFile()
{
	LoadFile(CurrentFileName->c_str());
}

void Musicplayer::LoadFile(const char *name, bool display_change)
{
	if(FileNames.empty())
	{
		FileNames.push_back(PLUGIN_DOMAIN);
		CurrentFileName = FileNames.begin();
	}
	else if(FileNames.size() == 1 && strcmp(name, PLUGIN_DOMAIN) == 0)
	{
		MusicFile.FreeMemory();// GuiSound is MusicFile in gui_sound.cpp
		MusicStopped = true;
		return;
	}
	MusicFile.Load(name);
	SetVolume(CurrentVolume);
	MusicFile.Play();
	CurrentPosition = 0;
	MusicStopped = false;
	MusicChanged = display_change;
}

void Musicplayer::ReLoadCurrentFile()
{
	char curFile[1024];
	strcpy(curFile, MusicFile.GetName());
	if(strlen(curFile) == 0)
		return;
	Stop();
	LoadFile(curFile, false);
}
	
/* For our GUI */
wstringEx Musicplayer::GetFileName()
{
	wstringEx CurrentFile;
	string CurrentFileStr((*CurrentFileName).begin()+(*CurrentFileName).find_last_of('/')+1, 
			(*CurrentFileName).begin()+(*CurrentFileName).find_last_of('.'));
	CurrentFile.fromUTF8(CurrentFileStr.c_str());
	return CurrentFile;
}

bool Musicplayer::SongChanged()
{
	if(!MusicChanged)
		return false;
	MusicChanged = false;
	return true;
}
