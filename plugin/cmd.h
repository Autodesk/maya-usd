#ifndef __HDMAYA_CMD_H__
#define __HDMAYA_CMD_H__

#include <pxr/pxr.h>

#include <maya/MPxCommand.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaCmd : public MPxCommand {
public:
    static void* creator() { return new HdMayaCmd(); }
    static MSyntax createSyntax();

    MStatus doIt(const MArgList& args) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_CMD_H__
