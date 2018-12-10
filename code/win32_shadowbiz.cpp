
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Alexander Udov $
   $Notice: (C) Copyright 2015 by Shadowbiz. All Rights Reserved. $
   ======================================================================== */

#include <stdint.h>
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#include "shadowbiz.cpp"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "win32_shadowbiz.h"

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

global_variable bool IsRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfomanceFrequency;

inline uint32
SafeTruncateUInt64(uint64 value)
{
    Assert(value <= 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return (result);
}

internal debug_read_file
DebugPlatformReadEntireFile(char *filename)
{
    debug_read_file result = {};

    HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize))
        {
            result.FileSize = SafeTruncateUInt64(fileSize.QuadPart);
            result.Data = VirtualAlloc(0, result.FileSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (result.Data)
            {
                DWORD bytesRead;
                if (ReadFile(fileHandle, result.Data, result.FileSize, &bytesRead, 0) &&
                    (bytesRead == result.FileSize))
                {
                }
                else
                {
                    // TODO: Log
                    DebugPlatformFreeFileMemory(result.Data);
                    result.FileSize = 0;
                    result.Data = 0;
                }
            }
            else
            {
                // TODO: Log
            }
        }
        CloseHandle(fileHandle);
    }
    else
    {
        // TODO: Log
    }
    return (result);
}

internal void
DebugPlatformFreeFileMemory(void *memory)
{
    if (memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

internal bool32
DebugPlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory)
{
    bool32 result = false;
    HANDLE fileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if (WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0))
        {
            result = (bytesWritten == memorySize);
        }
        else
        {
            // TODO: Log
        }
        CloseHandle(fileHandle);
    }
    else
    {
        // TODO: Log
    }
    return (result);
}

internal void Win32InitDSound(HWND window, int samplesPerSecond, int bufferSize)
{
    // Load the library
    HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");
    if (dSoundLibrary)
    {
        direct_sound_create *directSoundCreate = (direct_sound_create *)
            GetProcAddress(dSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND directSound;
        if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0)))
        {
            WAVEFORMATEX waveFormat = {};
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.nSamplesPerSec = samplesPerSecond;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize = 0;

            if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(bufferDescription);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER primaryBuffer;
                if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
                {
                    HRESULT error = primaryBuffer->SetFormat(&waveFormat);
                    if (SUCCEEDED(error))
                    {
                        OutputDebugStringA("Primary buffer format was set.\n");
                    }
                    else
                    {
                        // TODO: Log
                    }
                }
                else
                {
                    // TODO: Log
                }
            }
            else
            {
                // TODO: Log
            }

            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = 0;
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            HRESULT error = directSound->CreateSoundBuffer(&bufferDescription, &GlobalSecondaryBuffer, 0);
            if (SUCCEEDED(error))
            {
                OutputDebugStringA("Secondary buffer created successfully.\n");
            }
        }
        else
        {
            // TODO: Log
        }
    }
    else
    {
        // TODO: Log
    }
}

internal void
Win32LoadXInput(void)
{
    HMODULE library = LoadLibraryA("xinput1_4.dll");
    if (!library)
    {
        // TODO: Log
        library = LoadLibraryA("xinput1_3.dll");
    }

    if (library)
    {
        XInputSetState = (x_input_set_state *)GetProcAddress(library, "XInputSetState");
        XInputGetState = (x_input_get_state *)GetProcAddress(library, "XInputGetState");
    }
    else
    {
        // TODO: Log
    }
}

internal win32_window_dimension
Win32GetWindowDimension(HWND window)
{
    win32_window_dimension result = {};

    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.Width = clientRect.right - clientRect.left;
    result.Height = clientRect.bottom - clientRect.top;

    return (result);
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height)
{
    if (buffer->Memory)
    {
        VirtualFree(buffer->Memory, 0, MEM_RELEASE);
    }

    buffer->Width = width;

    buffer->Height = height;
    buffer->BytesPerPixel = 4;
    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = width;
    buffer->Info.bmiHeader.biHeight = -height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = buffer->BytesPerPixel * width * height;
    buffer->Memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->Pitch = width * buffer->BytesPerPixel;
}

