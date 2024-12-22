// Fill out your copyright notice in the Description page of Project Settings.


#include "MyMIDIManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Materials/MaterialInstanceDynamic.h"

static const auto POWDER = FLinearColor(0.74f, 1.0f, 0.74f);
static const auto CYAN = FLinearColor(0.0f, 1.0f, 1.0f);
static const auto YELLOW = FLinearColor(1.0f, 1.0f, 0.0f);
static const auto MAGENTA = FLinearColor(1.0f, 0.0f, 0.0f);

static const auto L_DECK_CHANNEL = 0x01;
static const auto R_DECK_CHANNEL = 0x02;

static const auto L_PAD_CHANNEL = 0x08;
static const auto R_PAD_CHANNEL = 0x0a;
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
	MIDIActionMap.Add(makeMapKey(L_DECK_CHANNEL, PLATTER_SPIN_ID), [this](int32 Velocity) { CoarseX(Velocity); });
	MIDIActionMap.Add(makeMapKey(L_DECK_CHANNEL, PLATTER_SPIN_ID), [this](int32 Velocity) { CoarseX(Velocity); });
	MIDIActionMap.Add(makeMapKey(L_DECK_CHANNEL, SIDE_SPIN_ID), [this](int32 Velocity) {FineX(Velocity); });
	MIDIActionMap.Add(makeMapKey(R_DECK_CHANNEL, PLATTER_SPIN_ID), [this](int32 Velocity) {CoarseY(Velocity); });
	MIDIActionMap.Add(makeMapKey(R_DECK_CHANNEL, SIDE_SPIN_ID), [this](int32 Velocity) {FineY(Velocity); });
	MIDIActionMap.Add(makeMapKey(L_PAD_CHANNEL, 0), [this](int32 Velocity) { SetColor(0, FLinearColor::White); });
	MIDIActionMap.Add(makeMapKey(L_PAD_CHANNEL, 1), [this](int32 Velocity) { SetColor(0, FLinearColor::Red); });
	MIDIActionMap.Add(makeMapKey(L_PAD_CHANNEL, 2), [this](int32 Velocity) { SetColor(0, FLinearColor::Green); });
	MIDIActionMap.Add(makeMapKey(L_PAD_CHANNEL, 3), [this](int32 Velocity) { SetColor(0, FLinearColor::Blue); });
	MIDIActionMap.Add(makeMapKey(L_PAD_CHANNEL, 4), [this](int32 Velocity) { SetColor(0, CYAN); });
	MIDIActionMap.Add(makeMapKey(L_PAD_CHANNEL, 5), [this](int32 Velocity) { SetColor(0, YELLOW); });
	MIDIActionMap.Add(makeMapKey(L_PAD_CHANNEL, 6), [this](int32 Velocity) { SetColor(0, MAGENTA); });
	MIDIActionMap.Add(makeMapKey(L_PAD_CHANNEL, 7), [this](int32 Velocity) { SetColor(0, FLinearColor::Black); });
	MIDIActionMap.Add(makeMapKey(R_PAD_CHANNEL, 0), [this](int32 Velocity) { SetColor(0, FLinearColor::White); });
	MIDIActionMap.Add(makeMapKey(R_PAD_CHANNEL, 1), [this](int32 Velocity) { SetColor(0, FLinearColor::Red); });
	MIDIActionMap.Add(makeMapKey(R_PAD_CHANNEL, 2), [this](int32 Velocity) { SetColor(0, FLinearColor::Green); });
	MIDIActionMap.Add(makeMapKey(R_PAD_CHANNEL, 3), [this](int32 Velocity) { SetColor(0, FLinearColor::Blue); });
	MIDIActionMap.Add(makeMapKey(R_PAD_CHANNEL, 4), [this](int32 Velocity) { SetColor(0, CYAN); });
	MIDIActionMap.Add(makeMapKey(R_PAD_CHANNEL, 5), [this](int32 Velocity) { SetColor(0, YELLOW); });
	MIDIActionMap.Add(makeMapKey(R_PAD_CHANNEL, 6), [this](int32 Velocity) { SetColor(0, MAGENTA); });
	MIDIActionMap.Add(makeMapKey(R_PAD_CHANNEL, 7), [this](int32 Velocity) { SetColor(0, FLinearColor::Black); });


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
	DrawingInstructions.RenderToCanvas(Canvas);
}

void AMyMIDIManager::Clear() {
	DrawingInstructions.ClearInstructions();
	CanvasRenderTarget->UpdateResource(); // Ensure the render target is updated
}

void AMyMIDIManager::Reset() {
	Clear();
	CurrentPosition = { SIZE.X / 2, SIZE.Y / 2 };


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
		DrawingInstructions.AddInstruction(CurrentPosition, clampedv, CurrentColor);
	}
	CanvasRenderTarget->UpdateResource(); // Ensure the render target is updated
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
