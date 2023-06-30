#ifndef MH_CPPTESTS_CMD
#define MH_CPPTESTS_CMD

#include <maya/MPxCommand.h>

class mayaHydraCppTestCmd : public MPxCommand
{
public:
    static void*   creator() { return new mayaHydraCppTestCmd(); }
    static MSyntax createSyntax();

    static const MString name;

    MStatus doIt(const MArgList& args) override;
};

#endif // MH_CPPTESTS_CMD