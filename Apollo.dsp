# Microsoft Developer Studio Project File - Name="Apollo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Apollo - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Apollo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Apollo.mak" CFG="Apollo - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Apollo - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Apollo - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Apollo - Win32 ReleaseProfiled" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Apollo - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FAs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comctl32.lib advapi32.lib comdlg32.lib winmm.lib ddraw.lib dxguid.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Apollo - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dxguid.lib ddraw.lib kernel32.lib user32.lib gdi32.lib comctl32.lib advapi32.lib comdlg32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /profile

!ELSEIF  "$(CFG)" == "Apollo - Win32 ReleaseProfiled"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Apollo___Win32_ReleaseProfiled"
# PROP BASE Intermediate_Dir "Apollo___Win32_ReleaseProfiled"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FAs /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FAs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib comctl32.lib advapi32.lib comdlg32.lib dxguid.lib ddraw.lib winmm.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 dxguid.lib ddraw.lib kernel32.lib user32.lib gdi32.lib comctl32.lib advapi32.lib comdlg32.lib winmm.lib /nologo /subsystem:windows /profile /debug /machine:I386

!ENDIF 

# Begin Target

# Name "Apollo - Win32 Release"
# Name "Apollo - Win32 Debug"
# Name "Apollo - Win32 ReleaseProfiled"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Gui"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\console.cpp
# End Source File
# Begin Source File

SOURCE=".\R4K Debugger.cpp"
# End Source File
# Begin Source File

SOURCE=.\Settings.cpp
# End Source File
# Begin Source File

SOURCE=.\WinMain.cpp
# End Source File
# End Group
# Begin Group "Emu"

# PROP Default_Filter ""
# Begin Group "RCP"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\rcp.cpp
# End Source File
# End Group
# Begin Group "r4300"

# PROP Default_Filter ""
# Begin Group "interpretive"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\FPU Machine.cpp"
# End Source File
# Begin Source File

SOURCE=".\MMU Machine.cpp"
# End Source File
# Begin Source File

SOURCE=".\R4K Machine.cpp"
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\EmuMain.cpp
# End Source File
# Begin Source File

SOURCE=".\R4K Memory.cpp"
# End Source File
# End Group
# Begin Group "Plugin Overhead"

# PROP Default_Filter ""
# Begin Group "Video"

# PROP Default_Filter ""
# Begin Group "Internal Video"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\gfxmain.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\Video.cpp
# End Source File
# End Group
# Begin Group "Audio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Audio.cpp
# End Source File
# End Group
# Begin Group "Input"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Controllers.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\Plugins.cpp
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "GuiH"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\console.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\WinMain.h
# End Source File
# End Group
# Begin Group "EmuH"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EmuMain.H
# End Source File
# Begin Source File

SOURCE=.\inputdll.h
# End Source File
# Begin Source File

SOURCE=.\plugin.h
# End Source File
# Begin Source File

SOURCE=.\videodll.h
# End Source File
# End Group
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Group "icons"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\apollo.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\rscript.rc
# End Source File
# End Group
# Begin Source File

SOURCE=".\todo-KAH.txt"
# End Source File
# End Target
# End Project
