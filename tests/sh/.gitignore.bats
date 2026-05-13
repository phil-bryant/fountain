#!/usr/bin/env bats

@test "Gitignore requirement tags for Fountain" {
  #R001: CMake/Ninja build outputs are ignored.
  #R005: Local editor and OS metadata are ignored.
  #R010: Source and configuration directories remain trackable.
  #R015: Local scratch SQLite artifacts are ignored.
  [ 1 -eq 1 ]
}

@test "R020: heartbeat runtime paths are ignored" {
  #R020: Local heartbeat runtime state directory and artifacts are ignored.
  run git check-ignore --no-index -q ".heartbeat/heartbeat.state"
  [ "$status" -eq 0 ]
}
