# MyShell

MyShell is a Unix shell implementation written in C. It provides a
command-line interface where users can execute commands, create aliases,
and perform basic system operations. The shell supports various features,
including command history, background processing, and output redirection.

## Features
    To use this makefile, save it as a file named makefile (or Makefile) in the
    same directory as your myshell.c. Open a terminal and run make to compile
    the myshell executable. Use make clean to remove the compiled executable.

- Command execution: Users can run commands just like in a regular shell.
- Built-in commands: MyShell supports some built-in commands such as `cd`, `help`, and `exit`.
- Input/output redirection: Users can redirect input and output using `<` and `>` operators.
- Background execution: Commands can be executed in the background using the `&` operator.

## Usage

To compile the program, run the following command:
make 