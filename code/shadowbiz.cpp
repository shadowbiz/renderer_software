/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Alexander Udov $
   $Notice: (C) Copyright 2015 by Shadowbiz. All Rights Reserved. $
   ======================================================================== */

#include "shadowbiz.h"

internal void
GameOutputSound(game_sound_buffer *soundBuffer, int toneHz)
{
	local_persist real32 tSine;
	const int16 toneVolume = 300;
	int wavePeriod = soundBuffer->SamplesPerSecond / toneHz;

	int16 *SampleOut = soundBuffer->Samples;
	for (int SampleIndex = 0;
		 SampleIndex < soundBuffer->SampleCount;
		 ++SampleIndex)
	{
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * toneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += 2.0f * Pi32 * 1.0f / (real32)wavePeriod;
	}
}

internal void
RenderGradient(game_offscreen_buffer *buffer, int xOffset, int yOffset)
{
	uint8 *row = (uint8 *)buffer->Memory;

	for (int y = 0; y < buffer->Height; ++y)
	{
		uint32 *pixel = (uint32 *)row;
		for (int x = 0; x < buffer->Width; ++x)
		{
			uint8 r = (uint8)((x + xOffset) * (x + xOffset));
			uint8 g = (uint8)((y + yOffset) * (y + yOffset));
			uint8 b = 0;

			uint32 color = (r << 16) | (g << 8) | (b << 0);

			*pixel = color;
			pixel++;
		}

		row += buffer->Pitch;
	}
}

internal void
GameUpdateAndRender(game_memory *memory, game_input *gameInput,
					game_offscreen_buffer *screenBuffer, game_sound_buffer *soundBuffer)
{

	Assert(sizeof(game_state) <= memory->PermamentStorageSize);

	game_state *gameState = (game_state *)memory->PermamentStorage;
	if (!memory->IsInitialized)
	{

		gameState->ToneHz = 256;
		memory->IsInitialized = true;
	}

	game_controller_input *input0 = &gameInput->Controllers[0];
	if (input0->MoveDown.EndedDown)
	{
		gameState->yOffset++;
		gameState->ToneHz++;
	}
	if (input0->MoveUp.EndedDown)
	{
		gameState->ToneHz--;
		gameState->yOffset--;
	}

	if (input0->MoveLeft.EndedDown)
	{
		gameState->xOffset--;
	}

	if (input0->MoveRight.EndedDown)
	{
		gameState->xOffset++;
	}

	if (gameState->ToneHz < 100)
	{
		gameState->ToneHz = 100;
	}
	else if (gameState->ToneHz > 500)
	{
		gameState->ToneHz = 500;
	}

	GameOutputSound(soundBuffer, gameState->ToneHz);
	RenderGradient(screenBuffer, gameState->xOffset, gameState->yOffset);
}
