#include "space.h"
#include "display.h"
#include "window.h"
#include "tree.h"
#include "border.h"
#include "keys.h"
#include "helpers.h"

#include "axlib/axlib.h"
extern std::map<CFStringRef, space_info> WindowTree;
extern ax_application *FocusedApplication;

extern kwm_mach KWMMach;
extern kwm_tiling KWMTiling;
extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_toggles KWMToggles;
extern kwm_mode KWMMode;
extern kwm_thread KWMThread;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;

extern void AddWindowToSpace(int CGSpaceID, int WindowID);
extern void RemoveWindowFromSpace(int CGSpaceID, int WindowID);

/* NOTE(koekeishiya): Causes segfault if this function is called exactly as the window is destroyed.
 *                    We probably have to use a lock on FocusedApplication->Focus */
void GetTagForMonocleSpace(space_info *Space, std::string &Tag)
{
    tree_node *Node = Space->RootNode;
    bool FoundFocusedWindow = false;
    int FocusedIndex = 0;
    int NumberOfWindows = 0;

    if(Node && FocusedApplication && FocusedApplication->Focus)
    {
        link_node *Link = Node->List;
        while(Link)
        {
            if(!FoundFocusedWindow)
                ++FocusedIndex;

            if(Link->WindowID == FocusedApplication->Focus->ID)
                FoundFocusedWindow = true;

            ++NumberOfWindows;
            Link = Link->Next;
        }
    }

    if(FoundFocusedWindow)
        Tag = "[" + std::to_string(FocusedIndex) + "/" + std::to_string(NumberOfWindows) + "]";
    else
        Tag = "[" + std::to_string(NumberOfWindows) + "]";
}

void GetTagForCurrentSpace(std::string &Tag)
{
    ax_display *Display = AXLibMainDisplay();
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->Initialized)
    {
        if(SpaceInfo->Settings.Mode == SpaceModeBSP)
            Tag = "[bsp]";
        else if(SpaceInfo->Settings.Mode == SpaceModeFloating)
            Tag = "[float]";
        else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
            GetTagForMonocleSpace(SpaceInfo, Tag);
    }
    else
    {
        if(KWMMode.Space == SpaceModeBSP)
            Tag = "[bsp]";
        else if(KWMMode.Space == SpaceModeFloating)
            Tag = "[float]";
        else if(KWMMode.Space == SpaceModeMonocle)
            Tag = "[monocle]";
    }
}

void GoToPreviousSpace(bool MoveFocusedWindow)
{
    /*
    if(IsSpaceTransitionInProgress())
        return;

    Assert(KWMScreen.Current);
    if(!KWMScreen.Current->History.empty())
    {
        int CGSpaceID = KWMScreen.Current->History.top();
        KWMScreen.Current->History.pop();

        int Workspace = GetSpaceNumberFromCGSpaceID(KWMScreen.Current, CGSpaceID);
        std::string WorkspaceStr = std::to_string(Workspace);
        KWMScreen.Current->TrackSpaceChange = false;

        if(MoveFocusedWindow)
            MoveFocusedWindowToSpace(WorkspaceStr);
        else
            KwmEmitKeystroke(KWMHotkeys.SpacesKey, WorkspaceStr);
    }
    */
}

bool IsActiveSpaceFloating()
{
    return IsSpaceFloating(KWMScreen.Current->ActiveSpace);
}

bool IsSpaceFloating(int SpaceID)
{
    bool Result = false;

    if(IsSpaceInitializedForScreen(KWMScreen.Current))
    {
        std::map<int, space_info>::iterator It = KWMScreen.Current->Space.find(SpaceID);
        if(It != KWMScreen.Current->Space.end())
            Result = KWMScreen.Current->Space[SpaceID].Settings.Mode == SpaceModeFloating;
    }

    return Result;
}

space_info *GetActiveSpaceOfScreen(screen_info *Screen) { }

bool IsSpaceInitializedForScreen(screen_info *Screen)
{
    if(!Screen)
        return false;

    std::map<int, space_info>::iterator It = Screen->Space.find(Screen->ActiveSpace);
    if(It == Screen->Space.end())
        return false;
    else
        return It->second.Initialized;
}

bool DoesSpaceExistInMapOfScreen(screen_info *Screen)
{
    if(!Screen)
        return false;

    std::map<int, space_info>::iterator It = Screen->Space.find(Screen->ActiveSpace);
    if(It == Screen->Space.end())
        return false;
    else
        return It->second.RootNode != NULL && It->second.Initialized;
}

