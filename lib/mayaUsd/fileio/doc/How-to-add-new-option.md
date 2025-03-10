# How to add a new option to import or export

Search for an existing option in the code and do the same for the new option
everywhere you find the old existing option. Use an existing option with a
very unique name to avoid false matches.

## Adding the option UI, command flag and import/export argument

Usually, you will need to modify these places:

- baseImportCommand.h: (or baseExportCommand.h)
	- Add the short flag

- baseImportCommand.cpp: (or baseExportCommand.cpp)
	- Add the flag to the syntax

- lib/mayaUsd/commands/Readme.md:
	- Document the new flag

- lib/mayaUsd/fileio/jobs/jobArgs.h:
	- Add the token to the import or export tokens
	- Add data item to the import or export struct

- lib/mayaUsd/fileio/jobs/jobArgs.cpp:
	- Handle parsing the flag in the constructor
	- Add default value to GetDefaultDictionary
	- Add the type to GetGuideDictionary
	- Add writing the value to the operator<<

- wrapPrimReader.cpp:
	- Expose the option to Python

- mayaUSDRegisterStrings.py:
	Add the new UI labels

- mayaUsdTranslatorImport.mel: (or mayaUsdTranslatorExport.mel)
	- Add new UI elements in mayaUsdTranslatorImport "post"
	- Read the UI values in mayaUsdTranslatorImport "query"
	- Add the UI in EnableAllControls
	- Add filling the UI with data in SetFromOptions
	- Maybe add some callback when UI valeu change when UI affect other UI.

## Handling the option in code

Afterward, modify the import/export code itself to handle the new option and
add new behavior during the actual import or export.
