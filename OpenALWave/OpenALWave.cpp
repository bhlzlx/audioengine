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
	
	
	//auto blob = arch->Open("��Т�� - 10 MINUTES.mp3", iflib::IBlob::eStreamBlob);
#ifdef TEST_WAV
	auto blob = arch->Open("WANDS - ���礬�K��ޤǤ�.ogg", iflib::IBlob::eStreamBlob);
#endif
#ifdef TEST_OGG
	auto blob = arch->Open("��Т�� - 10 MINUTES.ogg", iflib::IBlob::eStreamBlob);
	//auto blob = arch->Open("Bandari - Annie's Wonderland.ogg", iflib::IBlob::eStreamBlob);
#endif
#ifdef TEST_MP3
	auto blob = arch->Open("��Т�� - 10 MINUTES.mp3", iflib::IBlob::eStreamBlob);
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
		audioStream->SetTitle("��Т�� - 10 MINUTES.mp3");
		ui.lbMusicTitle->setText(u8"��Т�� - 10 MINUTES");
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
	// ���û�����ɫ
	painter.setPen(QColor(0, 160, 230));
	// �������壺΢���źڡ����С50��б��
	QFont font;
	font.setFamily("Microsoft YaHei");
	font.setPointSize(50);
	font.setItalic(true);
	painter.setFont(font);
	// �����ı�
	//painter.drawText(rect(), Qt::AlignCenter, "Qt");
	// ������
	painter.setRenderHint(QPainter::Antialiasing, true);
	// ���û�����ɫ
	painter.setPen(QColor(0, 160, 230));
	// ����ֱ��
	//double hmax = abs(fftOut[0].r );
	const float baseLine = size().height();
	float xpos = 0;
	float realtimeWaveHeight;
	float lastWaveHeight;

	// ǰ�ķ�֮һԭ������
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
	// �ķ�֮һ������֮һȡ2��2������
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
	// ������ֱ֮��
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
	//���Ϊ�ļ�����֧���Ϸ�
	if (event->mimeData()->hasFormat("text/uri-list"))
		event->acceptProposedAction();
}

//���û���������ļ��󣬾ͻᴥ��dropEvent�¼�
void OpenALWave::dropEvent(QDropEvent *event)
{
	//ע�⣺��������ж��ļ����ڣ���˼���û�һ�����϶��˶���ļ����������϶�һ��Ŀ¼
	//������ȡ����Ŀ¼�����ڲ�ͬ�Ĳ���ƽ̨�£��Լ���д����ʵ�ֶ�ȡ����Ŀ¼�ļ���
	QList<QUrl> urls = event->mimeData()->urls();
	if (urls.isEmpty())
		return;

	//���ı�����׷���ļ���
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
