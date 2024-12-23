// Fill out your copyright notice in the Description page of Project Settings.


#include "MyMIDIManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/PlatformProcess.h"

static const auto POWDER = FLinearColor(0.74f, 1.0f, 0.74f);
static const auto CYAN = FLinearColor(0.0f, 1.0f, 1.0f);
static const auto YELLOW = FLinearColor(1.0f, 1.0f, 0.0f);
static const auto MAGENTA = FLinearColor(1.0f, 0.0f, 0.0f);

static const auto L_CONTROL_STATUS = 0xb0;
static const auto R_CONTROL_STATUS = 0xb1;
static const auto L_NOTE_STATUS = 0x97;
static const auto R_NOTE_STATUS = 0x99;

static const auto PLATTER_SPIN_ID = 0x22;
static const auto SIDE_SPIN_ID = 0x21;

static const float SCALE = 0.8f;
static const float FINE_SCALE = SCALE * 0.2;

static const FVector2D SIZE{ 1024, 768 };

// Sets default values
AMyMIDIManager::AMyMIDIManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create and initialize the Static Mesh Component
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	RootComponent = StaticMeshComponent;

	// Initialize pointers
	DynamicMaterialInstance = nullptr;
	CanvasRenderTarget = nullptr;
}

inline constexpr int32 makeMapKey(int32 channel, int32 controlId) {
	return (channel << 8) | controlId;
}

void AMyMIDIManager::OnMidiEvent(uint8 status, int8 data1, int8 data2) {
	UE_LOG(LogTemp, Log, TEXT("MIDI Event: status=0x%02x, data1=0x%02x, data2=0x%02x"),
		(int32)status, data1, data2);

	const auto mapKey = makeMapKey(status, data1);
	auto entry = MIDIActionMap.Find(mapKey);

	if (nullptr != entry) {
		(*entry)(data2);
	}

}

// Called when the game starts or when spawned
void AMyMIDIManager::BeginPlay()
{
	Super::BeginPlay();
	CurrentPosition = { SIZE.X / 2, SIZE.Y / 2 };
	MIDIActionMap.Add(makeMapKey(L_CONTROL_STATUS, PLATTER_SPIN_ID), [this](int32 Velocity) { CoarseX(Velocity); });
	MIDIActionMap.Add(makeMapKey(L_CONTROL_STATUS, SIDE_SPIN_ID), [this](int32 Velocity) {FineX(Velocity); });
	MIDIActionMap.Add(makeMapKey(R_CONTROL_STATUS, PLATTER_SPIN_ID), [this](int32 Velocity) {CoarseY(Velocity); });
	MIDIActionMap.Add(makeMapKey(R_CONTROL_STATUS, SIDE_SPIN_ID), [this](int32 Velocity) {FineY(Velocity); });
	MIDIActionMap.Add(makeMapKey(L_NOTE_STATUS, 0), [this](int32 Velocity) { SetColor(0, FLinearColor::White); });
	MIDIActionMap.Add(makeMapKey(L_NOTE_STATUS, 1), [this](int32 Velocity) { SetColor(0, FLinearColor::Red); });
	MIDIActionMap.Add(makeMapKey(L_NOTE_STATUS, 2), [this](int32 Velocity) { SetColor(0, FLinearColor::Green); });
	MIDIActionMap.Add(makeMapKey(L_NOTE_STATUS, 3), [this](int32 Velocity) { SetColor(0, FLinearColor::Blue); });
	MIDIActionMap.Add(makeMapKey(L_NOTE_STATUS, 4), [this](int32 Velocity) { SetColor(0, CYAN); });
	MIDIActionMap.Add(makeMapKey(L_NOTE_STATUS, 5), [this](int32 Velocity) { SetColor(0, YELLOW); });
	MIDIActionMap.Add(makeMapKey(L_NOTE_STATUS, 6), [this](int32 Velocity) { SetColor(0, MAGENTA); });
	MIDIActionMap.Add(makeMapKey(L_NOTE_STATUS, 7), [this](int32 Velocity) { SetColor(0, FLinearColor::Black); });
	MIDIActionMap.Add(makeMapKey(R_NOTE_STATUS, 0), [this](int32 Velocity) { SetColor(0, FLinearColor::White); });
	MIDIActionMap.Add(makeMapKey(R_NOTE_STATUS, 1), [this](int32 Velocity) { SetColor(0, FLinearColor::Red); });
	MIDIActionMap.Add(makeMapKey(R_NOTE_STATUS, 2), [this](int32 Velocity) { SetColor(0, FLinearColor::Green); });
	MIDIActionMap.Add(makeMapKey(R_NOTE_STATUS, 3), [this](int32 Velocity) { SetColor(0, FLinearColor::Blue); });
	MIDIActionMap.Add(makeMapKey(R_NOTE_STATUS, 4), [this](int32 Velocity) { SetColor(0, CYAN); });
	MIDIActionMap.Add(makeMapKey(R_NOTE_STATUS, 5), [this](int32 Velocity) { SetColor(0, YELLOW); });
	MIDIActionMap.Add(makeMapKey(R_NOTE_STATUS, 6), [this](int32 Velocity) { SetColor(0, MAGENTA); });
	MIDIActionMap.Add(makeMapKey(R_NOTE_STATUS, 7), [this](int32 Velocity) { SetColor(0, FLinearColor::Black); });


	// Create a Dynamic Material Instance from the material on the Static Mesh
	if (StaticMeshComponent && StaticMeshComponent->GetMaterial(0))
	{
		DynamicMaterialInstance = StaticMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	}

	// Ensure the CanvasRenderTarget is set
	if (CanvasRenderTarget)
	{
		CanvasRenderTarget->InitAutoFormat(1024, 768);
		// Bind the drawing function to the CanvasRenderTarget
		CanvasRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &AMyMIDIManager::DrawToCanvasRenderTarget);
		// Set the CanvasRenderTarget as a parameter for the dynamic material
		DynamicMaterialInstance->SetTextureParameterValue(FName("DynamicTexture"), CanvasRenderTarget);
		CanvasRenderTarget->UpdateResource(); // Ensure the render target is updated
	}

	MidiPtr = Tribe::PlatformMidi::create();

	auto inDevices = MidiPtr->enumerateDevices();
	for (const auto& device : inDevices) {
		if (device.name == "DDJ-FLX2" || device.name == "DDJ-FLX4") {
			MidiPtr->openDevice(device.name, [this](uint8 status, int8 data1, int8 data2) {
				OnMidiEvent(status, data1, data2);
				});
			break;
		}
	}
}

