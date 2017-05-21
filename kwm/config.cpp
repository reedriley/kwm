#include "config.h"
#include "tokenizer.h"
#include "interpreter.h"
#include "rules.h"
#include "helpers.h"
#include "keys.h"

#include "daemon.h"
#include "display.h"
#include "space.h"
#include "window.h"
#include "container.h"
#include "node.h"
#include "tree.h"
#include "border.h"
#include "serializer.h"
#include "scratchpad.h"
#include "cursor.h"
#include "event.h"
#include "../axlib/axlib.h"

#define internal static
#define INVALID_SOCKFD -1
internal int ClientSockFD = INVALID_SOCKFD;

extern std::map<std::string, space_info> WindowTree;
extern ax_application *FocusedApplication;
extern ax_window *MarkedWindow;

extern kwm_settings KWMSettings;;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;

extern kwm_path KWMPath;
extern kwm_settings KWMSettings;

internal inline void
ReportInvalidCommand(std::string Command)
{
    if(ClientSockFD != INVALID_SOCKFD)
        KwmWriteToSocket(Command, ClientSockFD);
    else
        std::cerr << "Parse error: " << Command << std::endl;
}

internal void
KwmParseConfigOptionTiling(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "bsp"))
        KWMSettings.Space = SpaceModeBSP;
    else if(TokenEquals(Token, "monocle"))
        KWMSettings.Space = SpaceModeMonocle;
    else if(TokenEquals(Token, "float"))
        KWMSettings.Space = SpaceModeFloating;
    else
        ReportInvalidCommand("Unknown command 'config tiling " + std::string(Token.Text, Token.TextLength) + "'");
}

internal void
KwmParseConfigOptionPadding(tokenizer *Tokenizer)
{
    bool IsValid = true;
    token TokenTop = GetToken(Tokenizer);
    token TokenBottom = GetToken(Tokenizer);
    token TokenLeft = GetToken(Tokenizer);
    token TokenRight = GetToken(Tokenizer);

    if(TokenTop.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config padding top value '" + std::string(TokenTop.Text, TokenTop.TextLength) + "'");
        IsValid = false;
    }
    if(TokenBottom.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config padding bottom value '" + std::string(TokenBottom.Text, TokenBottom.TextLength) + "'");
        IsValid = false;
    }
    if(TokenLeft.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config padding left value '" + std::string(TokenLeft.Text, TokenLeft.TextLength) + "'");
        IsValid = false;
    }
    if(TokenRight.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config padding right value '" + std::string(TokenRight.Text, TokenRight.TextLength) + "'");
        IsValid = false;
    }

    if(IsValid)
    {
        container_offset Offset = { ConvertStringToDouble(std::string(TokenTop.Text, TokenTop.TextLength)),
                                    ConvertStringToDouble(std::string(TokenBottom.Text, TokenBottom.TextLength)),
                                    ConvertStringToDouble(std::string(TokenLeft.Text, TokenLeft.TextLength)),
                                    ConvertStringToDouble(std::string(TokenRight.Text, TokenRight.TextLength)),
                                    0,
                                    0
                                  };

        SetDefaultPaddingOfDisplay(Offset);
    }
}

internal void
KwmParseConfigOptionGap(tokenizer *Tokenizer)
{
    bool IsValid = true;
    token TokenVertical = GetToken(Tokenizer);
    token TokenHorizontal = GetToken(Tokenizer);

    if(TokenVertical.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config gap vertical value '" + std::string(TokenVertical.Text, TokenVertical.TextLength) + "'");
        IsValid = false;
    }
    if(TokenHorizontal.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown config gap horizontal value '" + std::string(TokenHorizontal.Text, TokenHorizontal.TextLength) + "'");
        IsValid = false;
    }

    if(IsValid)
    {
        container_offset Offset = { 0,
                                    0,
                                    0,
                                    0,
                                    ConvertStringToDouble(std::string(TokenVertical.Text, TokenVertical.TextLength)),
                                    ConvertStringToDouble(std::string(TokenHorizontal.Text, TokenHorizontal.TextLength))
                                  };

        SetDefaultGapOfDisplay(Offset);
    }
}

internal void
KwmParseConfigOptionFocusFollowsMouse(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "follows"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "mouse"))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "on"))
                        KWMSettings.Focus = FocusModeAutoraise;
                    else if(TokenEquals(Token, "off"))
                        KWMSettings.Focus = FocusModeDisabled;
                    else if(TokenEquals(Token, "toggle"))
                    {
                        if(KWMSettings.Focus == FocusModeDisabled)
                            KWMSettings.Focus = FocusModeAutoraise;
                        else if(KWMSettings.Focus == FocusModeAutoraise)
                            KWMSettings.Focus = FocusModeDisabled;
                    }
                    else
                        ReportInvalidCommand("Unknown command 'config focus-follows-mouse " + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                    ReportInvalidCommand("Unknown command 'config focus-follows-" + std::string(Token.Text, Token.TextLength) + "'");
            }
            else
                ReportInvalidCommand("Expected token '-' after 'config focus-follows'");
        }
        else
        {
            ReportInvalidCommand("Unknown command 'config focus-" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config focus'");
    }
}

internal void
KwmParseConfigOptionMouse(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "follows"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "focus"))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "on"))
                        AddFlags(&KWMSettings, Settings_MouseFollowsFocus);
                    else if(TokenEquals(Token, "off"))
                        ClearFlags(&KWMSettings, Settings_MouseFollowsFocus);
                    else
                        ReportInvalidCommand("Unknown command 'config mouse-follows-focus " + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                    ReportInvalidCommand("Unknown command 'config mouse-follows-" + std::string(Token.Text, Token.TextLength) + "'");
            }
            else
                ReportInvalidCommand("Expected token '-' after 'config mouse-follows'");
        }
        else if(TokenEquals(Token, "drag"))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "on"))
                AddFlags(&KWMSettings, Settings_MouseDrag);
            else if(TokenEquals(Token, "off"))
                ClearFlags(&KWMSettings, Settings_MouseDrag);
            else if(TokenEquals(Token, "mod"))
                KwmSetMouseDragKey(GetTextTilEndOfLine(Tokenizer));
            else
                ReportInvalidCommand("Unknown command 'config mouse-drag " + std::string(Token.Text, Token.TextLength) + "'");
        }
        else
        {
            ReportInvalidCommand("Unknown command 'config mouse-" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config mouse'");
    }
}

internal void
KwmParseConfigOptionStandbyOnFloat(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "on"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "float"))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "on"))
                        AddFlags(&KWMSettings, Settings_StandbyOnFloat);
                    else if(TokenEquals(Token, "off"))
                        ClearFlags(&KWMSettings, Settings_StandbyOnFloat);
                    else
                        ReportInvalidCommand("Unknown command 'config standby-on-float " + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                    ReportInvalidCommand("Unknown command 'config standby-on-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Unknown command 'config standby-" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config standby'");
    }
}

internal void
KwmParseConfigOptionCenterOnFloat(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "on"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "float"))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "on"))
                        AddFlags(&KWMSettings, Settings_CenterOnFloat);
                    else if(TokenEquals(Token, "off"))
                        ClearFlags(&KWMSettings, Settings_CenterOnFloat);
                    else
                        ReportInvalidCommand("Unknown command 'config center-on-float " + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                    ReportInvalidCommand("Unknown command 'config center-on-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Unknown command 'config center-" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config center'");
    }
}

internal void
KwmParseConfigOptionFloatNonResizable(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "non"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "resizable"))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "on"))
                        AddFlags(&KWMSettings, Settings_FloatNonResizable);
                    else if(TokenEquals(Token, "off"))
                        ClearFlags(&KWMSettings, Settings_FloatNonResizable);
                    else
                        ReportInvalidCommand("Unknown command 'config float-non-resizable " + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                    ReportInvalidCommand("Unknown command 'config float-non-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Unknown command 'config float-" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config float'");
    }
}

