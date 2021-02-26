
set PYTHONPATH=T:\Dev\built\usd\20.08\lib\python;%PYTHONPATH%;
::set PXR_PLUGINPATH_NAME=%USD_INSTALL_ROOT%\plugin\usdview;%USD_INSTALL_ROOT%\share\usd\plugins;%PXR_PLUGINPATH_NAME%


set PATH=T:\Dev\built\usd\20.08\bin;T:\Dev\built\usd\20.08\lib;c:\trees\iw9\game\bin\python37;c:\trees\iw9\game\bin\python37\scripts;%PATH%
c:\trees\iw9\game\bin\python37\python build.py --maya-location "C:\Program Files\Autodesk\Maya2021" --pxrusd-location T:\Dev\built\usd\20.08 --devkit-location maya_devkit_2021 T:\Dev\built\maya --redirect-outstream-file False --stages=build,install --build-release --build-args="-DBUILD_WITH_PYTHON_3=ON,-DBUILD_PXR_PLUGIN=OFF,-DBUILD_AL_PLUGIN=OFF"