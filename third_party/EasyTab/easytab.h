/*
    EasyTab.h - Single-header multi-platform tablet library
    https://github.com/ApoorvaJ/EasyTab

    ----------------------------------------------------------------------------
    USAGE
    ----------------------------------------------------------------------------
    1) Add the following lines in exactly one of your cpp files to compile the
       implementation.

           #define EASYTAB_IMPLEMENTATION
           #include "easytab.h"

    2) Call EasyTab_Load() with correct parameters to initialize EasyTab. These
       parameters vary per OS, so look at the function declarations or examples
       below. Function returns EASYTAB_OK if initialization was successful.

    3) Call EasyTab_HandleEvent() in your message-handling code. The function
       returns EASYTAB_OK if the message was a tablet message, and
       EASYTAB_EVENT_NOT_HANDLED otherwise.

    4) Call EasyTab_Unload() in your shutdown code.

    5) Once initialized, you can query tablet state using the EasyTab pointer.
       e.g.:

           EasyTab->PosX        // X position of the pen
           EasyTab->PosY        // Y position of the pen
           EasyTab->Pressure    // Pressure of the pen ranging from 0.0f to 1.0f

       For more info, have a look at the EasyTabInfo struct below.


    * Add -lXi to compiler options to link XInput on Linux.

    ----------------------------------------------------------------------------
    EXAMPLES
    ----------------------------------------------------------------------------
    1) Windows:

        int CALLBACK WinMain(...)
        {
            HWND Window;

            ...

            if (EasyTab_Load(Window) != EASYTAB_OK)                                  // Load
            {
                OutputDebugStringA("Tablet init failed\n");
            }

            ...

            // Once you've set up EasyTab loading, unloading and event handling,
            // use the EasyTab variable at any point in your program to access
            // the tablet state:
            //    EasyTab->PosX
            //    EasyTab->PosY
            //    EasyTab->Pressure
            // For more tablet information, look at the EasyTabInfo struct.

            ...

            EasyTab_Unload();                                                      // Unload
        }

        LRESULT CALLBACK WindowProc(
            HWND Window,
            UINT Message,
            WPARAM WParam,
            LPARAM LParam)
        {
            if (EasyTab_HandleEvent(Window, Message, LParam, WParam) == EASYTAB_OK) // Event
            {
                return true; // Tablet event handled
            }

            switch (Message)
            {
                ...
            }
        }


    2) Linux:

        int main(...)
        {
            Display* Disp;
            Window   Win;

            ...

            if (EasyTab_Load(Disp, Win) != EASYTAB_OK)                   // Load
            {
                printf("Tablet init failed\n");
            }

            ...

            while (XPending(Disp)) // Event loop
            {
                XEvent Event;
                XNextEvent(XlibDisplay, &Event);

                if (EasyTab_HandleEvent(&Event) == EASYTAB_OK)          // Event
                {
                    continue; // Tablet event handled
                }

                switch (Event.type)
                {
                    ...
                }
            }

            ...

            // Once you've set up EasyTab loading, unloading and event handling,
            // use the EasyTab variable at any point in your program to access
            // the tablet state:
            //    EasyTab->PosX
            //    EasyTab->PosY
            //    EasyTab->Pressure
            // For more tablet information, look at the EasyTabInfo struct.

            ...

            EasyTab_Unload();                                          // Unload
        }

    ----------------------------------------------------------------------------
    CREDITS
    ----------------------------------------------------------------------------
    Apoorva Joshi       apoorvaj.io
    Sergio Gonzalez     s3rg.io

    This library is coded in the spirit of the stb libraries and follows the stb
    guidelines.

*/

// TODO: Null checks and warnings for EasyTab
// TODO: Differentiate between stylus and eraser in the API
// TODO: Linux support for relative mode
// TODO: Documentation for relative mode

// =============================================================================
// EasyTab header section
// =============================================================================

#ifndef EASYTAB_H
#define EASYTAB_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <X11/extensions/XInput.h>
#endif // __linux__

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#include <regex>

typedef enum
{
    EASYTAB_OK = 0,

    // Errors
    EASYTAB_MEMORY_ERROR           = -1,
    EASYTAB_X11_ERROR              = -2,
    EASYTAB_DLL_LOAD_ERROR         = -3,
    EASYTAB_WACOM_WIN32_ERROR      = -4,
    EASYTAB_INVALID_FUNCTION_ERROR = -5,

    EASYTAB_EVENT_NOT_HANDLED = -16,
} EasyTabResult;