internal void
KwmParseConfigOptionLockToContainer(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "to"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "container"))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "on"))
                        AddFlags(&KWMSettings, Settings_LockToContainer);
                    else if(TokenEquals(Token, "off"))
                        ClearFlags(&KWMSettings, Settings_LockToContainer);
                    else
                        ReportInvalidCommand("Unknown command 'config lock-to-container " + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                    ReportInvalidCommand("Unknown command 'config lock-to-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Unknown command 'config lock-" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config lock'");
    }
}

internal void
KwmParseConfigOptionCycleFocus(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "focus"))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "on"))
                KWMSettings.Cycle = CycleModeScreen;
            else if(TokenEquals(Token, "off"))
                KWMSettings.Cycle = CycleModeDisabled;
            else
                ReportInvalidCommand("Unknown command 'config cycle-focus " + std::string(Token.Text, Token.TextLength) + "'");
        }
        else
            ReportInvalidCommand("Unknown command 'config cycle-" + std::string(Token.Text, Token.TextLength) + "'");
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config cycle'");
    }
}

internal void
KwmParseConfigOptionSplitRatio(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "ratio"))
        {
            token Token = GetToken(Tokenizer);
            switch(Token.Type)
            {
                case Token_Digit:
                {
                    double Value = ConvertStringToDouble(std::string(Token.Text, Token.TextLength));
                    if(Value > 0.0 && Value < 1.0)
                    {
                        KWMSettings.SplitRatio = Value;
                    }
                } break;
                default:
                {
                    ReportInvalidCommand("Unknown command 'config split-ratio " + std::string(Token.Text, Token.TextLength) + "'");
                } break;
            }
        }
        else
            ReportInvalidCommand("Unknown command 'config split-" + std::string(Token.Text, Token.TextLength) + "'");
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config cycle'");
    }
}

internal void
KwmParseConfigOptionOptimalRatio(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "ratio"))
        {
            token Token = GetToken(Tokenizer);
            switch(Token.Type)
            {
                case Token_Digit:
                {
                    KWMSettings.OptimalRatio = ConvertStringToDouble(std::string(Token.Text, Token.TextLength));
                } break;
                default:
                {
                    ReportInvalidCommand("Unknown command 'config optimal-ratio " + std::string(Token.Text, Token.TextLength) + "'");
                } break;
            }
        }
        else
            ReportInvalidCommand("Unknown command 'config optimal-" + std::string(Token.Text, Token.TextLength) + "'");
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'config optimal'");
    }
}

internal void
KwmParseConfigOptionSpawn(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "left"))
        AddFlags(&KWMSettings, Settings_SpawnAsLeftChild);
    else if(TokenEquals(Token, "right"))
        ClearFlags(&KWMSettings, Settings_SpawnAsLeftChild);
    else
        ReportInvalidCommand("Unknown command 'config spawn " + std::string(Token.Text, Token.TextLength) + "'");
}

internal void
KwmParseConfigOptionBorder(tokenizer *Tokenizer)
{
    token TokenBorder = GetToken(Tokenizer);
    if(TokenEquals(TokenBorder, "focused"))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "on"))
        {
            FocusedBorder.Enabled = true;
            UpdateBorder(&FocusedBorder, FocusedApplication->Focus);
        }
        else if(TokenEquals(Token, "off"))
        {
            FocusedBorder.Enabled = false;
            CloseBorder(&FocusedBorder);
        }
        else if(TokenEquals(Token, "size"))
        {
            token Token = GetToken(Tokenizer);
            switch(Token.Type)
            {
                case Token_Digit:
                {
                    FocusedBorder.Width = ConvertStringToInt(std::string(Token.Text, Token.TextLength));
                } break;
                default:
                {
                    std::string BorderSize(Token.Text, Token.TextLength);
                    ReportInvalidCommand("Unknown command 'config border focused size " + BorderSize + "'");
                } break;
            }
        }
        else if(TokenEquals(Token, "radius"))
        {
            token Token = GetToken(Tokenizer);
            switch(Token.Type)
            {
                case Token_Digit:
                {
                    FocusedBorder.Radius = ConvertStringToDouble(std::string(Token.Text, Token.TextLength));
                } break;
                default:
                {
                    std::string BorderSize(Token.Text, Token.TextLength);
                    ReportInvalidCommand("Unknown command 'config border focused radius " + BorderSize + "'");
                } break;
            }
        }
        else if(TokenEquals(Token, "color"))
        {
            token Token = GetToken(Tokenizer);
            std::string BorderColor(Token.Text, Token.TextLength);
            FocusedBorder.Color = ConvertHexRGBAToColor(ConvertHexStringToInt(BorderColor));
            if(FocusedApplication && FocusedApplication->Focus)
                UpdateBorder(&FocusedBorder, FocusedApplication->Focus);
        }
    }
    else if(TokenEquals(TokenBorder, "marked"))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "on"))
        {
            MarkedBorder.Enabled = true;
        }
        else if(TokenEquals(Token, "off"))
        {
            MarkedBorder.Enabled = false;
            CloseBorder(&MarkedBorder);
        }
        else if(TokenEquals(Token, "size"))
        {
            token Token = GetToken(Tokenizer);
            switch(Token.Type)
            {
                case Token_Digit:
                {
                    MarkedBorder.Width = ConvertStringToInt(std::string(Token.Text, Token.TextLength));
                } break;
                default:
                {
                    std::string BorderSize(Token.Text, Token.TextLength);
                    ReportInvalidCommand("Unknown command 'config border marked size " + BorderSize + "'");
                } break;
            }
        }
        else if(TokenEquals(Token, "radius"))
        {
            token Token = GetToken(Tokenizer);
            switch(Token.Type)
            {
                case Token_Digit:
                {
                    MarkedBorder.Radius = ConvertStringToDouble(std::string(Token.Text, Token.TextLength));
                } break;
                default:
                {
                    std::string BorderSize(Token.Text, Token.TextLength);
                    ReportInvalidCommand("Unknown command 'config border marked radius " + BorderSize + "'");
                } break;
            }
        }
        else if(TokenEquals(Token, "color"))
        {
            token Token = GetToken(Tokenizer);
            std::string BorderColor(Token.Text, Token.TextLength);
            MarkedBorder.Color = ConvertHexRGBAToColor(ConvertHexStringToInt(BorderColor));
        }
    }
    else
    {
        ReportInvalidCommand("Unknown command 'config border " + std::string(TokenBorder.Text, TokenBorder.TextLength) + "'");
    }
}

