#include "AudioPlayer.h"
#include <QObject>
#include <QDebug>

namespace Audio
{
	int AudioPlayer::EqualizerBandsNumber = 5;

	AudioPlayer::AudioPlayer()
	{
		_pausedByResourceBlock = false;
		_currentState = Ready;
		_needToAcquire = true;

		gst_init(NULL, NULL);

		_audioResource = new AudioResource();
		_audioResource->Init();

		QObject::connect(_audioResource, SIGNAL(OnAquireStateChanged(bool)), this, SLOT(OnAudioResourceAquireStateChanged(bool)));
	}

	AudioPlayer::~AudioPlayer()
	{
		gst_element_set_state(_pipeline, GST_STATE_NULL);

		_audioResource->Free();

		gst_object_unref(GST_OBJECT(_pipeline));

		delete _audioResource;
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

	void AudioPlayer::OnDecoderPadAdded(GstElement* element, GstPad* pad, gpointer data)
	{
		Q_UNUSED(element);

		GstElement* sink = (GstElement*) data;

		GstPad* sinkpad = gst_element_get_static_pad(sink, "sink");
		gst_pad_link(pad, sinkpad);
		gst_object_unref(sinkpad);
	}

	void AudioPlayer::OnDeinterleavePadAdded(GstElement* element, GstPad* pad, gpointer data)
	{
		QList<GstElement>* audioSliders = (QList<GstElement>*)data;
		GstElement* volume = gst_element_factory_make("volume", NULL);
		GstPad* newPad = gst_element_get_static_pad(element, "src_%u");

		if(gst_pad_is_linked(newPad))
		{
			g_warning("We are already linked. Ignoring.");
			return;
		}

		gst_pad_link(pad, newPad);
		gst_object_unref(newPad);
		audioSliders->append(&volume);
	}

	void AudioPlayer::OnInterleavePadAdded(GstElement* element, GstPad* pad, gpointer data)
	{
		static int currentSlider = 0;
		QList<GstElement>* audioSliders = (QList<GstElement>*)data;
		GstPad* newPad = gst_element_get_request_pad (element, "sink_%d");

		if(gst_pad_is_linked(newPad))
		{
			g_warning("We are already linked. Ignoring.");
			return;
		}

		gst_pad_link(audioSliders[currentSlider], newPad);
		gst_object_unref(newPad);
		currentSlider++;
	}

	gboolean AudioPlayer::OnBusCall(GstBus* bus, GstMessage* msg, gpointer user_data)
	{
		Q_UNUSED(bus);
		Q_UNUSED(user_data);

		switch (GST_MESSAGE_TYPE (msg))
		{
			case GST_MESSAGE_EOS:
			  break;

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

	bool AudioPlayer::Init()
	{
		_pipeline = gst_pipeline_new("audio-player");
		_source = gst_element_factory_make("filesrc", NULL);
		_decoder = gst_element_factory_make("decodebin", NULL);
		_equalizer = gst_element_factory_make("equalizer-nbands", NULL);
		_deinterleave = gst_element_factory_make("deinterleave", NULL);
		_volumeL = gst_element_factory_make("volume", "volume-l");
		_volumeR = gst_element_factory_make("volume", "volume-r");
		_interleave = gst_element_factory_make("interleave", NULL);
		_sink = gst_element_factory_make("autoaudiosink", NULL);

		if (!_pipeline || !_source || !_decoder || !_equalizer || !_deinterleave || !_volumeL || !_volumeR || !_interleave || !_sink)
		{
			g_warning("Failed to initialize elements!");
			return false;
		}

		// Bus messages

		GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
		gst_bus_add_watch(bus, OnBusCall, NULL);
		gst_object_unref(bus);

		// Registering plugings
		gst_bin_add_many(GST_BIN(_pipeline), _source, _decoder, _equalizer, _deinterleave, _volumeL, _volumeR, _interleave, _sink, NULL);

		// Linking static pads

		if (!gst_element_link(_source, _decoder) || !gst_element_link_many(_equalizer, _deinterleave, NULL) || !gst_element_link_many(_interleave, _sink, NULL))
		{
			g_warning("Failed to link elements!");
			return false;
		}

		// Linking dynamic pads

		g_signal_connect(_decoder, "pad-added", G_CALLBACK(OnDeinterleavePadAdded), _equalizer);
		g_signal_connect(_deinterleave, "pad-added", G_CALLBACK(OnDecoderPadAdded), &_audioSliders);
		g_signal_connect(_interleave, "pad-added", G_CALLBACK(OnDecoderPadAdded), &_audioSliders);

		// Initializing plugins

		g_object_set(G_OBJECT(_source), "location", "/home/nemo/Music/Passage.ogg", NULL);
		g_object_set(G_OBJECT (_equalizer), "num-bands", EqualizerBandsNumber, NULL);

		SetEqualizerData();
		SetPan();

		return true;
	}

	void AudioPlayer::play()
	{\
		if(_needToAcquire)
			_audioResource->Acquire();

		_currentState = Playing;
		gst_element_set_state(_pipeline, GST_STATE_PLAYING);
	}

	void AudioPlayer::stop()
	{
		gst_element_set_state (_pipeline, GST_STATE_READY);
		_currentState = Ready;
		_needToAcquire = true;
		_audioResource->Release();
	}

	void AudioPlayer::pause()
	{
		_pausedByResourceBlock = false;
		gst_element_set_state (_pipeline, GST_STATE_PAUSED);
		_currentState = Paused;
		_needToAcquire = false;
	}

	void AudioPlayer::SetEqualizerData()
	{
		gint i;
		GstObject *band;
		GstEqualizerBandState equalizerData[] = {
			{ 120.0,	5.0,	0 },
			{ 500.0,	2.0,	0 },
			{ 1503.0,	2.0,	0 },
			{ 6000.0,	2.0,	0 },
			{ 3000.0,	120.0,	0 }
		};

		for (i = 0; i < EqualizerBandsNumber; i++)
		{
			band = gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(_equalizer), i);
			g_object_set(G_OBJECT(band), "freq", equalizerData[i].freq, "bandwidth", equalizerData[i].width, "gain", equalizerData[i].gain, NULL);

			g_object_unref(G_OBJECT(band));
		}
	}

	void AudioPlayer::SetPan()
	{
//		g_object_new (GST_TYPE_OSSMIXER_TRACK, NULL);
//		gst_mixer_set_volume(_volume, );
	}
}
