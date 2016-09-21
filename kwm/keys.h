#ifndef KEYS_H
#define KEYS_H

#include "types.h"

/* Taken from: /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/
				Developer/SDKs/MacOSX10.11.sdk/System/Library/Frameworks/IOKit.framework/
				Versions/A/Headers/hidsystem/IOLLEvent.h

NOTE(koekeishiya): All left and right-masks are device-dependent for some reason. */

enum osx_event_mask
{
    Event_Mask_Alt = 0x00080000,
    Event_Mask_LAlt = 0x00000020,
    Event_Mask_RAlt = 0x00000040,

    Event_Mask_Shift = 0x00020000,
    Event_Mask_LShift = 0x00000002,
    Event_Mask_RShift = 0x00000004,

    Event_Mask_Cmd = 0x00100000,
    Event_Mask_LCmd = 0x00000008,
    Event_Mask_RCmd = 0x00000010,

    Event_Mask_Control = 0x00040000,
    Event_Mask_LControl = 0x00000001,
    Event_Mask_RControl = 0x00002000,
};

enum modifier_flag
{
    Modifier_Flag_Alt = (1 << 0),
    Modifier_Flag_LAlt = (1 << 1),
    Modifier_Flag_RAlt = (1 << 2),

    Modifier_Flag_Shift = (1 << 3),
    Modifier_Flag_LShift = (1 << 4),
    Modifier_Flag_RShift = (1 << 5),

    Modifier_Flag_Cmd = (1 << 6),
    Modifier_Flag_LCmd = (1 << 7),
    Modifier_Flag_RCmd = (1 << 8),

    Modifier_Flag_Control = (1 << 9),
    Modifier_Flag_LControl = (1 << 10),
    Modifier_Flag_RControl = (1 << 11),
};

bool MouseDragKeyMatchesCGEvent(CGEventRef Event);
void KwmSetMouseDragKey(std::string KeySym);

void KwmExecuteSystemCommand(std::string Command);

#endif
