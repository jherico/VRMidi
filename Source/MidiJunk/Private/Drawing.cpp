#include "Drawing.h"
#include "Engine/Canvas.h"


// Helper function to draw a single line
void FDrawingInstructions::DrawLine(UCanvas* Canvas, const FDrawingInstruction& Instruction) {
	FCanvasLineItem LineItem(Instruction.StartPoint, Instruction.EndPoint);
	LineItem.SetColor(Instruction.LineColor);

	// Apply glow effect if needed
	if ((Instruction.Effects & EDrawEffect::Glow) != EDrawEffect::None) {
		LineItem.BlendMode = SE_BLEND_Translucent;
	}

	Canvas->DrawItem(LineItem);
}