internal void
KwmParseConfigOptionSpace(tokenizer *Tokenizer)
{
    token TokenDisplay = GetToken(Tokenizer);
    std::string Display(TokenDisplay.Text, TokenDisplay.TextLength);
    if(TokenDisplay.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown command 'config space " + Display + "'");
        return;
    }

    token TokenSpace = GetToken(Tokenizer);
    std::string Space(TokenSpace.Text, TokenSpace.TextLength);
    if(TokenSpace.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown command 'config space " + Display + " " + Space + "'");
        return;
    }

    int ScreenID = ConvertStringToInt(Display);
    int DesktopID = ConvertStringToInt(Space);
    space_settings *SpaceSettings = GetSpaceSettingsForDesktopID(ScreenID, DesktopID);
    if(!SpaceSettings)
    {
        space_identifier Lookup = { ScreenID, DesktopID };
        space_settings NULLSpaceSettings = { KWMSettings.DefaultOffset, SpaceModeDefault, {0, 0}, "", ""};

        space_settings *ScreenSettings = GetSpaceSettingsForDisplay(ScreenID);
        if(ScreenSettings)
            NULLSpaceSettings = *ScreenSettings;

        KWMSettings.SpaceSettings[Lookup] = NULLSpaceSettings;
        SpaceSettings = &KWMSettings.SpaceSettings[Lookup];
    }

    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "mode"))
    {
        token Token = GetToken(Tokenizer);
        std::string Mode(Token.Text, Token.TextLength);
        if(TokenEquals(Token, "bsp"))
            SpaceSettings->Mode = SpaceModeBSP;
        else if(TokenEquals(Token, "monocle"))
            SpaceSettings->Mode = SpaceModeMonocle;
        else if(TokenEquals(Token, "float"))
            SpaceSettings->Mode = SpaceModeFloating;
        else
            ReportInvalidCommand("Unknown command 'config space " + Display + " " + Space + " mode " + Mode + "'");
    }
    else if(TokenEquals(Token, "padding"))
    {
        bool IsValid = true;
        token TokenTop = GetToken(Tokenizer);
        token TokenBottom = GetToken(Tokenizer);
        token TokenLeft = GetToken(Tokenizer);
        token TokenRight = GetToken(Tokenizer);

        if(TokenTop.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding top value '" + std::string(TokenTop.Text, TokenTop.TextLength) + "'");
            IsValid = false;
        }
        if(TokenBottom.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding bottom value '" + std::string(TokenBottom.Text, TokenBottom.TextLength) + "'");
            IsValid = false;
        }
        if(TokenLeft.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding left value '" + std::string(TokenLeft.Text, TokenLeft.TextLength) + "'");
            IsValid = false;
        }
        if(TokenRight.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding right value '" + std::string(TokenRight.Text, TokenRight.TextLength) + "'");
            IsValid = false;
        }

        if(IsValid)
        {
            SpaceSettings->Offset.PaddingTop = ConvertStringToDouble(std::string(TokenTop.Text, TokenTop.TextLength));
            SpaceSettings->Offset.PaddingBottom = ConvertStringToDouble(std::string(TokenBottom.Text, TokenBottom.TextLength));
            SpaceSettings->Offset.PaddingLeft = ConvertStringToDouble(std::string(TokenLeft.Text, TokenLeft.TextLength));
            SpaceSettings->Offset.PaddingRight = ConvertStringToDouble(std::string(TokenRight.Text, TokenRight.TextLength));
        }
    }
    else if(TokenEquals(Token, "gap"))
    {
        bool IsValid = true;
        token TokenVertical = GetToken(Tokenizer);
        token TokenHorizontal = GetToken(Tokenizer);

        if(TokenVertical.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config gap vertical value '" + std::string(TokenVertical.Text, TokenVertical.TextLength) + "'");
            IsValid = false;
        }
        if(TokenHorizontal.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config gap horizontal value '" + std::string(TokenHorizontal.Text, TokenHorizontal.TextLength) + "'");
            IsValid = false;
        }

        if(IsValid)
        {
            SpaceSettings->Offset.VerticalGap = ConvertStringToDouble(std::string(TokenVertical.Text, TokenVertical.TextLength));
            SpaceSettings->Offset.HorizontalGap = ConvertStringToDouble(std::string(TokenHorizontal.Text, TokenHorizontal.TextLength));
        }
    }
    else if(TokenEquals(Token, "name"))
    {
        token Token = GetToken(Tokenizer);
        SpaceSettings->Name = std::string(Token.Text, Token.TextLength);
    }
    else if(TokenEquals(Token, "tree"))
    {
        token Token = GetToken(Tokenizer);
        SpaceSettings->Layout = std::string(Token.Text, Token.TextLength);
    }
    else
    {
        ReportInvalidCommand("Unknown command 'config space " + Display + " " + Space + " " + std::string(Token.Text, Token.TextLength) + "'");
    }
}

internal void
KwmParseConfigOptionDisplay(tokenizer *Tokenizer)
{
    token TokenDisplay = GetToken(Tokenizer);
    std::string Display(TokenDisplay.Text, TokenDisplay.TextLength);
    if(TokenDisplay.Type != Token_Digit)
    {
        ReportInvalidCommand("Unknown command 'config display " + Display + "'");
        return;
    }

    int ScreenID = ConvertStringToInt(Display);
    space_settings *DisplaySettings = GetSpaceSettingsForDisplay(ScreenID);
    if(!DisplaySettings)
    {
        space_settings NULLSpaceSettings = { KWMSettings.DefaultOffset, SpaceModeDefault, {0, 0}, "", "" };
        KWMSettings.DisplaySettings[ScreenID] = NULLSpaceSettings;
        DisplaySettings = &KWMSettings.DisplaySettings[ScreenID];
    }

    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "mode"))
    {
        token Token = GetToken(Tokenizer);
        std::string Mode(Token.Text, Token.TextLength);
        if(TokenEquals(Token, "bsp"))
            DisplaySettings->Mode = SpaceModeBSP;
        else if(TokenEquals(Token, "monocle"))
            DisplaySettings->Mode = SpaceModeMonocle;
        else if(TokenEquals(Token, "float"))
            DisplaySettings->Mode = SpaceModeFloating;
        else
            ReportInvalidCommand("Unknown command 'config display " + Display + " mode " + Mode + "'");
    }
    else if(TokenEquals(Token, "padding"))
    {
        bool IsValid = true;
        token TokenTop = GetToken(Tokenizer);
        token TokenBottom = GetToken(Tokenizer);
        token TokenLeft = GetToken(Tokenizer);
        token TokenRight = GetToken(Tokenizer);

        if(TokenTop.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding top value '" + std::string(TokenTop.Text, TokenTop.TextLength) + "'");
            IsValid = false;
        }
        if(TokenBottom.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding bottom value '" + std::string(TokenBottom.Text, TokenBottom.TextLength) + "'");
            IsValid = false;
        }
        if(TokenLeft.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding left value '" + std::string(TokenLeft.Text, TokenLeft.TextLength) + "'");
            IsValid = false;
        }
        if(TokenRight.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config padding right value '" + std::string(TokenRight.Text, TokenRight.TextLength) + "'");
            IsValid = false;
        }

        if(IsValid)
        {
            DisplaySettings->Offset.PaddingTop = ConvertStringToDouble(std::string(TokenTop.Text, TokenTop.TextLength));
            DisplaySettings->Offset.PaddingBottom = ConvertStringToDouble(std::string(TokenBottom.Text, TokenBottom.TextLength));
            DisplaySettings->Offset.PaddingLeft = ConvertStringToDouble(std::string(TokenLeft.Text, TokenLeft.TextLength));
            DisplaySettings->Offset.PaddingRight = ConvertStringToDouble(std::string(TokenRight.Text, TokenRight.TextLength));
        }
    }
    else if(TokenEquals(Token, "gap"))
    {
        bool IsValid = true;
        token TokenVertical = GetToken(Tokenizer);
        token TokenHorizontal = GetToken(Tokenizer);

        if(TokenVertical.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config gap vertical value '" + std::string(TokenVertical.Text, TokenVertical.TextLength) + "'");
            IsValid = false;
        }
        if(TokenHorizontal.Type != Token_Digit)
        {
            ReportInvalidCommand("Unknown config gap horizontal value '" + std::string(TokenHorizontal.Text, TokenHorizontal.TextLength) + "'");
            IsValid = false;
        }

        if(IsValid)
        {
            DisplaySettings->Offset.VerticalGap = ConvertStringToDouble(std::string(TokenVertical.Text, TokenVertical.TextLength));
            DisplaySettings->Offset.HorizontalGap = ConvertStringToDouble(std::string(TokenHorizontal.Text, TokenHorizontal.TextLength));
        }
    }
    else if(TokenEquals(Token, "float"))
    {
        if(RequireToken(Tokenizer, Token_Dash))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "dim"))
            {
                bool IsValid = true;
                token TokenWidth = GetToken(Tokenizer);
                token TokenHeight = GetToken(Tokenizer);

                if(TokenWidth.Type != Token_Digit)
                {
                    ReportInvalidCommand("Unknown float-dim width value '" + std::string(TokenWidth.Text, TokenWidth.TextLength) + "'");
                    IsValid = false;
                }
                if(TokenHeight.Type != Token_Digit)
                {
                    ReportInvalidCommand("Unknown float-dim height value '" + std::string(TokenHeight.Text, TokenHeight.TextLength) + "'");
                    IsValid = false;
                }

                if(IsValid)
                {
                    DisplaySettings->FloatDim.width = ConvertStringToDouble(std::string(TokenWidth.Text, TokenWidth.TextLength));
                    DisplaySettings->FloatDim.height = ConvertStringToDouble(std::string(TokenHeight.Text, TokenHeight.TextLength));
                }
            }
            else
            {
                ReportInvalidCommand("Unknown command 'config display " + Display + " float-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Expected token '-' after 'config display " + Display + " float'");
        }
    }
    else
    {
        ReportInvalidCommand("Unknown command 'config display " + Display + " " + std::string(Token.Text, Token.TextLength) + "'");
    }
}

