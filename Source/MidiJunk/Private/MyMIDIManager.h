// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PlatformMidi.h"
#include "Drawing.h"
#include "MyMIDIManager.generated.h"

using FMIDIAction = TFunction<void(int32)>;
using MIDIHANDLE = void*;




UCLASS()
class AMyMIDIManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMyMIDIManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnMidiEvent(uint8 status, int8 data1, int8 data2);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Reference to the Static Mesh Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* StaticMeshComponent{ nullptr };

	// Dynamic Material Instance for interacting with the texture
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UMaterialInstanceDynamic* DynamicMaterialInstance{ nullptr };

	// Canvas Render Target to draw on
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCanvasRenderTarget2D* CanvasRenderTarget{ nullptr };

	// Function to draw on the Canvas Render Target
	UFUNCTION(BlueprintCallable)
	void DrawToCanvasRenderTarget(UCanvas* Canvas, int32 Width, int32 Height);

private:
	TMap<int32, FMIDIAction> MIDIActionMap;
	FDrawingInstructions DrawingInstructions;
	FVector2D CurrentPosition{ 0, 0 };
	FLinearColor CurrentColor{ FLinearColor::White };
	Tribe::PlatformMidi* MidiPtr{ nullptr };

	void CoarseX(int32 velocity);
	void CoarseY(int32 velocity);
	void FineX(int32 velocity);
	void FineY(int32 velocity);
	void SetColor(int32 velocity, const FLinearColor& color);
	void DrawTo(const FVector2D& v);
	void Draw(const FVector2D& delta);
	void MoveTo(const FVector2D& v);
	void Move(const FVector2D& delta);
	void Clear();
	void Reset();

};
