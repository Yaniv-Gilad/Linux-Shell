Linux Shell

The program implements a simple shell of linux operating system.
Support pipe and redirection commands.
The program saves all the commands that the user input in a log file.
The program will keep getting inputs from the user until the user insert "done".

Input - simple commands as:
mkdir [arg]
ls [arg]
sleep [arg]
echo [arg]
pwd
ls -l | wc -l
sort < [file_name]
ls -l | wc -l >> [file_name]
done    // will end the run of the program.


Output:
the regular output of linux shell and in the end the program will print the next statistics:
num of commands: [number]
total length of all commands: [number]
average length of all commands: [number]
Number of command that include pipe: [number]
Number of command that include redirection: [number]
see you next time!


How to compile:
In VSCode ctrl+shift+b.
In linux: gcc shell.c -o shell.

How to run:
In VSCode ctrl+f5.
In linux: open the terminal in the folder and write "./shell log.txt" and press enter.

Program files:
makefile
shell.c   // main
shell    // Executable file
