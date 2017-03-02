#include "kwm.h"
#include "daemon.h"
#include "display.h"
#include "window.h"
#include "keys.h"
#include "scratchpad.h"
#include "border.h"
#include "config.h"
#include "../axlib/axlib.h"
#include <getopt.h>

#define internal static
const char *KwmVersion = "Kwm Version 4.0.5";
std::map<std::string, space_info> WindowTree;

ax_display *FocusedDisplay = NULL;
ax_application *FocusedApplication = NULL;
ax_window *MarkedWindow = NULL;

kwm_mach KWMMach = {};
kwm_path KWMPath = {};
kwm_settings KWMSettings = {};
kwm_border FocusedBorder = {};
kwm_border MarkedBorder = {};
scratchpad Scratchpad = {};
modifier_keys MouseDragKey = {};

internal CGEventRef
CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon)
{
    switch(Type)
    {
        case kCGEventTapDisabledByTimeout:
        case kCGEventTapDisabledByUserInput:
        {
            DEBUG("Notice: Restarting Event Tap");
            CGEventTapEnable(KWMMach.EventTap, true);
        } break;
        case kCGEventMouseMoved:
        {
            if(KWMSettings.Focus == FocusModeAutoraise)
            {
                CGEventFlags Flags = CGEventGetFlags(Event);
                if(!(Flags & Event_Mask_Alt))
                {
                    AXLibConstructEvent(AXEvent_MouseMoved, NULL, false);
                }
            }
        } break;
        case kCGEventLeftMouseDown:
        {
            if(HasFlags(&KWMSettings, Settings_MouseDrag))
            {
                if(MouseDragKeyMatchesCGEvent(Event))
                {
                    AXLibConstructEvent(AXEvent_LeftMouseDown, NULL, false);
                    return NULL;
                }
            }
        } break;
        case kCGEventLeftMouseUp:
        {
            if(HasFlags(&KWMSettings, Settings_MouseDrag))
                AXLibConstructEvent(AXEvent_LeftMouseUp, NULL, false);
        } break;
        case kCGEventLeftMouseDragged:
        {
            if(HasFlags(&KWMSettings, Settings_MouseDrag))
                AXLibConstructEvent(AXEvent_LeftMouseDragged, NULL, false);
        } break;
        case kCGEventRightMouseDown:
        {
            if(HasFlags(&KWMSettings, Settings_MouseDrag))
            {
                if(MouseDragKeyMatchesCGEvent(Event))
                {
                    AXLibConstructEvent(AXEvent_RightMouseDown, NULL, false);
                    return NULL;
                }
            }
        } break;
        case kCGEventRightMouseUp:
        {
            if(HasFlags(&KWMSettings, Settings_MouseDrag))
                AXLibConstructEvent(AXEvent_RightMouseUp, NULL, false);
        } break;
        case kCGEventRightMouseDragged:
        {
            if(HasFlags(&KWMSettings, Settings_MouseDrag))
                AXLibConstructEvent(AXEvent_RightMouseDragged, NULL, false);
        } break;

        default: {} break;
    }

    return Event;
}

internal inline bool
CheckPrivileges()
{
    bool Result = false;
    const void *Keys[] = { kAXTrustedCheckOptionPrompt };
    const void *Values[] = { kCFBooleanTrue };

    CFDictionaryRef Options;
    Options = CFDictionaryCreate(kCFAllocatorDefault,
                                 Keys, Values, sizeof(Keys) / sizeof(*Keys),
                                 &kCFCopyStringDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);

    Result = AXIsProcessTrustedWithOptions(Options);
    CFRelease(Options);

    return Result;
}

internal bool
GetKwmFilePath()
{
    bool Result = false;
    char PathBuf[PROC_PIDPATHINFO_MAXSIZE];
    pid_t Pid = getpid();

    int Ret = proc_pidpath(Pid, PathBuf, sizeof(PathBuf));
    if(Ret > 0)
    {
        KWMPath.FilePath = PathBuf;
        std::size_t Split = KWMPath.FilePath.find_last_of("/\\");
        KWMPath.FilePath = KWMPath.FilePath.substr(0, Split);
        Result = true;
    }

    return Result;
}

internal void
SignalHandler(int Signum)
{
    ShowAllScratchpadWindows();
    DEBUG("SignalHandler() " << Signum);

    CloseBorder(&FocusedBorder);
    CloseBorder(&MarkedBorder);
    exit(Signum);
}

internal inline void
Fatal(const char *Err)
{
    printf("%s\n", Err);
    exit(1);
}