internal void
Win32CopyBufferToWindow(win32_offscreen_buffer *buffer, HDC deviceContext,
                        int windowWidth, int windowHeight)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer->Width, buffer->Height,
                  buffer->Memory, &buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

internal void
Win32ProcessKeyboardMessage(game_button_state *newState, bool32 isDown)
{
    Assert(newState->EndedDown != isDown);
    newState->EndedDown = isDown;
    ++newState->HalfTransitionCount;
}

internal void
Win32ProcessPendingMessages(game_controller_input *keyboardController)
{
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        switch (message.message)
        {
        case WM_SIZE:
        {
        }
        break;

        case WM_CLOSE:
            IsRunning = false;
            break;

        case WM_ACTIVATEAPP:
            OutputDebugStringA("WM_ACTIVATEAPP\n");
            break;

        case WM_DESTROY:
            IsRunning = false;
            break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = (uint32)message.wParam;
            bool wasDown = (message.lParam & (1 << 30)) != 0;
            bool isDown = (message.lParam & (1 << 31)) == 0;

            if (wasDown != isDown)
            {
                if (VKCode == 'W')
                {
                    Win32ProcessKeyboardMessage(&keyboardController->MoveUp, isDown);
                }
                else if (VKCode == 'S')
                {
                    Win32ProcessKeyboardMessage(&keyboardController->MoveDown, isDown);
                }
                else if (VKCode == 'A')
                {
                    Win32ProcessKeyboardMessage(&keyboardController->MoveLeft, isDown);
                }
                else if (VKCode == 'D')
                {
                    Win32ProcessKeyboardMessage(&keyboardController->MoveRight, isDown);
                }
                else if (VKCode == 'Q')
                {
                    Win32ProcessKeyboardMessage(&keyboardController->ActionA, isDown);
                }
                else if (VKCode == 'E')
                {
                    Win32ProcessKeyboardMessage(&keyboardController->ActionB, isDown);
                }
                else if (VKCode == VK_UP)
                {
                    Win32ProcessKeyboardMessage(&keyboardController->MoveUp, isDown);
                }
                else if (VKCode == VK_DOWN)
                {
                    Win32ProcessKeyboardMessage(&keyboardController->MoveDown, isDown);
                }
                else if (VKCode == VK_LEFT)
                {
                    Win32ProcessKeyboardMessage(&keyboardController->MoveLeft, isDown);
                }
                else if (VKCode == VK_RIGHT)
                {
                    Win32ProcessKeyboardMessage(&keyboardController->MoveRight, isDown);
                }
                else if (VKCode == VK_ESCAPE)
                {
                    IsRunning = false;
                }
                else if (VKCode == VK_SPACE)
                {
                    if (isDown)
                        OutputDebugStringA("SPACE IS DOWN");
                }
            }
            bool32 altKeyIsDowm = (message.lParam & (1 << 29));
            if ((VKCode == VK_F4) && altKeyIsDowm)
            {
                IsRunning = false;
            }
        }
        break;
        default:
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        break;
        }
    }
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
    case WM_SIZE:
    {
    }
    break;

    case WM_CLOSE:
        IsRunning = false;
        break;

    case WM_ACTIVATEAPP:
        OutputDebugStringA("WM_ACTIVATEAPP\n");
        break;

    case WM_DESTROY:
        IsRunning = false;
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        int x = paint.rcPaint.left;
        int y = paint.rcPaint.top;
        int width = paint.rcPaint.right - paint.rcPaint.left;
        int height = paint.rcPaint.bottom - paint.rcPaint.top;
        win32_window_dimension windowDim = Win32GetWindowDimension(window);
        Win32CopyBufferToWindow(&GlobalBackBuffer, deviceContext,
                                windowDim.Width, windowDim.Height);
    }
    break;

    default:
    {
        result = DefWindowProc(window, message, wParam, lParam);
    }
    break;
    }

    return (result);
}

