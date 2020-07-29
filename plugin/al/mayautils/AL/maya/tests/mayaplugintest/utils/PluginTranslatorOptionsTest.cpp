#include "AL/maya/utils/PluginTranslatorOptions.h"

#include <maya/MGlobal.h>
#include <maya/MStringArray.h>

#include <gtest/gtest.h>

using namespace AL::maya::utils;

//----------------------------------------------------------------------------------------------------------------------
//  void PluginTranslatorOptionsContext::registerPluginTranslatorOptions(PluginTranslatorOptions*
//  options); void PluginTranslatorOptionsContext::unregisterPluginTranslatorOptions(const char*
//  const pluginTranslatorGrouping); bool PluginTranslatorOptionsContext::isRegistered(const char*
//  const pluginTranslatorGrouping) const; const MString& PluginTranslatorOptions::grouping() const;
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_PluginTranslatorOptionsContext, registerPluginTranslatorOptions)
{
    PluginTranslatorOptionsContext ctx;
    EXPECT_FALSE(ctx.isRegistered("testOptions"));
    {
        PluginTranslatorOptions options(ctx, "testOptions");
        EXPECT_TRUE(ctx.isRegistered("testOptions"));
        EXPECT_EQ(MString("testOptions"), options.grouping());
    }
    EXPECT_FALSE(ctx.isRegistered("testOptions"));
}

//----------------------------------------------------------------------------------------------------------------------
//  bool PluginTranslatorOptions::addBool(const char* optionName, bool defaultValue = false, Mode
//  mode = kBoth);
TEST(maya_PluginTranslatorOptionsContext, addBool)
{
    PluginTranslatorOptionsContext ctx;
    PluginTranslatorOptions        options(ctx, "testOptions");

    // check we can register the option
    EXPECT_TRUE(options.addBool("option1", true));

    // make sure it fails if we try to register the same option again
    EXPECT_FALSE(options.addBool("option1", true));
    EXPECT_TRUE(options.isOption("option1"));
    EXPECT_EQ(true, options.defaultBool("option1"));
    EXPECT_EQ(OptionType::kBool, options.optionType("option1"));

    // register a second option to validate alternate values
    EXPECT_TRUE(options.addBool("option2", false));
    EXPECT_TRUE(options.isOption("option1"));
    EXPECT_TRUE(options.isOption("option2"));
    EXPECT_FALSE(options.defaultBool("option2"));

    PluginTranslatorOptionsInstance instance(ctx);
    EXPECT_TRUE(instance.getBool("option1"));
    EXPECT_FALSE(instance.getBool("option2"));
    instance.setBool("option1", true);
    EXPECT_TRUE(instance.getBool("option1"));
    instance.setBool("option1", false);
    EXPECT_FALSE(instance.getBool("option1"));
}

//----------------------------------------------------------------------------------------------------------------------
//  bool PluginTranslatorOptions::addInt(const char* optionName, int defaultValue = 0, Mode mode =
//  kBoth);
TEST(maya_PluginTranslatorOptionsContext, addInt)
{
    PluginTranslatorOptionsContext ctx;
    PluginTranslatorOptions        options(ctx, "testOptions");

    // check we can register the option
    EXPECT_TRUE(options.addInt("option1", 42));

    // make sure it fails if we try to register the same option again
    EXPECT_FALSE(options.addInt("option1", 42));
    EXPECT_TRUE(options.isOption("option1"));
    EXPECT_EQ(OptionType::kInt, options.optionType("option1"));
    EXPECT_EQ(42, options.defaultInt("option1"));

    // register a second option to validate alternate values
    EXPECT_TRUE(options.addInt("option2", 44));
    EXPECT_TRUE(options.isOption("option1"));
    EXPECT_TRUE(options.isOption("option2"));
    EXPECT_EQ(44, options.defaultInt("option2"));

    PluginTranslatorOptionsInstance instance(ctx);
    EXPECT_EQ(42, instance.getInt("option1"));
    EXPECT_EQ(44, instance.getInt("option2"));
    instance.setInt("option1", 52);
    EXPECT_EQ(52, instance.getInt("option1"));
    instance.setInt("option1", 54);
    EXPECT_EQ(54, instance.getInt("option1"));
}

