import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sail.player.AudioPlayer 1.0
import harbour.sail.player.AudioPlayerState 1.0
import harbour.sail.player.PlaylistModel 1.0
import harbour.sail.player.PlayDirection 1.0
import harbour.sail.player.PlayOrder 1.0
import "../controls"
import "../controls/playlist"
import "../Util.js" as Util

Page
{
	id: page

	property bool needToSetStartupPosition: false
	property var playbackErrorPage

	function onPlaybackError(value)
	{
		if(playbackErrorPage !== null && pageStack.currentPage === playbackErrorPage)
			return;

		playbackErrorPage = pageStack.push(Qt.resolvedUrl("../pages/PlaybackErrorInfoPage.qml"), { message: value });
	}

	AudioPlayer
	{
		id: player

		onStreamStarted:
		{
			if(needToSetStartupPosition)
			{
				needToSetStartupPosition = false;

				var currentPosition = playlist.loadCurrentPosition();

				if(currentPosition !== -1)
					seek(currentPosition);
			}

			if(player.isStreamFromNextTrack() && !playlist.setTrackToPlayAndPlayingFromNextTrack())
				player.stop();
		}

		onAboutToFinish:
		{
			var path = playlist.requestNextTrack();
			var startPosition = playlist.getNextStartPosition();
			var endPosition = playlist.getNextEndPosition();

			player.setNextTrackToPlay(path, startPosition, endPosition);
		}

		onEndOfStream: player.stop()
	}

	allowedOrientations: Orientation.All

	Component.onCompleted:
	{
		playlist.currentTrackToPlayDataUpdated.connect(player.setTrackToPlay);
		player.currentDurationUpdated.connect(playerControlPanel.setTrackDuration);
		player.currentPositionUpdated.connect(playerControlPanel.setTrackPosition);
		player.stateChanged.connect(playerControlPanel.setPlayerState);
		player.stateChanged.connect(playlist.setPlayerState);
		player.playbackError.connect(page.onPlaybackError);

		playlist.loadPlaylist();

		var currentTrackIndex = playlist.loadCurrentTrackIndex();

		if(currentTrackIndex !== -1)
		{
			needToSetStartupPosition = true;
			playlist.calculateAndSetTrackToPlay(PlayDirection.ByIndex, currentTrackIndex);
			player.pause();
		}

		playOrderControl.setOrder(playlist.playOrder);
	}

	Component.onDestruction:
	{
		if(player.getCurrentState() === AudioPlayerState.Playing)
			player.pause();

		playlist.savePlaylist();
		playlist.saveCurrentPlayingState(playlist.getCurrentTrackIndex(), player.getCurrentPosition());
		player.stateChanged.disconnect(playerControlPanel.setPlayerState);
		player.currentDurationUpdated.disconnect(playerControlPanel.setTrackDuration);
		player.currentPositionUpdated.disconnect(playerControlPanel.setTrackPosition);
	}

	SilicaListView
	{
		id: listView

		anchors.fill: parent
		anchors.bottomMargin: playerControlPanel.visible ? playerControlPanel.visibleSize : 0
		header: PageHeader { title: "Default Playlist" }

		clip: true

		model: PlaylistModel { id: playlist }

		delegate: PlaylistItem
		{
			onClicked:
			{
				playerControlPanel.show();
				playlist.toggleSelectTrack(index);
			}

			onPushAndHold:
			{
				player.stop();

				if(playlist.calculateAndSetTrackToPlay(PlayDirection.ByIndex, index))
					player.play();
			}
		}

		section
		{
			property: "section"
			delegate: PlaylistSectionHeader {}
		}

		VerticalScrollDecorator {}

		PullDownMenu
		{
			MenuItem
			{
				text: qsTr("About")
				onClicked: pageStack.push(Qt.resolvedUrl("../pages/About.qml"))
			}

			MenuItem
			{
				text: qsTr("Settings")
				onClicked: pageStack.push(Qt.resolvedUrl("../pages/Settings.qml"))
			}

			MenuItem
			{
				text: qsTr("Add Folder")

				onClicked:
				{
					var dialog = pageStack.push(Qt.resolvedUrl("../pages/AddFilesDialog.qml"));

					dialog.accepted.connect(function()
					{
						playlist.addTracks(dialog.directoryPath);
					});
				}
			}
		}

		RemorsePopup
		{
			id: remorse
		}

		PushUpMenu
		{
			Column
			{
				width: parent.width

				MenuItem
				{
					text: qsTr("Clear Playlist")
					onClicked: remorse.execute(qsTr("Clearing"), function() { playlist.clearPlaylist() })
				}

				PlayOrderControl
				{
					id: playOrderControl
					onOrderChange: playlist.playOrder = order
				}
			}
		}
	}

	PlayerControlPanel
	{
		id: playerControlPanel

		onPrevious:
		{
			player.stop();
			if(playlist.calculateAndSetTrackToPlay(PlayDirection.Previous) && state != AudioPlayerState.Ready)
				player.play();
		}

		onPlayPause:
		{
			if(state == AudioPlayerState.Playing)
				player.pause();
			else
			{
				if(state == AudioPlayerState.Ready && !playlist.hasTrackToPlay())
					playlist.calculateAndSetTrackToPlay();

				if(player.hasTrackToPlay())
					player.play();
			}
		}

		onStop: player.stop()
		onSeek: player.seek(milliseconds)
		onNext:
		{
			player.stop();
			if(playlist.calculateAndSetTrackToPlay(PlayDirection.NextWithForce) && state != AudioPlayerState.Ready)
				player.play();
		}
	}
}
