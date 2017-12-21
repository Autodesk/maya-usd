/**
 *
\defgroup  events  USDMaya Event System
\ingroup  mayautils
\brief
The AL_USDMaya event system attempts to provide a more robust event system for Maya that works around some of the
short comings of the MMessage/scriptJob approach. This system is employed within AL_USDMaya to expose programming hooks
that can be used to execute your own code during the internal processes of AL_USDMaya <i>(e.g. before/after a variant switch)</i>

\subsection MMessage Why not use scriptJob / MMessage?

Maya already has its own event management system, which are exposed via the MMessage <i>(and derived classes)</i> in the API,
and scriptJob within MEL/python. These systems work, but they have a number of drawbacks when deployed in a medium to large
sized studio with multiple shows in flight. As an example of some of the problems that can arise, consider this scenario:

\code
// A callback that will create a node we will store some shot settings on
global proc onFileNew_createMagicNode()
{
  $node = `createNode "transform"`;
  rename $node "essential_shot_settings";
}
global int $scriptJob1 = `scriptJob -e "NewSceneOpened" "onFileNew_createMagicNode"`;

// A callback that will group all nodes into a set we can ignore at export time
global proc onFileNew_createDefaultSetOfNodes()
{
  // add all transforms into a set to ignore at export
  sets -n "doNotExport" `ls -type transform`;
}
global int $scriptJob2 = `scriptJob -e "NewSceneOpened" "onFileNew_createDefaultSetOfNodes"`;
\endcode

Now in this case, since we are registering those script jobs in a specific order, when a 'file new' occurs, our custom
shot node will be created, and it will be added to the set of nodes to ignore at export time. All well and good!

If however we registered scriptJob2 first, we'd end up with the set being created first, and then we'd create our shot
settings node <i>(which would not be part of the set)</i>. Now who knows which is the right way around in this context
<i>(it is after all an illustrative example!)</i>, but the important take home message is that there can be behavioural
changes when scriptJobs and MMessages are registered in different orders.

This is often a problem in most studios, since it's likely that those two scriptJobs <i>(or MMessage events)</i> are
actually located in different plug-ins, and therefore small bugs can be introduced if the events are accidentally
registered in an incorrect order.

In cases where small bugs are introduced, it is often extremely hard to track down what has caused the offending
bug, since the maya event system doesn't really give you an adequate way to track down which events triggered which
callbacks, and more importantly any ideas in how to track down the code that contained the events.

\subsection terminology Some Terminology

\li \b Event : An event is a point in code that can trigger multiple callbacks
\li \b Callback : This is a small bit of code that the user can execute when a specific event is triggered
\li \b Node \b Event : an event that is bound to a specific maya node
\li \b Global \b Event : an event that is not bound to any particular node


\subsection globalEventsC Global Events in C++

The following code sample provides a simple example of how the C++ api works in practice.

\code

#include "AL/usdmaya/EventHandler.h"

// a global identifier for the event we have created
AL::usdmaya::EventId g_mySimpleEvent = 0;

class SimpleEventExample
   : public MPxCommand
{
public:
  static void* creator() { return new SimpleEventExample; }

  MStatus doIt(const MArgList& args)
  {
    // ask the scheduler to trigger any callbacks registered against our event
    AL::usdmaya::EventScheduler::getScheduler().triggerEvent(g_mySimpleEvent);

    return MS::kSuccess;
  }
};

AL::usdmaya::CallbackId g_myCallbackId = 0;

void myCallbackFunction(void* userData)
{
  MGlobal::displayInfo("I am a callback!\n");
}


MStatus initializePlugin(MObject obj)
{
  // to access the global scheduler
  auto& scheduler = AL::usdmaya::EventScheduler::getScheduler();

  // lets register a simple event
  g_mySimpleEvent = scheduler.registerEvent("OnSomethingHappened");

  // make sure the event registered correctly
  if(g_mySimpleEvent == 0)
  {
    std::cout << "event failed to register (name is in use!)" << std::endl;
  }

  // Simply as an example, we may wish to register a C callback on the event!
  g_myCallbackId = scheduler.registerCallback(
    g_mySimpleEvent,                  ///< the event Id we wish to have our callback triggered on
    "myToolName_myCallbackFunction",  ///< a unique tag to identify the who owns the callback, and its purpose
    myCallbackFunction,               ///< the C function we wish to execute
    10000,                            ///< a weight value for the callback. Smaller values are executed first, larger last
    nullptr);                         ///< an optional userData pointer

  // make sure the callback registered correctly
  if(g_myCallbackId == 0)
  {
    std::cout << "callback failed to register (tag is in use, or event id is invalid)" << std::endl;
  }


  MFnPlugin fn(obj);
  fn.registerCommand("simpleEventExample", SimpleEventExample::creator);
  return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
  // unregister the callback
  AL::usdmaya::EventScheduler::getScheduler().unregisterCallback(g_mySimpleEvent);

  // unregister the event
  AL::usdmaya::EventScheduler::getScheduler().unregisterEvent(g_mySimpleEvent);

  MFnPlugin fn(obj);
  fn.unregisterCommand("simpleEventExample");

  return MS::kSuccess;
}
\endcode

It should be noted that once this plugin have been loaded, there are a number of MEL commands exposed that allow you
to interact with that event in MEL/python script. Frstly we can get a list of the global events registered:

\code
print `AL_usdmaya_ListEvents`;

// will print:
//
//  OnSomethingHappened
\endcode

We can also trigger the event from MEL/python if we wish:

\code
AL_usdmaya_TriggerEvent "OnSomethingHappened";

// will execute 'myCallbackFunction' and print the following to the script editor output:
//
//  I am a callback!
\endcode

Via the MEL command AL_usdmaya_Callback it is possible to assign a callback from a MEL or python script.

\code
// simple callback
string $melCodeToExecute = "print \"mel callback!\n\"";

// -me/-melEvent flag arguments: (note: -pe/-pythonEvent will treat the callback code as python)
//
// * The event name
// * A unique tag to identify the callback
// * The integer weight for the callback
// * The MEL script to execute
//
global int $callbackId[] = `AL_usdmaya_Callback -me "OnSomethingHappened" "MyMelScript_operation" 10001 $melCodeToExecute`;
\endcode

You will notice that the callback id's are returned as an array. This is simply because the C++ Callback ID's are 64bit,
however sadly MEL does not support 64bit integer values, so the callbacks are returned as a pair of 32bit integers.
These pair of callback values can be used to query some information about the callback using the command AL_usdmaya_CallbackQuery

\code
// print the tag for the callback
print ("tag: " + `AL_usdmaya_CallbackQuery -tag $callbackId[0] $callbackId[1]` + "\n");

// print the eventId for the callback
print ("eventId: " + `AL_usdmaya_CallbackQuery -e $callbackId[0] $callbackId[1]` + "\n");

// print the type of the callback (returns "Python", "MEL", or "C")
print ("type: " + `AL_usdmaya_CallbackQuery -ty $callbackId[0] $callbackId[1]` + "\n");

// print the callback weight
print ("weight: " + `AL_usdmaya_CallbackQuery -w $callbackId[0] $callbackId[1]` + "\n");

// print the callback code
print ("code: " + `AL_usdmaya_CallbackQuery -c $callbackId[0] $callbackId[1]` + "\n");
\endcode

If you wish to see which callbacks are registered against a specific event, you can use the AL_usdmaya_ListCallbacks command, e.g.

\code
proc printCallbackInfo(string $eventName)
{
  int $callbackIds[] = `AL_usdmaya_ListCallbacks $eventName`;
  print ("EventBreakdown: " + $eventName + "\n");
  for(int $i = 0; $i < size($callbackIds); $i += 2)
  {
    print ("callback " + ($i / 2 + 1) + " : [" + $callbackIds[$i] + ", " + $callbackIds[$i + 1] + "]\n");

    // print the tag for the callback
    print ("  tag: " + `AL_usdmaya_CallbackQuery -tag $callbackIds[$i] $callbackIds[$i + 1]` + "\n");

    // print the eventId for the callback
    print ("  eventId: " + `AL_usdmaya_CallbackQuery -e $callbackIds[$i] $callbackIds[$i + 1]` + "\n");

    // print the type of the callback (returns "Python", "MEL", or "C")
    print ("  type: " + `AL_usdmaya_CallbackQuery -ty $callbackIds[$i] $callbackIds[$i + 1]` + "\n");

    // print the callback weight
    print ("  weight: " + `AL_usdmaya_CallbackQuery -w $callbackIds[$i] $callbackIds[$i + 1]` + "\n");

    // print the callback code
    print ("  code: \n----------------------------------------------------------------\n" +
              `AL_usdmaya_CallbackQuery -c $callbackIds[$i] $callbackIds[$i + 1]` +
              "\n----------------------------------------------------------------\n");
  }
}

// find out info about the OnSomethingHappened event
printCallbackInfo("OnSomethingHappened");

\endcode

If you wish to delete a callback, then you can do it in one of two ways:

\code
// either use the -d/-delete flag to delete the callback,
AL_usdmaya_Callback -d $callbackId[0] $callbackId[1];

// or pass an array of callback id pairs to the AL_usdmaya_DeleteCallbacks command
AL_usdmaya_DeleteCallbacks $callbackId[0];
\endcode

It is also possible to define entirely new events in your own MEL or python scripts, e.g.

\code

// create a new event
AL_usdmaya_Event "AnEventDefinedInMEL";

// you can now trigger the event (and attach callbacks)
AL_usdmaya_TriggerEvent "AnEventDefinedInMEL";

// and to delete the event
AL_usdmaya_Event -d "AnEventDefinedInMEL";

\endcode


\subsection nodeEventsC Node Events in C++

To make use of the maya node events, you're node should derive from the AL::usdmaya::nodes::MayaNodeEvents class.
A simple example of setting a node up with the events system would look like so:

\code

#include "AL/usdmaya/EventHandler.h"

// To make use of the node events, ensure you derive a node from the MayaNodeEvents class.
class MyMayaNode
  : public MPxNode,
    public AL::usdmaya::nodes::MayaNodeEvents
{
public:

  MyMayaNode()
  {
    // simply call registerEvent() for each event you wish to register
    registerEvent("PreThingHappened");
    registerEvent("PostThingHappened");
  }

  ~MyMayaNode()
  {
    // you don't need to unregister events (events are automatically unregistered in the MayaNodeEvents dtor).
    // This is only here for example purposes
    unregisterEvent("PreThingHappened");
    unregisterEvent("PostThingHappened");
  }

  void thingHappened()
  {
    // trigger the first event
    triggerEvent("PreThingHappened");

    // do some magical operation
    doTheThing();

    // trigger second event
    triggerEvent("PostThingHappened");
  }

  //
  // Typical Maya node implementation assumed to be here
  //
};
\endcode

That's basically the only setup you need to perform in order to make a custom plugin node compatible with
the events system. We can now use AL_usdmaya_ListEvents to get a list of the events that the node supports

\code
// create a node that supports events
$node = `createNode "MyMayaNode"`;

// list the events available on the node
print `AL_usdmaya_ListEvents $node`;

// will print:
//
//  PreThingHappened
//  PostThingHappened
\endcode

Via the MEL command AL_usdmaya_Callback it is possible to assign a callback from a MEL or python script to that node.

\code
// simple callback
string $melCodeToExecute = "print \"mel callback!\n\"";

// -mne/-melNodeEvent flag arguments: (note: -pne/-pythonNodeEvent will treat the callback code as python)
// * The node name
// * The event name
// * A unique tag to identify the callback
// * The integer weight for the callback
// * The MEL script to execute
//
global int $callbackId[] = `AL_usdmaya_Callback -mne $node "PreThingHappened" "MyMelScript_operation" 10001 $melCodeToExecute`;
\endcode

We can also trigger the event from MEL/python if we wish:

\code
AL_usdmaya_TriggerEvent "PreThingHappened" $node;

// will execute 'myCallbackFunction' and print the following to the script editor output:
//
//  mel callback!
\endcode


You will notice that the callback id's are returned as an array. This is simply because the C++ Callback ID's are 64bit,
however sadly MEL does not support 64bit integer values, so the callbacks are returned as a pair of 32bit integers.
These pair of callback values can be used to query some information about the callback using the command AL_usdmaya_CallbackQuery

\code
// print the tag for the callback
print ("tag: " + `AL_usdmaya_CallbackQuery -tag $callbackId[0] $callbackId[1]` + "\n");

// print the eventId for the callback
print ("eventId: " + `AL_usdmaya_CallbackQuery -e $callbackId[0] $callbackId[1]` + "\n");

// print the type of the callback (returns "Python", "MEL", or "C")
print ("type: " + `AL_usdmaya_CallbackQuery -ty $callbackId[0] $callbackId[1]` + "\n");

// print the callback weight
print ("weight: " + `AL_usdmaya_CallbackQuery -w $callbackId[0] $callbackId[1]` + "\n");

// print the callback code
print ("code: " + `AL_usdmaya_CallbackQuery -c $callbackId[0] $callbackId[1]` + "\n");
\endcode

If you wish to see which callbacks are registered against a specific event, you can use the AL_usdmaya_ListCallbacks command, e.g.

\code
proc printNodeCallbackInfo(string $eventName, string $node)
{
  int $callbackIds[] = `AL_usdmaya_ListCallbacks $eventName $node`;
  print ("EventBreakdown for node: " + $node + " and event: " + $eventName + "\n");
  for(int $i = 0; $i < size($callbackIds); $i += 2)
  {
    print ("callback " + ($i / 2 + 1) + " : [" + $callbackIds[$i] + ", " + $callbackIds[$i + 1] + "]\n");

    // print the tag for the callback
    print ("  tag: " + `AL_usdmaya_CallbackQuery -tag $callbackIds[$i] $callbackIds[$i + 1]` + "\n");

    // print the eventId for the callback
    print ("  eventId: " + `AL_usdmaya_CallbackQuery -e $callbackIds[$i] $callbackIds[$i + 1]` + "\n");

    // print the type of the callback (returns "Python", "MEL", or "C")
    print ("  type: " + `AL_usdmaya_CallbackQuery -ty $callbackIds[$i] $callbackIds[$i + 1]` + "\n");

    // print the callback weight
    print ("  weight: " + `AL_usdmaya_CallbackQuery -w $callbackIds[$i] $callbackIds[$i + 1]` + "\n");

    // print the callback code
    print ("  code: \n----------------------------------------------------------------\n" +
              `AL_usdmaya_CallbackQuery -c $callbackIds[$i] $callbackIds[$i + 1]` +
              "\n----------------------------------------------------------------\n");
  }
}

// find out info about the OnSomethingHappened event
printNodeCallbackInfo("PreThingHappened", $node);

\endcode

If you wish to delete a callback, then you can do it in one of two ways:

\code
// either use the -d/-delete flag to delete the callback,
AL_usdmaya_Callback -d $callbackId[0] $callbackId[1];

// or pass an array of callback id pairs to the AL_usdmaya_DeleteCallbacks command
AL_usdmaya_DeleteCallbacks $callbackId[0];
\endcode

It is also possible to define entirely new events on the node in your own MEL or python scripts, e.g.

\code

// create a new event
AL_usdmaya_Event "AnEventDefinedInMEL" $node;

// you can now trigger the event (and attach callbacks)
AL_usdmaya_TriggerEvent "AnEventDefinedInMEL" $node;

// and to delete the event
AL_usdmaya_Event -d "AnEventDefinedInMEL" $node;

\endcode

*/