typedef enum
{
    EASYTAB_TRACKING_MODE_SYSTEM   = 0,
    EASYTAB_TRACKING_MODE_RELATIVE = 1,
} EasyTabTrackingMode;

#ifdef WIN32
// -----------------------------------------------------------------------------
// wintab.h
// -----------------------------------------------------------------------------
#if 1

    DECLARE_HANDLE(HMGR);
    DECLARE_HANDLE(HCTX);

    typedef DWORD WTPKT;
    typedef DWORD FIX32;

    // Messages
    #if 1

        #define WT_DEFBASE          0x7FF0
        #define WT_MAXOFFSET        0xF

        #define _WT_PACKET(b)       ((b)+0)
        #define _WT_CTXOPEN(b)      ((b)+1)
        #define _WT_CTXCLOSE(b)     ((b)+2)
        #define _WT_CTXUPDATE(b)    ((b)+3)
        #define _WT_CTXOVERLAP(b)   ((b)+4)
        #define _WT_PROXIMITY(b)    ((b)+5)
        #define _WT_INFOCHANGE(b)   ((b)+6)
        #define _WT_CSRCHANGE(b)    ((b)+7) /* 1.1 */
        #define _WT_PACKETEXT(b)    ((b)+8) /* 1.4 */
        #define _WT_MAX(b)          ((b)+WT_MAXOFFSET)

        #define WT_PACKET           _WT_PACKET(WT_DEFBASE)
        #define WT_CTXOPEN          _WT_CTXOPEN(WT_DEFBASE)
        #define WT_CTXCLOSE         _WT_CTXCLOSE(WT_DEFBASE)
        #define WT_CTXUPDATE        _WT_CTXUPDATE(WT_DEFBASE)
        #define WT_CTXOVERLAP       _WT_CTXOVERLAP(WT_DEFBASE)
        #define WT_PROXIMITY        _WT_PROXIMITY(WT_DEFBASE)
        #define WT_INFOCHANGE       _WT_INFOCHANGE(WT_DEFBASE)
        #define WT_CSRCHANGE        _WT_CSRCHANGE(WT_DEFBASE) /* 1.1 */
        #define WT_PACKETEXT        _WT_PACKETEXT(WT_DEFBASE) /* 1.4 */
        #define WT_MAX              _WT_MAX(WT_DEFBASE)

    #endif // Messages

    // Flags
    #if 1

        #define CTX_NAME        1
        #define CTX_OPTIONS     2
        #define CTX_STATUS      3
        #define CTX_LOCKS       4
        #define CTX_MSGBASE     5
        #define CTX_DEVICE      6
        #define CTX_PKTRATE     7
        #define CTX_PKTDATA     8
        #define CTX_PKTMODE     9
        #define CTX_MOVEMASK    10
        #define CTX_BTNDNMASK   11
        #define CTX_BTNUPMASK   12
        #define CTX_INORGX      13
        #define CTX_INORGY      14
        #define CTX_INORGZ      15
        #define CTX_INEXTX      16
        #define CTX_INEXTY      17
        #define CTX_INEXTZ      18
        #define CTX_OUTORGX     19
        #define CTX_OUTORGY     20
        #define CTX_OUTORGZ     21
        #define CTX_OUTEXTX     22
        #define CTX_OUTEXTY     23
        #define CTX_OUTEXTZ     24
        #define CTX_SENSX       25
        #define CTX_SENSY       26
        #define CTX_SENSZ       27
        #define CTX_SYSMODE     28
        #define CTX_SYSORGX     29
        #define CTX_SYSORGY     30
        #define CTX_SYSEXTX     31
        #define CTX_SYSEXTY     32
        #define CTX_SYSSENSX    33
        #define CTX_SYSSENSY    34
        #define CTX_MAX         34

        // Context option values
        #define CXO_SYSTEM      0x0001
        #define CXO_PEN         0x0002
        #define CXO_MESSAGES    0x0004
        #define CXO_MARGIN      0x8000
        #define CXO_MGNINSIDE   0x4000
        #define CXO_CSRMESSAGES 0x0008 /* 1.1 */

        #define DVC_NAME        1
        #define DVC_HARDWARE    2
        #define DVC_NCSRTYPES   3
        #define DVC_FIRSTCSR    4
        #define DVC_PKTRATE     5
        #define DVC_PKTDATA     6
        #define DVC_PKTMODE     7
        #define DVC_CSRDATA     8
        #define DVC_XMARGIN     9
        #define DVC_YMARGIN     10
        #define DVC_ZMARGIN     11
        #define DVC_X           12
        #define DVC_Y           13
        #define DVC_Z           14
        #define DVC_NPRESSURE   15
        #define DVC_TPRESSURE   16
        #define DVC_ORIENTATION 17
        #define DVC_ROTATION    18 /* 1.1 */
        #define DVC_PNPID       19 /* 1.1 */
        #define DVC_MAX         19

        #define PK_CONTEXT           0x0001 // reporting context
        #define PK_STATUS            0x0002 // status bits
        #define PK_TIME              0x0004 // time stamp
        #define PK_CHANGED           0x0008 // change bit vector
        #define PK_SERIAL_NUMBER     0x0010 // packet serial number
        #define PK_CURSOR            0x0020 // reporting cursor
        #define PK_BUTTONS           0x0040 // button information
        #define PK_X                 0x0080 // x axis
        #define PK_Y                 0x0100 // y axis
        #define PK_Z                 0x0200 // z axis
        #define PK_NORMAL_PRESSURE   0x0400 // normal or tip pressure
        #define PK_TANGENT_PRESSURE  0x0800 // tangential or barrel pressure
        #define PK_ORIENTATION       0x1000 // orientation info: tilts
        #define PK_ROTATION          0x2000 // rotation info; 1.1

        // constants for use with pktdef.h
        #define PKEXT_ABSOLUTE  1
        #define PKEXT_RELATIVE  2

        #define WTI_DEFCONTEXT  3
        #define WTI_DEFSYSCTX   4
        #define WTI_DEVICES     100
        #define WTI_DDCTXS      400 /* 1.1 */
        #define WTI_DSCTXS      500 /* 1.1 */

    #endif // Flags

    typedef struct tagAXIS {
        LONG    axMin;
        LONG    axMax;
        UINT    axUnits;
        FIX32   axResolution;
    } AXIS, *PAXIS, NEAR *NPAXIS, FAR *LPAXIS;

    #define LCNAMELEN 40
    typedef struct tagLOGCONTEXTA {
        char    lcName[LCNAMELEN];
        UINT    lcOptions;
        UINT    lcStatus;
        UINT    lcLocks;
        UINT    lcMsgBase;
        UINT    lcDevice;
        UINT    lcPktRate;
        WTPKT   lcPktData;
        WTPKT   lcPktMode;
        WTPKT   lcMoveMask;
        DWORD   lcBtnDnMask;
        DWORD   lcBtnUpMask;
        LONG    lcInOrgX;
        LONG    lcInOrgY;
        LONG    lcInOrgZ;
        LONG    lcInExtX;
        LONG    lcInExtY;
        LONG    lcInExtZ;
        LONG    lcOutOrgX;
        LONG    lcOutOrgY;
        LONG    lcOutOrgZ;
        LONG    lcOutExtX;
        LONG    lcOutExtY;
        LONG    lcOutExtZ;
        FIX32   lcSensX;
        FIX32   lcSensY;
        FIX32   lcSensZ;
        BOOL    lcSysMode;
        int     lcSysOrgX;
        int     lcSysOrgY;
        int     lcSysExtX;
        int     lcSysExtY;
        FIX32   lcSysSensX;
        FIX32   lcSysSensY;
    } LOGCONTEXTA, *PLOGCONTEXTA, NEAR *NPLOGCONTEXTA, FAR *LPLOGCONTEXTA;

    typedef struct tagEXTENSIONBASE { /* 1.4 */
        HCTX    nContext;
        UINT    nStatus;
        DWORD   nTime;
        UINT    nSerialNumber;
    } EXTENSIONBASE;

