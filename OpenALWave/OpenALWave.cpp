#include "OpenALWave.h"
#include "qlayout.h"
#include "qtimer"
#include "qpainter.h"
#include "fftw3.h"
#include "lame.h"
#include "qdrag.h"
#include "qmimedata.h"
#include "qevent.h"
#include "qfile.h"
#include "qfileinfo.h"

#include "iflib/utils/JsonBlob.h"
#include "iflib/utils/JsonStructDecl.h"

iflib::JsonBlob::JsonBlobPtr jsonBlob;

fftw_plan fftwPlan;

const int FFT_SAMPLE_NUM = 2048;

//#define TEST_OGG
#define TEST_MP3
//#define TEST_WAV

OpenALWave::OpenALWave(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addWidget(ui.verticalLayoutWidget);
	ui.centralwidget->setLayout(mainLayout);

	IBlob * cfgBlob = iflib::GetDefArchive()->Open("configure.json");
	if (cfgBlob)
	{
		jsonBlob = iflib::JsonBlob::FromFile(cfgBlob);
		auto item = jsonBlob->FindItem("player/boss");
		if (item)
		{
			PlayerData pd;
			if (pd.ParseJson(item->item))
				printf("%s", pd.name.c_str());
		}
		cfgBlob->Release();
	}

	setAcceptDrops(true);
	//

	//
	device = openal::GetDevice(nullptr);
	context = device->CreateContext(nullptr);
	context->MakeCurrent();
	auto arch = iflib::GetDefArchive();
	//auto blob = arch->Open("Bandari - Annie's Wonderland.ogg", iflib::IBlob::eStreamBlob);
	//auto blob = arch->Open("sample.ogg", iflib::IBlob::eStreamBlob);
	connect(ui.sldTimeline, SIGNAL(sliderPressed()), SLOT(timelineSliderPressed()));
	connect(ui.sldTimeline, SIGNAL(sliderReleased()), SLOT(timelineSliderReleased()));
	connect(ui.sldTimeline, SIGNAL(sliderMoved(int)), SLOT(timelineSliderMoved(int)));
	
	
	//auto blob = arch->Open("李孝利 - 10 MINUTES.mp3", iflib::IBlob::eStreamBlob);
#ifdef TEST_WAV
	auto blob = arch->Open("WANDS - 世界が終るまでは.ogg", iflib::IBlob::eStreamBlob);
#endif
#ifdef TEST_OGG
	auto blob = arch->Open("李孝利 - 10 MINUTES.ogg", iflib::IBlob::eStreamBlob);
	//auto blob = arch->Open("Bandari - Annie's Wonderland.ogg", iflib::IBlob::eStreamBlob);
#endif
#ifdef TEST_MP3
	auto blob = arch->Open("李孝利 - 10 MINUTES.mp3", iflib::IBlob::eStreamBlob);
	//auto blob = arch->Open("Bandari - Annie's Wonderland.mp3", iflib::IBlob::eStreamBlob);	
#endif
	source = new StreamAudioSource();
	if (blob)
	{
#ifdef TEST_WAV
		audioStream = IAudioStream::FromPCM(blob);
#endif
#ifdef TEST_OGG
		audioStream = IAudioStream::FromOGG(blob);
#endif
#ifdef TEST_MP3
		audioStream = IAudioStream::FromMP3(blob);
#endif
		audioStream->SetTitle("李孝利 - 10 MINUTES.mp3");
		ui.lbMusicTitle->setText(u8"李孝利 - 10 MINUTES");
		source->Init(audioStream);
		source->Play();
		fftAnylizer.Init(source, FFT_SAMPLE_NUM);
	}
	else
	{
		audioStream = nullptr;
	}
	QTimer::singleShot(200, this, SLOT(tickAudio()));
	QTimer::singleShot(100, this, SLOT(tickWave()));

	timelinePressed = false;
	timelineMoved = false;
}

void OpenALWave::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	// 设置画笔颜色
	painter.setPen(QColor(0, 160, 230));
	// 设置字体：微软雅黑、点大小50、斜体
	QFont font;
	font.setFamily("Microsoft YaHei");
	font.setPointSize(50);
	font.setItalic(true);
	painter.setFont(font);
	// 绘制文本
	//painter.drawText(rect(), Qt::AlignCenter, "Qt");
	// 反走样
	painter.setRenderHint(QPainter::Antialiasing, true);
	// 设置画笔颜色
	painter.setPen(QColor(0, 160, 230));
	// 绘制直线
	//double hmax = abs(fftOut[0].r );
	const float baseLine = size().height();
	float xpos = 0;
	float realtimeWaveHeight;
	float lastWaveHeight;

	// 前四分之一原样画吧
	int availFFTLength = fftAnylizer.fftArrayf.size() / 2;
	const int rectW = 4;

	int batchN = 2;
	for (size_t i = 0; i < availFFTLength / 4; i+=batchN)
	{
		realtimeWaveHeight = 0;
		for (int j = 0; j < batchN; ++j)
		{
			realtimeWaveHeight += fftAnylizer.fftArrayArtf[i + batchN] * baseLine;
		}
		realtimeWaveHeight /= batchN;
		painter.drawRect(QRectF(xpos, baseLine - realtimeWaveHeight, rectW, realtimeWaveHeight));
		xpos += rectW;
	}
	// 四分之一到二分之一取2个2个采样
	batchN = 4;
	for (size_t i = availFFTLength / 4; i < availFFTLength / 2; i += batchN)
	{
		realtimeWaveHeight = 0;
		for (int j = 0; j < batchN; ++j)
		{
			realtimeWaveHeight += fftAnylizer.fftArrayArtf[i + batchN] * baseLine;
		}
		realtimeWaveHeight /= batchN;
		painter.drawRect(QRectF(xpos, baseLine - realtimeWaveHeight, rectW, realtimeWaveHeight));
		xpos += rectW;
	}
	// 最后二分之直接
	batchN = (availFFTLength / 2) / ((availFFTLength / 4) + (availFFTLength / 4 / 2)) * 4;
	for (size_t i = availFFTLength / 2; i < availFFTLength; i+=batchN)
	{
		realtimeWaveHeight = 0;
		for (int j = 0; j < batchN; ++j)
		{
			realtimeWaveHeight += fftAnylizer.fftArrayArtf[i + batchN] * baseLine;
		}
		realtimeWaveHeight /= batchN;
		//realtimeWaveHeight = realtimeWaveHeight > 196 ? 196 : realtimeWaveHeight;
		//painter.drawRect( QRectF(i + 128,);
		painter.drawRect(QRectF(xpos, baseLine - realtimeWaveHeight, rectW, realtimeWaveHeight));
		xpos += rectW;
	}
