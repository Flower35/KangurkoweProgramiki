@ECHO OFF

SETLOCAL ENABLEEXTENSIONS 2>NUL
IF NOT ERRORLEVEL 1 GOTO MAKE_START
    ECHO * Error: Could not enable Command Extensions!
    GOTO DONE

:MAKE_START
    if NOT []==[%1] GOTO MAKE_ARGS_LOOP
    ECHO Available "make" arguments:
    ECHO.
    ECHO * all
    ECHO.
    ECHO * tas_win32-release
    ECHO * tas_win32-debug
    ECHO * tas_win64-release
    ECHO * tas_win64-debug
    ECHO.
    ECHO * gameinfo_win32-release
    ECHO * gameinfo_win32-debug
    ECHO * gameinfo_win64-release
    ECHO * gameinfo_win64-debug
    ECHO.
    ECHO * digitweaks_win32-release
    ECHO * digitweaks_win32-debug
    ECHO * digitweaks_win64-release
    ECHO * digitweaks_win64-debug
    ECHO.
    ECHO * clean
    GOTO DONE

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:MAKE_ARGS_LOOP
    SET TARGET=
    SET PLATFORM=
    SET CC=
    SET SRCDIR=
    SET TARGETDIR=

    IF [all]==[%1]               GOTO START_ALL
    
    IF [tas_win32-release]==[%1] GOTO START_KAO2TAS_WIN32_RELEASE
    IF [tas_win32-debug]==[%1]   GOTO START_KAO2TAS_WIN32_DEBUG
    IF [tas_win64-release]==[%1] GOTO START_KAO2TAS_WIN64_RELEASE
    IF [tas_win64-debug]==[%1]   GOTO START_KAO2TAS_WIN64_DEBUG
    
    IF [gameinfo_win32-release]==[%1]  GOTO START_KAO2GAMEINFO_WIN32_RELEASE
    IF [gameinfo_win32-debug]==[%1]    GOTO START_KAO2GAMEINFO_WIN32_DEBUG
    IF [gameinfo_win64-release]==[%1]  GOTO START_KAO2GAMEINFO_WIN64_RELEASE
    IF [gameinfo_win64-debug]==[%1]    GOTO START_KAO2GAMEINFO_WIN64_DEBUG
    
    IF [digitweaks_win32-release]==[%1]  GOTO START_KAO2DIGITWEAKS_WIN32_RELEASE
    IF [digitweaks_win32-debug]==[%1]    GOTO START_KAO2DIGITWEAKS_WIN32_DEBUG
    IF [digitweaks_win64-release]==[%1]  GOTO START_KAO2DIGITWEAKS_WIN64_RELEASE
    IF [digitweaks_win64-debug]==[%1]    GOTO START_KAO2DIGITWEAKS_WIN64_DEBUG
    
    IF [clean]==[%1]             GOTO START_CLEANING
    ECHO * Unrecognized argument "%1"
    GOTO MAKE_NEXT_ARG

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:RMDIR
    IF NOT EXIST %1 GOTO DONE
    ECHO Removing the %1 directory...
    DEL /S /Q %1\*.o %1\*.inc %1\*.res %1\*.exe 2>NUL
    GOTO DONE

:START_CLEANING
    ECHO --------------------------------
    ECHO Cleaning...
    ECHO --------------------------------
    CALL :RMDIR "bin\"
    CALL :RMDIR "res\out\"
    GOTO MAKE_NEXT_ARG

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:GCC_SET_PROJECT_FLAGS
    :: CPPFLAGS = "C Preprocessor flags"
    :: CFLAGS   = "C Compiler flags"
    :: LDFLAGS  = "C Linker flags"

    :: Include directory for project header files
    SET CPPFLAGS=-I./%SRCDIR%

    :: Enable all warnings
    SET CFLAGS=-Wall -pedantic-errors

    :: Windows Subsystem ("kernel32" and "user32" are somehow linked "by default")
    SET LDFLAGS=-Wl,--subsystem,%SUBSYSTEM%

    GOTO DONE

