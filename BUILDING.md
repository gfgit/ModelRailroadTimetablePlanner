# Building using CMake

ModelRailroadTimetablePlanner uses CMake build system.

CMake Minimun Version: **3.5**


## Dependencies

1. **Qt**

    Components:
    - Core
    - Gui
    - Widgets
    - Svg
    - PrintSupport
    - LinguistTools

2. **SQLite 3**

3. **libZip**

4. **ZLib**


## Ubuntu

- Install GCC and CMake
  >`sudo apt install build-essential cmake`

- Install Qt 5:
  >`sudo apt install qtbase5-dev libqt5svg5-dev qttools5-dev qttools5-dev-tools`
    
- Install [SQLite 3](https://sqlite.org/index.html)
  >`sudo apt install libsqlite3-dev`

- Install [libzip](https://libzip.org)
  >`sudo apt install libzip-dev`

- Install [zlib](https://www.zlib.net)
  > NOTE: automatically installed if installing libzip
  
  >`sudo apt install zlib1g-dev`


## Windows

### Setup Qt
> Set `QT5_DIR` CMake variable:  
> Example: `C:/Qt/5.15.2/mingw81_64/lib/cmake/Qt5`


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
  > 'cmake --build . -t install`

- Run
  > `mrtplanner`
  
  > NOTE: the location depends on where you installed the program

## Doxygen

Enable `BUILD_DOXYGEN` in CMake configuration


## NSIS Installer

> TODO: document `makensis`