internal void
KwmParseConfigOption(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    switch(Token.Type)
    {
        case Token_EndOfStream:
        {
            ReportInvalidCommand("Unexpected end of stream!");
            return;
        } break;
        case Token_Identifier:
        {
            if(TokenEquals(Token, "tiling"))
                KwmParseConfigOptionTiling(Tokenizer);
            else if(TokenEquals(Token, "padding"))
                KwmParseConfigOptionPadding(Tokenizer);
            else if(TokenEquals(Token, "gap"))
                KwmParseConfigOptionGap(Tokenizer);
            else if(TokenEquals(Token, "focus"))
                KwmParseConfigOptionFocusFollowsMouse(Tokenizer);
            else if(TokenEquals(Token, "mouse"))
                KwmParseConfigOptionMouse(Tokenizer);
            else if(TokenEquals(Token, "standby"))
                KwmParseConfigOptionStandbyOnFloat(Tokenizer);
            else if(TokenEquals(Token, "center"))
                KwmParseConfigOptionCenterOnFloat(Tokenizer);
            else if(TokenEquals(Token, "float"))
                KwmParseConfigOptionFloatNonResizable(Tokenizer);
            else if(TokenEquals(Token, "lock"))
                KwmParseConfigOptionLockToContainer(Tokenizer);
            else if(TokenEquals(Token, "cycle"))
                KwmParseConfigOptionCycleFocus(Tokenizer);
            else if(TokenEquals(Token, "split"))
                KwmParseConfigOptionSplitRatio(Tokenizer);
            else if(TokenEquals(Token, "optimal"))
                KwmParseConfigOptionOptimalRatio(Tokenizer);
            else if(TokenEquals(Token, "spawn"))
                KwmParseConfigOptionSpawn(Tokenizer);
            else if(TokenEquals(Token, "border"))
                KwmParseConfigOptionBorder(Tokenizer);
            else if(TokenEquals(Token, "space"))
                KwmParseConfigOptionSpace(Tokenizer);
            else if(TokenEquals(Token, "display"))
                KwmParseConfigOptionDisplay(Tokenizer);
            else if(TokenEquals(Token, "reload"))
                KwmReloadConfig();
            else
                ReportInvalidCommand("Unknown command 'config " + std::string(Token.Text, Token.TextLength) + "'");
        } break;
        default:
        {
            ReportInvalidCommand("Unknown token '" + std::string(Token.Text, Token.TextLength) + "'");
        } break;
    }
}