bool IsSpaceTransitionInProgress()
{
    bool Result = false;
    std::map<unsigned int, screen_info>::iterator It;
    for(It = KWMTiling.DisplayMap.begin(); It != KWMTiling.DisplayMap.end(); ++It)
    {
        screen_info *Screen = &It->second;
        Result = Result || CGSManagedDisplayIsAnimating(CGSDefaultConnection, Screen->Identifier);
    }

    if(Result)
    {
        DEBUG("IsSpaceTransitionInProgress() Space transition detected");
        if(!KWMMach.DisableEventTapInternal)
        {
            KWMMach.DisableEventTapInternal = true;
            CGEventTapEnable(KWMMach.EventTap, false);
            KWMScreen.Transitioning = true;
        }
    }
    else if(KWMMach.DisableEventTapInternal)
    {
        KWMMach.DisableEventTapInternal = false;
        CGEventTapEnable(KWMMach.EventTap, true);
    }

    return Result;
}

bool IsActiveSpaceManaged()
{
    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    return Space->Managed;
}

void ShouldActiveSpaceBeManaged()
{
    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    Space->Managed = CGSSpaceGetType(CGSDefaultConnection, KWMScreen.Current->ActiveSpace) == CGSSpaceTypeUser;
}

void FloatFocusedSpace()
{
    if(KWMToggles.EnableTilingMode &&
       !IsSpaceTransitionInProgress() &&
       IsActiveSpaceManaged())
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        if(Space->Settings.Mode == SpaceModeFloating)
            return;

        DestroyNodeTree(Space->RootNode);
        Space->RootNode = NULL;

        Space->Settings.Mode = SpaceModeFloating;
        Space->Initialized = true;
        Space->NeedsUpdate = false;
        ClearFocusedWindow();
    }
}

void TileFocusedSpace(space_tiling_option Mode)
{
    if(KWMToggles.EnableTilingMode &&
       !IsSpaceTransitionInProgress() &&
       IsActiveSpaceManaged() &&
       FilterWindowList(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        if(Space->Settings.Mode == Mode)
            return;

        DestroyNodeTree(Space->RootNode);
        Space->RootNode = NULL;

        Space->Settings.Mode = Mode;
        std::vector<window_info*> WindowsOnDisplay = GetAllWindowsOnDisplay(KWMScreen.Current->ID);
        CreateWindowNodeTree(KWMScreen.Current, &WindowsOnDisplay);
    }
}

void UpdateActiveSpace()
{
    /*
    ClearFocusedWindow();
    ClearMarkedWindow();

    Assert(KWMScreen.Current);

    if(KWMTiling.KwmOverlay[0] != 0)
        RemoveWindowFromSpace(KWMScreen.Current->ActiveSpace, KWMTiling.KwmOverlay[0]);
    if(KWMTiling.KwmOverlay[1] != 0)
        RemoveWindowFromSpace(KWMScreen.Current->ActiveSpace, KWMTiling.KwmOverlay[1]);

    KWMScreen.PrevSpace = KWMScreen.Current->ActiveSpace;
    KWMScreen.Current->ActiveSpace = GetActiveSpaceOfDisplay(KWMScreen.Current);
    ShouldActiveSpaceBeManaged();
    AXLibRunningApplications();

    space_info *Space = NULL;
    if(KWMScreen.PrevSpace != KWMScreen.Current->ActiveSpace)
    {
        DEBUG("UpdateActiveSpace() Space transition ended " << KWMScreen.PrevSpace << " -> " << KWMScreen.Current->ActiveSpace);
        if(KWMScreen.Current->TrackSpaceChange)
            KWMScreen.Current->History.push(KWMScreen.PrevSpace);
        else
            KWMScreen.Current->TrackSpaceChange = true;

        Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        UpdateActiveWindowList(KWMScreen.Current);

        if(Space->NeedsUpdate)
            UpdateSpaceOfScreen(Space, KWMScreen.Current);

        if(KWMScreen.Current->RestoreFocus &&
           Space->FocusedWindowID != -1)
        {
            FocusWindowByID(Space->FocusedWindowID);
            MoveCursorToCenterOfFocusedWindow();
        }
        else
        {
            FocusWindowOfOSX();
            MoveCursorToCenterOfFocusedWindow();
            KWMScreen.Current->RestoreFocus = true;
        }
    }
    else
    {
        std::map<unsigned int, screen_info>::iterator It;
        for(It = KWMTiling.DisplayMap.begin(); It != KWMTiling.DisplayMap.end(); ++It)
        {
            screen_info *Screen = &It->second;
            if(Screen->ID == KWMScreen.Current->ID)
                continue;

            int ScreenCurrentSpace = Screen->ActiveSpace;
            int ScreenNewSpace = GetActiveSpaceOfDisplay(Screen);
            if(ScreenCurrentSpace != ScreenNewSpace)
            {
                DEBUG("space changed on monitor: " << Screen->ID);

                Screen->History.push(Screen->ActiveSpace);
                Screen->ActiveSpace = ScreenNewSpace;
                KWMScreen.PrevSpace = KWMScreen.Current->ActiveSpace;
                KWMScreen.Current = Screen;
                ShouldActiveSpaceBeManaged();
                Space = GetActiveSpaceOfScreen(Screen);
                UpdateActiveWindowList(Screen);
                FilterWindowList(Screen);
                break;
            }
        }
    }

    if(KWMTiling.KwmOverlay[0] != 0)
        AddWindowToSpace(KWMScreen.Current->ActiveSpace, KWMTiling.KwmOverlay[0]);
    if(KWMTiling.KwmOverlay[1] != 0)
        AddWindowToSpace(KWMScreen.Current->ActiveSpace, KWMTiling.KwmOverlay[1]);
    */
}

space_settings *GetSpaceSettingsForDesktopID(int ScreenID, int DesktopID)
{
    space_identifier Lookup = { ScreenID, DesktopID };
    std::map<space_identifier, space_settings>::iterator It = KWMTiling.SpaceSettings.find(Lookup);
    if(It != KWMTiling.SpaceSettings.end())
        return &It->second;
    else
        return NULL;
}

int GetSpaceFromName(ax_display *Display, std::string Name)
{
    std::map<CGSSpaceID, ax_space>::iterator It;
    for(It = Display->Spaces.begin(); It != Display->Spaces.end(); ++It)
    {
        ax_space *Space = &It->second;
        space_info *SpaceInfo = &WindowTree[Space->Identifier];
        if(SpaceInfo->Settings.Name == Name)
            return Space->ID;
    }

    return -1;
}

void SetNameOfActiveSpace(screen_info *Screen, std::string Name) {}

void SetNameOfActiveSpace(ax_display *Display, std::string Name)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo) SpaceInfo->Settings.Name = Name;
}

