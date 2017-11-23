#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_OpenALWave.h"
#include <iflib/Archive.h>
#include <iflib/openal/device.h>
#include <iflib/openal/StreamAudioSource.h>
#include <iflib/openal/FFTIndicator.h>
#include <memory>

struct cplx
{
	double r;
	double i;
};

using namespace iflib::openal;
using namespace iflib;

class OpenALWave : public QMainWindow
{
	Q_OBJECT
public:
	OpenALWave(QWidget *parent = Q_NULLPTR);
private:

	bool timelinePressed;
	bool timelineMoved;

	Q_SLOT void tickAudio();
	Q_SLOT void tickWave();
	void updateMusicInfo();
	void paintEvent(QPaintEvent *event);

	Q_SLOT void timelineSliderPressed();
	Q_SLOT void timelineSliderMoved(int _value);
	Q_SLOT void timelineSliderReleased();
private:
	Ui_MainWindow ui;
	std::shared_ptr<iflib::openal::Device> device;
	std::shared_ptr<iflib::openal::Context> context;
	iflib::openal::IAudioStream * audioStream;
	iflib::openal::StreamAudioSource* source;
	iflib::openal::FFTAnylizer fftAnylizer;

/*	short waveSamples[SAMPLENUM * 2];
	cplx	fftIn[SAMPLENUM];
	cplx	fftOut[SAMPLENUM];*/

	void dragEnterEvent(QDragEnterEvent *event);
	//当用户放下这个文件后，就会触发dropEvent事件
	void dropEvent(QDropEvent *event);

};