:GCC_SET_RELEASE_FLAGS
    CALL :GCC_SET_PROJECT_FLAGS

    :: Enable highest level of optimization
    SET CFLAGS=%CFLAGS% -O3

    :: Remove all symbol table and relocation information from the executable
    SET LDFLAGS=%LDFLAGS% -s

    :: Link-time optimization
    SET LDFLAGS=%LDFLAGS% %LIBRARIES% -Wl,-O1

    GOTO DONE

:GCC_SET_DEBUG_FLAGS
    CALL :GCC_SET_PROJECT_FLAGS

    :: Enable debugging code paths
    SET CPPFLAGS=%CPPFLAGS% -D_DEBUG

    :: Include debug information in the compiled object files
    SET CFLAGS=%CFLAGS% -g

    :: Include debug information in the final executable
    SET LDFLAGS=%LDFLAGS% %LIBRARIES% -g

    GOTO DONE

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:USING_KAO2TAS
    SET TARGET=Kao2_TAS
    SET SUBSYSTEM=windows
    SET SRCDIR=sources\Kao2_TAS
    SET LIBRARIES=-lgdi32 -lcomdlg32
    GOTO DONE
    
:USING_KAO2GAMEINFO
    SET TARGET=Kao2_GameInfo
    SET SUBSYSTEM=windows
    SET SRCDIR=sources\Kao2_GameInfo
    SET LIBRARIES=-lgdi32 
    GOTO DONE

:USING_KAO2DIGITWEAKS
    SET TARGET=Kao2_DigiTweaks
    SET SUBSYSTEM=windows
    SET SRCDIR=sources\Kao2_DigiTweaks
    SET LIBRARIES=-lgdi32 
    GOTO DONE

:USING_PLATFORM_WIN32
    SET PLATFORM=Win32
    SET CC=i686-w64-mingw32-gcc.exe
    GOTO DONE

:USING_PLATFORM_WIN64
    SET PLATFORM=Win64
    SET CC=x86_64-w64-mingw32-gcc.exe
    GOTO DONE

:USING_BUILD_RELEASE
    SET BUILD=Release
    CALL :GCC_SET_RELEASE_FLAGS
    SET TARGETDIR=bin\%PLATFORM%\%BUILD%
    GOTO DONE

:USING_BUILD_DEBUG
    SET BUILD=Debug
    CALL :GCC_SET_DEBUG_FLAGS
    SET TARGETDIR=bin\%PLATFORM%\%BUILD%
    GOTO DONE

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:START_ALL

    CALL :START_KAO2TAS_WIN32_RELEASE
    CALL :START_KAO2TAS_WIN32_DEBUG
    CALL :START_KAO2TAS_WIN64_RELEASE
    CALL :START_KAO2TAS_WIN64_DEBUG
    
    CALL :START_KAO2GAMEINFO_WIN32_RELEASE
    CALL :START_KAO2GAMEINFO_WIN32_DEBUG
    CALL :START_KAO2GAMEINFO_WIN64_RELEASE
    CALL :START_KAO2GAMEINFO_WIN64_DEBUG
    
    CALL :START_KAO2DIGITWEAKS_WIN32_RELEASE
    CALL :START_KAO2DIGITWEAKS_WIN32_DEBUG
    CALL :START_KAO2DIGITWEAKS_WIN64_RELEASE
    CALL :START_KAO2DIGITWEAKS_WIN64_DEBUG
    
    GOTO MAKE_NEXT_ARG

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:START_KAO2TAS_WIN32_RELEASE
    CALL :USING_KAO2TAS
    CALL :USING_PLATFORM_WIN32
    CALL :USING_BUILD_RELEASE
    GOTO VIBECHECK_1

:START_KAO2TAS_WIN32_DEBUG
    CALL :USING_KAO2TAS
    CALL :USING_PLATFORM_WIN32
    CALL :USING_BUILD_DEBUG
    GOTO VIBECHECK_1

