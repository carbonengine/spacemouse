@echo off
echo Checking out project and filters file
p4 edit SpaceMouse.vcxproj
p4 edit SpaceMouse.vcxproj.filters
echo Regenerating
..\..\..\..\..\..\shared_tools\python\27\python.exe ..\..\tools\ProjectFileGenerator\ProjectFileGenerator.py -i SpaceMouse.ccpproj
pause