#endif // wintab.h
// -----------------------------------------------------------------------------

#define PACKETDATA PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE
#define PACKETMODE 0

// -----------------------------------------------------------------------------
// pktdef.h
// -----------------------------------------------------------------------------
#if 1

    // TODO: Simplify this file if we have a fixed packet format.
    //       The macros here are too ugly.

    #ifndef PACKETNAME
    /* if no packet name prefix */
    #define __PFX(x)    x
    #define __IFX(x,y)  x ## y
    #else
    /* add prefixes and infixes to packet format names */
    #define __PFX(x)        __PFX2(PACKETNAME,x)
    #define __PFX2(p,x)     __PFX3(p,x)
    #define __PFX3(p,x)     p ## x
    #define __IFX(x,y)      __IFX2(x,PACKETNAME,y)
    #define __IFX2(x,i,y)   __IFX3(x,i,y)
    #define __IFX3(x,i,y)   x ## i ## y
    #endif

    #define __SFX2(x,s)     __SFX3(x,s)
    #define __SFX3(x,s)     x ## s

    #define __TAG   __IFX(tag,PACKET)
    #define __TYPES __PFX(PACKET), * __IFX(P,PACKET), NEAR * __IFX(NP,PACKET), FAR * __IFX(LP,PACKET)

    #define __TAGE      __IFX(tag,PACKETEXT)
    #define __TYPESE    __PFX(PACKETEXT), * __IFX(P,PACKETEXT), NEAR * __IFX(NP,PACKETEXT), FAR * __IFX(LP,PACKETEXT)

    #define __DATA      (__PFX(PACKETDATA))
    #define __MODE      (__PFX(PACKETMODE))
    #define __EXT(x)    __SFX2(__PFX(PACKET),x)


    typedef struct __TAG {
    #if (__DATA & PK_CONTEXT)
        HCTX            pkContext;
    #endif
    #if (__DATA & PK_STATUS)
        UINT            pkStatus;
    #endif
    #if (__DATA & PK_TIME)
        DWORD           pkTime;
    #endif
    #if (__DATA & PK_CHANGED)
        WTPKT           pkChanged;
    #endif
    #if (__DATA & PK_SERIAL_NUMBER)
        UINT            pkSerialNumber;
    #endif
    #if (__DATA & PK_CURSOR)
        UINT            pkCursor;
    #endif
    #if (__DATA & PK_BUTTONS)
        DWORD           pkButtons;
    #endif
    #if (__DATA & PK_X)
        LONG            pkX;
    #endif
    #if (__DATA & PK_Y)
        LONG            pkY;
    #endif
    #if (__DATA & PK_Z)
        LONG            pkZ;
    #endif
    #if (__DATA & PK_NORMAL_PRESSURE)
    #if (__MODE & PK_NORMAL_PRESSURE)
        /* relative */
        int         pkNormalPressure;
    #else
        /* absolute */
        UINT        pkNormalPressure;
    #endif
    #endif
    #if (__DATA & PK_TANGENT_PRESSURE)
    #if (__MODE & PK_TANGENT_PRESSURE)
        /* relative */
        int         pkTangentPressure;
    #else
        /* absolute */
        UINT        pkTangentPressure;
    #endif
    #endif
    #if (__DATA & PK_ORIENTATION)
        ORIENTATION     pkOrientation;
    #endif
    #if (__DATA & PK_ROTATION)
        ROTATION        pkRotation; /* 1.1 */
    #endif

    #ifndef NOWTEXTENSIONS
                                    /* extensions begin here. */
    #if (__EXT(FKEYS) == PKEXT_RELATIVE) || (__EXT(FKEYS) == PKEXT_ABSOLUTE)
        UINT            pkFKeys;
    #endif
    #if (__EXT(TILT) == PKEXT_RELATIVE) || (__EXT(TILT) == PKEXT_ABSOLUTE)
        TILT            pkTilt;
    #endif
    #endif

    } __TYPES;

    #ifndef NOWTEXTENSIONS
    typedef struct __TAGE {
        EXTENSIONBASE   pkBase;

    #if (__EXT(EXPKEYS) == PKEXT_RELATIVE) || (__EXT(EXPKEYS) == PKEXT_ABSOLUTE)
        EXPKEYSDATA pkExpKeys; /* 1.4 */
    #endif
    #if (__EXT(TOUCHSTRIP) == PKEXT_RELATIVE) || (__EXT(TOUCHSTRIP) == PKEXT_ABSOLUTE)
        SLIDERDATA  pkTouchStrip; /* 1.4 */
    #endif
    #if (__EXT(TOUCHRING) == PKEXT_RELATIVE) || (__EXT(TOUCHRING) == PKEXT_ABSOLUTE)
        SLIDERDATA  pkTouchRing; /* 1.4 */
    #endif

    } __TYPESE;
    #endif

    #undef PACKETNAME
    #undef __TAG
    #undef __TAGE
    #undef __TAG2
    #undef __TYPES
    #undef __TYPESE
    #undef __TYPES2
    #undef __DATA
    #undef __MODE
    #undef __PFX
    #undef __PFX2
    #undef __PFX3
    #undef __IFX
    #undef __IFX2
    #undef __IFX3
    #undef __SFX2
    #undef __SFX3

