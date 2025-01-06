#include "TribeMIDISubsystem.h"

#if PLATFORM_WINDOWS
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
static void CALLBACK MidiInCallback(HMIDIIN, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR)
{
	if (wMsg == MIM_DATA)
	{
		uint32 msg = (uint32)dwParam1;
		uint8 s = (uint8)(msg & 0xFF);
		int8 d1 = (int8)((msg >> 8) & 0xFF), d2 = (int8)((msg >> 16) & 0xFF);
		if (UTribeMIDISubsystem* Sys = (UTribeMIDISubsystem*)dwInstance)
			Sys->OnMidiEvent.Broadcast(s, d1, d2);
	}
}
#endif

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
static UTribeMIDISubsystem* GTribeMIDISubsystem = nullptr;
extern "C"
{
	JNIEXPORT void JNICALL Java_com_tribexr_tribemidi_TribeMidiBridge_nativeOnMidiEvent(JNIEnv* env, jclass, jbyte status, jbyte data1, jbyte data2)
	{
		if (GTribeMIDISubsystem)
		{
			uint8 s = (uint8)status;
			int8 d1 = (int8)data1, d2 = (int8)data2;
			GTribeMIDISubsystem->OnMidiEvent.Broadcast(s, d1, d2);
		}
	}

	JNIEXPORT void JNICALL Java_com_tribexr_tribemidi_TribeMidiBridge_nativeOnDeviceOpened(JNIEnv* env, jclass, jstring jName, jboolean success)
	{
		if (!GTribeMIDISubsystem) return;
		if (!success)
		{
			UE_LOG(LogTemp, Warning, TEXT("TribeMidiBridge: Device failed to open on Android side."));
		}
		else
		{
			const char* chars = env->GetStringUTFChars(jName, nullptr);
			FString DeviceName(UTF8_TO_TCHAR(chars));
			env->ReleaseStringUTFChars(jName, chars);
			UE_LOG(LogTemp, Log, TEXT("TribeMidiBridge: Successfully opened device '%s' on Android side."), *DeviceName);
		}
	}
}
#endif

void UTribeMIDISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if PLATFORM_WINDOWS
	// Do nothing automatically here unless you want to open a device on startup
#endif

#if PLATFORM_ANDROID
	GTribeMIDISubsystem = this;
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		TribeMidiBridgeClass = AndroidJavaEnv::FindJavaClassGlobalRef("com/tribexr/tribemidi/TribeMidiBridge");
		if (TribeMidiBridgeClass)
		{
			MidEnumerateDevices = Env->GetStaticMethodID(
				TribeMidiBridgeClass, "enumerateDevices", "()[Ljava/lang/String;");
			MidOpenDevice = Env->GetStaticMethodID(
				TribeMidiBridgeClass, "openDevice", "(Ljava/lang/String;)Z");
			MidCloseDevice = Env->GetStaticMethodID(
				TribeMidiBridgeClass, "closeDevice", "()V");
			MidSendEvent = Env->GetStaticMethodID(
				TribeMidiBridgeClass, "sendMidiEvent", "(BBB)V");

			// Call init(...)
			jobject Activity = FAndroidApplication::GetGameActivityThis();
			jmethodID Init = Env->GetStaticMethodID(TribeMidiBridgeClass, "init", "(Landroid/app/Activity;)V");
			if (Init) Env->CallStaticVoidMethod(TribeMidiBridgeClass, Init, Activity);
		}
	}
#endif
}

void UTribeMIDISubsystem::Deinitialize()
{
	CloseDevice();
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		if (TribeMidiBridgeClass) Env->DeleteGlobalRef(TribeMidiBridgeClass);
		TribeMidiBridgeClass = nullptr;
		MidEnumerateDevices = nullptr;
		MidOpenDevice = nullptr;
		MidCloseDevice = nullptr;
		MidSendEvent = nullptr;
	}
	GTribeMIDISubsystem = nullptr;
#endif
	Super::Deinitialize();
}

