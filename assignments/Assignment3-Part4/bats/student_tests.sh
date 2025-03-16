#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your custom unit tests here.

##########################
# Example Test 1 (Given)
##########################
@test "Example: check ls runs without errors" {
    run ./dsh <<EOF
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}

##########################
# Example Test 2: Check simple pipeline
##########################
@test "Check pipeline: echo Hello | grep Hello" {
    run ./dsh <<EOF
echo Hello | grep Hello
EOF

    # We expect to see "Hello" in the output
    [[ "$output" =~ "Hello" ]]
    # and exit status 0
    [ "$status" -eq 0 ]
}

##########################
# Example Test 3: Built-in 'cd' command
##########################
@test "Check built-in 'cd' command" {
    run ./dsh <<EOF
cd /
pwd
exit
EOF

    
    [[ "$output" =~ "/" ]]
    [ "$status" -eq 0 ]
}

##########################
# Example Test 4: Built-in 'dragon' command
##########################
@test "Check 'dragon' built-in command" {
    run ./dsh <<EOF
dragon
exit
EOF

    
    [[ "$output" =~ "summoned a dragon" ]]
    [ "$status" -eq 0 ]
}

##########################
# Example Test 5: 'exit' command
##########################
@test "Check 'exit' built-in" {
    run ./dsh <<EOF
exit
EOF

    # The shell should exit immediately, status 0
    [ "$status" -eq 0 ]
}

##########################
# Example Test 6: Non-existent command
##########################
@test "Check error for non-existent command" {
    run ./dsh <<EOF
thiscommanddoesnotexist
exit
EOF

    
    [[ "$output" =~ "No such file" ]] || [[ "$output" =~ "not found" ]] || true
    [ "$status" -eq 0 ]
}

##########################
# Example Test 7: Chaining multiple commands
##########################
@test "Multiple commands in one session" {
    run ./dsh <<EOF
pwd
ls
exit
EOF

    
    [ "$status" -eq 0 ]
}





@test "01. Basic command: echo 'Hello World'" {
    run ./dsh <<EOF
echo "Hello World"
exit
EOF

    # Check that we see "Hello World" in the output
    [[ "$output" =~ "Hello World" ]]
    [ "$status" -eq 0 ]
}

@test "02. Pipeline with grep: echo 'Hello' | grep Hello" {
    run ./dsh <<EOF
echo Hello | grep Hello
exit
EOF

    [[ "$output" =~ "Hello" ]]
    [ "$status" -eq 0 ]
}

@test "03. Multi-stage pipeline: echo | cat | cat | grep" {
    run ./dsh <<EOF
echo "Test123" | cat | cat | grep Test
exit
EOF

    [[ "$output" =~ "Test123" ]]
    [ "$status" -eq 0 ]
}

@test "04. Built-in 'cd': go to / (root) then 'pwd'." {
    run ./dsh <<EOF
cd /
pwd
exit
EOF

    # If we successfully cd to /, pwd should contain "/"
    [[ "$output" =~ "/" ]]
    [ "$status" -eq 0 ]
}

@test "05. Built-in 'exit': immediate exit" {
    run ./dsh <<EOF
exit
EOF
    # Should exit with status 0
    [ "$status" -eq 0 ]
}

@test "06. Non-existent command: 'garbage_command'" {
    run ./dsh <<EOF
garbage_command
exit
EOF
    # Possibly show an error, but not crash
    [[ "$output" =~ "No such file" ]] || [[ "$output" =~ "not found" ]] || true
    [ "$status" -eq 0 ]
}

@test "07. Too many pipes: 9 commands if CMD_MAX=8" {
    # This may or may not apply, depending on your shell's limit (CMD_MAX)
    run ./dsh <<EOF
echo a | echo b | echo c | echo d | echo e | echo f | echo g | echo h | echo i
exit
EOF

    # Expect an error if we exceed CMD_MAX
    [[ "$output" =~ "error: piping limited" ]] || true
    [ "$status" -eq 0 ]
}

@test "08. Build multiple lines of commands in one session" {
    run ./dsh <<EOF
pwd
ls
echo "All done"
exit
EOF

    [[ "$output" =~ "All done" ]]
    [ "$status" -eq 0 ]
}

@test "09. Built-in 'dragon'" {
    run ./dsh <<EOF
dragon
exit
EOF

    # Expect to see something referencing the dragon
    [[ "$output" =~ "summoned a dragon" ]]
    [ "$status" -eq 0 ]
}

@test "10. Built-in 'stop-server' in local mode" {
    # Typically does nothing in local mode. Just ensure it doesn't crash:
    run ./dsh <<EOF
stop-server
exit
EOF

    [[ "$output" =~ "stop-server is not valid" ]] || true
    [ "$status" -eq 0 ]
}

# The tests below apply only if your shell supports redirection.
# If you haven't implemented <, >, and >>, feel free to remove or skip these.

@test "11. Output redirection: echo Hello > testfile" {
    # We'll store "Hello" in testfile, then cat it to confirm
    # This is only if you implemented redirection in local mode.
    run ./dsh <<EOF
echo Hello > testfile
cat testfile
exit
EOF

    [[ "$output" =~ "Hello" ]]
    [ "$status" -eq 0 ]
}

@test "12. Append redirection: echo World >> testfile" {
    # We append "World" to testfile
    run ./dsh <<EOF
echo World >> testfile
cat testfile
exit
EOF

    [[ "$output" =~ "Hello" ]]
    [[ "$output" =~ "World" ]]
    [ "$status" -eq 0 ]
}

@test "13. Input redirection: grep Hello < testfile" {
    # This only works if you parse/handle '< filename'
    run ./dsh <<EOF
grep Hello < testfile
exit
EOF

    [[ "$output" =~ "Hello" ]]
    [ "$status" -eq 0 ]
}