#endif // pktdef.h
// -----------------------------------------------------------------------------

typedef UINT (WINAPI * WTINFOA) (UINT, UINT, LPVOID);
typedef HCTX (WINAPI * WTOPENA) (HWND, LPLOGCONTEXTA, BOOL);
typedef BOOL (WINAPI * WTGETA) (HCTX, LPLOGCONTEXTA);
typedef BOOL (WINAPI * WTSETA) (HCTX, LPLOGCONTEXTA);
typedef BOOL (WINAPI * WTCLOSE) (HCTX);
typedef BOOL (WINAPI * WTENABLE) (HCTX, BOOL);
typedef BOOL (WINAPI * WTPACKET) (HCTX, UINT, LPVOID);
typedef BOOL (WINAPI * WTOVERLAP) (HCTX, BOOL);
typedef BOOL (WINAPI * WTSAVE) (HCTX, LPVOID);
typedef BOOL (WINAPI * WTCONFIG) (HCTX, HWND);
typedef HCTX (WINAPI * WTRESTORE) (HWND, LPVOID, BOOL);
typedef BOOL (WINAPI * WTEXTSET) (HCTX, UINT, LPVOID);
typedef BOOL (WINAPI * WTEXTGET) (HCTX, UINT, LPVOID);
typedef BOOL (WINAPI * WTQUEUESIZESET) (HCTX, int);
typedef int  (WINAPI * WTDATAPEEK) (HCTX, UINT, UINT, int, LPVOID, LPINT);
typedef int  (WINAPI * WTPACKETSGET) (HCTX, int, LPVOID);
typedef HMGR (WINAPI * WTMGROPEN) (HWND, UINT);
typedef BOOL (WINAPI * WTMGRCLOSE) (HMGR);
typedef HCTX (WINAPI * WTMGRDEFCONTEXT) (HMGR, BOOL);
typedef HCTX (WINAPI * WTMGRDEFCONTEXTEX) (HMGR, UINT, BOOL);