internal void
KwmParseWindowOption(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "f"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "north"))
                ShiftWindowFocusDirected(0);
            else if(TokenEquals(Selector, "east"))
                ShiftWindowFocusDirected(90);
            else if(TokenEquals(Selector, "south"))
                ShiftWindowFocusDirected(180);
            else if(TokenEquals(Selector, "west"))
                ShiftWindowFocusDirected(270);
            else if(TokenEquals(Selector, "prev"))
                ShiftWindowFocus(-1);
            else if(TokenEquals(Selector, "next"))
                ShiftWindowFocus(1);
            else if(TokenEquals(Selector, "curr"))
                FocusWindowBelowCursor();
            else if(Selector.Type == Token_Digit)
                FocusWindowByID(ConvertStringToUint(std::string(Selector.Text, Selector.TextLength)));
            else
                FocusWindowByName(std::string(Selector.Text, Selector.TextLength));
        }
        else if(TokenEquals(Token, "fm"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "prev"))
                ShiftSubTreeWindowFocus(-1);
            else if(TokenEquals(Selector, "next"))
                ShiftSubTreeWindowFocus(1);
            else
                ReportInvalidCommand("Unknown selector '" + std::string(Selector.Text, Selector.TextLength) + "'");
        }
        else if(TokenEquals(Token, "s"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "north"))
                SwapFocusedWindowDirected(0);
            else if(TokenEquals(Selector, "east"))
                SwapFocusedWindowDirected(90);
            else if(TokenEquals(Selector, "south"))
                SwapFocusedWindowDirected(180);
            else if(TokenEquals(Selector, "west"))
                SwapFocusedWindowDirected(270);
            else if(TokenEquals(Selector, "prev"))
                SwapFocusedWindowWithNearest(-1);
            else if(TokenEquals(Selector, "next"))
                SwapFocusedWindowWithNearest(1);
            else if(TokenEquals(Selector, "mark"))
                SwapFocusedWindowWithMarked();
            else
                ReportInvalidCommand("Unknown selector '" + std::string(Selector.Text, Selector.TextLength) + "'");
        }
        else if(TokenEquals(Token, "z"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "fullscreen"))
                ToggleFocusedWindowFullscreen();
            else if(TokenEquals(Selector, "parent"))
                ToggleFocusedWindowParentContainer();
            else
                ReportInvalidCommand("Unknown selector '" + std::string(Selector.Text, Selector.TextLength) + "'");
        }
        else if(TokenEquals(Token, "t"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "focused"))
                ToggleFocusedWindowFloating();
            else if(TokenEquals(Selector, "next"))
                AddFlags(&KWMSettings, Settings_FloatNextWindow);
            else
                ReportInvalidCommand("Unknown selector '" + std::string(Selector.Text, Selector.TextLength) + "'");
        }
        else if(TokenEquals(Token, "r"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "focused"))
                ResizeWindowToContainerSize();
            else
                ReportInvalidCommand("Unknown selector '" + std::string(Selector.Text, Selector.TextLength) + "'");
        }
        else if(TokenEquals(Token, "c"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "split"))
            {
                if(RequireToken(Tokenizer, Token_Dash))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "mode"))
                    {
                        token Token = GetToken(Tokenizer);
                        if(TokenEquals(Token, "toggle"))
                            ToggleFocusedNodeSplitMode();
                        else
                            ReportInvalidCommand("Unknown command 'window -c split-mode " + std::string(Token.Text, Token.TextLength) + "'");
                    }
                    else
                    {
                        ReportInvalidCommand("Unknown command 'window -c split-" + std::string(Token.Text, Token.TextLength) + "'");
                    }
                }
                else
                {
                    ReportInvalidCommand("Expected token '-' after 'window -c split'");
                }
            }
            else if(TokenEquals(Selector, "type"))
            {
                token Selector = GetToken(Tokenizer);
                if(TokenEquals(Selector, "monocle"))
                    ChangeTypeOfFocusedNode(NodeTypeLink);
                else if(TokenEquals(Selector, "bsp"))
                    ChangeTypeOfFocusedNode(NodeTypeTree);
                else if(TokenEquals(Selector, "toggle"))
                    ToggleTypeOfFocusedNode();
                else
                    ReportInvalidCommand("Unknown selector '" + std::string(Selector.Text, Selector.TextLength) + "'");
            }
            else if(TokenEquals(Selector, "reduce") || TokenEquals(Selector, "expand"))
            {
                token Value = GetToken(Tokenizer);
                if(Value.Type == Token_Digit)
                {
                    double Ratio = ConvertStringToDouble(std::string(Value.Text, Value.TextLength));
                    Ratio = TokenEquals(Selector, "reduce") ? -Ratio : Ratio;

                    token Direction = GetToken(Tokenizer);
                    if(TokenEquals(Direction, "north"))
                        ModifyContainerSplitRatio(Ratio, 0);
                    else if(TokenEquals(Direction, "east"))
                        ModifyContainerSplitRatio(Ratio, 90);
                    else if(TokenEquals(Direction, "south"))
                        ModifyContainerSplitRatio(Ratio, 180);
                    else if(TokenEquals(Direction, "west"))
                        ModifyContainerSplitRatio(Ratio, 270);
                    else if(TokenEquals(Direction, "focused"))
                        ModifyContainerSplitRatio(Ratio);
                    else
                        ReportInvalidCommand("Unknown selector '" + std::string(Direction.Text, Direction.TextLength) + "'");
                }
                else
                {
                    ReportInvalidCommand("Expected token of type 'Token_Digit' after '" + std::string(Selector.Text, Selector.TextLength) + "'");
                }
            }
            else
            {
                ReportInvalidCommand("Unknown command 'window -c " + std::string(Selector.Text, Selector.TextLength) + "'");
            }
        }
        else if(TokenEquals(Token, "m"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "space"))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "previous"))
                    GoToPreviousSpace(true);
                else
                    MoveFocusedWindowToSpace(std::string(Token.Text, Token.TextLength));
            }
            else if(TokenEquals(Selector, "display"))
            {
                ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
                if(Window)
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "prev"))
                        MoveWindowToDisplay(Window, -1, true);
                    else if(TokenEquals(Token, "next"))
                        MoveWindowToDisplay(Window, 1, true);
                    else
                        MoveWindowToDisplay(Window, ConvertStringToInt(std::string(Token.Text, Token.TextLength)), false);
                }
            }
            else if(TokenEquals(Selector, "north"))
            {
                ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
                if(Window)
                    DetachAndReinsertWindow(Window->ID, 0);
            }
            else if(TokenEquals(Selector, "east"))
            {
                ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
                if(Window)
                    DetachAndReinsertWindow(Window->ID, 90);
            }
            else if(TokenEquals(Selector, "south"))
            {
                ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
                if(Window)
                    DetachAndReinsertWindow(Window->ID, 180);
            }
            else if(TokenEquals(Selector, "west"))
            {
                ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
                if(Window)
                    DetachAndReinsertWindow(Window->ID, 270);
            }
            else if(TokenEquals(Selector, "mark"))
            {
                if(MarkedWindow)
                    DetachAndReinsertWindow(MarkedWindow->ID, 0);
            }
            else
            {
                bool Valid = true;
                token XToken = GetToken(Tokenizer);
                token YToken = GetToken(Tokenizer);

                if(XToken.Type != Token_Digit || YToken.Type != Token_Digit)
                {
                    Valid = false;
                    ReportInvalidCommand("Expected token of type 'Token_Digit'");
                }

                if(Valid)
                {
                    int XOff = ConvertStringToInt(std::string(XToken.Text, XToken.TextLength));
                    int YOff = ConvertStringToInt(std::string(YToken.Text, YToken.TextLength));
                    MoveFloatingWindow(XOff, YOff);
                }
            }
        }
        else if(TokenEquals(Token, "mk"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "focused"))
            {
                MarkFocusedWindowContainer();
            }
            else if(TokenEquals(Selector, "north"))
            {
                ax_window *ClosestWindow = NULL;
                token Token = GetToken(Tokenizer);
                bool Wrap = TokenEquals(Token, "wrap") ? true : false;
                if((FindClosestWindow(0, &ClosestWindow, Wrap)) && (ClosestWindow))
                    MarkWindowContainer(ClosestWindow);
            }
            else if(TokenEquals(Selector, "east"))
            {
                ax_window *ClosestWindow = NULL;
                token Token = GetToken(Tokenizer);
                bool Wrap = TokenEquals(Token, "wrap") ? true : false;
                if((FindClosestWindow(90, &ClosestWindow, Wrap)) && (ClosestWindow))
                    MarkWindowContainer(ClosestWindow);
            }
            else if(TokenEquals(Selector, "south"))
            {
                ax_window *ClosestWindow = NULL;
                token Token = GetToken(Tokenizer);
                bool Wrap = TokenEquals(Token, "wrap") ? true : false;
                if((FindClosestWindow(180, &ClosestWindow, Wrap)) && (ClosestWindow))
                    MarkWindowContainer(ClosestWindow);
            }
            else if(TokenEquals(Selector, "west"))
            {
                ax_window *ClosestWindow = NULL;
                token Token = GetToken(Tokenizer);
                bool Wrap = TokenEquals(Token, "wrap") ? true : false;
                if((FindClosestWindow(270, &ClosestWindow, Wrap)) && (ClosestWindow))
                    MarkWindowContainer(ClosestWindow);
            }
            else
            {
                ReportInvalidCommand("Unknown command 'window -mk " + std::string(Selector.Text, Selector.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Unknown command 'window -" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'window'");
    }
}

internal void
KwmParseTreeOption(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    if(Token.Type == Token_Dash)
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "pseudo"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "create"))
                CreatePseudoNode();
            else if(TokenEquals(Selector, "destroy"))
                RemovePseudoNode();
            else
                ReportInvalidCommand("Unknown command 'tree -pseudo " + std::string(Selector.Text, Selector.TextLength) + "'");
        }
        else
        {
            ReportInvalidCommand("Unknown command 'tree -" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else if(TokenEquals(Token, "rotate"))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "90") || TokenEquals(Token, "270") || TokenEquals(Token, "180"))
            RotateBSPTree(ConvertStringToInt(std::string(Token.Text, Token.TextLength)));
        else
            ReportInvalidCommand("Unknown command 'tree rotate " + std::string(Token.Text, Token.TextLength) + "'");
    }
    else if(TokenEquals(Token, "equalize"))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "root"))
            EqualizeBSPTree(std::string(Token.Text, Token.TextLength));
        else
            ReportInvalidCommand("Unknown command 'tree equalize " + std::string(Token.Text, Token.TextLength) + "'");
    }
    else if(TokenEquals(Token, "save"))
    {
        ax_display *Display = AXLibMainDisplay();
        space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
        token Token = GetToken(Tokenizer);
        if(Token.Type != Token_EndOfStream)
            SaveBSPTreeToFile(Display, SpaceInfo, std::string(Token.Text, Token.TextLength));
        else
            ReportInvalidCommand("Invalid command 'tree save " + std::string(Token.Text, Token.TextLength) + "'");
    }
    else if(TokenEquals(Token, "restore"))
    {
        token Token = GetToken(Tokenizer);
        if(Token.Type != Token_EndOfStream)
            LoadWindowNodeTree(AXLibMainDisplay(), std::string(Token.Text, Token.TextLength));
        else
            ReportInvalidCommand("Invalid command 'tree restore " + std::string(Token.Text, Token.TextLength) + "'");
    }
    else
    {
        ReportInvalidCommand("Unknown command 'tree " + std::string(Token.Text, Token.TextLength) + "'");
    }
}

