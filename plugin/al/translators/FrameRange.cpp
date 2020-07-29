#include "AL/usd/schemas/maya/FrameRange.h"

#include "AL/usdmaya/DebugCodes.h"
#include "FrameRange.h"

#include <maya/MAnimControl.h>
#include <maya/MStatus.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

AL_USDMAYA_DEFINE_TRANSLATOR(FrameRange, AL_usd_FrameRange)

//----------------------------------------------------------------------------------------------------------------------
MStatus FrameRange::setFrameRange(const UsdPrim& prim, bool setCurrentFrame)
{
    if (!prim.IsValid()) {
        return MS::kFailure;
    }

    UsdStageWeakPtr stage = prim.GetStage();
    if (!stage) {
        return MStatus::kFailure;
    }

    // Retrieve the start end frames from schemas:
    AL_usd_FrameRange frameRange(prim);
    UsdAttribute      animStartAttr = frameRange.GetAnimationStartFrameAttr();
    UsdAttribute      animEndAttr = frameRange.GetAnimationEndFrameAttr();

    UsdAttribute visibleStartAttr = frameRange.GetStartFrameAttr();
    UsdAttribute visibleEndAttr = frameRange.GetEndFrameAttr();

    UsdAttribute currentFrameAttr = frameRange.GetCurrentFrameAttr();

    // Retrieve the fallback start end frames from stage:
    double animationStartFrame = 0.0;
    double animationEndFrame = 0.0;

    double visibleStartFrame = 0.0;
    double visibleEndFrame = 0.0;

    double currentFrame = 0.0;

    TfToken startTimeCodeKey("startTimeCode");
    TfToken endTimeCodeKey("endTimeCode");

    MStatus animStartStat
        = getFrame(stage, prim, animStartAttr, startTimeCodeKey, &animationStartFrame);
    MStatus animEndStat = getFrame(stage, prim, animEndAttr, endTimeCodeKey, &animationEndFrame);

    MStatus visibleStartStat
        = getFrame(stage, prim, visibleStartAttr, startTimeCodeKey, &visibleStartFrame);
    MStatus visibleEndStat
        = getFrame(stage, prim, visibleEndAttr, endTimeCodeKey, &visibleEndFrame);

    MStatus currentFrameStat
        = getFrame(stage, prim, currentFrameAttr, startTimeCodeKey, &currentFrame);

    bool hasFrameRange = bool(animStartStat);
    if (!animStartStat) {
        if (visibleStartStat) {
            animationStartFrame = visibleStartFrame;
            hasFrameRange = true;
        } else if (visibleEndStat) {
            animationStartFrame = visibleEndFrame;
            hasFrameRange = true;
        } else if (animEndStat) {
            animationStartFrame = animationEndFrame;
            hasFrameRange = true;
        }
    }

    if (!animEndStat) {
        if (visibleEndStat) {
            animationEndFrame = visibleEndFrame;
        } else if (visibleStartStat) {
            animationEndFrame = visibleStartFrame;
        } else if (animStartStat) {
            animationEndFrame = animationStartFrame;
        }
    }

    if (!visibleStartStat) {
        if (animStartStat) {
            visibleStartFrame = animationStartFrame;
        } else if (visibleEndStat) {
            visibleStartFrame = visibleEndFrame;
        } else if (animEndStat) {
            visibleStartFrame = animationEndFrame;
        }
    }

    if (!visibleEndStat) {
        if (animEndStat) {
            visibleEndFrame = animationEndFrame;
        } else if (visibleStartStat) {
            visibleEndFrame = visibleStartFrame;
        } else if (animStartStat) {
            visibleEndFrame = animationStartFrame;
        }
    }

    MTime::Unit unit = MTime::uiUnit();
    MStatus     stat = MS::kSuccess;
    if (hasFrameRange) {
        // Set the start end frames:
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg(
                "FrameRange::setFrameRange(%f, %f, %f, %f) on prim %s\n",
                animationStartFrame,
                visibleStartFrame,
                visibleEndFrame,
                animationEndFrame,
                prim.GetPath().GetText());

        MTime startAnimTime(animationStartFrame, unit);
        MTime endAnimTime(animationEndFrame, unit);
        MTime visibleStartTime(visibleStartFrame, unit);
        MTime visibleEndTime(visibleEndFrame, unit);
        stat = MAnimControl::setAnimationStartEndTime(startAnimTime, endAnimTime);
        stat = MAnimControl::setMinMaxTime(visibleStartTime, visibleEndTime);
    }

    if (setCurrentFrame) {
        bool hasCurrentFrame = bool(currentFrameStat);
        if (!currentFrameStat) {
            if (hasFrameRange) {
                currentFrame = visibleStartFrame;
                hasCurrentFrame = true;
            }
        }

        if (hasCurrentFrame) {
            // Set the current frames:
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "FrameRange::setCurrentFrame(%f) on prim %s\n",
                    currentFrame,
                    prim.GetPath().GetText());

            MTime currentTime(currentFrame, unit);
            stat = MAnimControl::setCurrentTime(currentTime);
        }
    }

    return stat;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus FrameRange::getFrame(
    const UsdStageWeakPtr& stage,
    const UsdPrim&         prim,
    const UsdAttribute&    attr,
    const TfToken&         fallbackMetadataKey,
    double*                frame) const
{
    *frame = 0.0;

    if (attr.HasAuthoredValueOpinion()) {
        attr.Get<double>(frame);
        return MS::kSuccess;
    }

    if (stage->HasAuthoredMetadata(fallbackMetadataKey)) {
        stage->GetMetadata<double>(fallbackMetadataKey, frame);
        return MS::kSuccess;
    }

    return MS::kFailure;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus FrameRange::import(const UsdPrim& prim, MObject& parent, MObject& createdObj)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("FrameRange::import %s\n", prim.GetPath().GetText());
    return setFrameRange(prim, true);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus FrameRange::postImport(const UsdPrim& prim) { return MS::kSuccess; }

//----------------------------------------------------------------------------------------------------------------------
MStatus FrameRange::preTearDown(UsdPrim& prim) { return MS::kSuccess; }

//----------------------------------------------------------------------------------------------------------------------
MStatus FrameRange::tearDown(const SdfPath& primPath) { return MStatus::kSuccess; }

//----------------------------------------------------------------------------------------------------------------------
MStatus FrameRange::update(const UsdPrim& prim)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("FrameRange::update %s\n", prim.GetPath().GetText());
    return setFrameRange(prim, false);
}

} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