#endif // WIN32

// -----------------------------------------------------------------------------
// Enums
// -----------------------------------------------------------------------------

/*
    Use this enum in conjunction with EasyTab->Buttons to check for tablet button
    presses.
    e.g. To check for lower pen button press, use:

    if (EasyTab->Buttons & EasyTab_Buttons_Pen_Lower)
    {
        // Lower button is pressed
    }
*/
enum EasyTab_Buttons_
{
    EasyTab_Buttons_Pen_Touch = 1 << 0, // Pen is touching tablet
    EasyTab_Buttons_Pen_Lower = 1 << 1, // Lower pen button is pressed
    EasyTab_Buttons_Pen_Upper = 1 << 2, // Upper pen button is pressed
};

// -----------------------------------------------------------------------------
// Structs
// -----------------------------------------------------------------------------
typedef struct
{
    int32_t PosX, PosY;
    float   Pressure; // Range: 0.0f to 1.0f
    int32_t Buttons; // Bit field. Use with the EasyTab_Buttons_ enum.

    int32_t RangeX, RangeY;
    int32_t MaxPressure;

#ifdef __linux__
    XDevice* Device;
    uint32_t MotionType;
    XEventClass EventClasses[1024];
    uint32_t NumEventClasses;
#endif // __linux__

#ifdef WIN32
    HINSTANCE Dll;
    HCTX      Context;

    WTINFOA           WTInfoA;
    WTOPENA           WTOpenA;
    WTGETA            WTGetA;
    WTSETA            WTSetA;
    WTCLOSE           WTClose;
    WTPACKET          WTPacket;
    WTENABLE          WTEnable;
    WTOVERLAP         WTOverlap;
    WTSAVE            WTSave;
    WTCONFIG          WTConfig;
    WTRESTORE         WTRestore;
    WTEXTSET          WTExtSet;
    WTEXTGET          WTExtGet;
    WTQUEUESIZESET    WTQueueSizeSet;
    WTDATAPEEK        WTDataPeek;
    WTPACKETSGET      WTPacketsGet;
    WTMGROPEN         WTMgrOpen;
    WTMGRCLOSE        WTMgrClose;
    WTMGRDEFCONTEXT   WTMgrDefContext;
    WTMGRDEFCONTEXTEX WTMgrDefContextEx;
#endif // WIN32
} EasyTabInfo;