internal void
KwmParseDisplayOption(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "f"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "prev"))
            {
                ax_display *Display = AXLibMainDisplay();
                if(Display)
                    FocusDisplay(AXLibPreviousDisplay(Display));
            }
            else if(TokenEquals(Selector, "next"))
            {
                ax_display *Display = AXLibMainDisplay();
                if(Display)
                    FocusDisplay(AXLibNextDisplay(Display));
            }
            else if(Selector.Type == Token_Digit)
            {
                int DisplayID = ConvertStringToInt(std::string(Selector.Text, Selector.TextLength));
                ax_display *Display = AXLibArrangementDisplay(DisplayID);
                if(Display)
                    FocusDisplay(Display);
            }
            else
            {
                ReportInvalidCommand("Unknown selector '" + std::string(Selector.Text, Selector.TextLength) + "'");
            }
        }
        else if(TokenEquals(Token, "c"))
        {
            token Selector = GetToken(Tokenizer);
            if(TokenEquals(Selector, "optimal"))
                KWMSettings.SplitMode = SPLIT_OPTIMAL;
            else if(TokenEquals(Selector, "vertical"))
                KWMSettings.SplitMode = SPLIT_VERTICAL;
            else if(TokenEquals(Selector, "horizontal"))
                KWMSettings.SplitMode = SPLIT_HORIZONTAL;
            else
                ReportInvalidCommand("Unknown selector '" + std::string(Selector.Text, Selector.TextLength) + "'");
        }
        else
        {
            ReportInvalidCommand("Unknown selector '" + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'display'");
    }
}

internal void
KwmParseSpaceOption(tokenizer *Tokenizer)
{
    if(RequireToken(Tokenizer, Token_Dash))
    {
        token Selector = GetToken(Tokenizer);
        if(TokenEquals(Selector, "fExperimental"))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "previous"))
                GoToPreviousSpace(false);
            else if(Token.Type != Token_EndOfStream)
                ActivateSpaceWithoutTransition(std::string(Token.Text, Token.TextLength));
            else
                ReportInvalidCommand("Unknown selector '" + std::string(Token.Text, Token.TextLength) + "'");
        }
        else if(TokenEquals(Selector, "t"))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "bsp"))
                ResetWindowNodeTree(AXLibMainDisplay(), SpaceModeBSP);
            else if(TokenEquals(Token, "monocle"))
                ResetWindowNodeTree(AXLibMainDisplay(), SpaceModeMonocle);
            else if(TokenEquals(Token, "float"))
                ResetWindowNodeTree(AXLibMainDisplay(), SpaceModeFloating);
            else
                ReportInvalidCommand("Unknown selector '" + std::string(Token.Text, Token.TextLength) + "'");
        }
        else if(TokenEquals(Selector, "r"))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "focused"))
            {
                ax_display *Display = AXLibMainDisplay();
                if(Display)
                {
                    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
                    ApplyTreeNodeContainer(SpaceInfo->RootNode);
                }
            }
            else
            {
                ReportInvalidCommand("Unknown selector '" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else if(TokenEquals(Selector, "p"))
        {
            token Token = GetToken(Tokenizer);
            token Direction = GetToken(Tokenizer);
            if(TokenEquals(Direction, "left") || TokenEquals(Direction, "right") ||
               TokenEquals(Direction, "top") || TokenEquals(Direction, "bottom") ||
               TokenEquals(Direction, "all"))
            {
                int Value = 0;
                if(TokenEquals(Token, "increase"))
                    Value = 10;
                else if(TokenEquals(Token, "decrease"))
                    Value = -10;

                ChangePaddingOfDisplay(std::string(Direction.Text, Direction.TextLength), Value);
            }
            else
            {
                ReportInvalidCommand("Unknown selector '" + std::string(Direction.Text, Direction.TextLength) + "'");
            }
        }
        else if(TokenEquals(Selector, "g"))
        {
            token Token = GetToken(Tokenizer);
            token Direction = GetToken(Tokenizer);
            if(TokenEquals(Direction, "vertical") ||
               TokenEquals(Direction, "horizontal") ||
               TokenEquals(Direction, "all"))
            {
                int Value = 0;
                if(TokenEquals(Token, "increase"))
                    Value = 10;
                else if(TokenEquals(Token, "decrease"))
                    Value = -10;

                ChangeGapOfDisplay(std::string(Direction.Text, Direction.TextLength), Value);
            }
            else
            {
                ReportInvalidCommand("Unknown selector '" + std::string(Direction.Text, Direction.TextLength) + "'");
            }
        }
        else if(TokenEquals(Selector, "n"))
        {
            ax_display *Display = AXLibMainDisplay();
            if(Display)
            {
                token Token = GetToken(Tokenizer);
                SetNameOfActiveSpace(Display, std::string(Token.Text, Token.TextLength));
            }
        }
        else
        {
            ReportInvalidCommand("Unknown selector '" + std::string(Selector.Text, Selector.TextLength) + "'");
        }
    }
    else
    {
        ReportInvalidCommand("Expected token '-' after 'space'");
    }
}

internal void
KwmParseScratchpadOption(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "show"))
    {
        token Value = GetToken(Tokenizer);
        ShowScratchpadWindow(ConvertStringToInt(std::string(Value.Text, Value.TextLength)));
    }
    else if(TokenEquals(Token, "hide"))
    {
        token Value = GetToken(Tokenizer);
        HideScratchpadWindow(ConvertStringToInt(std::string(Value.Text, Value.TextLength)));
    }
    else if(TokenEquals(Token, "toggle"))
    {
        token Value = GetToken(Tokenizer);
        ToggleScratchpadWindow(ConvertStringToInt(std::string(Value.Text, Value.TextLength)));
    }
    else if(TokenEquals(Token, "add"))
    {
        ax_application *Application = AXLibGetFocusedApplication();
        if(Application && Application->Focus)
            AddWindowToScratchpad(Application->Focus);
    }
    else if(TokenEquals(Token, "remove"))
    {
        ax_application *Application = AXLibGetFocusedApplication();
        if(Application && Application->Focus)
            RemoveWindowFromScratchpad(Application->Focus);
    }
    else
    {
        ReportInvalidCommand("Unknown command 'scratchpad " + std::string(Token.Text, Token.TextLength) + "'");
    }
}

