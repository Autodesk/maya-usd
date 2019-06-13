#-
# =======================================================================
# Copyright 2019 Autodesk, Inc. All rights reserved.
#
# This computer source code and related instructions and comments are the
# unpublished confidential  and proprietary information of Autodesk, Inc.
# and are protected under applicable copyright and trade secret law. They 
# may not be disclosed to, copied  or used by any third party without the 
# prior written consent of Autodesk, Inc.
# =======================================================================
#+

# Ufe and USD Python plugins
set(python_plugin_files
    maya-ufe-usd-plugin.py
)

# Ufe python scripts
set(python_script_files
    ufeScripts/usdRunTime.py
    ufeScripts/usdSubject.py
    ufeScripts/usdXformOpCmds.py
    ufeScripts/__init__.py
)

install(FILES ${python_plugin_files} DESTINATION python)
install(FILES ${python_script_files} DESTINATION python/ufeScripts)
