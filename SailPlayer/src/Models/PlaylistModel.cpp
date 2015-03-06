#include "PlaylistModel.hpp"
#include <QDebug>

#include "../SailPlayerSettings.hpp"
#include "../Playlist/PlaylistHelper.hpp"

namespace Models
{
	PlaylistModel::PlaylistModel(QObject* parent)
	{
		Q_UNUSED(parent);

		_currentPlayOrder = SailPlayer::RepeatPlaylist;
		_nextTrackToPlay = NULL;
		_currentTrackToPlay = NULL;
		_currentPlayingTrack = NULL;

		_tracksLoader = new TracksLoader(_tracksFactory, _filesFactory);
	}

	PlaylistModel::~PlaylistModel()
	{		
		delete _tracksLoader;
	}

	void PlaylistModel::addTracksFromPath(QString directoryPath)
	{
		_filesFactory.SetDirectoryPath(directoryPath);
		AddTracks(_tracksLoader->Build());
	}

	void PlaylistModel::clearPlaylist()
	{
		beginRemoveRows(QModelIndex(), 0, _tracksList.count() - 1);

		ResetTracksData();
		Cleanup();

		endRemoveRows();
	}

	void PlaylistModel::loadPlaylist()
	{
		AddTracks(SailPlayerSettings::Default().GetPlaylist());
	}

	void PlaylistModel::savePlaylist()
	{
		SailPlayerSettings::Default().SetPlaylist(_tracksList);
	}

	void PlaylistModel::toggleSelectTrack(int itemIndex)
	{
		Track* track = _tracksList.at(itemIndex);

		track->SetSelected(!track->IsSelected());

		emit dataChanged(index(itemIndex, 0), index(itemIndex, 0), QVector<int>(1, IsSelectedRole));
	}

	bool PlaylistModel::calculateNextTrackToPlay(SailPlayer::PlayDirection direction, int customIndex)
	{
		int index = PlaylistHelper::CalculateNextTrackIndex(direction, _currentPlayOrder, customIndex, _tracksList, _currentTrackToPlay);

		if(index == -1)
			return false;

		_nextTrackToPlay = _tracksList.at(index);

		return true;
	}

	bool PlaylistModel::calculateAndSetTrackToPlay(SailPlayer::PlayDirection direction, int customIndex)
	{
		bool result = calculateNextTrackToPlay(direction, customIndex);

		if(result == false)
			return false;

		SetTrackToPlayFromNextTrack();

		emit currentTrackToPlayDataUpdated(_currentTrackToPlay->GetFullFilePath(), _currentTrackToPlay->GetStartPosition(), _currentTrackToPlay->GetEndPosition());

		return true;
	}

	bool PlaylistModel::setTrackToPlayAndPlayingFromNextTrack()
	{
		bool result = SetTrackToPlayFromNextTrack();

		SetPlayingTrack(true);

		return result;
	}

	int PlaylistModel::getCurrentTrackIndex()
	{
		if(_currentTrackToPlay == NULL)
			return -1;

		return _tracksList.indexOf(_currentTrackToPlay);
	}

	bool PlaylistModel::SetTrackToPlayFromNextTrack()
	{
		if(_nextTrackToPlay == NULL)
			return false;

		if(_currentTrackToPlay != NULL)
			_currentTrackToPlay->SetAsTrackToPlay(false);

		_currentTrackToPlay = _nextTrackToPlay;

		_currentTrackToPlay->SetAsTrackToPlay(true);

		emit dataChanged(index(0, 0), index(_tracksList.count() - 1, 0), QVector<int>(1, IsTrackToPlay));

		_nextTrackToPlay = NULL;

		return true;
	}

	void PlaylistModel::setPlayerState(AudioPlayer::AudioPlayerState state)
	{
		if(state == AudioPlayer::Ready || state == AudioPlayer::Paused)
			SetPlayingTrack(false);
		else
			SetPlayingTrack(true);
	}

	QString PlaylistModel::requestNextTrack()
	{
		bool result = calculateNextTrackToPlay();

		if(result == true)
			return _nextTrackToPlay->GetFullFilePath();

		return QString();
	}

	int PlaylistModel::getNextStartPosition()
	{
		if(_nextTrackToPlay != NULL)
			return _nextTrackToPlay->GetStartPosition();
		else
			return -1;
	}

	int PlaylistModel::getNextEndPosition()
	{
		if(_nextTrackToPlay != NULL)
			return _nextTrackToPlay->GetEndPosition();
		else
			return -1;
	}

	void PlaylistModel::ResetTracksData()
	{
		_nextTrackToPlay = NULL;
		_currentTrackToPlay = NULL;
		_currentPlayingTrack = NULL;
	}

	void PlaylistModel::SetPlayingTrack(bool isPlaying)
	{
		if(_currentPlayingTrack != NULL)
			_currentPlayingTrack->SetPlaying(false);

		_currentPlayingTrack = _currentTrackToPlay;

		if(_currentPlayingTrack != NULL)
			_currentPlayingTrack->SetPlaying(isPlaying);

		emit dataChanged(index(0, 0), index(_tracksList.count() - 1, 0), QVector<int>(1, IsPlayingRole));
	}
}