internal void
KwmParseQueryOption(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "tiling"))
    {
        token Selector = GetToken(Tokenizer);
        if(TokenEquals(Selector, "mode"))
            KwmConstructEvent(KWMEvent_QueryTilingMode, KwmCreateContext(ClientSockFD));
        else if(TokenEquals(Selector,"spawn"))
            KwmConstructEvent(KWMEvent_QuerySpawnPosition, KwmCreateContext(ClientSockFD));
        else if(TokenEquals(Selector, "split"))
        {
            if(RequireToken(Tokenizer, Token_Dash))
            {
                token Token = GetToken(Tokenizer);
                if(TokenEquals(Token, "mode"))
                    KwmConstructEvent(KWMEvent_QuerySplitMode, KwmCreateContext(ClientSockFD));
                else if(TokenEquals(Token, "ratio"))
                    KwmConstructEvent(KWMEvent_QuerySplitRatio, KwmCreateContext(ClientSockFD));
                else
                    ReportInvalidCommand("Unknown command 'query split-" + std::string(Token.Text, Token.TextLength) + "'");
            }
            else
            {
                ReportInvalidCommand("Expected token '-' after 'query split'");
            }
        }
        else
        {
            ReportInvalidCommand("Unknown command 'query tiling " + std::string(Selector.Text, Selector.TextLength) + "'");
        }
    }
    else if(TokenEquals(Token, "window"))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "focused"))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "id"))
                KwmConstructEvent(KWMEvent_QueryFocusedWindowId, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "owner"))
                KwmConstructEvent(KWMEvent_QueryFocusedWindowOwner, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "name"))
                KwmConstructEvent(KWMEvent_QueryFocusedWindowName, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "split"))
                KwmConstructEvent(KWMEvent_QueryFocusedWindowSplit, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "float"))
                KwmConstructEvent(KWMEvent_QueryFocusedWindowFloat, KwmCreateContext(ClientSockFD));
            else
            {
                int *Args = (int *) malloc(sizeof(int) * 2);
                *Args = ClientSockFD;

                if(TokenEquals(Token, "north"))
                    *(Args + 1) = 0;
                else if(TokenEquals(Token, "east"))
                    *(Args + 1) = 90;
                else if(TokenEquals(Token, "south"))
                    *(Args + 1) = 180;
                else if(TokenEquals(Token, "west"))
                    *(Args + 1) = 270;
                else
                    *(Args + 1) = 0;

                KwmConstructEvent(KWMEvent_QueryWindowIdInDirectionOfFocusedWindow, Args);
            }
        }
        else if(TokenEquals(Token, "marked"))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "id"))
                KwmConstructEvent(KWMEvent_QueryMarkedWindowId, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "owner"))
                KwmConstructEvent(KWMEvent_QueryMarkedWindowOwner, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "name"))
                KwmConstructEvent(KWMEvent_QueryMarkedWindowName, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "split"))
                KwmConstructEvent(KWMEvent_QueryMarkedWindowSplit, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "float"))
                KwmConstructEvent(KWMEvent_QueryMarkedWindowFloat, KwmCreateContext(ClientSockFD));
            else
                ReportInvalidCommand("Unknown command 'query window marked " + std::string(Token.Text, Token.TextLength) + "'");
        }
        else if(TokenEquals(Token, "parent"))
        {
            bool Valid = true;
            token Token1 = GetToken(Tokenizer);
            token Token2 = GetToken(Tokenizer);
            if(Token1.Type != Token_Digit || Token2.Type != Token_Digit)
            {
                Valid = false;
                ReportInvalidCommand("Expected token of type 'Token_Digit'");
            }

            if(Valid)
            {
                int *Args = (int *) malloc(sizeof(int) * 3);
                *Args = ClientSockFD;
                *(Args + 1) = ConvertStringToInt(std::string(Token1.Text, Token1.TextLength));
                *(Args + 2) = ConvertStringToInt(std::string(Token2.Text, Token2.TextLength));
                KwmConstructEvent(KWMEvent_QueryParentNodeState, Args);
            }
        }
        else if(TokenEquals(Token, "child"))
        {
            bool Valid = true;
            token Token = GetToken(Tokenizer);
            if(Token.Type != Token_Digit)
            {
                Valid = false;
                ReportInvalidCommand("Expected token of type 'Token_Digit'");
            }

            if(Valid)
            {
                int *Args = (int *) malloc(sizeof(int) * 2);
                *Args = ClientSockFD;
                *(Args + 1) = ConvertStringToInt(std::string(Token.Text, Token.TextLength));
                KwmConstructEvent(KWMEvent_QueryNodePosition, Args);
            }
        }
        else if(TokenEquals(Token, "list"))
        {
            KwmConstructEvent(KWMEvent_QueryWindowList, KwmCreateContext(ClientSockFD));
        }
        else
        {
            ReportInvalidCommand("Unknown command 'query window " + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else if(TokenEquals(Token, "cycle"))
    {
        if(RequireToken(Tokenizer, Token_Dash))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "focus"))
                KwmConstructEvent(KWMEvent_QueryCycleFocus, KwmCreateContext(ClientSockFD));
            else
                ReportInvalidCommand("Unknown command 'query cycle-" + std::string(Token.Text, Token.TextLength) + "'");
        }
        else
        {
            ReportInvalidCommand("Expected token '-' after 'query cycle'");
        }
    }
    else if(TokenEquals(Token, "float"))
    {
        if(RequireToken(Tokenizer, Token_Dash))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "non"))
            {
                if(RequireToken(Tokenizer, Token_Dash))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "resizable"))
                        KwmConstructEvent(KWMEvent_QueryFloatNonResizable, KwmCreateContext(ClientSockFD));
                    else
                        ReportInvalidCommand("Unknown command 'query float-non-" + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                {
                    ReportInvalidCommand("Expected token '-' after 'query float-non'");
                }
            }
            else
            {
                ReportInvalidCommand("Unknown command 'query float-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Expected token '-' after 'query float'");
        }
    }
    else if(TokenEquals(Token, "lock"))
    {
        if(RequireToken(Tokenizer, Token_Dash))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "to"))
            {
                if(RequireToken(Tokenizer, Token_Dash))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "container"))
                        KwmConstructEvent(KWMEvent_QueryLockToContainer, KwmCreateContext(ClientSockFD));
                    else
                        ReportInvalidCommand("Unknown command 'query lock-to-" + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                {
                    ReportInvalidCommand("Expected token '-' after 'query lock-to'");
                }
            }
            else
            {
                ReportInvalidCommand("Unknown command 'query lock-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Expected token '-' after 'query lock'");
        }
    }
    else if(TokenEquals(Token, "standby"))
    {
        if(RequireToken(Tokenizer, Token_Dash))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "on"))
            {
                if(RequireToken(Tokenizer, Token_Dash))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "float"))
                        KwmConstructEvent(KWMEvent_QueryStandbyOnFloat, KwmCreateContext(ClientSockFD));
                    else
                        ReportInvalidCommand("Unknown command 'query standby-on-" + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                {
                    ReportInvalidCommand("Expected token '-' after 'query standby-on'");
                }
            }
            else
            {
                ReportInvalidCommand("Unknown command 'query standby-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Expected token '-' after 'query standby'");
        }
    }
    else if(TokenEquals(Token, "focus"))
    {
        if(RequireToken(Tokenizer, Token_Dash))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "follows"))
            {
                if(RequireToken(Tokenizer, Token_Dash))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "mouse"))
                        KwmConstructEvent(KWMEvent_QueryFocusFollowsMouse, KwmCreateContext(ClientSockFD));
                    else
                        ReportInvalidCommand("Unknown command 'query focus-follows-" + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                {
                    ReportInvalidCommand("Expected token '-' after 'query focus-follows'");
                }
            }
            else
            {
                ReportInvalidCommand("Unknown command 'query focus-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Expected token '-' after 'query focus'");
        }
    }
    else if(TokenEquals(Token, "mouse"))
    {
        if(RequireToken(Tokenizer, Token_Dash))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "follows"))
            {
                if(RequireToken(Tokenizer, Token_Dash))
                {
                    token Token = GetToken(Tokenizer);
                    if(TokenEquals(Token, "focus"))
                        KwmConstructEvent(KWMEvent_QueryMouseFollowsFocus, KwmCreateContext(ClientSockFD));
                    else
                        ReportInvalidCommand("Unknown command 'query mouse-follows-" + std::string(Token.Text, Token.TextLength) + "'");
                }
                else
                {
                    ReportInvalidCommand("Expected token '-' after 'query mouse-follows'");
                }
            }
            else
            {
                ReportInvalidCommand("Unknown command 'query mouse-" + std::string(Token.Text, Token.TextLength) + "'");
            }
        }
        else
        {
            ReportInvalidCommand("Expected token '-' after 'query mouse'");
        }
    }
    else if(TokenEquals(Token, "scratchpad"))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "list"))
        {
            KwmConstructEvent(KWMEvent_QueryScratchpad, KwmCreateContext(ClientSockFD));
        }
        else
        {
            ReportInvalidCommand("Unknown command 'query scratchpad " + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else if(TokenEquals(Token, "space"))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "active"))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "tag"))
                KwmConstructEvent(KWMEvent_QueryCurrentSpaceTag, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "name"))
                KwmConstructEvent(KWMEvent_QueryCurrentSpaceName, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "id"))
                KwmConstructEvent(KWMEvent_QueryCurrentSpaceId, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "mode"))
                KwmConstructEvent(KWMEvent_QueryCurrentSpaceMode, KwmCreateContext(ClientSockFD));
            else
                ReportInvalidCommand("Unknown command 'query space active " + std::string(Token.Text, Token.TextLength) + "'");
        }
        else if(TokenEquals(Token, "previous"))
        {
            token Token = GetToken(Tokenizer);
            if(TokenEquals(Token, "name"))
                KwmConstructEvent(KWMEvent_QueryPreviousSpaceName, KwmCreateContext(ClientSockFD));
            else if(TokenEquals(Token, "id"))
                KwmConstructEvent(KWMEvent_QueryPreviousSpaceId, KwmCreateContext(ClientSockFD));
            else
                ReportInvalidCommand("Unknown command 'query space previous " + std::string(Token.Text, Token.TextLength) + "'");
        }
        else if(TokenEquals(Token, "list"))
        {
            KwmConstructEvent(KWMEvent_QuerySpaces, KwmCreateContext(ClientSockFD));
        }
        else
        {
            ReportInvalidCommand("Unknown command 'query space " + std::string(Token.Text, Token.TextLength) + "'");
        }
    }
    else if(TokenEquals(Token, "border"))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "focused"))
            KwmConstructEvent(KWMEvent_QueryFocusedBorder, KwmCreateContext(ClientSockFD));
        else if(TokenEquals(Token, "marked"))
            KwmConstructEvent(KWMEvent_QueryMarkedBorder, KwmCreateContext(ClientSockFD));
        else
            ReportInvalidCommand("Unknown command 'query border " + std::string(Token.Text, Token.TextLength) + "'");
    }
    else
    {
        ReportInvalidCommand("Unknown command 'query " + std::string(Token.Text, Token.TextLength) + "'");
        shutdown(ClientSockFD, SHUT_RDWR);
        close(ClientSockFD);
    }
}

