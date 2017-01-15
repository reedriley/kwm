#include "keys.h"
#include "helpers.h"
#include "interpreter.h"

#define internal static
#define local_persist static

extern modifier_keys MouseDragKey;

internal const char *Shell = "/bin/bash";
internal const char *ShellArgs = "-c";

internal inline bool
HasFlags(modifier_keys *Modifier, uint32_t Flag)
{
    bool Result = Modifier->Flags & Flag;
    return Result;
}

internal inline void
AddFlags(modifier_keys *Modifier, uint32_t Flag)
{
    Modifier->Flags |= Flag;
}

internal inline bool
CompareCmdKey(modifier_keys *A, modifier_keys *B)
{
    if(HasFlags(A, Modifier_Flag_Cmd))
    {
        return (HasFlags(B, Modifier_Flag_LCmd) ||
                HasFlags(B, Modifier_Flag_RCmd) ||
                HasFlags(B, Modifier_Flag_Cmd));
    }
    else
    {
        return ((HasFlags(A, Modifier_Flag_LCmd) == HasFlags(B, Modifier_Flag_LCmd)) &&
                (HasFlags(A, Modifier_Flag_RCmd) == HasFlags(B, Modifier_Flag_RCmd)) &&
                (HasFlags(A, Modifier_Flag_Cmd) == HasFlags(B, Modifier_Flag_Cmd)));
    }
}

internal inline bool
CompareShiftKey(modifier_keys *A, modifier_keys *B)
{
    if(HasFlags(A, Modifier_Flag_Shift))
    {
        return (HasFlags(B, Modifier_Flag_LShift) ||
                HasFlags(B, Modifier_Flag_RShift) ||
                HasFlags(B, Modifier_Flag_Shift));
    }
    else
    {
        return ((HasFlags(A, Modifier_Flag_LShift) == HasFlags(B, Modifier_Flag_LShift)) &&
                (HasFlags(A, Modifier_Flag_RShift) == HasFlags(B, Modifier_Flag_RShift)) &&
                (HasFlags(A, Modifier_Flag_Shift) == HasFlags(B, Modifier_Flag_Shift)));
    }
}

internal inline bool
CompareAltKey(modifier_keys *A, modifier_keys *B)
{
    if(HasFlags(A, Modifier_Flag_Alt))
    {
        return (HasFlags(B, Modifier_Flag_LAlt) ||
                HasFlags(B, Modifier_Flag_RAlt) ||
                HasFlags(B, Modifier_Flag_Alt));
    }
    else
    {
        return ((HasFlags(A, Modifier_Flag_LAlt) == HasFlags(B, Modifier_Flag_LAlt)) &&
                (HasFlags(A, Modifier_Flag_RAlt) == HasFlags(B, Modifier_Flag_RAlt)) &&
                (HasFlags(A, Modifier_Flag_Alt) == HasFlags(B, Modifier_Flag_Alt)));
    }
}

internal inline bool
CompareControlKey(modifier_keys *A, modifier_keys *B)
{
    if(HasFlags(A, Modifier_Flag_Control))
    {
        return (HasFlags(B, Modifier_Flag_LControl) ||
                HasFlags(B, Modifier_Flag_RControl) ||
                HasFlags(B, Modifier_Flag_Control));
    }
    else
    {
        return ((HasFlags(A, Modifier_Flag_LControl) == HasFlags(B, Modifier_Flag_LControl)) &&
                (HasFlags(A, Modifier_Flag_RControl) == HasFlags(B, Modifier_Flag_RControl)) &&
                (HasFlags(A, Modifier_Flag_Control) == HasFlags(B, Modifier_Flag_Control)));
    }
}