//----------------------------------------------------------------------------------------------------------------------
//  bool PluginTranslatorOptions::addFloat(const char* optionName, float defaultValue = 0.0f, Mode
//  mode = kBoth);
TEST(maya_PluginTranslatorOptionsContext, addFloat)
{
    PluginTranslatorOptionsContext ctx;
    PluginTranslatorOptions        options(ctx, "testOptions");

    // check we can register the option
    EXPECT_TRUE(options.addFloat("option1", 13.24f));

    // make sure it fails if we try to register the same option again
    EXPECT_FALSE(options.addFloat("option1", 13.25f));
    EXPECT_TRUE(options.isOption("option1"));
    EXPECT_EQ(OptionType::kFloat, options.optionType("option1"));
    EXPECT_EQ(13.24f, options.defaultFloat("option1"));

    // register a second option to validate alternate values
    EXPECT_TRUE(options.addFloat("option2", 23.24f));
    EXPECT_TRUE(options.isOption("option1"));
    EXPECT_TRUE(options.isOption("option2"));
    EXPECT_EQ(23.24f, options.defaultFloat("option2"));

    PluginTranslatorOptionsInstance instance(ctx);
    EXPECT_EQ(13.24f, instance.getFloat("option1"));
    EXPECT_EQ(23.24f, instance.getFloat("option2"));
    instance.setFloat("option1", 12.24f);
    EXPECT_EQ(12.24f, instance.getFloat("option1"));
    instance.setFloat("option1", 11.24f);
    EXPECT_EQ(11.24f, instance.getFloat("option1"));
}

//----------------------------------------------------------------------------------------------------------------------
//  bool PluginTranslatorOptions::addString(const char* optionName, const char* const defaultValue =
//  "", Mode mode = kBoth);
TEST(maya_PluginTranslatorOptionsContext, addString)
{
    PluginTranslatorOptionsContext ctx;
    PluginTranslatorOptions        options(ctx, "testOptions");

    // check we can register the option
    EXPECT_TRUE(options.addString("option1", "hello"));

    // make sure it fails if we try to register the same option again
    EXPECT_FALSE(options.addString("option1", "hel2lo"));
    EXPECT_TRUE(options.isOption("option1"));
    EXPECT_EQ(OptionType::kString, options.optionType("option1"));
    EXPECT_EQ(MString("hello"), options.defaultString("option1"));

    // register a second option to validate alternate values
    EXPECT_TRUE(options.addString("option2", "byebye"));
    EXPECT_TRUE(options.isOption("option1"));
    EXPECT_TRUE(options.isOption("option2"));
    EXPECT_EQ(MString("byebye"), options.defaultString("option2"));

    PluginTranslatorOptionsInstance instance(ctx);
    EXPECT_EQ(MString("hello"), instance.getString("option1"));
    EXPECT_EQ(MString("byebye"), instance.getString("option2"));
    instance.setString("option1", "hello");
    EXPECT_EQ(MString("hello"), instance.getString("option1"));
    instance.setString("option1", "hello2");
    EXPECT_EQ(MString("hello2"), instance.getString("option1"));
}