:START_KAO2TAS_WIN64_RELEASE
    CALL :USING_KAO2TAS
    CALL :USING_PLATFORM_WIN64
    CALL :USING_BUILD_RELEASE
    GOTO VIBECHECK_1

:START_KAO2TAS_WIN64_DEBUG
    CALL :USING_KAO2TAS
    CALL :USING_PLATFORM_WIN64
    CALL :USING_BUILD_DEBUG
    GOTO VIBECHECK_1

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:START_KAO2GAMEINFO_WIN32_RELEASE
    CALL :USING_KAO2GAMEINFO
    CALL :USING_PLATFORM_WIN32
    CALL :USING_BUILD_RELEASE
    GOTO VIBECHECK_1

:START_KAO2GAMEINFO_WIN32_DEBUG
    CALL :USING_KAO2GAMEINFO
    CALL :USING_PLATFORM_WIN32
    CALL :USING_BUILD_DEBUG
    GOTO VIBECHECK_1

:START_KAO2GAMEINFO_WIN64_RELEASE
    CALL :USING_KAO2GAMEINFO
    CALL :USING_PLATFORM_WIN64
    CALL :USING_BUILD_RELEASE
    GOTO VIBECHECK_1

:START_KAO2GAMEINFO_WIN64_DEBUG
    CALL :USING_KAO2GAMEINFO
    CALL :USING_PLATFORM_WIN64
    CALL :USING_BUILD_DEBUG
    GOTO VIBECHECK_1

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:START_KAO2DIGITWEAKS_WIN32_RELEASE
    CALL :USING_KAO2DIGITWEAKS
    CALL :USING_PLATFORM_WIN32
    CALL :USING_BUILD_RELEASE
    GOTO VIBECHECK_1

:START_KAO2DIGITWEAKS_WIN32_DEBUG
    CALL :USING_KAO2DIGITWEAKS
    CALL :USING_PLATFORM_WIN32
    CALL :USING_BUILD_DEBUG
    GOTO VIBECHECK_1

:START_KAO2DIGITWEAKS_WIN64_RELEASE
    CALL :USING_KAO2DIGITWEAKS
    CALL :USING_PLATFORM_WIN64
    CALL :USING_BUILD_RELEASE
    GOTO VIBECHECK_1

:START_KAO2DIGITWEAKS_WIN64_DEBUG
    CALL :USING_KAO2DIGITWEAKS
    CALL :USING_PLATFORM_WIN64
    CALL :USING_BUILD_DEBUG
    GOTO VIBECHECK_1

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:VIBECHECK_1
    ECHO --------------------------------
    ECHO Building "%TARGET%" (%PLATFORM% %BUILD%)!
    ECHO --------------------------------
    
    WHERE %CC% >NUL 2>NUL
    IF NOT ERRORLEVEL 1 GOTO VIBECHECK_2
    ECHO * Error: "%CC%" not installed!
    GOTO MAKE_NEXT_ARG

:VIBECHECK_2
    WHERE windres.exe >NUL 2>NUL
    IF NOT ERRORLEVEL 1 GOTO COMPILE_SPLASH_SCREEN_1
    ECHO * Error: "windres.exe" not installed!
    GOTO MAKE_NEXT_ARG

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:GCC_COMPILE_OBJ
    IF [YES]==[%ABORTFLAG%] GOTO DONE

    :: Remove quotes from Arg1
    SET C_NAME=%~1

    :: Remove "%CD%\" from the full path
    CALL SET C_NAME=%%C_NAME:%CD%\=%%

    :: Replace ".c" with ".o"
    SET O_NAME=%C_NAME:.c=.o%

    :: Replace "%SRCDIR%\" with "%TARGETDIR%\%TARGET%_obj\"
    :: (In batch files, unlike in CMD.EXE prompt,
    :: double %% are expanded to single % first!)
    CALL SET O_NAME=%%O_NAME:%SRCDIR%\=%TARGETDIR%\%TARGET%_obj\%%

    IF EXIST "%O_NAME%" GOTO GCC_COMPILE_OBJ_OK
    
    ECHO Compiling "%C_NAME%"...
    %CC% -c %CPPFLAGS% %CFLAGS% -o "%O_NAME%" "%C_NAME%"
    IF NOT ERRORLEVEL 1 GOTO GCC_COMPILE_OBJ_OK
    SET ABORTFLAG=YES
    GOTO DONE
    
