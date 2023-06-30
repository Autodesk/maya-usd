#include <gtest/gtest.h>

// Test to ensure the C++ Testing Framework works as expected 

TEST (CppFramework, pass) { 
    EXPECT_EQ (1, 1);
}

TEST (CppFramework, fail) { 
    EXPECT_EQ (0, 1);
}