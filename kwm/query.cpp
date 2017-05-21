#include "types.h"
#include "window.h"
#include "space.h"
#include "daemon.h"
#include "tree.h"
#include "node.h"

#include "../axlib/axlib.h"

#define internal static

extern std::map<std::string, space_info> WindowTree;
extern ax_window *MarkedWindow;

extern kwm_settings KWMSettings;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;
extern scratchpad Scratchpad;

internal std::string
GetSplitModeOfWindow(ax_window *Window)
{
    std::string Output;
    if(!Window)
        return "";

    ax_display *Display = AXLibWindowDisplay(Window);
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];

    tree_node *Node = GetTreeNodeFromWindowIDOrLinkNode(SpaceInfo->RootNode, Window->ID);
    if(Node)
    {
        if(Node->SplitMode == SPLIT_VERTICAL)
            Output = "Vertical";
        else if(Node->SplitMode == SPLIT_HORIZONTAL)
            Output = "Horizontal";
    }

    return Output;
}

EVENT_CALLBACK(Callback_KWMEvent_QueryTilingMode)
{
    int *SockFD = (int *) Event->Context;
    std::string Output;

    if(KWMSettings.Space == SpaceModeBSP)
        Output = "bsp";
    else if(KWMSettings.Space == SpaceModeMonocle)
        Output = "monocle";
    else
        Output = "float";

    printf("QueryTilingMode: %d\n", *SockFD);
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QuerySplitMode)
{
    int *SockFD = (int *) Event->Context;
    std::string Output;

    if(KWMSettings.SplitMode == SPLIT_OPTIMAL)
        Output = "Optimal";
    else if(KWMSettings.SplitMode == SPLIT_VERTICAL)
        Output = "Vertical";
    else if(KWMSettings.SplitMode == SPLIT_HORIZONTAL)
        Output = "Horizontal";

    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QuerySplitRatio)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = std::to_string(KWMSettings.SplitRatio);
    Output.erase(Output.find_last_not_of('0') + 1, std::string::npos);

    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QuerySpawnPosition)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = HasFlags(&KWMSettings, Settings_SpawnAsLeftChild) ? "left" : "right";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryFocusFollowsMouse)
{
    int *SockFD = (int *) Event->Context;
    std::string Output;

    if(KWMSettings.Focus == FocusModeAutoraise)
        Output = "autoraise";
    else if(KWMSettings.Focus == FocusModeDisabled)
        Output = "off";

    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryMouseFollowsFocus)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = HasFlags(&KWMSettings, Settings_MouseFollowsFocus) ? "on" : "off";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryCycleFocus)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = KWMSettings.Cycle == CycleModeScreen ? "screen" : "off";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryFloatNonResizable)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = HasFlags(&KWMSettings, Settings_FloatNonResizable) ? "on" : "off";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryLockToContainer)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = HasFlags(&KWMSettings, Settings_LockToContainer) ? "on" : "off";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryStandbyOnFloat)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = HasFlags(&KWMSettings, Settings_StandbyOnFloat) ? "on" : "off";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QuerySpaces)
{
    int *SockFD = (int *) Event->Context;

    std::string Output;
    ax_display *Display = AXLibMainDisplay();
    if(Display)
    {
        int SubtractIndex = 0;
        int TotalSpaces = AXLibDisplaySpacesCount(Display);
        for(int SpaceID = 1; SpaceID <= TotalSpaces; ++SpaceID)
        {
            int CGSSpaceID = AXLibCGSSpaceIDFromDesktopID(Display, SpaceID);
            std::map<int, ax_space>::iterator It = Display->Spaces.find(CGSSpaceID);
            if(It != Display->Spaces.end())
            {
                if(It->second.Type == kCGSSpaceUser)
                {
                    std::string Name = GetNameOfSpace(Display, &It->second);
                    Output += std::to_string(SpaceID - SubtractIndex) + ", " + Name;
                    if(SpaceID < TotalSpaces && SpaceID < Display->Spaces.size())
                        Output += "\n";
                }
                else
                {
                    ++SubtractIndex;
                }
            }
        }

        if(Output[Output.size()-1] == '\n')
            Output.erase(Output.begin() + Output.size()-1);
    }

    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryCurrentSpaceName)
{
    int *SockFD = (int *) Event->Context;
    std::string Output;

    ax_display *Display = AXLibMainDisplay();
    Output = GetNameOfSpace(Display, Display->Space);

    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryPreviousSpaceName)
{
    int *SockFD = (int *) Event->Context;

    std::string Output;
    ax_display *Display = AXLibMainDisplay();
    if(Display)
        Output = GetNameOfSpace(Display, Display->PrevSpace);

    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryCurrentSpaceMode)
{
    int *SockFD = (int *) Event->Context;

    std::string Output;
    ax_window *Window = NULL;
    ax_application *Application = AXLibGetFocusedApplication();
    if(Application)
        Window = Application->Focus;

    GetTagForCurrentSpace(Output, Window);
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryCurrentSpaceTag)
{
    int *SockFD = (int *) Event->Context;
    std::string Output;

    ax_application *Application = AXLibGetFocusedApplication();
    if(Application)
    {
        ax_window *Window = Application->Focus;
        GetTagForCurrentSpace(Output, Window);
        Output += " " + Application->Name;
        if(Window && Window->Name)
            Output += " - " + std::string(Window->Name);
    }
    else
    {
        GetTagForCurrentSpace(Output, NULL);
    }


    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryCurrentSpaceId)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = "-1";
    ax_display *Display = AXLibMainDisplay();
    if(Display)
        Output = std::to_string(AXLibDesktopIDFromCGSSpaceID(Display, Display->Space->ID));

    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryPreviousSpaceId)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = "-1";
    ax_display *Display = AXLibMainDisplay();
    if(Display)
        Output = std::to_string(AXLibDesktopIDFromCGSSpaceID(Display, Display->PrevSpace->ID));

    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedBorder)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = FocusedBorder.Enabled ? "true" : "false";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedBorder)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = MarkedBorder.Enabled ? "true" : "false";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedWindowId)
{
    int *SockFD = (int *) Event->Context;


    ax_application *Application = AXLibGetFocusedApplication();
    std::string Output = Application && Application->Focus ? std::to_string(Application->Focus->ID) : "-1";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedWindowOwner)
{
    int *SockFD = (int *) Event->Context;


    ax_application *Application = AXLibGetFocusedApplication();
    std::string Output = Application && Application->Focus ? Application->Name : "";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedWindowName)
{
    int *SockFD = (int *) Event->Context;


    ax_application *Application = AXLibGetFocusedApplication();
    std::string Output = Application && Application->Focus ? Application->Focus->Name : "";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedWindowSplit)
{
    int *SockFD = (int *) Event->Context;

    ax_application *Application = AXLibGetFocusedApplication();
    std::string Output = Application ? GetSplitModeOfWindow(Application->Focus) : "";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedWindowFloat)
{
    int *SockFD = (int *) Event->Context;

    ax_application *Application = AXLibGetFocusedApplication();
    std::string Output = Application && Application->Focus ? (AXLibHasFlags(Application->Focus, AXWindow_Floating) ? "true" : "false") : "false";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedWindowId)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = MarkedWindow ? std::to_string(MarkedWindow->ID) : "-1";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedWindowOwner)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = MarkedWindow && MarkedWindow->Application ? MarkedWindow->Application->Name : "";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedWindowName)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = MarkedWindow && MarkedWindow->Name ? MarkedWindow->Name : "";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedWindowSplit)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = GetSplitModeOfWindow(MarkedWindow);
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedWindowFloat)
{
    int *SockFD = (int *) Event->Context;

    std::string Output = MarkedWindow ? (AXLibHasFlags(MarkedWindow, AXWindow_Floating) ? "true" : "false") : "";
    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryWindowList)
{
    int *SockFD = (int *) Event->Context;

    std::string Output;
    std::vector<ax_window *> Windows = AXLibGetAllVisibleWindows();
    for(std::size_t Index = 0; Index < Windows.size(); ++Index)
    {
        ax_window *Window = Windows[Index];
        Output += std::to_string(Window->ID) + ", " + Window->Application->Name;
        if(Window->Name)
            Output +=  ", " + std::string(Window->Name);
        if(Index < Windows.size() - 1)
            Output += "\n";
    }

    KwmWriteToSocket(Output, *SockFD);
    free(SockFD);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryNodePosition)
{
    int *Args = (int *) Event->Context;
    int SockFD = *(Args + 0);
    int WindowID = *(Args + 1);

    std::string Output;
    ax_display *Display = AXLibMainDisplay();
    if(Display)
    {
        space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
        tree_node *Node = GetTreeNodeFromWindowIDOrLinkNode(SpaceInfo->RootNode, WindowID);
        if(Node)
            Output = IsLeftChild(Node) ? "left" : "right";
    }

    KwmWriteToSocket(Output, SockFD);
    free(Args);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryParentNodeState)
{
    int *Args = (int *) Event->Context;
    int SockFD = *(Args + 0);
    int FirstID = *(Args + 1);
    int SecondID = *(Args + 2);

    std::string Output = "false";
    ax_display *Display = AXLibMainDisplay();
    if(Display)
    {
        space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
        tree_node *FirstNode = GetTreeNodeFromWindowIDOrLinkNode(SpaceInfo->RootNode, FirstID);
        tree_node *SecondNode = GetTreeNodeFromWindowIDOrLinkNode(SpaceInfo->RootNode, SecondID);
        if(FirstNode && SecondNode)
            Output = SecondNode->Parent == FirstNode->Parent ? "true" : "false";
    }

    KwmWriteToSocket(Output, SockFD);
    free(Args);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryWindowIdInDirectionOfFocusedWindow)
{
    int *Args = (int *) Event->Context;
    int SockFD = *(Args + 0);
    int Degrees = *(Args + 1);

    ax_window *ClosestWindow = NULL;
    std::string Output = "-1";

    if(FindClosestWindow(Degrees, &ClosestWindow, true))
        Output = std::to_string(ClosestWindow->ID);

    KwmWriteToSocket(Output, SockFD);
    free(Args);
}

EVENT_CALLBACK(Callback_KWMEvent_QueryScratchpad)
{
    int *SockFD = (int *) Event->Context;
    std::string Result;

    int Index = 0;
    std::map<int, ax_window*>::iterator It;
    for(It = Scratchpad.Windows.begin(); It != Scratchpad.Windows.end(); ++It)
    {
        Result += std::to_string(It->first) + ": " +
                  std::to_string(It->second->ID) + ", " +
                  It->second->Application->Name + ", " +
                  It->second->Name;

        if(Index++ < Scratchpad.Windows.size() - 1)
            Result += "\n";
    }

    KwmWriteToSocket(Result, *SockFD);
    free(SockFD);
}
