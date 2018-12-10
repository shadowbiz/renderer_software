#if !defined(SHADOWBIZ_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Alexander Udov $
   $Notice: (C) Copyright 2015 by Shadowbiz. All Rights Reserved. $
   ======================================================================== */

#if SHADOWBIZ_SLOW
#define Assert(expression) \
    if (!(expression))     \
    {                      \
        *(int *)0 = 0;     \
    }
#else
#define Assert(expression)
#endif

#define ArraySize(array) (sizeof(array) / sizeof((array)[0]))
#define Kilobytes(bytes) ((bytes)*1024)
#define Megabytes(bytes) ((bytes)*1024 * 1024)
#define Gigabytes(bytes) ((bytes)*1024 * 1024 * 1024)
#define Terabytes(bytes) ((bytes)*1024 * 1024 * 1024 * 1024)

#if SHADOWBIZ_INTERNAL

struct debug_read_file
{
    uint32 FileSize;
    void *Data;
};

internal debug_read_file DebugPlatformReadEntireFile(char *filename);
internal void DebugPlatformFreeFileMemory(void *memory);
internal bool32 DebugPlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory);
#endif

struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct game_sound_buffer
{
    int32 SampleCount;
    int32 SamplesPerSecond;
    int16 *Samples;
};

struct game_memory
{
    bool32 IsInitialized;
    uint64 PermamentStorageSize;
    void *PermamentStorage;
    uint64 TransientStorageSize;
    void *TransientStorage;
};

struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    bool32 IsAnalog;

    real32 AverageX;
    real32 AverageY;

    union {
        game_button_state Buttons[6];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionA;
            game_button_state ActionB;
        };
    };
};

struct game_input
{
    game_controller_input Controllers[4];
};

struct game_state
{
    int xOffset;
    int yOffset;
    int ToneHz;
};

internal void
GameUpdateAndRender(game_memory *memory, game_offscreen_buffer *screenBuffer, game_sound_buffer *soundBuffer);

#define SHADOWBIZ_H
#endif
