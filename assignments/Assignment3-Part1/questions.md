1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**:  fgets() is well-suited for reading user input line by line. It reads characters from stdin until it encounters a newline or an end-of-file (EOF), which aligns perfectly with a shell’s need to process one command per line. Additionally, fgets() ensures we don't read more characters than fit in our buffer, helping prevent buffer overflows.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  Allocating memory dynamically with malloc() allows the shell to handle variable-length input more flexibly. A fixed-size array imposes a strict limit on input length; if the user types a very long command, it may be truncated or cause overflow issues. By using malloc(), we can allocate exactly as much memory as we expect to need—potentially resizing later if needed.


3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**:  Trimming spaces ensures each command and its arguments are parsed cleanly. Leading or trailing spaces could produce empty tokens, potentially resulting in incorrect parsing (e.g., an empty executable name). In a real shell, extra spaces might cause errors or lead to unexpected behavior like commands failing to execute because the shell interprets a space as an empty command.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  
    Output Redirection (STDOUT)
    Example: ls > output.txt
    What it does: Writes the standard output of ls into output.txt instead of printing to the terminal.
    Challenge: The shell must parse the > symbol, open/create output.txt, and then duplicate the file descriptor so that STDOUT for ls is redirected to this file.

    Input Redirection (STDIN)
    Example: sort < data.txt
    What it does: Feeds the content of data.txt into the sort command as if typed manually.
    Challenge: The shell must parse <, open data.txt for reading, and duplicate the file descriptor so that sort reads from data.txt instead of the keyboard.

    Error Redirection (STDERR)
    Example: gcc code.c 2> errors.log
    What it does: Sends error messages (file descriptor 2) to errors.log while normal output still appears on the terminal.
    Challenge: The shell must handle redirection of STDERR separately from STDOUT and parse 2> specifically, duplicating file descriptor 2 correctly.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:  
    Redirection involves sending a command’s input or output to or from a file (or device). For example, cmd > file writes cmd’s output to file, and cmd < file reads input from file.
    Piping (|) connects the output of one command directly into the input of another, such as cmd1 | cmd2. No file is needed; data streams directly from cmd1’s output to cmd2’s input.

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  Separating error messages (STDERR) from normal output (STDOUT) helps users and scripts distinguish between successful command output and errors or warnings. This allows better debugging, logging, and scripting because you can direct errors to a different place than normal output (e.g., 2> error.log and > output.log).

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:  
    Detecting failure: Typically, the shell should check the command’s exit status (the value returned by wait() or waitpid() after fork()/exec()).
    Handling stderr: By default, errors go to STDERR; the user may redirect it if they want (e.g., 2> file). The shell doesn’t merge STDOUT and STDERR automatically unless the user explicitly requests it (2>&1).
    Option to merge: The shell can let users merge STDOUT and STDERR with special syntax (e.g., ls > output.txt 2>&1), but by default it’s best to keep them separate. This helps with clarity and troubleshooting.