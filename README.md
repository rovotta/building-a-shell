# building-a-shell
simple command-line environment to perform basic tasks

## When run, this shell can:

1. Print a prompt and wait for the user to type in a command.
2. Read in the command string entered by the user.
3. Tokenize the command line string.
4. If the command (i.e., the first token in the parsed list) is a built-in shell command,
then handle this internally (without forking a child process).\\
5. Otherwise, if it’s not a built-in command, fork a child process to execute the command
and wait for it to finish. (Unless if the command is to be run in the background, in
which case the shell does not wait.)\\
6. Repeat steps 1–5 until the user enters the built-in command exit to exit the shell
program.