internal void
Win32ClearSoundBuffer(win32_sound_output *soundOutput)
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, soundOutput->SecondaryBufferSize,
                                              &region1, &region1Size,
                                              &region2, &region2Size,
                                              0)))
    {
        uint8 *destSample = (uint8 *)region1;
        for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex)
        {
            *destSample++ = 0;
        }

        destSample = (uint8 *)region2;
        for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex)
        {
            *destSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output *soundOutput,
                     DWORD byteToLock, DWORD bytesToWrite,
                     game_sound_buffer *sourceBuffer)
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(byteToLock, bytesToWrite,
                                              &region1, &region1Size,
                                              &region2, &region2Size,
                                              0)))
    {
        DWORD region1SampleCount = region1Size / soundOutput->BytesPerSample;
        int16 *destSample = (int16 *)region1;
        int16 *sourceSample = sourceBuffer->Samples;

        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            ++soundOutput->RunningSampleIndex;
        }

        DWORD region2SampleCount = region2Size / soundOutput->BytesPerSample;
        destSample = (int16 *)region2;
        for (DWORD sampleIndex = 0;
             sampleIndex < region2SampleCount;
             ++sampleIndex)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            ++soundOutput->RunningSampleIndex;
        }

        GlobalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

inline LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return (result);
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER begin, LARGE_INTEGER end)
{
    real32 result = (real32)(end.QuadPart - begin.QuadPart) / (real32)GlobalPerfomanceFrequency;
    return (result);
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int cmdShow)
{
    UINT desiredScedularMs = 1;
    bool32 timerIsGranular = (timeBeginPeriod(desiredScedularMs) == TIMERR_NOERROR);

    LARGE_INTEGER perfomanceFrequencyLarge;
    QueryPerformanceFrequency(&perfomanceFrequencyLarge);
    GlobalPerfomanceFrequency = perfomanceFrequencyLarge.QuadPart;

    Win32LoadXInput();

    WNDCLASSA windowClass = {};

    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "ShadowbizWindowClass";

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    int monitorRefreshRate = 60;
    int gameUpdateHz = monitorRefreshRate / 2;
    real32 targetSecondsPerFrame = 1.0f / (real32)gameUpdateHz;

    if (RegisterClassA(&windowClass))
    {
        HWND window = CreateWindowExA(0, windowClass.lpszClassName, "Shadowbiz",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, instance, 0);
        if (window)
        {
            int xOffset = 0;
            int yOffset = 0;

            HDC deviceContext = GetDC(window);

            win32_sound_output soundOutput = {};
            soundOutput.SamplesPerSecond = 48000;
            soundOutput.BytesPerSample = sizeof(int16) * 2;
            soundOutput.SecondaryBufferSize = soundOutput.SamplesPerSecond * soundOutput.BytesPerSample;
            soundOutput.LatencySampleCount = soundOutput.SamplesPerSecond / 15;

            Win32InitDSound(window, soundOutput.SamplesPerSecond, soundOutput.SecondaryBufferSize);
            Win32ClearSoundBuffer(&soundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

#if SHADOWBIZ_INTERNAL
            LPVOID baseAddress = (LPVOID)Terabytes((uint64)2);
#else
            LPVOID baseAddress = 0;
#endif

            game_memory gameMemory;
            gameMemory = {};
            gameMemory.PermamentStorageSize = Megabytes(64);
            gameMemory.TransientStorageSize = Gigabytes((uint64)2);

            uint64 totalMemorySize = gameMemory.PermamentStorageSize + gameMemory.TransientStorageSize;

            gameMemory.PermamentStorage =
                VirtualAlloc(baseAddress, totalMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            gameMemory.TransientStorage =
                ((uint8 *)gameMemory.PermamentStorage + gameMemory.PermamentStorageSize);

            int16 *samples = (int16 *)VirtualAlloc(0, soundOutput.SecondaryBufferSize,
                                                   MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            if (samples && gameMemory.PermamentStorage && gameMemory.TransientStorage)
            {

                game_input input[2] = {};
                game_input *newInput = &input[0];
                game_input *oldInput = &input[1];

                IsRunning = true;
                LARGE_INTEGER lastCounter = Win32GetWallClock();
                uint64 lastCycleCount = __rdtsc();

                while (IsRunning)
                {
                    game_controller_input *oldKeyboardController = &oldInput->Controllers[0];
                    game_controller_input *newKeyboardController = &newInput->Controllers[0];
                    game_controller_input zeroController = {};
                    *newKeyboardController = zeroController;

                    for (int keyIndex = 0;
                         keyIndex < ArraySize(oldKeyboardController->Buttons);
                         ++keyIndex)
                    {
                        newKeyboardController->Buttons[keyIndex].EndedDown =
                            oldKeyboardController->Buttons[keyIndex].EndedDown;
                    }

                    Win32ProcessPendingMessages(newKeyboardController);

                    bool32 soundIsValid = false;
                    DWORD bytesToLock = 0;
                    DWORD bytesToWrite = 0;
                    DWORD targetCursor = 0;
                    DWORD playCursor = 0;
                    DWORD writeCursor = 0;
                    if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                    {
                        bytesToLock = (soundOutput.RunningSampleIndex * soundOutput.BytesPerSample) %
                                      soundOutput.SecondaryBufferSize;
                        targetCursor = ((playCursor +
                                         (soundOutput.LatencySampleCount * soundOutput.BytesPerSample)) %
                                        soundOutput.SecondaryBufferSize);

                        if (bytesToLock > targetCursor)
                        {
                            bytesToWrite = soundOutput.SecondaryBufferSize - bytesToLock;
                            bytesToWrite += targetCursor;
                        }
                        else
                        {
                            bytesToWrite = targetCursor - bytesToLock;
                        }
                        soundIsValid = true;
                    }

                    game_offscreen_buffer screenBuffer = {};
                    screenBuffer.Memory = GlobalBackBuffer.Memory;
                    screenBuffer.Width = GlobalBackBuffer.Width;
                    screenBuffer.Height = GlobalBackBuffer.Height;
                    screenBuffer.Pitch = GlobalBackBuffer.Pitch;
                    screenBuffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;

                    game_sound_buffer soundBuffer = {};
                    soundBuffer.SamplesPerSecond = soundOutput.SamplesPerSecond;
                    soundBuffer.SampleCount = bytesToWrite / soundOutput.BytesPerSample;
                    soundBuffer.Samples = samples;

                    GameUpdateAndRender(&gameMemory, newInput, &screenBuffer, &soundBuffer);

                    if (soundIsValid)
                    {
                        Win32FillSoundBuffer(&soundOutput, bytesToLock, bytesToWrite, &soundBuffer);
                    }

                    LARGE_INTEGER workCounter = Win32GetWallClock();
                    real32 workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, workCounter);

                    real32 secondsElapsedForFrame = workSecondsElapsed;
                    if (secondsElapsedForFrame < targetSecondsPerFrame)
                    {
                        if (timerIsGranular)
                        {
                            DWORD sleepMs = (DWORD)(1000.f * (targetSecondsPerFrame - secondsElapsedForFrame));
                            if (sleepMs > 0)
                            {
                                Sleep(sleepMs);
                            }
                        }

                        while (secondsElapsedForFrame < targetSecondsPerFrame)
                        {
                            secondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
                        }
                    }
                    else
                    {
                        // TODO: Log
                    }

                    win32_window_dimension windowDim = Win32GetWindowDimension(window);
                    Win32CopyBufferToWindow(&GlobalBackBuffer, deviceContext, windowDim.Width, windowDim.Height);

                    game_input *temp = newInput;
                    newInput = oldInput;
                    oldInput = temp;

                    uint64 endCycleCount = __rdtsc();
                    uint64 cycleCount = (endCycleCount - lastCycleCount);
                    lastCycleCount = endCycleCount;

                    LARGE_INTEGER endCounter = Win32GetWallClock();

#if 0
                    uint64 megaCyclesCount = cycleCount/(1000*1000);
                    real32 msPerFrame = (Win32GetSecondsElapsed(lastCounter, endCounter)*1000.0f);
                    real32 fps = ((real32)GlobalPerfomanceFrequency/(endCounter.QuadPart - lastCounter.QuadPart));
                    char buffer[256];
                    sprintf_s(buffer, "ms/frame: %.3fms; FPS: %.3f frames; %d mega cycles\n", msPerFrame, fps, megaCyclesCount);
                    OutputDebugStringA(buffer);
#endif
                    lastCounter = endCounter;
                }
            }
            else
            {
                // TODO: Log
            }
        }
        else
        {
            // TODO: Log
        }
    }
    else
    {
        // TODO: Log
    }
    return (0);
}