extern EasyTabInfo* EasyTab;

// -----------------------------------------------------------------------------
// Function declarations
// -----------------------------------------------------------------------------
#if defined(__linux__)

    EasyTabResult EasyTab_Load(Display* Disp, Window Win);
    EasyTabResult EasyTab_HandleEvent(XEvent* Event);
    void EasyTab_Unload(Display* Disp);

#elif defined(_WIN32)

    EasyTabResult EasyTab_Load(HWND Window);
    EasyTabResult EasyTab_Load_Ex(HWND Window,
                                  EasyTabTrackingMode Mode,
                                  float RelativeModeSensitivity,
                                  int32_t MoveCursor);
    EasyTabResult EasyTab_HandleEvent(HWND Window,
                                      UINT Message,
                                      LPARAM LParam,
                                      WPARAM WParam);
    void EasyTab_Unload();

#else

    // Save some trouble when porting.
    #error "Unsupported platform."

#endif // __linux__ _WIN32
// -----------------------------------------------------------------------------

#endif // EASYTAB_H



// =============================================================================
// EasyTab implementation section
// =============================================================================

#ifdef EASYTAB_IMPLEMENTATION

EasyTabInfo* EasyTab;

// -----------------------------------------------------------------------------
// Linux implementation
// -----------------------------------------------------------------------------
#ifdef __linux__

EasyTabResult EasyTab_Load(Display* Disp, Window Win)
{
    EasyTab = (EasyTabInfo*)calloc(1, sizeof(EasyTabInfo)); // We want init to zero, hence calloc.
    if (!EasyTab) { return EASYTAB_MEMORY_ERROR; }

    int32_t Count;
    XDeviceInfoPtr Devices = (XDeviceInfoPtr)XListInputDevices(Disp, &Count);
    if (!Devices) { return EASYTAB_X11_ERROR; }

    std::regex isTabletTest {"\\s+(pen|stylus|eraser)(?:\\s+|$)", std::regex::icase};

    for (int32_t i = 0; i < Count; i++)
    {
	bool match = std::regex_search(Devices[i].name, isTabletTest);
	/* std::cout << "Tablet? " << Devices[i].name << " = " << match << std::endl; */
        if (!match) { continue; }

        EasyTab->Device = XOpenDevice(Disp, Devices[i].id);
        XAnyClassPtr ClassPtr = Devices[i].inputclassinfo;

        for (int32_t j = 0; j < Devices[i].num_classes; j++)
        {
#if defined(__cplusplus)
            switch (ClassPtr->c_class)
#else
            switch (ClassPtr->class)
#endif
            {
                case ValuatorClass:
                {
                    XValuatorInfo *Info = (XValuatorInfo *)ClassPtr;
                    // X
                    if (Info->num_axes > 0)
                    {
                        int32_t min     = Info->axes[0].min_value;
                        EasyTab->RangeX = Info->axes[0].max_value;
                        //printf("Max/min x values: %d, %d\n", min, EasyTab->RangeX); // TODO: Platform-print macro
                    }

                    // Y
                    if (Info->num_axes > 1)
                    {
                        int32_t min     = Info->axes[1].min_value;
                        EasyTab->RangeY = Info->axes[1].max_value;
                        //printf("Max/min y values: %d, %d\n", min, EasyTab->RangeY);
                    }

                    // Pressure
                    if (Info->num_axes > 2)
                    {
                        int32_t min          = Info->axes[2].min_value;
                        EasyTab->MaxPressure = Info->axes[2].max_value;
                        //printf("Max/min pressure values: %d, %d\n", min, EasyTab->MaxPressure);
                    }

                    XEventClass EventClass;
                    DeviceMotionNotify(EasyTab->Device, EasyTab->MotionType, EventClass);
                    if (EventClass)
                    {
                        EasyTab->EventClasses[EasyTab->NumEventClasses] = EventClass;
                        EasyTab->NumEventClasses++;
                    }
                } break;
            }

            ClassPtr = (XAnyClassPtr) ((uint8_t*)ClassPtr + ClassPtr->length); // TODO: Access this as an array to avoid pointer arithmetic?
        }

        XSelectExtensionEvent(Disp, Win, EasyTab->EventClasses, EasyTab->NumEventClasses);
    }

    XFreeDeviceList(Devices);

    if (EasyTab->Device != 0) { return EASYTAB_OK; }
    else                      { std::cout << "ERROR2" << std::endl; return EASYTAB_X11_ERROR; }
}