internal void
KwmInit()
{
    if(!CheckPrivileges())
        Fatal("Error: Could not access OSX Accessibility!");

    signal(SIGCHLD, SIG_IGN);
#ifndef DEBUG_BUILD
    signal(SIGSEGV, SignalHandler);
    signal(SIGABRT, SignalHandler);
    signal(SIGTRAP, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGKILL, SignalHandler);
    signal(SIGINT, SignalHandler);
#else
    printf("Notice: Signal handlers disabled!\n");
#endif

    KWMSettings.SplitRatio = 0.5;
    KWMSettings.SplitMode = SPLIT_OPTIMAL;
    KWMSettings.DefaultOffset = CreateDefaultDisplayOffset();
    KWMSettings.OptimalRatio = 1.618;

    AddFlags(&KWMSettings,
            Settings_MouseFollowsFocus |
            Settings_StandbyOnFloat |
            Settings_CenterOnFloat |
            Settings_LockToContainer);

    KWMSettings.Space = SpaceModeBSP;
    KWMSettings.Focus = FocusModeAutoraise;
    KWMSettings.Cycle = CycleModeScreen;

    FocusedBorder.Radius = -1;
    FocusedBorder.Type = BORDER_FOCUSED;
    MarkedBorder.Radius = -1;
    MarkedBorder.Type = BORDER_MARKED;

    char *HomeP = std::getenv("HOME");
    if(HomeP)
    {
        KWMPath.EnvHome = HomeP;
        KWMPath.Home = KWMPath.EnvHome + "/.kwm";
        KWMPath.Include = KWMPath.Home;
        KWMPath.Layouts = KWMPath.Home + "/layouts";

        if(KWMPath.Config.empty())
            KWMPath.Config = KWMPath.Home + "/kwmrc";
    }
    else
    {
        Fatal("Error: Failed to get environment variable 'HOME'");
    }

    GetKwmFilePath();
}

void KwmQuit()
{
    ShowAllScratchpadWindows();
    CloseBorder(&FocusedBorder);
    CloseBorder(&MarkedBorder);

    exit(0);
}

/* NOTE(koekeishiya): Returns true for operations that cause Kwm to exit. */
internal inline bool
ParseArguments(int argc, char **argv)
{
    int Option;
    const char *ShortOptions = "vc:";
    struct option LongOptions[] =
    {
        {"version", no_argument, NULL, 'v'},
        {"config", required_argument, NULL, 'c'},
        {NULL, 0, NULL, 0}
    };

    while((Option = getopt_long(argc, argv, ShortOptions, LongOptions, NULL)) != -1)
    {
        switch(Option)
        {
            case 'v':
            {
                printf("%s\n", KwmVersion);
                return true;
            } break;
            case 'c':
            {
                DEBUG("Notice: Using config file " << optarg);
                KWMPath.Config = optarg;
            } break;
        }
    }

    return false;
}

internal inline void
ConfigureRunLoop()
{
    KWMMach.EventMask = ((1 << kCGEventMouseMoved) |
                         (1 << kCGEventLeftMouseDragged) |
                         (1 << kCGEventLeftMouseDown) |
                         (1 << kCGEventLeftMouseUp) |
                         (1 << kCGEventRightMouseDragged) |
                         (1 << kCGEventRightMouseDown) |
                         (1 << kCGEventRightMouseUp));

    KWMMach.EventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, KWMMach.EventMask, CGEventCallback, NULL);
    if(!KWMMach.EventTap || !CGEventTapIsEnabled(KWMMach.EventTap))
        Fatal("Error: Could not create event-tap!");


    CFRunLoopAddSource(CFRunLoopGetMain(),
                       CFMachPortCreateRunLoopSource(kCFAllocatorDefault, KWMMach.EventTap, 0),
                       kCFRunLoopCommonModes);
}

int main(int argc, char **argv)
{
    if(ParseArguments(argc, argv))
        return 0;

    NSApplicationLoad();
    if(!AXLibDisplayHasSeparateSpaces())
        Fatal("Error: 'Displays have separate spaces' must be enabled!");

    if(!AXLibInit())
        Fatal("Error: Could not initialize AXLib!");

    AXLibStartEventLoop();
    if(!KwmStartDaemon())
        Fatal("Error: Could not start daemon!");

	OverlayLibInitialize();
	DEBUG("OverlayLib initialized!");

    ax_display *MainDisplay = AXLibMainDisplay();
    ax_display *Display = MainDisplay;
    do
    {
        ax_space *PrevSpace = Display->Space;
        Display->Space = AXLibGetActiveSpace(Display);
        Display->PrevSpace = PrevSpace;
        Display = AXLibNextDisplay(Display);
    } while(Display != MainDisplay);

    FocusedDisplay = MainDisplay;
    FocusedApplication = AXLibGetFocusedApplication();

    KwmInit();
    KwmParseConfig(KWMPath.Config);

    CreateWindowNodeTree(MainDisplay);

    /* TODO(koekeishiya): Probably want to defer this to run at some point where we know that
     * the focused application is set. This is usually the case as 'Finder' is always reported
     * as the active application when nothing is running. The following behaviour requries
     * refinement, because we will (sometimes ?) get NULL when started by launchd at login */
    if(FocusedApplication && FocusedApplication->Focus)
        UpdateBorder(&FocusedBorder, FocusedApplication->Focus);

    ConfigureRunLoop();
    CFRunLoopRun();
    return 0;
}
