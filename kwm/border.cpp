#include "border.h"
#include "window.h"

#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreFoundation/CoreFoundation.h>

#define internal static
#define local_persist static

extern kwm_path KWMPath;

typedef void* (*CBridgeInitiaizeType)(
Class ObjCClass, SEL
);

typedef uint32_t (*CBridgeCreateBorderType)(
Class ObjCClass, SEL,
double x, double y, double width, double height,
double r, double g, double b, double a,
double BorderWidth, double CornerRadius
);

typedef void (*CBridgeUpdateBorderType)(
Class ObjCClass, SEL, uint32_t BorderId,
double x, double y, double width, double height,
double r, double g, double b, double a,
double BorderWidth, double CornerRadius
);

typedef void (*CBridgeRemoveBorderType)(
Class ObjCClass, SEL, uint32_t BorderId
);

CBridgeInitiaizeType	CBridgeInitialize	=    (CBridgeInitiaizeType)(objc_msgSend);
CBridgeCreateBorderType	CBridgeCreateBorder	= (CBridgeCreateBorderType)(objc_msgSend);
CBridgeUpdateBorderType	CBridgeUpdateBorder	= (CBridgeUpdateBorderType)(objc_msgSend);
CBridgeRemoveBorderType	CBridgeRemoveBorder	= (CBridgeRemoveBorderType)(objc_msgSend);

internal Class CBridge = objc_getClass("overlaylib.CBridge");

void OverlayLibInitialize()
{
    CBridgeInitialize(CBridge, sel_getUid("initializeOverlay"));
}

internal uint32_t
OverlayLibCreateBorder(double x, double y, double width, double height,
                       double r, double g, double b, double a,
                       double borderWidth, double cornerRadius)
{
	uint32_t BorderId = CBridgeCreateBorder(
							CBridge,
							sel_getUid("createBorderWithX:y:width:height:r:g:b:a:borderWidth:cornerRadius:"),
								x, y, width, height,
								r, g, b, a,
								borderWidth, cornerRadius
						);
	return BorderId;
}

internal void
OverlayLibUpdateBorder(uint32_t BorderId,
                       double x, double y, double width, double height,
                       double r, double g, double b, double a,
                       double borderWidth, double cornerRadius)
{
	CBridgeUpdateBorder(
		CBridge,
		sel_getUid("updateBorderWithId:x:y:width:height:r:g:b:a:borderWidth:cornerRadius:"),
			BorderId,
			x, y, width, height,
			r, g, b, a,
			borderWidth, cornerRadius
	);
}

internal void
OverlayLibRemoveBorder(uint32_t BorderId)
{
	CBridgeRemoveBorder(
		CBridge,
		sel_getUid("removeBorderWithId:"),
			BorderId
	);
}


internal void
OpenBorder(kwm_border *Border)
{
    Border->BorderId = OverlayLibCreateBorder(0, 0, 100, 100, 0, 0, 0, 0, 0, 0);
}

void CloseBorder(kwm_border *Border)
{
    if(Border->BorderId)
    {
        OverlayLibRemoveBorder(Border->BorderId);
        Border->BorderId = 0;
    }
}

void ClearBorder(kwm_border *Border)
{
    if(Border->BorderId)
    {
        OverlayLibUpdateBorder(
            Border->BorderId,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0
        );
    }
}

internal inline void
RefreshBorder(kwm_border *Border, ax_window *Window)
{
	OverlayLibUpdateBorder(
		Border->BorderId,
		Window->Position.x, Window->Position.y, Window->Size.width, Window->Size.height,
		Border->Color.Red, Border->Color.Green, Border->Color.Blue, Border->Color.Alpha,
		Border->Width, Border->Radius
	);
}

internal inline void
RefreshBorder(kwm_border *Border, tree_node *Node)
{
    OverlayLibUpdateBorder(
		Border->BorderId,
		Node->Container.X, Node->Container.Y, Node->Container.Width, Node->Container.Height,
		Border->Color.Red, Border->Color.Green, Border->Color.Blue, Border->Color.Alpha,
		Border->Width, Border->Radius
	);
}

void UpdateBorder(kwm_border *Border, ax_window *Window)
{
    if(Border && Border->Enabled)
    {
        if(Window)
        {
            if(!Border->BorderId)
                OpenBorder(Border);

            if(Border->BorderId)
                RefreshBorder(Border, Window);
        }
        else
        {
            ClearBorder(Border);
        }
    }
}

void UpdateBorder(kwm_border *Border, tree_node *Node)
{
    if(Border && Border->Enabled)
    {
        if(Node)
        {
            if(!Border->BorderId)
                OpenBorder(Border);

            if(Border->BorderId)
                RefreshBorder(Border, Node);
        }
        else
        {
            ClearBorder(Border);
        }
    }
}