// Called every frame
void AMyMIDIManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMyMIDIManager::DrawToCanvasRenderTarget(UCanvas* Canvas, int32 Width, int32 Height)
{
	if (!Canvas)
		return;
	FScopeLock Lock(&Mutex); // Lock on this thread
	DrawingInstructions.RenderToCanvas(Canvas);
}

void AMyMIDIManager::Clear() {
	{
		FScopeLock Lock(&Mutex); // Lock on this thread
		DrawingInstructions.ClearInstructions();
	}
	Redraw();
}

void AMyMIDIManager::Reset() {
	Clear();
	CurrentPosition = { SIZE.X / 2, SIZE.Y / 2 };
}

void AMyMIDIManager::Redraw() {
	if (IsInGameThread()) {
		if (CanvasRenderTarget)
		{
			// Ensure the render target is updated
			CanvasRenderTarget->UpdateResource();
		}
	} else {
		// Enqueue a task on the game thread
		AsyncTask(ENamedThreads::GameThread, [this]() { Redraw(); });
	}

}

void AMyMIDIManager::DrawTo(const FVector2D& v) {
	const auto clampedv = FVector2D(
		std::clamp(v.X, 0.0, SIZE.X),
		std::clamp(v.Y, 0.0, SIZE.Y)
	);
	if (clampedv != v) {
		UE_LOG(LogTemp, Log, TEXT("Clamped position"));
	}

	if (v == CurrentPosition) {
		// no actual change so no draw
		return;
	}

	if (CurrentColor != FLinearColor::Black) {
		FScopeLock Lock(&Mutex); // Lock on this thread
		DrawingInstructions.AddInstruction(CurrentPosition, clampedv, CurrentColor);
		Redraw();
	}


	MoveTo(clampedv);
}

void AMyMIDIManager::Draw(const FVector2D& delta) {
	DrawTo(CurrentPosition + delta);
}

void AMyMIDIManager::MoveTo(const FVector2D& v) {
	const auto clampedv = FVector2D(
		std::clamp(v.X, 0.0, SIZE.X),
		std::clamp(v.Y, 0.0, SIZE.Y)
	);
	CurrentPosition = clampedv;
}

void AMyMIDIManager::Move(const FVector2D& delta) {
	MoveTo(CurrentPosition + delta);
}


void AMyMIDIManager::CoarseX(int32 velocity) {
	velocity -= 64;
	const auto delta = FVector2D(velocity * SCALE, 0);
	Draw(delta);
}

void AMyMIDIManager::CoarseY(int32 velocity) {
	velocity -= 64;
	const auto delta = FVector2D(0, velocity * SCALE);
	Draw(delta);
}

void AMyMIDIManager::FineX(int32 velocity) {
	velocity -= 64;
	const auto delta = FVector2D(velocity * FINE_SCALE, 0);
	Draw(delta);
}
void AMyMIDIManager::FineY(int32 velocity) {
	velocity -= 64;
	const auto delta = FVector2D(0, velocity * FINE_SCALE);
	Draw(delta);
}
void AMyMIDIManager::SetColor(int32 velocity, const FLinearColor& color) {
	if (velocity > 0) {
		CurrentColor = color;
	}
}
