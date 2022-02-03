# Building using CMake

ModelRailroadTimetablePlanner uses CMake build system.

CMake Minimum Version: **3.5**


## Dependencies

1. **Qt**: [qt.io](https://www.qt.io)

    Components:
    - Core
    - Gui
    - Widgets
    - Svg
    - PrintSupport
    - LinguistTools

2. **SQLite 3**: [sqlite.org](https://sqlite.org/index.html)

3. **libzip**: [libzip.org](https://libzip.org)

4. **zlib**: [zlib.net](https://www.zlib.net)


## Ubuntu

- Install GCC and CMake
  >`sudo apt install build-essential cmake`

- Install Qt 5:
  >`sudo apt install qtbase5-dev libqt5svg5-dev qttools5-dev qttools5-dev-tools`

- Install SQLite 3
  >`sudo apt install libsqlite3-dev`

- Install libzip
  >`sudo apt install libzip-dev`

- Install zlib
  > NOTE: automatically installed if installing libzip

  >`sudo apt install zlib1g-dev`


## Windows

### Setup Qt
> Set `QT5_DIR` CMake variable:  
> Set to the folder which contains `Qt5Config.cmake` file  
> Example: `C:\Qt\5.15.2\mingw81_64\lib\cmake\Qt5`

### Setup libzip
If compiled from source `libzip` should generate CMake config package files  
This is the preferred method to include it:  
> Set `LibZip_DIR` CMake variable:  
> Set to the folder which contains `libzip-config.cmake` file  
> Example: `%LIBZIP_INSTALL_DIRECTORY%\lib\cmake\libzip`  
> (replace `%LIBZIP_INSTALL_DIRECTORY%` with the directory in which you installed `libzip`)  

Alternative method:  
Include headers directory and libraries directory in CMake variables  
> Append `include` directory to `CMAKE_INCLUDE_PATH` or set `LibZip_INCLUDE_DIRS`  
> Append `lib` and `bin` directory to `CMAKE_LIBRARY_PATH`  

## Troubleshooting

### Windows CMake Doesn't Find DLL

MinGW can link directly to `*.dll` dynamic libraries but CMake is set to look for
import libraries `*.dll.a`.  

**SQLite 3** does not provide an import library so CMake will NOT find it.  
To manually create an import library from a `*.dll` and associated `*.def` file, go to library directory and run:  
> `dlltool --dllname sqlite3.dll --def sqlite3.def --output-lib sqlite3.dll.a`  

For more information see [DLL Import Library Tool](https://www.willus.com/mingw/colinp/win32/tools/dlltool.html)  

## Compile

- Clone the repository
  > `git clone https://github.com/gfgit/ModelRailroadTimetablePlanner`

- Go to project directory
  > `cd ModelRailroadTimetablePlanner`

- Create directory for building the program
  > `mkdir build`  
  > `cd build`

- Run CMake
  > `cmake ..`

  > NOTE: you can use `cmake-gui` to configure CMake variables

- Build
  > `cmake --build .`

  > NOTE: if you have problems locating `libzip` on Ubuntu  
  >       Set `LibZip_LIBRARIES=/usr/lib/x86_64-linux-gnu/libzip.so`

- Install
  > `cmake --build . -t install`

- Run
  > `mrtplanner`

  > NOTE: the location depends on where you installed the program  
  >       Look at `CMAKE_INSTALL_PREFIX` variable

## Doxygen

Enable `BUILD_DOXYGEN` in CMake configuration to generate doxygen documentation.  
The output will go in `build/docs` directory.


## NSIS Installer

> TODO: document `makensis.exe`
