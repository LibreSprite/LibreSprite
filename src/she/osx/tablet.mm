//
//  tablet.c
//  TabletLibC
//
//  Created by Anomalous Underdog on 5/6/12.
//
//  Copyright (c) 2012 Anomalous Underdog
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to
//  deal in the Software without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
//  sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//

#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include <ApplicationServices/ApplicationServices.h>

extern float penPressure;

namespace osx_tablet {

pthread_t tablet_thread_id;
CFRunLoopRef tablet_run_loop_ref;

long long point_x = 0;
long long point_y = 0;
long long point_z = 0;
long long buttons = 0;
double tilt_x = 0;
double tilt_y = 0;
double rotation = 0.0;
double pressure = 0.0;
double tangent_pressure = 0.0;

long long vendor1 = 0;
long long vendor2 = 0;
long long vendor3 = 0;

long long vendor_id = 0;
long long tablet_id = 0;
long long pointer_id = 0;
long long device_id = 0;
long long system_tablet_id = 0;
long long vendor_pointer_type = 0;
long long vendor_pointer_serial_number = 0;
long long vendor_unique_id = 0;
long long capability_mask = 0;
long long pointer_type = 0;
long long enter_proximity = 0;

long long get_point_x()
{
    return point_x;
}
long long get_point_y()
{
    return point_y;
}
long long get_point_z()
{
    return point_z;
}

long long get_buttons()
{
    return buttons;
}


double get_tilt_x()
{
    return tilt_x;
}
double get_tilt_y()
{
    return tilt_y;
}
double get_rotation()
{
    return rotation;
}
double get_pressure()
{
    return pressure;
}
double get_tangent_pressure()
{
    return tangent_pressure;
}


long long get_vendor1()
{
    return vendor1;
}
long long get_vendor2()
{
    return vendor2;
}
long long get_vendor3()
{
    return vendor3;
}


long long get_vendor_id()
{
    return vendor_id;
}
long long get_tablet_id()
{
    return tablet_id;
}
long long get_pointer_id()
{
    return pointer_id;
}
long long get_device_id()
{
    return device_id;
}
long long get_system_tablet_id()
{
    return system_tablet_id;
}
long long get_vendor_pointer_type()
{
    return vendor_pointer_type;
}
long long get_vendor_pointer_serial_number()
{
    return vendor_pointer_serial_number;
}
long long get_vendor_unique_id()
{
    return vendor_unique_id;
}
long long get_capability_mask()
{
    return capability_mask;
}
long long get_pointer_type()
{
    return pointer_type;
}
long long get_enter_proximity()
{
    return enter_proximity;
}


//
// from http://stackoverflow.com/questions/3134901/mouse-tracking-daemon
//

static CGEventRef tabletEventTapCallback (
                                          CGEventTapProxy proxy,
                                          CGEventType type,
                                          CGEventRef event,
                                          void * refcon
                                          ) {

    if (type == kCGEventTabletProximity)
    {
        vendor_id = CGEventGetIntegerValueField(event, kCGTabletProximityEventVendorID);
        tablet_id = CGEventGetIntegerValueField(event, kCGTabletProximityEventTabletID);
        pointer_id = CGEventGetIntegerValueField(event, kCGTabletProximityEventPointerID);
        device_id = CGEventGetIntegerValueField(event, kCGTabletProximityEventDeviceID);
        system_tablet_id = CGEventGetIntegerValueField(event, kCGTabletProximityEventSystemTabletID);
        vendor_pointer_type = CGEventGetIntegerValueField(event, kCGTabletProximityEventVendorPointerType);
        vendor_pointer_serial_number = CGEventGetIntegerValueField(event, kCGTabletProximityEventVendorPointerSerialNumber);
        vendor_unique_id = CGEventGetIntegerValueField(event, kCGTabletProximityEventVendorUniqueID);
        capability_mask = CGEventGetIntegerValueField(event, kCGTabletProximityEventCapabilityMask);
        pointer_type = CGEventGetIntegerValueField(event, kCGTabletProximityEventPointerType);
        enter_proximity = CGEventGetIntegerValueField(event, kCGTabletProximityEventEnterProximity);
    }

    point_x = CGEventGetIntegerValueField(event, kCGTabletEventPointX);
    point_y = CGEventGetIntegerValueField(event, kCGTabletEventPointY);
    point_z = CGEventGetIntegerValueField(event, kCGTabletEventPointZ);
    buttons = CGEventGetIntegerValueField(event, kCGTabletEventPointButtons);

    tilt_x = CGEventGetDoubleValueField(event, kCGTabletEventTiltX);
    tilt_y = CGEventGetDoubleValueField(event, kCGTabletEventTiltY);
    rotation = CGEventGetDoubleValueField(event, kCGTabletEventRotation);
    pressure = CGEventGetDoubleValueField(event, kCGTabletEventPointPressure);
    tangent_pressure = CGEventGetDoubleValueField(event, kCGTabletEventTangentialPressure);

    penPressure = pressure;

    device_id = CGEventGetIntegerValueField(event, kCGTabletEventDeviceID);
    vendor1 = CGEventGetIntegerValueField(event, kCGTabletEventVendor1);
    vendor2 = CGEventGetIntegerValueField(event, kCGTabletEventVendor2);
    vendor3 = CGEventGetIntegerValueField(event, kCGTabletEventVendor3);

    // Pass on the event, we must not modify it anyway, we are a listener
    return event;
}

void* _init(void* data)
{
    CGEventMask emask;
    CFMachPortRef myEventTap;
    CFRunLoopSourceRef eventTapRLSrc;
    tablet_run_loop_ref = CFRunLoopGetCurrent();

    emask = CGEventMaskBit(kCGEventLeftMouseDragged) | CGEventMaskBit(kCGEventTabletProximity) | CGEventMaskBit(kCGEventTabletPointer);

    // Create the Tap
    myEventTap = CGEventTapCreate (
                                   kCGSessionEventTap, // Catch all events for current user session
                                   kCGHeadInsertEventTap, // Append to end of EventTap list
                                   kCGEventTapOptionListenOnly, // We only listen, we don't modify
                                   emask,
                                   &tabletEventTapCallback,
                                   NULL // We need no extra data in the callback
                                   );

    // Create a RunLoop Source for it
    eventTapRLSrc = CFMachPortCreateRunLoopSource(
                                                  kCFAllocatorDefault,
                                                  myEventTap,
                                                  0
                                                  );

    // Add the source to the current RunLoop
    CFRunLoopAddSource(
                       tablet_run_loop_ref,
                       eventTapRLSrc,
                       kCFRunLoopDefaultMode
                       );

    // Keep the RunLoop running forever
    CFRunLoopRun();

    return NULL;
}

int init()
{
    // Create the thread using POSIX routines.
    pthread_attr_t attr;
    int returnVal;

    returnVal = pthread_attr_init(&attr);
    if  (returnVal) return -1;
    returnVal = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (returnVal) return -2;

    int threadError = pthread_create(&tablet_thread_id, &attr, &_init, NULL);

    returnVal = pthread_attr_destroy(&attr);
    if (returnVal) return -3;

    return threadError;
}

int stop()
{
    // not sure if removing this will cause problems
    // but it crashes if I add it
    //CFRunLoopStop(tablet_run_loop_ref);

    // I'm pretty sure there's a better way than this...
    return pthread_kill(tablet_thread_id, 0);
    //pthread_cancel(posixThreadID);
}

}
