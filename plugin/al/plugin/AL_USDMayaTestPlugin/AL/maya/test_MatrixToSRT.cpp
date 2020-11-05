#include "AL/usdmaya/utils/Utils.h"
#include "test_usdmaya.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MEulerRotation.h>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test matrixToSRT
//----------------------------------------------------------------------------------------------------------------------
TEST(usdmaya_Utils, matrixToSRT)
{
    // Test one-axis negative scale
    {
        constexpr double epsilon = 1e-5;

        GfMatrix4d     inputMatrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1);
        double         S[3];
        MEulerRotation R;
        double         T[3];
        AL::usdmaya::utils::matrixToSRT(inputMatrix, S, R, T);

        EXPECT_EQ(0.0, T[0]);
        EXPECT_EQ(0.0, T[1]);
        EXPECT_EQ(0.0, T[2]);

        GfMatrix4d rotXMat;
        rotXMat.SetRotate(GfRotation(GfVec3d(1, 0, 0), R.x));
        GfMatrix4d rotYMat;
        rotYMat.SetRotate(GfRotation(GfVec3d(0, 1, 0), R.y));
        GfMatrix4d rotZMat;
        rotZMat.SetRotate(GfRotation(GfVec3d(0, 0, 1), R.z));
        GfMatrix4d scaleMat;
        scaleMat.SetScale(GfVec3d(S));

        GfMatrix4d resultMatrix = rotXMat * rotYMat * rotZMat * scaleMat;
        EXPECT_NEAR(inputMatrix[0][0], resultMatrix[0][0], epsilon);
        EXPECT_NEAR(inputMatrix[0][1], resultMatrix[0][1], epsilon);
        EXPECT_NEAR(inputMatrix[0][2], resultMatrix[0][2], epsilon);
        EXPECT_NEAR(inputMatrix[0][3], resultMatrix[0][3], epsilon);
        EXPECT_NEAR(inputMatrix[1][0], resultMatrix[1][0], epsilon);
        EXPECT_NEAR(inputMatrix[1][1], resultMatrix[1][1], epsilon);
        EXPECT_NEAR(inputMatrix[1][2], resultMatrix[1][2], epsilon);
        EXPECT_NEAR(inputMatrix[1][3], resultMatrix[1][3], epsilon);
        EXPECT_NEAR(inputMatrix[2][0], resultMatrix[2][0], epsilon);
        EXPECT_NEAR(inputMatrix[2][1], resultMatrix[2][1], epsilon);
        EXPECT_NEAR(inputMatrix[2][2], resultMatrix[2][2], epsilon);
        EXPECT_NEAR(inputMatrix[2][3], resultMatrix[2][3], epsilon);
        EXPECT_NEAR(inputMatrix[3][0], resultMatrix[3][0], epsilon);
        EXPECT_NEAR(inputMatrix[3][1], resultMatrix[3][1], epsilon);
        EXPECT_NEAR(inputMatrix[3][2], resultMatrix[3][2], epsilon);
        EXPECT_NEAR(inputMatrix[3][3], resultMatrix[3][3], epsilon);
    }
}
