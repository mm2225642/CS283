#!/usr/bin/env bats

# File: student_tests.sh
# Student unit test suite for dsh shell with pipes and extra credit

@test "Single command: pwd" {
    run ./dsh <<EOF
pwd
EOF

    current=$(pwd)
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="dsh3>${current}dsh3>cmdloopreturned0"

    echo "Output: $output"
    echo "Stripped: $stripped_output -> $expected_output"

    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Exit command terminates shell" {
    run ./dsh <<EOF
exit
EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="dsh3>exiting...cmdloopreturned-7"

    echo "Output: $output"
    echo "Stripped: $stripped_output -> $expected_output"

    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Empty input prints warning" {
    run ./dsh <<EOF

EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="dsh3>warning:nocommandsprovideddsh3>cmdloopreturned0"

    echo "Output: $output"
    echo "Stripped: $stripped_output -> $expected_output"

    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Pipe: ls | grep dshlib.c" {
    run ./dsh <<EOF
ls | grep dshlib.c
EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="dsh3>dshlib.cdsh3>cmdloopreturned0"

    echo "Output: $output"
    echo "Stripped: $stripped_output -> $expected_output"

    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "cd to invalid directory sets rc" {
    run ./dsh <<EOF
cd /nonexistentdir
rc
EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="dsh3>cdfailed:Nosuchfileordirectorydsh3>2dsh3>cmdloopreturned0"

    echo "Output: $output"
    echo "Stripped: $stripped_output -> $expected_output"

    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Redirection: echo > out.txt" {
    run ./dsh <<EOF
echo "test output" > out.txt
cat out.txt
EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="dsh3>dsh3>testoutputdsh3>cmdloopreturned0"

    echo "Output: $output"
    echo "Stripped: $stripped_output -> $expected_output"

    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]

    rm -f out.txt
}

@test "Pipe with redirection: ls | grep .c > out.txt" {
    run ./dsh <<EOF
ls | grep .c > out.txt
cat out.txt
EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="dsh3>dsh3>dshlib.cdsh3>cmdloopreturned0"

    echo "Output: $output"
    echo "Stripped: $stripped_output -> $expected_output"

    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]

    rm -f out.txt
}

@test "Long pipeline: ls | grep .c | sort | uniq" {
    run ./dsh <<EOF
ls | grep .c | sort | uniq
EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="dsh3>dsh_cli.cdsh3>dshlib.cdsh3>cmdloopreturned0"

    echo "Output: $output"
    echo "Stripped: $stripped_output -> $expected_output"

    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "cd changes directory" {
    run ./dsh <<EOF
mkdir -p testdir
cd testdir
pwd
cd ..
rm -r testdir
EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="dsh3>dsh3>/path/to/testdir dsh3>cmdloopreturned0"  # Modify accordingly

    echo "Output: $output"
    echo "Stripped: $stripped_output -> $expected_output"

    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Run process in background" {
    run ./dsh <<EOF
sleep 1 &
ps
EOF

    echo "Output: $output"

    [ "$status" -eq 0 ]
}

@test "Pipe and input redirection together" {
    run ./dsh <<EOF
echo "test1\ntest2" > infile.txt
cat < infile.txt | grep test1
rm infile.txt
EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="dsh3>test1dsh3>cmdloopreturned0"

    echo "Output: $output"
    echo "Stripped: $stripped_output -> $expected_output"

    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

