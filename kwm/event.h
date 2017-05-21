#ifndef KWM_EVENT_H
#define KWM_EVENT_H

#include "../axlib/event.h"

/* NOTE(koekeishiya): Declare kwm_event_type callbacks as external functions. */
extern EVENT_CALLBACK(Callback_KWMEvent_QueryTilingMode);
extern EVENT_CALLBACK(Callback_KWMEvent_QuerySplitMode);
extern EVENT_CALLBACK(Callback_KWMEvent_QuerySplitRatio);
extern EVENT_CALLBACK(Callback_KWMEvent_QuerySpawnPosition);

extern EVENT_CALLBACK(Callback_KWMEvent_QueryFocusFollowsMouse);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryMouseFollowsFocus);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryCycleFocus);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryFloatNonResizable);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryLockToContainer);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryStandbyOnFloat);

extern EVENT_CALLBACK(Callback_KWMEvent_QuerySpaces);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryCurrentSpaceName);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryPreviousSpaceName);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryCurrentSpaceMode);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryCurrentSpaceTag);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryCurrentSpaceId);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryPreviousSpaceId);

extern EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedBorder);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedBorder);

extern EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedWindowId);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedWindowOwner);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedWindowName);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedWindowSplit);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryFocusedWindowFloat);

extern EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedWindowId);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedWindowOwner);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedWindowName);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedWindowSplit);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryMarkedWindowFloat);

extern EVENT_CALLBACK(Callback_KWMEvent_QueryWindowList);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryNodePosition);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryParentNodeState);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryWindowIdInDirectionOfFocusedWindow);
extern EVENT_CALLBACK(Callback_KWMEvent_QueryScratchpad);

enum kwm_event_type
{
    KWMEvent_QueryTilingMode,
    KWMEvent_QuerySplitMode,
    KWMEvent_QuerySplitRatio,
    KWMEvent_QuerySpawnPosition,

    KWMEvent_QueryFocusFollowsMouse,
    KWMEvent_QueryMouseFollowsFocus,
    KWMEvent_QueryCycleFocus,
    KWMEvent_QueryFloatNonResizable,
    KWMEvent_QueryLockToContainer,
    KWMEvent_QueryStandbyOnFloat,

    KWMEvent_QuerySpaces,
    KWMEvent_QueryCurrentSpaceId,
    KWMEvent_QueryCurrentSpaceName,
    KWMEvent_QueryCurrentSpaceMode,
    KWMEvent_QueryCurrentSpaceTag,
    KWMEvent_QueryPreviousSpaceId,
    KWMEvent_QueryPreviousSpaceName,

    KWMEvent_QueryFocusedBorder,
    KWMEvent_QueryMarkedBorder,

    KWMEvent_QueryFocusedWindowId,
    KWMEvent_QueryFocusedWindowOwner,
    KWMEvent_QueryFocusedWindowName,
    KWMEvent_QueryFocusedWindowSplit,
    KWMEvent_QueryFocusedWindowFloat,

    KWMEvent_QueryMarkedWindowId,
    KWMEvent_QueryMarkedWindowOwner,
    KWMEvent_QueryMarkedWindowName,
    KWMEvent_QueryMarkedWindowSplit,
    KWMEvent_QueryMarkedWindowFloat,

    KWMEvent_QueryWindowList,
    KWMEvent_QueryNodePosition,
    KWMEvent_QueryParentNodeState,
    KWMEvent_QueryWindowIdInDirectionOfFocusedWindow,
    KWMEvent_QueryScratchpad,
};

inline void *
KwmCreateContext(int Id)
{
    int *IdContext = (int *) malloc(sizeof(Id));
    *IdContext = Id;
    return IdContext;
}

/* NOTE(koekeishiya): Construct an ax_event with the appropriate callback through macro expansion. */
#define KwmConstructEvent(EventType, EventContext) \
    do { ax_event Event = {}; \
         Event.Context = EventContext; \
         Event.Intrinsic = false; \
         Event.Handle = &Callback_##EventType; \
         AXLibAddEvent(Event); \
       } while(0)

#endif
