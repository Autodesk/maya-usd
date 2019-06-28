# FrameRange 
AL_USDMaya comes with ALFrameRange USD typed schema and during the translation of its prim, Maya's animation start & end frames, min & max frames and current frame could be setup.

In a ALFrameRange prim, available range attributes are "animationStartFrame", "startFrame", "endFrame" and "animationEndFrame", 
corresponding to command flags `animationStartTime`, `minTime`, `maxTime`, `animationEndTime` of `playbackOptions` command in Maya. 

The values of these 4 range attributes will override the "startTimeCode" and "endTimeCode" metaData of root USD layer, 
unless none of them are authored in the prim and the relevant metaData of root layer are available.
If just one of the start range is authored, then the end range will be the same as start range and vice versa.

If no range values are available from prim or USD layer, no action will be taken.

Beside that, The "currentFrame" attribute of ALFrameRange prim maps to the current frame return by Maya command `cmds.currentTime(q=True)`, 
the translator will set current frame in Maya if it is authored.

## Demo
The following Demos will show you how ALFrameRange works.
 
Run the following commands in Maya's script editor with the AL_USDMaya plugin loaded.


***

Demo1: ALFrameRange prim with range and currentFrame values:
```
cmds.AL_usdmaya_ProxyShapeImport(file="<PATH_TO_ASSETS_FOLDER_IN_THIS_DIRECTORY>/frame_range_prim.usda")
```

You should notice that the animation and visible range in Maya RangeSlider changed, as well as the current frame in TimeSlider.
The usd data for the reference:
```
#usda 1.0
(
    defaultPrim = "shot_name"    
)

def Xform "shot_name" ()
{
    def ALFrameRange "frame_range" ()
    {
        double animationStartFrame = 1072
        double startFrame = 1080
        double endFrame = 1200
        double animationEndFrame = 1290
        
        double currentFrame = 1100
    }
}
```

***

Demo2: ALFrameRange prim without frame values, which will fall-back to ones in root layer metaData:
```
cmds.AL_usdmaya_ProxyShapeImport(file="<PATH_TO_ASSETS_FOLDER_IN_THIS_DIRECTORY>/frame_range_stage.usda")
```

You should notice that the animation and visible range in Maya RangeSlider changed to the range from root layer metaData.
The usd data for the reference:
```
#usda 1.0
(
    startTimeCode = 1072
    endTimeCode = 1290
    defaultPrim = "shot_name"    
)

def Xform "shot_name" ()
{
    def ALFrameRange "frame_range" ()
    {
    }
}
```