/*	for (size_t i = 1; i < fftAnylizer.sampleNum/2; ++i)
	{
		realtimeWaveHeight = fftAnylizer.fftArrayArtf[i] * baseLine;
		//realtimeWaveHeight = realtimeWaveHeight > 196 ? 196 : realtimeWaveHeight;
		//painter.drawRect( QRectF(i + 128,);
		painter.drawRect(QRectF(xpos, baseLine - realtimeWaveHeight, 4, realtimeWaveHeight));
		xpos += 4;
	}*/
	/*xpos = 0;
	const short * pcmData = (const short*)fftAnylizer.sampleData.data();
	for (size_t i = 0; i < fftAnylizer.sampleNum; ++i)
	{
		painter.drawLine(QPointF(xpos, baseLine * 2), QPointF(xpos, baseLine * 2 + pcmData[i*2] / 256));
		xpos++;
	}*/
}

void OpenALWave::timelineSliderPressed()
{
	timelinePressed = true;
}

Q_SLOT void OpenALWave::timelineSliderMoved( int _value)
{
	timelineMoved = true;
}

void OpenALWave::timelineSliderReleased()
{
	timelinePressed = false;
	if (timelineMoved)
	{
		int tlvalue = ui.sldTimeline->value();
		float time = float(tlvalue) / 1000.0f * audioStream->TimeTotal();
		audioStream->Seek(time);
	}
	timelineMoved = false;
}

Q_SLOT void OpenALWave::tickAudio()
{
	QTimer::singleShot(200, this, SLOT(tickAudio()));
	if (!source->Tick())
	{
		source->Rewind();
	}
	updateMusicInfo();
}

Q_SLOT void OpenALWave::tickWave()
{
	QTimer::singleShot(24, this, SLOT(tickWave()));
	if (fftAnylizer.SamplingPcmData())
	{
		fftAnylizer.RunFFT();
	}
	update();
}

void OpenALWave::updateMusicInfo()
{
	float curr = audioStream->TimeCurr();
	float total = audioStream->TimeTotal();
	if (!timelinePressed)
	{
		int progress = curr / total * 1000;
		ui.sldTimeline->setValue(progress);
	}
	int min = (int)curr / 60;
	int sec = ((int)curr) % 60;
	QString fmt("%1:%2");
	QString str = fmt.arg(min).arg(sec);
	ui.lbTime->setText(str);
}

void OpenALWave::dragEnterEvent(QDragEnterEvent *event)
{
	//如果为文件，则支持拖放
	if (event->mimeData()->hasFormat("text/uri-list"))
		event->acceptProposedAction();
}

//当用户放下这个文件后，就会触发dropEvent事件
void OpenALWave::dropEvent(QDropEvent *event)
{
	//注意：这里如果有多文件存在，意思是用户一下子拖动了多个文件，而不是拖动一个目录
	//如果想读取整个目录，则在不同的操作平台下，自己编写函数实现读取整个目录文件名
	QList<QUrl> urls = event->mimeData()->urls();
	if (urls.isEmpty())
		return;

	//往文本框中追加文件名
	foreach(QUrl url, urls) {
		QString file_name = url.toLocalFile();
		QFileInfo fileInfo(file_name);
		QString suffix = fileInfo.suffix();
		if (suffix.compare(QString("ogg"), Qt::CaseInsensitive) == 0 )
		{
			source->Pause();
			if (audioStream)
				audioStream->Release();
			IBlob * blob = iflib::GetDefArchive()->WOpen((const wchar_t*)file_name.toStdU16String().c_str(),iflib::IBlob::eStreamBlob);
			audioStream = iflib::openal::IAudioStream::FromOGG(blob);
			source->Init(audioStream);
			fftAnylizer.Init(source, FFT_SAMPLE_NUM);
			source->Play();
		}
		else if (suffix.compare(QString("mp3"), Qt::CaseInsensitive) == 0)
		{
			source->Pause();
			if (audioStream)
				audioStream->Release();
			IBlob * blob = iflib::GetDefArchive()->WOpen((const wchar_t*)file_name.toStdU16String().c_str(), iflib::IBlob::eStreamBlob);
			audioStream = iflib::openal::IAudioStream::FromMP3(blob);
			source->Init(audioStream);
			fftAnylizer.Init(source, FFT_SAMPLE_NUM);
			source->Play();
		}
		ui.lbMusicTitle->setText(fileInfo.fileName());
		audioStream->SetTitle(std::string( fileInfo.fileName().toUtf8()));
	}
}