TArray<FMidiDevice> UTribeMIDISubsystem::EnumerateDevices()
{
#if PLATFORM_WINDOWS
	static bool bScanned = false;
	if (!bScanned)
	{
		bScanned = true;
		UINT inCount = midiInGetNumDevs();
		for (UINT i = 0; i < inCount; i++)
		{
			MIDIINCAPS caps;
			if (midiInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
				MidiInByName.Add(caps.szPname, i);
		}
		UINT outCount = midiOutGetNumDevs();
		for (UINT i = 0; i < outCount; i++)
		{
			MIDIOUTCAPS caps;
			if (midiOutGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
				MidiOutByName.Add(caps.szPname, i);
		}
		TMap<FString, FMidiDevice> devicesByName;
		for (const auto& InPair : MidiInByName)
		{
			FMidiDevice dev; dev.Name = InPair.Key; dev.bIn = true;
			if (MidiOutByName.Contains(dev.Name)) dev.bOut = true;
			devicesByName.Add(dev.Name, dev);
		}
		for (const auto& OutPair : MidiOutByName)
		{
			if (!devicesByName.Contains(OutPair.Key))
			{
				FMidiDevice dev; dev.Name = OutPair.Key; dev.bOut = true;
				devicesByName.Add(dev.Name, dev);
			}
		}
		devicesByName.GenerateValueArray(AllDevices);
	}
	return AllDevices;
#else
	TArray<FMidiDevice> Result;
	if (!TribeMidiBridgeClass || !MidEnumerateDevices) return Result;
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		jobject jobjArr = Env->CallStaticObjectMethod(TribeMidiBridgeClass, MidEnumerateDevices);
		if (jobjArr)
		{
			jobjectArray strArray = (jobjectArray)jobjArr;
			jsize len = Env->GetArrayLength(strArray);
			for (jsize i = 0; i < len; i++)
			{
				jstring jName = (jstring)Env->GetObjectArrayElement(strArray, i);
				const char* utf = Env->GetStringUTFChars(jName, nullptr);
				FMidiDevice dev; dev.Name = FString(UTF8_TO_TCHAR(utf));
				dev.bIn = true; dev.bOut = true; // Android doesn't easily differentiate. 
				Result.Add(dev);
				Env->ReleaseStringUTFChars(jName, utf);
				Env->DeleteLocalRef(jName);
			}
			Env->DeleteLocalRef(strArray);
		}
	}
	return Result;
#endif
}

bool UTribeMIDISubsystem::OpenDevice(const FString& DeviceName)
{
#if PLATFORM_WINDOWS
	if (hMidiIn || hMidiOut)return false;
	TArray<FMidiDevice> Devs = EnumerateDevices();
	if (MidiInByName.Contains(DeviceName))
	{
		UINT devID = MidiInByName[DeviceName];
		if (midiInOpen(&hMidiIn, devID, (DWORD_PTR)MidiInCallback, (DWORD_PTR)this, CALLBACK_FUNCTION) == MMSYSERR_NOERROR)
			midiInStart(hMidiIn);
		else
			hMidiIn = nullptr;
	}
	if (MidiOutByName.Contains(DeviceName))
	{
		UINT devID = MidiOutByName[DeviceName];
		midiOutOpen(&hMidiOut, devID, 0, 0, CALLBACK_NULL);
	}
	return(hMidiIn || hMidiOut);
#else
	if (!TribeMidiBridgeClass || !MidOpenDevice)return false;
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		jstring jStr = Env->NewStringUTF(TCHAR_TO_UTF8(*DeviceName));
		jboolean ok = (jboolean)Env->CallStaticBooleanMethod(TribeMidiBridgeClass, MidOpenDevice, jStr);
		Env->DeleteLocalRef(jStr);
		return (ok == JNI_TRUE);
	}
	return false;
#endif
}

void UTribeMIDISubsystem::CloseDevice()
{
#if PLATFORM_WINDOWS
	if (hMidiIn) { midiInStop(hMidiIn); midiInClose(hMidiIn); hMidiIn = nullptr; }
	if (hMidiOut) { midiOutClose(hMidiOut); hMidiOut = nullptr; }
#else
	if (!TribeMidiBridgeClass || !MidCloseDevice)return;
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		Env->CallStaticVoidMethod(TribeMidiBridgeClass, MidCloseDevice);
#endif
}

void UTribeMIDISubsystem::SendMidiEvent(uint8 Status, int8 Data1, int8 Data2)
{
#if PLATFORM_WINDOWS
	if (hMidiOut)
	{
		DWORD msg = (Status & 0xFF) | ((Data1 & 0xFF) << 8) | ((Data2 & 0xFF) << 16);
		midiOutShortMsg(hMidiOut, msg);
	}
#else
	if (!TribeMidiBridgeClass || !MidSendEvent)return;
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		Env->CallStaticVoidMethod(TribeMidiBridgeClass, MidSendEvent, (jbyte)Status, (jbyte)Data1, (jbyte)Data2);
#endif
}