void KwmParseKwmc(tokenizer *Tokenizer, int SockFD)
{
    ClientSockFD = SockFD;
    token Token = GetToken(Tokenizer);
    switch(Token.Type)
    {
        case Token_EndOfStream:
        {
            return;
        } break;
        case Token_Identifier:
        {
            if(TokenEquals(Token, "config"))
                KwmParseConfigOption(Tokenizer);
            else if(TokenEquals(Token, "window"))
                KwmParseWindowOption(Tokenizer);
            else if(TokenEquals(Token, "tree"))
                KwmParseTreeOption(Tokenizer);
            else if(TokenEquals(Token, "display"))
                KwmParseDisplayOption(Tokenizer);
            else if(TokenEquals(Token, "space"))
                KwmParseSpaceOption(Tokenizer);
            else if(TokenEquals(Token, "scratchpad"))
                KwmParseScratchpadOption(Tokenizer);
            else if(TokenEquals(Token, "query"))
                KwmParseQueryOption(Tokenizer);
            else if(TokenEquals(Token, "bindsym") ||
                    TokenEquals(Token, "bindcode") ||
                    TokenEquals(Token, "bindsym_passthrough") ||
                    TokenEquals(Token, "bindcode_passthrough") ||
                    TokenEquals(Token, "rule"))
                KwmInterpretCommand(std::string(Token.Text, Token.TextLength) + " " + GetTextTilEndOfLine(Tokenizer), INVALID_SOCKFD);
            else if(TokenEquals(Token, "whitelist"))
                KwmInterpretCommand(std::string(Token.Text, Token.TextLength) + " " + GetTextTilEndOfLine(Tokenizer), INVALID_SOCKFD);
            else
                ReportInvalidCommand("Unknown token '" + std::string(Token.Text, Token.TextLength) + "'");
        } break;
        default:
        {
            ReportInvalidCommand("Unknown token '" + std::string(Token.Text, Token.TextLength) + "'");
        } break;
    }
}

internal void
KwmParseInclude(tokenizer *Tokenizer)
{
    std::string File = KWMPath.Include + "/" + GetTextTilEndOfLine(Tokenizer);
    KwmParseConfig(File);
}

internal void
KwmParseDefine(tokenizer *Tokenizer, std::map<std::string, std::string> &Defines)
{
    token Token = GetToken(Tokenizer);
    std::string Variable(Token.Text, Token.TextLength);
    std::string Value = GetTextTilEndOfLine(Tokenizer);
    Defines[Variable] = Value;
}

internal void
KwmExpandVariables(std::map<std::string, std::string> &Defines, std::string &Text)
{
    std::map<std::string, std::string>::iterator It;
    for(It = Defines.begin(); It != Defines.end(); ++It)
    {
        std::size_t Pos = Text.find(It->first);
        while(Pos != std::string::npos)
        {
            Text.replace(Pos, It->first.size(), It->second);
            Pos = Text.find(It->first, Pos + It->second.size() + 1);
        }
    }
}

internal void
KwmPreprocessConfig(std::string &Text)
{
    std::map<std::string, std::string> Defines;
    tokenizer Tokenizer = {};
    Tokenizer.At = const_cast<char*>(Text.c_str());

    bool Parsing = true;
    while(Parsing)
    {
        token Token = GetToken(&Tokenizer);
        switch(Token.Type)
        {
            case Token_EndOfStream:
            {
                Parsing = false;
            } break;
            case Token_Comment:
            {
            } break;
            case Token_Identifier:
            {
                if(TokenEquals(Token, "define"))
                    KwmParseDefine(&Tokenizer, Defines);
            } break;
            default:
            {
            } break;
        }
    }

    KwmExpandVariables(Defines, Text);
}

/* NOTE(koekeishiya): The passed string has to include the absolute path to the file. */
void KwmParseConfig(std::string File)
{
    ClientSockFD = INVALID_SOCKFD;
    tokenizer Tokenizer = {};
    char *FileContents = ReadFile(File);

    if(FileContents)
    {
        std::string FileContentsString(FileContents);
        free(FileContents);

        /* NOTE(koekeishiya): Disable define statement
           KwmPreprocessConfig(FileContentsString); */

        Tokenizer.At = const_cast<char*>(FileContentsString.c_str());

        bool Parsing = true;
        while(Parsing)
        {
            token Token = GetToken(&Tokenizer);
            switch(Token.Type)
            {
                case Token_EndOfStream:
                {
                    Parsing = false;
                } break;
                case Token_Comment:
                {
                    // printf("%d Comment: %.*s\n", Token.Type, Token.TextLength, Token.Text);
                } break;
                case Token_Identifier:
                {
                    if(TokenEquals(Token, "kwmc"))
                        KwmParseKwmc(&Tokenizer, INVALID_SOCKFD);
                    else if(TokenEquals(Token, "exec"))
                        KwmExecuteSystemCommand(GetTextTilEndOfLine(&Tokenizer));
                    else if(TokenEquals(Token, "include"))
                        KwmParseInclude(&Tokenizer);
                    else if(TokenEquals(Token, "define"))
                        GetTextTilEndOfLine(&Tokenizer);
                    else if(TokenEquals(Token, "kwm_home"))
                        KWMPath.Home = GetTextTilEndOfLine(&Tokenizer);
                    else if(TokenEquals(Token, "kwm_include"))
                        KWMPath.Include = GetTextTilEndOfLine(&Tokenizer);
                    else if(TokenEquals(Token, "kwm_layouts"))
                        KWMPath.Layouts = GetTextTilEndOfLine(&Tokenizer);
                    else
                        ReportInvalidCommand("Unknown token '" + std::string(Token.Text, Token.TextLength) + "'");
                } break;
                default:
                {
                    ReportInvalidCommand("Unknown token '" + std::string(Token.Text, Token.TextLength) + "'");
                } break;
            }
        }
    }
}

internal void
KwmClearSettings()
{
    KWMSettings.WindowRules.clear();
    KWMSettings.SpaceSettings.clear();
    KWMSettings.DisplaySettings.clear();
}

void KwmReloadConfig()
{
    KwmClearSettings();
    KwmParseConfig(KWMPath.Config);
}