EasyTabResult EasyTab_HandleEvent(XEvent* Event)
{
    if (Event->type != EasyTab->MotionType) { return EASYTAB_EVENT_NOT_HANDLED; }

    XDeviceMotionEvent* MotionEvent = (XDeviceMotionEvent*)(Event);
    EasyTab->PosX     = MotionEvent->x;
    EasyTab->PosY     = MotionEvent->y;
    EasyTab->Pressure = (float)MotionEvent->axis_data[2] / (float)EasyTab->MaxPressure;
    return EASYTAB_OK;
}

void EasyTab_Unload(Display* Disp)
{
    XCloseDevice(Disp, EasyTab->Device);
    free(EasyTab);
    EasyTab = NULL;
}

#endif // __linux__


// -----------------------------------------------------------------------------
// Windows implementation
// -----------------------------------------------------------------------------
#ifdef WIN32

#define GETPROCADDRESS(type, func)                                              \
    EasyTab->func = (type)GetProcAddress(EasyTab->Dll, #func);                  \
    if (!EasyTab->func)                                                         \
    {                                                                           \
        OutputDebugStringA("Function " #func " not found in Wintab32.dll.\n");  \
        return EASYTAB_INVALID_FUNCTION_ERROR;                                                           \
    }


EasyTabResult EasyTab_Load(HWND Window)
{
    return EasyTab_Load_Ex(Window, EASYTAB_TRACKING_MODE_SYSTEM, 0, 1);
}

EasyTabResult EasyTab_Load_Ex(HWND Window,
                              EasyTabTrackingMode TrackingMode,
                              float RelativeModeSensitivity,
                              int32_t MoveCursor)
{
    EasyTab = (EasyTabInfo*)calloc(1, sizeof(EasyTabInfo)); // We want init to zero, hence calloc.
    if (!EasyTab) { return EASYTAB_MEMORY_ERROR; }

    // Load Wintab DLL and get function addresses
    {
        EasyTab->Dll = LoadLibraryA("Wintab32.dll");
        if (!EasyTab->Dll)
        {
            OutputDebugStringA("Wintab32.dll not found.\n");
            return EASYTAB_DLL_LOAD_ERROR;
        }

        GETPROCADDRESS(WTINFOA           , WTInfoA);
        GETPROCADDRESS(WTOPENA           , WTOpenA);
        GETPROCADDRESS(WTGETA            , WTGetA);
        GETPROCADDRESS(WTSETA            , WTSetA);
        GETPROCADDRESS(WTCLOSE           , WTClose);
        GETPROCADDRESS(WTPACKET          , WTPacket);
        GETPROCADDRESS(WTENABLE          , WTEnable);
        GETPROCADDRESS(WTOVERLAP         , WTOverlap);
        GETPROCADDRESS(WTSAVE            , WTSave);
        GETPROCADDRESS(WTCONFIG          , WTConfig);
        GETPROCADDRESS(WTRESTORE         , WTRestore);
        GETPROCADDRESS(WTEXTSET          , WTExtSet);
        GETPROCADDRESS(WTEXTGET          , WTExtGet);
        GETPROCADDRESS(WTQUEUESIZESET    , WTQueueSizeSet);
        GETPROCADDRESS(WTDATAPEEK        , WTDataPeek);
        GETPROCADDRESS(WTPACKETSGET      , WTPacketsGet);
        GETPROCADDRESS(WTMGROPEN         , WTMgrOpen);
        GETPROCADDRESS(WTMGRCLOSE        , WTMgrClose);
        GETPROCADDRESS(WTMGRDEFCONTEXT   , WTMgrDefContext);
        GETPROCADDRESS(WTMGRDEFCONTEXTEX , WTMgrDefContextEx);
    }

    if (!EasyTab->WTInfoA(0, 0, NULL))
    {
        OutputDebugStringA("Wintab services not available.\n");
        return EASYTAB_WACOM_WIN32_ERROR;
    }

    // Open context
    {
        LOGCONTEXTA LogContext = {0};
        AXIS        RangeX     = {0};
        AXIS        RangeY     = {0};
        AXIS        Pressure   = {0};

        EasyTab->WTInfoA(WTI_DEFCONTEXT, 0, &LogContext);
        EasyTab->WTInfoA(WTI_DEVICES, DVC_X, &RangeX);
        EasyTab->WTInfoA(WTI_DEVICES, DVC_Y, &RangeY);
        EasyTab->WTInfoA(WTI_DEVICES, DVC_NPRESSURE, &Pressure);

        LogContext.lcPktData = PACKETDATA; // ??
        LogContext.lcOptions |= CXO_MESSAGES;
        if (MoveCursor) { LogContext.lcOptions |= CXO_SYSTEM; }
        LogContext.lcPktMode = PACKETMODE;
        LogContext.lcMoveMask = PACKETDATA;
        LogContext.lcBtnUpMask = LogContext.lcBtnDnMask;

        LogContext.lcOutOrgX = 0;
        LogContext.lcOutOrgY = 0;
        LogContext.lcOutExtX = GetSystemMetrics(SM_CXSCREEN);
        LogContext.lcOutExtY = -GetSystemMetrics(SM_CYSCREEN);

        LogContext.lcSysOrgX = 0;
        LogContext.lcSysOrgY = 0;
        LogContext.lcSysExtX = GetSystemMetrics(SM_CXSCREEN);
        LogContext.lcSysExtY = GetSystemMetrics(SM_CYSCREEN);

        if (TrackingMode == EASYTAB_TRACKING_MODE_RELATIVE)
        {
            LogContext.lcPktMode |= PK_X | PK_Y; // TODO: Should this be included in the
                                                 //       PACKETMODE macro define up top?
            LogContext.lcSysMode = 1;

            if (RelativeModeSensitivity > 1.0f)
            {
                RelativeModeSensitivity = 1.0f;
            }
            else if (RelativeModeSensitivity < 0.0f)
            {
                RelativeModeSensitivity = 0.0f;
            }

            // Wintab expects sensitivity to be a 32-bit fixed point number
            // with the radix point between the two words. Thus, the type
            // contains 16 bits to the left of the radix point and 16 bits to
            // the right of it.
            // 
            // 0x10000 Hex
            // = 65,536 Decimal
            // = 0000 0000 0000 0001 . 0000 0000 0000 0000 Binary
            // = 1.0 Fixed Point
            uint32_t Sensitivity = (uint32_t)(0x10000 * RelativeModeSensitivity);

            if (MoveCursor)
            {
                LogContext.lcSysSensX = LogContext.lcSysSensY = Sensitivity;
            }
            else
            {
                LogContext.lcSensX = LogContext.lcSensY = Sensitivity;
            }
        }

        EasyTab->Context = EasyTab->WTOpenA(Window, &LogContext, TRUE);

        if (!EasyTab->Context)
        {
            OutputDebugStringA("Wintab context couldn't be opened.\n");
            return EASYTAB_WACOM_WIN32_ERROR;
        }

        // Get tablet capabilites
        {
            EasyTab->MaxPressure = Pressure.axMax;
            EasyTab->RangeX      = RangeX.axMax;
            EasyTab->RangeY      = RangeY.axMax;
        }
    }

    return EASYTAB_OK;
}

#undef GETPROCADDRESS

EasyTabResult EasyTab_HandleEvent(HWND Window, UINT Message, LPARAM LParam, WPARAM WParam)
{
    PACKET Packet = { 0 };

    if (Message == WT_PACKET &&
        (HCTX)LParam == EasyTab->Context &&
        EasyTab->WTPacket(EasyTab->Context, (UINT)WParam, &Packet))
    {
        POINT Point = { 0 };
        Point.x = Packet.pkX;
        Point.y = Packet.pkY;
        ScreenToClient(Window, &Point);
        EasyTab->PosX = Point.x;
        EasyTab->PosY = Point.y;

        EasyTab->Pressure = (float)Packet.pkNormalPressure / (float)EasyTab->MaxPressure;
        EasyTab->Buttons = Packet.pkButtons;
        return EASYTAB_OK;
    }

    return EASYTAB_EVENT_NOT_HANDLED;
}

void EasyTab_Unload()
{
    if (EasyTab->Context) { EasyTab->WTClose(EasyTab->Context); }
    if (EasyTab->Dll)     { FreeLibrary(EasyTab->Dll); }
    free(EasyTab);
    EasyTab = NULL;
}

#endif // WIN32
// -----------------------------------------------------------------------------


#endif // EASYTAB_IMPLEMENTATION