std::string GetNameOfSpace(screen_info *Screen, int CGSpaceID) { }

std::string GetNameOfSpace(ax_display *Display)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    std::string Result = "[no tag]";

    if(!SpaceInfo->Settings.Name.empty())
        Result = SpaceInfo->Settings.Name;

    return Result;
}

void ActivateSpaceWithoutTransition(std::string SpaceID)
{
    ax_display *Display = AXLibMainDisplay();
    if(Display)
    {
        int TotalSpaces = AXLibDisplaySpacesCount(Display);
        int ActiveSpace = AXLibDesktopIDFromCGSSpaceID(Display, Display->Space->ID);
        int DestinationSpaceID = ActiveSpace;
        if(SpaceID == "left")
        {
            DestinationSpaceID = ActiveSpace > 1 ? ActiveSpace-1 : 1;
        }
        else if(SpaceID == "right")
        {
            DestinationSpaceID = ActiveSpace < TotalSpaces ? ActiveSpace+1 : TotalSpaces;
        }
        else
        {
            int LookupSpace = GetSpaceFromName(Display, SpaceID);
            if(LookupSpace != -1)
                DestinationSpaceID = AXLibDesktopIDFromCGSSpaceID(Display, LookupSpace);
            else
                DestinationSpaceID = std::atoi(SpaceID.c_str());
        }

        if(DestinationSpaceID != ActiveSpace &&
           DestinationSpaceID > 0 && DestinationSpaceID <= TotalSpaces)
        {
            int CGSSpaceID = AXLibCGSSpaceIDFromDesktopID(Display, DestinationSpaceID);
            AXLibSpaceTransition(Display, CGSSpaceID);
        }
    }
}

void MoveWindowBetweenSpaces(ax_display *Display, int SourceSpaceID, int DestinationSpaceID, ax_window *Window)
{
    int SourceCGSpaceID = AXLibCGSSpaceIDFromDesktopID(Display, SourceSpaceID);
    int DestinationCGSpaceID = AXLibCGSSpaceIDFromDesktopID(Display, DestinationSpaceID);
    RemoveWindowFromNodeTree(Display, Window->ID);
    AXLibAddFlags(Window, AXWindow_Minimized);
    ax_space *Space = &Display->Spaces[DestinationCGSpaceID];
    AXLibAddFlags(Space, AXSpace_NeedsUpdate);
    AXLibSpaceRemoveWindow(SourceCGSpaceID, Window->ID);
    AXLibSpaceAddWindow(DestinationCGSpaceID, Window->ID);
}

void MoveFocusedWindowToSpace(std::string SpaceID)
{
    ax_window *Window = FocusedApplication->Focus;
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    int TotalSpaces = AXLibDisplaySpacesCount(Display);
    int ActiveSpace = AXLibDesktopIDFromCGSSpaceID(Display, Display->Space->ID);
    int DestinationSpaceID = ActiveSpace;
    if(SpaceID == "left")
    {
        DestinationSpaceID = ActiveSpace > 1 ? ActiveSpace-1 : 1;
    }
    else if(SpaceID == "right")
    {
        DestinationSpaceID = ActiveSpace < TotalSpaces ? ActiveSpace+1 : TotalSpaces;
    }
    else
    {
        int LookupSpace = GetSpaceFromName(Display, SpaceID);
        if(LookupSpace != -1)
            DestinationSpaceID = AXLibDesktopIDFromCGSSpaceID(Display, LookupSpace);
        else
            DestinationSpaceID = std::atoi(SpaceID.c_str());
    }

    MoveWindowBetweenSpaces(Display, ActiveSpace, DestinationSpaceID, Window);
}
