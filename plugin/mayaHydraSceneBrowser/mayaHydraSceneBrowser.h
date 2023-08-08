#ifndef MH_SCENEBROWSER_CMD
#define MH_SCENEBROWSER_CMD

#include <maya/MPxCommand.h>

class mayaHydraSceneBrowserCmd : public MPxCommand
{
public:
    static void*   creator() { return new mayaHydraSceneBrowserCmd(); }

    static const MString name;

    MStatus doIt(const MArgList& args) override;
};

#endif // MH_SCENEBROWSER_CMD