#ifndef EQUALIZERCONTROLLER_HPP
#define EQUALIZERCONTROLLER_HPP

#include <QObject>
#include "Audio/AudioPlayer.hpp"
#include "SailPlayerSettings.hpp"

#include "Equalizer/EqualizerPresetsModel.hpp"

using namespace Audio;
using namespace Equalizer;

class EqualizerController : public QObject
{
	Q_OBJECT

public:
	EqualizerController(EqualizerPresetsModel& equalizerPresetsModel, SailPlayerSettings& settings, AudioPlayerBase& player);
	~EqualizerController();

	static const QList<float> BaseFrequencies;
	static const QList<float> BaseWidths;

	Q_INVOKABLE void addNewPreset(QString name);
	Q_INVOKABLE void savePresets();
	Q_INVOKABLE void deletePreset(int presetIndex);
	Q_INVOKABLE void setSelectedPreset(int presetIndex);

	void SetPresets(QList<EqualizerPreset*> presets);
private:
	EqualizerPresetsModel& _model;
	SailPlayerSettings& _settings;
	AudioPlayerBase& _player;
};

#endif // EQUALIZERCONTROLLER_HPP
