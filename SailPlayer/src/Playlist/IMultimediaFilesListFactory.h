#ifndef IMULTIMEDIAFILESLISTFACTORY_H
#define IMULTIMEDIAFILESLISTFACTORY_H

#include <QList>
#include <QFileInfo>

namespace Playlist
{
	class IMultimediaFilesListFactory
	{
	public:
		virtual ~IMultimediaFilesListFactory(){}

		virtual QList<QFileInfo> Build() = 0;
	};
}

#endif // IMULTIMEDIAFILESLISTFACTORY_H