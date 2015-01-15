#include "FileHelper.h"

namespace IO
{
	FileType FileHelper::GetFileType(QString fileNameSuffix)
	{
		QString suffix = fileNameSuffix.toLower();

		if(suffix == "flac")
			return Flac;

		if(suffix == "ogg")
			return Ogg;

		if(suffix == "mp3")
			return Mp3;

		if(suffix == "wav")
			return Wav;

		if(suffix == "ape")
			return Ape;

		if(suffix == "mp4")
			return Mp4;

		if(suffix == "wv")
			return Wv;

		if(suffix == "cue")
			return Cue;

		return Undefined;
	}
}