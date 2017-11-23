// OpenAL++.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iflib/Archive.h>
#include <iflib/openal/StreamAudioSource.h>
#include <iflib/openal/AudioStream.h>
#include <windows.h>

int main()
{
	auto device = iflib::openal::GetDevice(nullptr);
	auto context = device->CreateContext(nullptr);
	context->MakeCurrent();
	auto arch = iflib::GetDefArchive();
	//auto blob = arch->Open("sample.ogg", iflib::IBlob::eStreamBlob);
	auto blob = arch->Open("sample.ogg", iflib::IBlob::eStreamBlob);
	auto audioStream = iflib::openal::IAudioStream::FromOGG(blob);
	//auto blob = arch->Open("sample.wav", iflib::IBlob::eStreamBlob);
	//auto audioStream = iflib::openal::IAudioStream::FromPCM(blob);
	iflib::openal::StreamAudioSource source;
	source.Init(audioStream);
	source.Play();
	while (true)
	{
		Sleep(250);
		if (!source.Tick())
		{
			source.Rewind();
		}
	}
    return 0;
}