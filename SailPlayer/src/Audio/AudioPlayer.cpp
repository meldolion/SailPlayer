#include <QObject>
#include <QDebug>

#include "AudioPlayer.hpp"

const int MillisecondsConvertion = 1000000;

namespace Audio
{
	int AudioPlayer::EqualizerBandsNumber = 5;

	AudioPlayer::AudioPlayer()
	{
		_pausedByResourceBlock = false;
		_currentState = Ready;
		_fileToPlayFullFilePath = "";
		_gstTimeFormat = GST_FORMAT_TIME;
		_currentPositionTimer.setTimerType(Qt::VeryCoarseTimer);
		_currentPositionTimer.setInterval(1000);
		_nextTrackDataReceived = true;
		_isStreamFromNextTrack = false;

		gst_init(NULL, NULL);

		_audioResource = new AudioResource();

		connect(_audioResource, SIGNAL(AquireStateChanged(bool)), this, SLOT(OnAudioResourceAquireStateChanged(bool)));
		connect(&_currentPositionTimer, SIGNAL(timeout()), this, SLOT(OnCurrentPositionTimerCallback()));

		Init();
	}

	AudioPlayer::~AudioPlayer()
	{
		gst_element_set_state(_pipeline, GST_STATE_NULL);

		_audioResource->Disconnect();

		gst_object_unref(GST_OBJECT(_pipeline));

		delete _audioResource;
	}

	gboolean AudioPlayer::OnBusCall(GstBus* bus, GstMessage* msg, gpointer userData)
	{
		Q_UNUSED(bus);

		switch (GST_MESSAGE_TYPE (msg))
		{
			case GST_MESSAGE_STATE_CHANGED:
			{
				AudioPlayer* player = static_cast<AudioPlayer*>(userData);
				player->OnStateChanged();
				break;
			}

			case GST_MESSAGE_EOS:
			{
				AudioPlayer* player = static_cast<AudioPlayer*>(userData);
				player->OnEndOfStreamReached();
				break;
			}

			case GST_MESSAGE_ASYNC_DONE:
			{
				AudioPlayer* player = static_cast<AudioPlayer*>(userData);
				player->OnAsyncDone();
				break;
			}

			case GST_MESSAGE_STREAM_START:
			{
				AudioPlayer* player = static_cast<AudioPlayer*>(userData);
				player->OnStreamStart();
				break;
			}

			case GST_MESSAGE_ERROR:
			{
				gchar  *debug;
				GError *error;

				gst_message_parse_error (msg, &error, &debug);
				g_free (debug);

				qDebug() << "Error:" << error->message;
				g_error_free (error);
				break;
			}

			default:
				break;
		}

		return TRUE;
	}

	void AudioPlayer::OnPipelineAboutToFinish(GstElement* pipeline, gpointer userData)
	{
		Q_UNUSED(pipeline);

		AudioPlayer* player = static_cast<AudioPlayer*>(userData);
		player->OnAboutToFinish();
	}

	void AudioPlayer::OnAudioResourceAquireStateChanged(bool acquired)
	{
		if(acquired)
		{
			if(_currentState == Paused && _pausedByResourceBlock)
				play();
		}
		else if(_currentState == Playing)
		{
			pause();

			_pausedByResourceBlock = true;
		}
	}

	void AudioPlayer::OnEndOfStreamReached()
	{
		emit endOfStreamReached();
	}

	void AudioPlayer::OnAsyncDone()
	{
		if(_currentState == Playing || _currentState == Paused)
		{
			int pos = GetCurrentDuration();
			qDebug() << pos;
			emit currentDurationUpdated(pos);
		}

		if(_currentState == Playing && !_currentPositionsSet)
		{
			_currentPositionsSet = true;
			SeekToCurrentPosition();
		}
	}

	void AudioPlayer::OnAboutToFinish()
	{
		_nextTrackDataReceived = false;
		emit aboutToFinish();

		WaitForNextTrackData();

		_isStreamFromNextTrack = true;

		if(!_fileToPlayFullFilePath.isNull())
			g_object_set(_pipeline, "uri", QString("file://" + _fileToPlayFullFilePath).toLocal8Bit().data(), NULL);
	}

	void AudioPlayer::OnStreamStart()
	{
		emit streamStarted();
		emit currentDurationUpdated(GetCurrentDuration());
		emit currentPositionUpdated(0);

//		if(_needtToSetOnlyEndPosition)
//			UpdateEndPosition();
//		else
//			SeekToCurrentPosition();
	}

	void AudioPlayer::OnStateChanged()
	{
//		GstState state = GST_STATE(_pipeline);

//		if(state == GST_STATE_PLAYING)
//		{
//			if(_currentState == Ready)
//			{
//			}

//			_currentState = Playing;
//		}
	}

	void AudioPlayer::play()
	{\
		_currentPositionsSet = false;
		_audioResource->Connect();

		_currentState = Playing;

		emit stateChanged(Playing);

		gst_element_set_state(_pipeline, GST_STATE_PLAYING);

		_currentPositionTimer.start();
	}

	void AudioPlayer::pause()
	{
		_pausedByResourceBlock = false;
		gst_element_set_state (_pipeline, GST_STATE_PAUSED);
		_currentState = Paused;
		emit stateChanged(Paused);

		_currentPositionTimer.stop();
	}

