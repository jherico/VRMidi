#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#if PLATFORM_WINDOWS
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

#include "TribeMIDISubsystem.generated.h"

USTRUCT(BlueprintType)
struct FMidiDevice
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) bool bIn = false;
	UPROPERTY(BlueprintReadOnly) bool bOut = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMidiEvent, uint8, Status, int8, Data1, int8, Data2);

UCLASS()
class TRIBEMIDI_API UTribeMIDISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite) FOnMidiEvent OnMidiEvent;

	UFUNCTION(BlueprintCallable) TArray<FMidiDevice> EnumerateDevices();
	UFUNCTION(BlueprintCallable) bool OpenDevice(const FString& DeviceName);
	UFUNCTION(BlueprintCallable) void CloseDevice();
	void SendMidiEvent(uint8 Status, int8 Data1, int8 Data2);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
#if PLATFORM_WINDOWS
	HMIDIIN hMidiIn = nullptr;
	HMIDIOUT hMidiOut = nullptr;
	TMap<FString, UINT> MidiInByName;
	TMap<FString, UINT> MidiOutByName;
	TArray<FMidiDevice> AllDevices;
#endif

#if PLATFORM_ANDROID
	// Cached JNI references so we don’t do repeated FindClass lookups.
	jclass TribeMidiBridgeClass = nullptr;
	jmethodID MidEnumerateDevices = nullptr;
	jmethodID MidOpenDevice = nullptr;
	jmethodID MidCloseDevice = nullptr;
	jmethodID MidSendEvent = nullptr;
#endif
};