//----------------------------------------------------------------------------------------------------------------------
//  bool PluginTranslatorOptions::addEnum(const char* optionName, const char* const enumValues[],
//  int defaultValue = 0, Mode mode = kBoth);
TEST(maya_PluginTranslatorOptionsContext, addEnum)
{
    PluginTranslatorOptionsContext ctx;
    PluginTranslatorOptions        options(ctx, "testOptions");

    const char* const enumVals[]
        = { "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday", 0 };

    // check we can register the option
    EXPECT_TRUE(options.addEnum("option1", enumVals, 3));

    // make sure it fails if we try to register the same option again
    EXPECT_FALSE(options.addEnum("option1", enumVals, 2));
    EXPECT_TRUE(options.isOption("option1"));
    EXPECT_EQ(OptionType::kEnum, options.optionType("option1"));
    EXPECT_EQ(3, options.defaultInt("option1"));

    // register a second option to validate alternate values
    EXPECT_TRUE(options.addEnum("option2", enumVals, 4));
    EXPECT_TRUE(options.isOption("option1"));
    EXPECT_TRUE(options.isOption("option2"));
    EXPECT_EQ(4, options.defaultInt("option2"));

    PluginTranslatorOptionsInstance instance(ctx);
    EXPECT_EQ(3, instance.getEnum("option1"));
    EXPECT_EQ(4, instance.getEnum("option2"));
    instance.setEnum("option1", 2);
    EXPECT_EQ(2, instance.getEnum("option1"));
    instance.setEnum("option1", 3);
    EXPECT_EQ(3, instance.getEnum("option1"));
}

//----------------------------------------------------------------------------------------------------------------------
//  void PluginTranslatorOptionsInstance::toOptionVars(const char* const prefix);
//  void PluginTranslatorOptionsInstance::fromOptionVars(const char* const prefix);
TEST(maya_PluginTranslatorOptionsInstance, toOptionVars)
{
    const char* const enumVals[]
        = { "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday", 0 };

    PluginTranslatorOptionsContext ctx;
    PluginTranslatorOptions        options(ctx, "testOptions");
    EXPECT_TRUE(options.addBool("bval", true));
    EXPECT_TRUE(options.addInt("ival", 22));
    EXPECT_TRUE(options.addFloat("fval", 23.4f));
    EXPECT_TRUE(options.addString("sval", "HALLO"));
    EXPECT_TRUE(options.addEnum("eval", enumVals, 3));

    PluginTranslatorOptionsInstance instance(ctx);
    instance.setBool("bval", false);
    instance.setInt("ival", 23);
    instance.setFloat("fval", 34.2f);
    instance.setString("sval", "bye");
    instance.setEnum("eval", 4);

    instance.toOptionVars("dave");
    MString optVarValue = MGlobal::optionVarStringValue(MString("dave") + "testOptions");
    EXPECT_TRUE(optVarValue.length() != 0);

    // now check to see if options were read back in
    PluginTranslatorOptionsInstance instance2(ctx);
    instance2.fromOptionVars("dave");
    EXPECT_FALSE(instance2.getBool("bval"));
    EXPECT_EQ(23, instance2.getInt("ival"));
    EXPECT_EQ(34.2f, instance2.getFloat("fval"));
    EXPECT_EQ(MString("bye"), instance2.getString("sval"));
    EXPECT_EQ(4, instance2.getEnum("eval"));
}

//----------------------------------------------------------------------------------------------------------------------
//  void PluginTranslatorOptionsInstance::toOptionVars(const char* const prefix);
//  void PluginTranslatorOptionsInstance::fromOptionVars(const char* const prefix);
TEST(maya_PluginTranslatorOptionsInstance, generateGUI)
{
    const char* const enumVals[]
        = { "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday", 0 };

    PluginTranslatorOptionsContext ctx;
    PluginTranslatorOptions        options(ctx, "testOptions");
    EXPECT_TRUE(options.addBool("bval", true));
    EXPECT_TRUE(options.addInt("ival", 22));
    EXPECT_TRUE(options.addFloat("fval", 23.4f));
    EXPECT_TRUE(options.addString("sval", "HALLO"));
    EXPECT_TRUE(options.addEnum("eval", enumVals, 3));

    PluginTranslatorOptions options2(ctx, "moreOptions");
    EXPECT_TRUE(options2.addBool("bval2", true));
    EXPECT_TRUE(options2.addInt("ival2", 22));
    EXPECT_TRUE(options2.addFloat("fval2", 23.4f));
    EXPECT_TRUE(options2.addString("sval2", "HALLO"));
    EXPECT_TRUE(options2.addEnum("eval2", enumVals, 3));

    MString                         code;
    PluginTranslatorOptionsInstance instance(ctx);
    ctx.generateGUI("dave", code);
    std::cout << '\n' << code << '\n';
}
