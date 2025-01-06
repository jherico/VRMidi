#include "PlatformMidi.h"

#if 0
#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

namespace Tribe {

	class Win32Midi : public PlatformMidi {
		HMIDIIN hMidiIn{ nullptr };
		HMIDIOUT hMidiOut{ nullptr };
		FMIDIEventHandler callback;
		TMap<FString, UINT> midiInByName;
		TMap<FString, UINT> midiOutByName;
		TArray<MidiDevice> allDevices;

		TArray<MidiDevice> enumerateDevices() override {
			if (allDevices.IsEmpty()) {
				auto devices = midiInGetNumDevs();
				for (UINT i = 0; i < devices; ++i) {
					MIDIINCAPS caps;
					MMRESULT result = midiInGetDevCaps(i, &caps, sizeof(caps));
					if (result == MMSYSERR_NOERROR)
					{
						midiInByName.Add(caps.szPname, i);
						UE_LOG(LogTemp, Log, TEXT("MIDI Input Device: index=%d, name=%s"),
							i, caps.szPname);
					}
				}

				devices = midiOutGetNumDevs();
				for (UINT i = 0; i < devices; ++i) {
					MIDIOUTCAPS caps;
					MMRESULT result = midiOutGetDevCaps(i, &caps, sizeof(caps));
					if (result == MMSYSERR_NOERROR)
					{
						midiOutByName.Add(caps.szPname, i);
						UE_LOG(LogTemp, Log, TEXT("MIDI Output Device: index=%d, name=%s"),
							i, caps.szPname);
					}
				}

				TMap<FString, MidiDevice> devicesByName;
				for (const auto& entry : midiInByName) {
					const auto& Key = entry.Key;
					MidiDevice arrayValue;
					arrayValue.name = Key;
					arrayValue.in = true;
					if (midiOutByName.Contains(Key)) {
						arrayValue.out = true;
					}

					devicesByName.Add(Key, arrayValue);
				}

				for (const auto& entry : midiOutByName) {
					const auto& Key = entry.Key;
					if (devicesByName.Contains(Key)) {
						continue;
					}
					MidiDevice arrayValue;
					arrayValue.name = Key;
					arrayValue.out = true;
					devicesByName.Add(Key, arrayValue);
				}

				devicesByName.GenerateValueArray(allDevices);
			}

			return allDevices;
		}

		bool openDevice(const FString& name, const FMIDIEventHandler& handler) override {
			if (hMidiIn != nullptr || hMidiOut != nullptr) {
				// Already opened
				return false;
			}

			if (midiInByName.Contains(name)) {
				auto deviceId = midiInByName[name];
				MMRESULT result = midiInOpen(&hMidiIn, deviceId, (DWORD_PTR)MidiInProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
				if (result != MMSYSERR_NOERROR)
				{
					// platform failure?
					hMidiIn = nullptr;
					return false;
				}

				result = midiInStart(hMidiIn);
				if (result != MMSYSERR_NOERROR) {
					midiInClose(hMidiIn);
					return false;
				}

				// Success!
				callback = handler;
			}

			if (midiOutByName.Contains(name)) {
				auto deviceId = midiOutByName[name];
				MMRESULT result = midiOutOpen(&hMidiOut, deviceId, 0, 0, CALLBACK_NULL);
				if (result != MMSYSERR_NOERROR)
				{
					hMidiOut = nullptr;
				}
			}
			return true;
		}

		void closeDevice() {
			if (hMidiIn != nullptr) {
				midiInStop(hMidiIn);
				midiInClose(hMidiIn);
				hMidiIn = nullptr;
			}

			if (hMidiOut != nullptr) {
				midiOutClose(hMidiOut);
				hMidiOut = nullptr;
			}
		}

		virtual ~Win32Midi() {
			// FIXME shut down resources
			closeDevice();
		}

		void sendMidiEvent(uint8 status, int8 data1, int8 data2) {
			if (hMidiOut) {
				DWORD message = (status & 0xFF) | ((data1 & 0xFF) << 8) | ((data2 & 0xFF) << 16);
				MMRESULT result = midiOutShortMsg(hMidiOut, message);
				if (result != MMSYSERR_NOERROR) {
					UE_LOG(LogTemp, Log, TEXT("Failed to send MIDI message: index=%d"),
						result);
				}
			}
		}

		// Static callback function
		static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance,
			DWORD_PTR dwParam1, DWORD_PTR dwParam2);

		TMap<int32, HMIDIIN> InDevices;
		TMap<HMIDIIN, FMIDIEventHandler> InHandlers;
		TMap<int32, HMIDIOUT> OutDevices;
		TMap<HMIDIOUT, FMIDIEventHandler> OutHandlers;
	};


	PlatformMidi* PlatformMidi::create() {
		return new Win32Midi();
	}




	// Callback function to handle MIDI input
	void CALLBACK Win32Midi::MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance,
		DWORD_PTR dwParam1, DWORD_PTR dwParam2)
	{

		if (wMsg == MIM_DATA)
		{
			Win32Midi* lib = reinterpret_cast<Win32Midi*>(dwInstance);
			DWORD midiMessage = static_cast<DWORD>(dwParam1);
			BYTE status = static_cast<BYTE>(midiMessage & 0xFF);
			BYTE data1 = static_cast<BYTE>((midiMessage >> 8) & 0xFF);
			BYTE data2 = static_cast<BYTE>((midiMessage >> 16) & 0xFF);
			lib->callback(status, data1, data2);
			//std::cout << "MIDI Message: Status=0x" << std::hex << (int)status
			//	<< " Data1=0x" << (int)data1
			//	<< " Data2=0x" << (int)data2 << std::dec << std::endl;
		}
	}

} // namespace tribe 
#elif defined(PLATFORM_ANDROID)
#include <amidi/AMidi.h>
namespace Tribe {

	PlatformMidi* PlatformMidi::create() {
		return nullptr;
	}

}
#else
#error "Invalid platform"
#endif
#else
namespace Tribe {
	PlatformMidi* PlatformMidi::create() {
		return nullptr;
	}
}
#endif
