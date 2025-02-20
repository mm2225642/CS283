1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**:  We use fork() followed by execvp() instead of calling execvp() directly because execvp() replaces the current process’s memory image with the new program, effectively terminating the original process. If the shell called execvp() directly (e.g., execvp("pwd", ...)), it would overwrite itself with pwd, and the shell would exit, unable to continue prompting for user input.

    The fork() system call creates a separate child process, allowing the parent process (the shell) to persist while the child executes the command. Key values of fork() include:

    Process Isolation: The shell remains active, unaffected by the child’s execution.
    Control & Monitoring: The parent can wait for the child (via waitpid) to finish, retrieve its exit status (for last_rc in my extra credit), and handle errors or successes.
    Multitasking: Enables the shell to manage multiple child processes independently, supporting features like background execution in future enhancements.

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  If fork() fails, it returns -1 to the calling process, and no child process is created. This typically occurs due to system resource constraints, such as exceeding the process limit (EAGAIN) or insufficient memory (ENOMEM), with errno set accordingly.

    In my implementation in dshlib.c, the exec_cmd() function handles this:
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            last_rc = errno;
            return ERR_EXEC_CMD;
        }
    When fork() fails, it:

    Prints an error message (e.g., "fork failed: Resource temporarily unavailable") using perror.
    Sets last_rc to the errno value (part of the extra credit) for later inspection via the rc command.
    Returns ERR_EXEC_CMD (-6), allowing exec_local_cmd_loop() to continue the shell loop and print the next prompt (dsh2>). This ensures the shell remains operational despite the failure.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**:  The execvp() function locates the command to execute by searching directories specified in the PATH environment variable. If the command includes a full path (e.g., /bin/pwd), execvp() uses it directly. Otherwise, for a bare command name (e.g., pwd), it prepends each directory in PATH (e.g., /usr/local/bin:/usr/bin:/bin) and checks for an executable file. If no match is found, it fails with errno set to ENOENT ("No such file or directory").

    The PATH environment variable is critical, enabling users to run commands without specifying their locations (e.g., echo $PATH might show /bin:/usr/bin). In my shell, this allows commands like which or echo to execute seamlessly, as tested in assignment_tests.sh.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**:  The wait() system call (or waitpid() in my code) suspends the parent process until a child process terminates, allowing the parent to collect the child’s exit status. In exec_cmd():
    int status;
    waitpid(pid, &status, 0);
    Its purpose is to:

    Reap the child process, preventing it from becoming a zombie (a terminated process lingering in the process table).
    Retrieve the exit status (via WEXITSTATUS), which I use to set last_rc for the extra credit rc command.
    If waitpid() weren’t called, the child would become a zombie upon termination, occupying a process table entry until the shell exits or the system reaps it (e.g., via init). Over many commands, this could exhaust system resources, slowing performance. Additionally, the shell couldn’t track command outcomes (e.g., success or failure), limiting features like return code reporting.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**:  WEXITSTATUS(status) extracts the exit code returned by a child process from the status value provided by waitpid(), assuming WIFEXITED(status) confirms normal termination. In my exec_cmd():
    if (WIFEXITED(status)) {
        last_rc = WEXITSTATUS(status);
    }
    It provides the integer value the child passed to exit() (e.g., 0 for success, 2 for ENOENT), reflecting the command’s outcome.

This is important because it allows the shell to:

Determine success (0) or failure (non-zero), aligning with Unix conventions.
Store the result in last_rc for the rc command (extra credit), enabling users to inspect command status, similar to Bash’s $?. This enhances debugging and scripting capabilities, as seen in my student_tests.sh tests like "Command not found sets rc to 2".

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**:  My build_cmd_buff() function handles quoted arguments by preserving spaces within double quotes, treating the quoted text as a single argument:

    It uses a in_quotes flag, toggled by ", to track when spaces should be preserved.
    For input like echo " hello world":
    Trims outer spaces, yielding echo " hello world".
    Splits into argv[0] = "echo", argv[1] = " hello world", with argc = 2.
    Spaces outside quotes are separators, but inside quotes, they remain part of the argument.
    This is necessary because Unix commands often treat quoted strings as single entities. For example, echo "hello world" should output "hello world", not split into "hello" and "world" as separate arguments. This ensures correct behavior, as tested in "It handles quoted spaces" from assignment_tests.sh, maintaining compatibility with shell standards.

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**:  n the previous assignment, my parsing logic likely processed a list of commands (possibly with pipes) into multiple structs, splitting on spaces without quote awareness. For this assignment, I refactored build_cmd_buff() to:

    Use a single cmd_buff_t struct with argc and argv, removing pipe support (deferred to future assignments).
    Trim all leading/trailing spaces and handle duplicates outside quotes.
    Add quote parsing to preserve spaces within "...", ensuring proper argument grouping.
    Null-terminate argv for execvp().
    Unexpected challenges included:

    Quote Handling: Implementing in_quotes logic was complex, especially for edge cases like unpaired quotes or nested spaces (e.g., " hello "), requiring careful pointer management.
    Prompt Synchronization: Refactoring broke prompt timing (e.g., extra dsh2> in output), fixed by adding printf("%s", SH_PROMPT) after each command in exec_local_cmd_loop().
    Memory Leaks: Switching to strdup necessitated robust cleanup in clear_cmd_buff(), unlike a static buffer approach.

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**:  Signals in Linux are asynchronous notifications sent to processes to signal events like interruptions (e.g., Ctrl+C), errors (e.g., segmentation fault), or termination requests. They allow the kernel or other processes to alert a process immediately, triggering default actions (e.g., terminate) or custom handlers if caught.

    Compared to other IPC methods (e.g., pipes, shared memory):

    Asynchronous: Signals interrupt execution instantly, unlike pipes requiring explicit read/write operations.
    One-Way: Carry only a signal number, no data payload, unlike message queues or pipes.
    Kernel-Driven: Often initiated by the system (e.g., SIGSEGV), not just inter-process, unlike most IPC needing mutual setup.

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  
    SIGINT (2):
        Description: Interrupt signal, sent via Ctrl+C.
        Use Case: Stops a running process (e.g., a looping script) gracefully; catchable for cleanup (e.g., saving state).
    SIGTERM (15):
        Description: Termination signal, a polite shutdown request.
        Use Case: Used by kill or system tools (e.g., systemctl stop) to end processes cleanly; catchable for orderly exit.
    SIGKILL (9):
        Description: Immediate kill signal, uncatchable.
        Use Case: Forces termination of unresponsive processes (e.g., kill -9 <pid> when a program hangs).

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  When a process receives SIGSTOP, it is paused immediately by the kernel, suspending execution until a SIGCONT signal resumes it (e.g., via kill -CONT <pid> or fg). This is used for job control (e.g., Ctrl+Z in a terminal).

    Unlike SIGINT, SIGSTOP cannot be caught or ignored. Attempts to handle it with signal() or sigaction() fail, as the kernel enforces stopping to ensure system-level control (e.g., for debugging or process management). This mandatory behavior distinguishes it from SIGINT, which processes can trap for custom responses.
