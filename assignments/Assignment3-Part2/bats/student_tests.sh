#!/usr/bin/env bats

@test "Exit command terminates shell" {
  run ./dsh <<EOF
exit
EOF

  [ "$status" -eq 0 ]
  [[ "$output" =~ "cmd loop returned -7" ]]
}

@test "Empty input prints warning" {
  run ./dsh <<EOF

EOF

  [ "$status" -eq 0 ]
  [[ "$output" =~ "warning: no commands provided" ]]
  [[ "$output" =~ "cmd loop returned 0" ]]
}

@test "External command execution: pwd" {
  run ./dsh <<EOF
pwd
EOF

  [ "$status" -eq 0 ]
  [[ "$output" =~ "$PWD" ]]
  [[ "$output" =~ "cmd loop returned 0" ]]
}

@test "Quoted string with multiple spaces preserved" {
  run ./dsh <<EOF
echo "  multiple   spaces  "
EOF

  [ "$status" -eq 0 ]
  [[ "$output" =~ "  multiple   spaces  " ]]
  [[ "$output" =~ "cmd loop returned 0" ]]
}

@test "Multiple arguments without quotes" {
  run ./dsh <<EOF
echo hello world
EOF

  [ "$status" -eq 0 ]
  [[ "$output" =~ "hello world" ]]
  [[ "$output" =~ "cmd loop returned 0" ]]
}
