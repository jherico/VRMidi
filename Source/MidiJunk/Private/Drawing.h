#pragma once
#include "CoreMinimal.h"


// Flags for special effects
enum EDrawEffect : uint8 {
	None = 0,
	Glow = 1 << 0,
	DashedLine = 1 << 1,
	// Add more effects as needed
};


// Structure to hold a single drawing instruction
struct FDrawingInstruction {
	FVector2D StartPoint;
	FVector2D EndPoint;
	FLinearColor LineColor;
	EDrawEffect Effects;

	// Constructor
	FDrawingInstruction(FVector2D InStart, FVector2D InEnd, FLinearColor InColor, EDrawEffect InEffects)
		: StartPoint(InStart), EndPoint(InEnd), LineColor(InColor), Effects(InEffects) {
	}
};

class FDrawingInstructions {
public:
	// Array to store instructions
	TArray<FDrawingInstruction> Instructions;

	// Add a new instruction
	void AddInstruction(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color, EDrawEffect Effects = EDrawEffect::None) {
		Instructions.Emplace(Start, End, Color, Effects);
	}

	// Clear all instructions
	void ClearInstructions() {
		Instructions.Empty();
	}

	// Render all instructions
	void RenderToCanvas(UCanvas* Canvas) {
		for (const auto& Instruction : Instructions) {
			DrawLine(Canvas, Instruction);
		}
	}

private:
	// Helper function to draw a single line
	void DrawLine(UCanvas* Canvas, const FDrawingInstruction& Instruction);
};