	void AudioPlayer::stop()
	{
		gst_element_set_state (_pipeline, GST_STATE_READY);
		_currentState = Ready;

		emit stateChanged(Ready);
		emit currentPositionUpdated(0);
		emit currentDurationUpdated(0);

		_currentPositionTimer.stop();

		_audioResource->Disconnect();
	}

	void AudioPlayer::setTrackToPlay(QString fullFilePath, int startPos, int endPos)
	{
		_fileToPlayFullFilePath = fullFilePath;
		_isStreamFromNextTrack = false;
		_currentStartPosition = startPos;
		_currentEndPosition = endPos;
		g_object_set(_pipeline, "uri", QString("file://" + _fileToPlayFullFilePath).toLocal8Bit().data(), NULL);
	}

	void AudioPlayer::seek(int milliseconds)
	{
		Seek(gint64(milliseconds + _currentStartPosition) * MillisecondsConvertion);
	}

	void AudioPlayer::setNextTrackToPlay(QString fullFilePath, int startPos, int endPos)
	{
		_fileToPlayFullFilePath = fullFilePath;
//		_needtToSetOnlyEndPosition = startPos == _currentEndPosition;
		_currentStartPosition = startPos;
		_currentEndPosition = endPos;
		_nextTrackDataReceived = true;
	}

	void AudioPlayer::OnCurrentPositionTimerCallback()
	{
		int currentPosition = GetCurrentPosition() / MillisecondsConvertion;
		if(_currentEndPosition - currentPosition)

		emit currentPositionUpdated( - _currentStartPosition);
	}

	bool AudioPlayer::Init()
	{
		_pipeline = gst_parse_launch("playbin", NULL);
		_equalizer = gst_element_factory_make ("equalizer-nbands", NULL);
		_sink = gst_element_factory_make("autoaudiosink", NULL);

		if (!_pipeline || !_equalizer || !_sink)
		{
			g_warning("Failed to initialize elements!");
			return false;
		}

		// Subscribe to pipeline messages

		GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
		gst_bus_add_watch(bus, OnBusCall, this);
		gst_object_unref(bus);

		// Initializing additional plugins bin

		_additionalPlugins = gst_bin_new("additional_plugins");
		gst_bin_add_many(GST_BIN(_additionalPlugins), _equalizer, _sink, NULL);

		if (!gst_element_link_many(_equalizer, _sink, NULL))
		{
			g_warning("Failed to link elements!");
			return false;
		}

		// Connecting with additional plugins bin

		GstPad* pad = gst_element_get_static_pad(_equalizer, "sink");
		GstPad* ghost_pad = gst_ghost_pad_new("sink", pad);
		gst_pad_set_active(ghost_pad, TRUE);
		gst_element_add_pad(_additionalPlugins, ghost_pad);
		gst_object_unref(pad);

		g_object_set(GST_OBJECT(_pipeline), "audio-sink", _additionalPlugins, NULL);

		// Set-up equalizer

		g_object_set(G_OBJECT(_equalizer), "num-bands", EqualizerBandsNumber, NULL);
		SetEqualizerData();

		// Subsribe to next track gapless playing handling
		g_signal_connect(_pipeline, "about-to-finish", G_CALLBACK(OnPipelineAboutToFinish), this);

		return true;
	}

	void AudioPlayer::SetEqualizerData()
	{
		gint i;
		GObject *band;

		// TODO
		GstEqualizerBandState equalizerData[] = {
			{ 120.0,   50.0, 0.0},
			{ 500.0,   20.0, 0.0},
			{1503.0,    2.0, 0.0},
			{6000.0, 1000.0, 0.0},
			{3000.0,  120.0, 0.0}
		};

		for (i = 0; i < EqualizerBandsNumber; i++)
		{
			band = gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(_equalizer), i);
			g_object_set(G_OBJECT(band), "freq", equalizerData[i].freq, "bandwidth", equalizerData[i].width, "gain", equalizerData[i].gain, NULL);

			g_object_unref(G_OBJECT(band));
		}
	}

	gint64 AudioPlayer::GetCurrentPosition()
	{
		gint64 value;

		if(gst_element_query_position(_pipeline, _gstTimeFormat, &value))
			return value;

		return 0;
	}

	int AudioPlayer::GetCurrentDuration()
	{
//		gint64 value;

//		if(gst_element_query_duration(_pipeline, _gstTimeFormat, &value))
//			return (value / MillisecondsConvertion) - _currentStartPosition;

		return _currentEndPosition - _currentStartPosition;

//		return 0;
	}

	void AudioPlayer::Seek(gint64 nanoseconds)
	{
		gst_element_seek(_pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, nanoseconds, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
	}

	void AudioPlayer::WaitForNextTrackData()
	{
		while (!_nextTrackDataReceived)
			usleep(1000);
	}

//	void AudioPlayer::SeekToCurrentPosition()
//	{
//		gst_element_seek (_pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
//					GST_SEEK_TYPE_SET, gint64(_currentStartPosition) * MillisecondsConvertion,
//					GST_SEEK_TYPE_SET, gint64(_currentEndPosition) * MillisecondsConvertion);
//	}

//	void AudioPlayer::UpdateEndPosition()
//	{
//		gst_element_seek (_pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
//					GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE,
//					GST_SEEK_TYPE_SET, gint64(_currentEndPosition) * MillisecondsConvertion);
//	}
}
