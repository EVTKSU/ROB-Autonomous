# Instructions on PlatformIO use

## only one file may be in src at one time
    - main.cpp is to be used as the running code

## lib stores libraries for use in src
    - Add neccesarry libraries as folders in lib

## How to run code
    - Open the TeensyDevelopment folder in VSCode *(BY ITSELF)*
    - Wait for platformio to load and configure
    - Open src\main.cpp
    - Check mark icon compiles code
    - -> arrow icon uploads code
    - Plug icon displays serial monitor

## How to store multiple files
    - Store files to test in TeensyTestCode
    - Comment the name of your file at the top of your cpp/ino file
    - Paste code from file to run in src\main.cpp

### Side note: any future PlatformIO projects go under the PlatformIO directory


# why doesnst it compile?!?
    -make sure that the ethernet max packet size in the teensy library is increased to 64. it's limited by default.
    
# what is the natural state machine? 

    - making a proper state machine setup always nukes the whole codebase every time. i've tried 6 times. please don't ask.