internal void
ParseModifiers(modifier_keys *Modifier, std::string KeySym)
{
    std::vector<std::string> Modifiers = SplitString(KeySym, '+');
    for(std::size_t ModIndex = 0; ModIndex < Modifiers.size(); ++ModIndex)
    {
        if(Modifiers[ModIndex] == "cmd")
            AddFlags(Modifier, Modifier_Flag_Cmd);
        else if(Modifiers[ModIndex] == "lcmd")
            AddFlags(Modifier, Modifier_Flag_LCmd);
        else if(Modifiers[ModIndex] == "rcmd")
            AddFlags(Modifier, Modifier_Flag_RCmd);
        else if(Modifiers[ModIndex] == "alt")
            AddFlags(Modifier, Modifier_Flag_Alt);
        else if(Modifiers[ModIndex] == "lalt")
            AddFlags(Modifier, Modifier_Flag_LAlt);
        else if(Modifiers[ModIndex] == "ralt")
            AddFlags(Modifier, Modifier_Flag_RAlt);
        else if(Modifiers[ModIndex] == "shift")
            AddFlags(Modifier, Modifier_Flag_Shift);
        else if(Modifiers[ModIndex] == "lshift")
            AddFlags(Modifier, Modifier_Flag_LShift);
        else if(Modifiers[ModIndex] == "rshift")
            AddFlags(Modifier, Modifier_Flag_RShift);
        else if(Modifiers[ModIndex] == "ctrl")
            AddFlags(Modifier, Modifier_Flag_Control);
        else if(Modifiers[ModIndex] == "lctrl")
            AddFlags(Modifier, Modifier_Flag_LControl);
        else if(Modifiers[ModIndex] == "rctrl")
            AddFlags(Modifier, Modifier_Flag_RControl);
    }
}

internal modifier_keys
ModifierFromCGEvent(CGEventRef Event)
{
    modifier_keys Modifier = {};
    CGEventFlags Flags = CGEventGetFlags(Event);

    if((Flags & Event_Mask_Cmd) == Event_Mask_Cmd)
    {
        if((Flags & Event_Mask_LCmd) == Event_Mask_LCmd)
            AddFlags(&Modifier, Modifier_Flag_LCmd);
        else if((Flags & Event_Mask_RCmd) == Event_Mask_RCmd)
            AddFlags(&Modifier, Modifier_Flag_RCmd);
        else
            AddFlags(&Modifier, Modifier_Flag_Cmd);
    }

    if((Flags & Event_Mask_Shift) == Event_Mask_Shift)
    {
        if((Flags & Event_Mask_LShift) == Event_Mask_LShift)
            AddFlags(&Modifier, Modifier_Flag_LShift);
        else if((Flags & Event_Mask_RShift) == Event_Mask_RShift)
            AddFlags(&Modifier, Modifier_Flag_RShift);
        else
            AddFlags(&Modifier, Modifier_Flag_Shift);
    }

    if((Flags & Event_Mask_Alt) == Event_Mask_Alt)
    {
        if((Flags & Event_Mask_LAlt) == Event_Mask_LAlt)
            AddFlags(&Modifier, Modifier_Flag_LAlt);
        else if((Flags & Event_Mask_RAlt) == Event_Mask_RAlt)
            AddFlags(&Modifier, Modifier_Flag_RAlt);
        else
            AddFlags(&Modifier, Modifier_Flag_Alt);
    }

    if((Flags & Event_Mask_Control) == Event_Mask_Control)
    {
        if((Flags & Event_Mask_LControl) == Event_Mask_LControl)
            AddFlags(&Modifier, Modifier_Flag_LControl);
        else if((Flags & Event_Mask_RControl) == Event_Mask_RControl)
            AddFlags(&Modifier, Modifier_Flag_RControl);
        else
            AddFlags(&Modifier, Modifier_Flag_Control);
    }

    return Modifier;
}

void KwmSetMouseDragKey(std::string KeySym)
{
    ParseModifiers(&MouseDragKey, KeySym);
}

bool MouseDragKeyMatchesCGEvent(CGEventRef Event)
{
    modifier_keys Modifier = ModifierFromCGEvent(Event);
    return (CompareCmdKey(&MouseDragKey, &Modifier) &&
            CompareShiftKey(&MouseDragKey, &Modifier) &&
            CompareAltKey(&MouseDragKey, &Modifier) &&
            CompareControlKey(&MouseDragKey, &Modifier));
}

void KwmExecuteSystemCommand(std::string Command)
{
    int ChildPID = fork();
    if(ChildPID == 0)
    {
        DEBUG("Exec: FORK SUCCESS");
        char *Exec[] = { (char *) Shell, (char *) ShellArgs, (char *) Command.c_str(), NULL};
        int StatusCode = execvp(Exec[0], Exec);
        DEBUG("Exec failed with code: " << StatusCode);
        exit(StatusCode);
    }
}
