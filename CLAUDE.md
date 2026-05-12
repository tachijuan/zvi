HiTech-C CP/M VI clone

Create a VI clone that will be run in the CP/M 2.2 or CP/M 3.0 environments
The app is written in the HiTech-C C CP/M dialect of the c-language. Documentation for what language features and how to use the compiler are in the project directory. You must conform to the capabilities as described in these manuals. 
The app should be complete and meet all of the following specifications
The app is written to support ANSI terminals, but can be patched to support other terminal types
The app should have an option to emit debug information so that we can troubleshoot any errors. The output of the debug info should go to the standard console.
The app leverages gap buffers to maximize editing performance and simplify code generation
The app will attempt to store the file contents in RAM, but can edit files larger than available RAM
The app must support basic VI movement, selection, insert, append, change, search, and undo capablities
The app must support the basic EX commands (:r, :q, :x, :w, :x!, :wq!)
The app should be optimized to be as small as possible to maximize user RAM
The app must be performant 
The app will accept a filename on the command line or start with a blank file. The user can save the file with the :w and :x commands
The app should start assuming the terminal is 80 columns by 24 lines, but be responsive to terminal size by using ANSI terminal size query commands
The app should be compilable by the standard Digital Research ASM 8080assembler in CP/M. The user will do all the compiling and provide feedback. The documentation should include all necessary steps to build and debug the app using CP/M native tools. The user will provide feedback and debugging info.
The app should be written in a way that is easy to maintain and extend.
The app should be written in a way that is easy to test.
The app source code should be well documented. Every source file should have a header with "Juan Orlandini" as the author and an MIT license
Document all the necessary steps to build the app in the README.md file
Document all the supported commands and keybindings in the README.md file
Make sure than any generated source code follows the CP/M and DOS file ending (CR+LF).

