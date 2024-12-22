#pragma once
#include "CoreMinimal.h"


namespace Tribe {

	struct MidiDevice {
		FString name;
		bool in{ false };
		bool out{ false };
	};

	using FMIDIEventHandler = TFunction<void(uint8, uint8, uint8)>;

	class PlatformMidi {
	public:
		virtual ~PlatformMidi() {};
		virtual TArray<MidiDevice> enumerateDevices() = 0;
		virtual bool openDevice(const FString& name, const FMIDIEventHandler& handler) = 0;
		virtual void closeDevice() = 0;
		virtual void sendMidiEvent(uint8 status, int8 data1, int8 data2) = 0;


		static PlatformMidi* create();
	};
}