
/**

\page  mayahelpers  Maya API Helper Classes

Most of the custom nodes and commands that form part of the AL_usdmaya plug-in utilize helper
classes to automate some of the boiler plate aspects of the Maya API. These include:

\li \b MPxCommand : creation of option box GUI's, option vars, and menu items
\li \b MPxNode : automatic generation of attribute editor templates
\li \b MPxFileTranslator : automatic generation of export / import GUIs, and option parsing
\li \b Menus

\section almaya_commandgui AL::maya::utils::CommandGuiHelper

Typically within Maya most MEL commands end up being exposed to the user via a fairly standard
pattern, where by you have a menu item on a menu somewhere, along with an option box that allows you
to configure some preferences that are stored between Maya sessions. The MEL code needed to
implement this is not overly complicated, but it can be rather tedious, and occasionally prone to
errors.

As a way to minimize the possibility of bugs, most MEL commands within the USD maya bridge have an
auto generated GUI to go along with them.

\code
    // initialise a new GUI for the AL_usdmaya_ProxyShapeImport command with:
    //
    //   * An option box title "Proxy Shape Import"
    //   * "Import" as a text label on the OK button
    //   * A menu item called "Import" found under the USD/Proxy Shape/ menu
    //
    AL::maya::utils::CommandGuiHelper commandGui("AL_usdmaya_ProxyShapeImport", "Proxy Shape
Import", "Import", "USD/Proxy Shape/Import", true);

    // and now we need to add GUI items for each of the flags to the actual command
    commandGui.addFilePathOption("file", "File Path", AL::maya::utils::CommandGuiHelper::kLoad, "USD
Ascii (*.usd) (*.usd)", AL::maya::utils::CommandGuiHelper::kStringMustHaveValue);
    commandGui.addStringOption("primPath", "USD Prim Path", "", false,
AL::maya::utils::CommandGuiHelper::kStringOptional); commandGui.addStringOption("excludePrimPath",
"Exclude Prim Path", "", false, AL::maya::utils::CommandGuiHelper::kStringOptional);
    commandGui.addStringOption("name", "Proxy Shape Node Name", "", false,
AL::maya::utils::CommandGuiHelper::kStringOptional); commandGui.addBoolOption("connectToTime",
"Connect to Time", true, true); commandGui.addBoolOption("unloaded", "Opens the layer with payloads
unloaded.", false, true); \endcode

All of that auto generates a series of MEL functions that in total make our GUI. Below is an
annotated version of the generated MEL script.

\code
// This method is called when clicking on an option box menu item, or (as in this case) when you
// simply click on the import menu item.
global proc build_AL_usdmaya_ProxyShapeImport_optionGUI()
{
  // only allow one option box to be visible
  if(`window -q -ex "AL_usdmaya_ProxyShapeImport_optionGUI"`) return;
  $window = `window -title "Proxy Shape Import" -w 550 -h 350
"AL_usdmaya_ProxyShapeImport_optionGUI"`;

  // Add the edit menu, with reset & save settings options
  $menuBarLayout = `menuBarLayout`;
    $menu = `menu -label "Edit"`;
      menuItem -label "Save Settings" -c "save_AL_usdmaya_ProxyShapeImport_optionGUI";
      menuItem -label "Reset Settings" -c "reset_AL_usdmaya_ProxyShapeImport_optionGUI";
  setParent $window;

  $form = `formLayout -numberOfDivisions 100`;
    $columnLayout = `frameLayout -cll 0 -bv 1 -lv 0`;

      // Create a 2D table, where we have the option names on the left,
      // and the controls on the right hand side.
      rowLayout -cw 1 170 -nc 2 -ct2 "left" "right" -adj 2 -rat 1 "top" 0 -rat 2 "top" 0;
        columnLayout -adj 1 -cat "both" 1 -rs 2;
          build_AL_usdmaya_ProxyShapeImport_labels(); //< construct labels
        setParent ..;
        columnLayout -adj 1 -cat "both" 1 -rs 2;
          build_AL_usdmaya_ProxyShapeImport_controls(); //< construct controls
        setParent ..;
      setParent ..;
    setParent ..;

    // Now add the OK, Save, and Close buttons
    // * OK : First save all of the control settings, then execute the command (using saved
settings), and finally destroy the option box window.
    // * Save: Just save the options
    // * Close: destroy the window
    $rowLayout = `paneLayout -cn "vertical3"`;
      $doit = `button -label "Import" -c
("save_AL_usdmaya_ProxyShapeImport_optionGUI;execute_AL_usdmaya_ProxyShapeImport_optionGUI;deleteUI
" + $window)`; $saveit = `button -label "Apply" -c "save_AL_usdmaya_ProxyShapeImport_optionGUI"`;
      $close = `button -label "Close" -c ("deleteUI " + $window)`;o
    setParent ..;
  formLayout -e
  -attachForm $columnLayout "top" 1
  -attachForm $columnLayout "left" 1
  -attachForm $columnLayout "right" 1
  -attachControl $columnLayout "bottom" 5 $rowLayout
  -attachForm $rowLayout "left" 5
  -attachForm $rowLayout "right" 5
  -attachForm $rowLayout "bottom" 5
  -attachNone $rowLayout "top"
  $form;

  // initialize the option var values (if this is the first time the user has ever run the tool)
  init_AL_usdmaya_ProxyShapeImport_optionGUI();

  // load all of the option vars into the controls
  load_AL_usdmaya_ProxyShapeImport_optionGUI();
  showWindow;
};

// This is a sanity check run after creating the user interface.
// If checks to see if the option vars exist, and if not will create some default values
global proc init_AL_usdmaya_ProxyShapeImport_optionGUI()
{
  if(!`optionVar -ex "AL_usdmaya_ProxyShapeImport_connectToTime"`)
    optionVar -iv "AL_usdmaya_ProxyShapeImport_connectToTime" 1;
  if(!`optionVar -ex "AL_usdmaya_ProxyShapeImport_unloaded"`)
    optionVar -iv "AL_usdmaya_ProxyShapeImport_unloaded" 0;
};

// This method is called when you click on the 'Apply' button, or 'Save' menu item.
// It retrieves the current values from the GUI, and then updates the option var values.
global proc save_AL_usdmaya_ProxyShapeImport_optionGUI()
{
  optionVar -iv "AL_usdmaya_ProxyShapeImport_connectToTime" `checkBox -q -v
AL_usdmaya_ProxyShapeImport_connectToTime`; optionVar -iv "AL_usdmaya_ProxyShapeImport_unloaded"
`checkBox -q -v AL_usdmaya_ProxyShapeImport_unloaded`;
};

// After the dialog is created, this method reads the current option var values,
// and sets them in the option box GUI controls.
global proc load_AL_usdmaya_ProxyShapeImport_optionGUI()
{
  textField -e -tx "" AL_usdmaya_ProxyShapeImport_primPath;
  textField -e -tx "" AL_usdmaya_ProxyShapeImport_excludePrimPath;
  textField -e -tx "" AL_usdmaya_ProxyShapeImport_name;
  checkBox -e -v `optionVar -q "AL_usdmaya_ProxyShapeImport_connectToTime"`
AL_usdmaya_ProxyShapeImport_connectToTime; checkBox -e -v `optionVar -q
"AL_usdmaya_ProxyShapeImport_unloaded"` AL_usdmaya_ProxyShapeImport_unloaded;
};

// If you select the 'Reset' menu item, this method will be called.
// It simply resets the GUI controls back to the default values.
global proc reset_AL_usdmaya_ProxyShapeImport_optionGUI()
{
  textFieldButtonGrp -e -fi "" AL_usdmaya_ProxyShapeImport_file;
  textField -e -tx "" AL_usdmaya_ProxyShapeImport_primPath;
  textField -e -tx "" AL_usdmaya_ProxyShapeImport_excludePrimPath;
  textField -e -tx "" AL_usdmaya_ProxyShapeImport_name;
  checkBox -e -v 1 AL_usdmaya_ProxyShapeImport_connectToTime;
  checkBox -e -v 0 AL_usdmaya_ProxyShapeImport_unloaded;
};

// This is the method that builds the the command string to execute.
global proc execute_AL_usdmaya_ProxyShapeImport_optionGUI()
{
  // the command to execute
  string $str = "AL_usdmaya_ProxyShapeImport ";

  // check for any strings that were flagged with 'kStringMustHaveValue' and raise an error
  // if the value has not been specified
  if(`textFieldButtonGrp -ex AL_usdmaya_ProxyShapeImport_file`)
    if(!size(`textFieldButtonGrp -q -fi AL_usdmaya_ProxyShapeImport_file`)) {
      error "File Path must be specified";
    return;
  }

  // extract all of the strings specified with 'kStringOptional'
  if(`textField -ex AL_usdmaya_ProxyShapeImport_primPath`)
    if(size(`textField -q -tx AL_usdmaya_ProxyShapeImport_primPath`))
       $str += " -primPath \"" + `textField -q -tx AL_usdmaya_ProxyShapeImport_primPath` + "\"";

  if(`textField -ex AL_usdmaya_ProxyShapeImport_excludePrimPath`)
    if(size(`textField -q -tx AL_usdmaya_ProxyShapeImport_excludePrimPath`))
       $str += " -excludePrimPath \"" + `textField -q -tx
AL_usdmaya_ProxyShapeImport_excludePrimPath` + "\"";

  if(`textField -ex AL_usdmaya_ProxyShapeImport_name`)
    if(size(`textField -q -tx AL_usdmaya_ProxyShapeImport_name`))
       $str += " -name \"" + `textField -q -tx AL_usdmaya_ProxyShapeImport_name` + "\"";

  // append all of the other command flags
  $str += " -file \"" + `textFieldButtonGrp -q -fi AL_usdmaya_ProxyShapeImport_file` + "\"";
  $str += " -connectToTime " + `optionVar -q "AL_usdmaya_ProxyShapeImport_connectToTime"`;
  $str += " -unloaded " + `optionVar -q "AL_usdmaya_ProxyShapeImport_unloaded"`;

  // run the command!
  eval $str;
};

// construct the text labels for each of the controls
global proc build_AL_usdmaya_ProxyShapeImport_labels()
{
  text -al "right" -h 20 -w 160 -l "File Path:";
  text -al "right" -h 20 -w 160 -l "USD Prim Path:";
  text -al "right" -h 20 -w 160 -l "Exclude Prim Path:";
  text -al "right" -h 20 -w 160 -l "Proxy Shape Node Name:";
  text -al "right" -h 20 -w 160 -l "Connect to Time:";
  text -al "right" -h 20 -w 160 -l "Opens the layer with payloads unloaded.:";
};

// construct each control
global proc build_AL_usdmaya_ProxyShapeImport_controls()
{
  textFieldButtonGrp -h 20 -bl "..." -bc "alFileDialogHandler(\"USD Ascii (*.usd) (*.usd)\",
\"AL_usdmaya_ProxyShapeImport_file\", 1)" AL_usdmaya_ProxyShapeImport_file; textField -h 20
AL_usdmaya_ProxyShapeImport_primPath; textField -h 20 AL_usdmaya_ProxyShapeImport_excludePrimPath;
  textField -h 20 AL_usdmaya_ProxyShapeImport_name;
  checkBox -l "" -h 20 AL_usdmaya_ProxyShapeImport_connectToTime;
  checkBox -l "" -h 20 AL_usdmaya_ProxyShapeImport_unloaded;
};
\endcode




*/