:GCC_COMPILE_OBJ_OK
    SET OBJ_LIST=%OBJ_LIST% "%O_NAME%"
    GOTO DONE

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:COMPILE_SPLASH_SCREEN_1
    IF NOT EXIST "res\%TARGET%_splash.bmp" GOTO COMPILE_OBJ_FILES_1
    IF EXIST "res\out\bmp2inc.exe" GOTO COMPILE_SPLASH_SCREEN_2
    IF NOT EXIST res\out\ MD res\out
    ECHO.
    ECHO Compiling "bmp2inc" mini-tool...
    %CC% -O3 -o "res/out/bmp2inc.exe" "res/bmp2inc.c" -s -Wl,--subsystem,console
    IF ERRORLEVEL 1 GOTO MAKE_NEXT_ARG
    ECHO "res\out\bmp2inc.exe": OK!

:COMPILE_SPLASH_SCREEN_2
    IF EXIST "res\out\%TARGET%_splash.inc" GOTO COMPILE_SPLASH_SCREEN_OK
    ECHO.
    ECHO Converting "res\%TARGET%_splash.bmp" into "res\out\%TARGET%_splash.inc"...
    "res\out\bmp2inc.exe" "res\%TARGET%_splash.bmp" "res\out\%TARGET%_splash.inc"
    IF ERRORLEVEL 1 GOTO MAKE_NEXT_ARG
    ECHO "res\out\%TARGET%_splash.inc": OK!

:COMPILE_SPLASH_SCREEN_OK
    SET CPPFLAGS=%CPPFLAGS% -I./res/out

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:COMPILE_OBJ_FILES_1
    ECHO.
    :: Creates same directory structure (source files to object files)
    XCOPY "%SRCDIR%\" "%TARGETDIR%\%TARGET%_obj\" /T
    IF NOT ERRORLEVEL 1 GOTO :COMPILE_OBJ_FILES_2
    ECHO * Error: Could not copy the directory structure!
    GOTO MAKE_NEXT_ARG

:COMPILE_OBJ_FILES_2
    SET ABORTFLAG=NO
    SET OBJ_LIST=
    FOR /R "%SRCDIR%\" %%a IN (*.c) DO CALL :GCC_COMPILE_OBJ "%%a"
    IF [YES]==[%ABORTFLAG%] GOTO MAKE_NEXT_ARG

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:COMPILE_RESOURCE
    IF NOT EXIST "res\%TARGET%.rc" GOTO LINK_EXE
    IF EXIST "res\out\%TARGET%.res" GOTO COMPILE_RESOURCE_OK
    IF NOT EXIST res\out\ MD res\out
    ECHO.
    ECHO Creating Windows resource "res\out\%TARGET%.res"...
    windres.exe "res/%TARGET%.rc" -O coff --target=pe-i386 "res/out/%TARGET%.res"
    IF ERRORLEVEL 1 GOTO MAKE_NEXT_ARG
    ECHO "res\out\%TARGET%.res": OK!

:COMPILE_RESOURCE_OK
    SET OBJ_LIST=%OBJ_LIST% "res/out/%TARGET%.res"

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:LINK_EXE
    ECHO.
    ECHO Linking "%TARGETDIR%/%TARGET%.exe"...
    %CC% -o "%TARGETDIR%/%TARGET%.exe" %OBJ_LIST% %LDFLAGS%
    IF ERRORLEVEL 1 GOTO MAKE_NEXT_ARG
    ECHO "%TARGET%" (%PLATFORM% %BUILD%): OK!

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:MAKE_NEXT_ARG
    SHIFT
    IF []==[%1] GOTO DONE
    GOTO MAKE_ARGS_LOOP

:DONE
