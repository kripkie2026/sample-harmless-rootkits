# FakeTime.c:

## Without the trick
$ date
Sun Jul 12 15:21:42 CEST 2026

## With LD_PRELOAD – always 13:37
$ LD_PRELOAD=./time_1337.so date
Sun Jul 12 13:37:00 CEST 2026

## You can also use it with any command that reads the time
$ LD_PRELOAD=./time_1337.so python3 -c "import time; print(time.ctime())"
Sun Jul 12 13:37:00 2026

unset LD_PRELOAD

--------
# no_name.c

## Create test files in /tmp
cd /tmp
touch miki nikola petar normal_file secret.txt
echo "Created test files"

## Normal listing - all files visible
ls -la /tmp/
## You'll see: miki, nikola, petar, normal_file, secret.txt

## With LD_PRELOAD - hidden files disappear!
LD_PRELOAD=./hide_files.so ls -la /tmp/
# You'll see: normal_file, secret.txt  (miki, nikola, petar are hidden!)

## Works with find command too
LD_PRELOAD=./hide_files.so find /tmp -name "miki"
## Returns nothing (file is hidden)

## Works with bash globbing
LD_PRELOAD=./hide_files.so bash -c 'ls /tmp/m*'
## Only shows files starting with 'm' that aren't miki

## Wildcard expansion also filtered
LD_PRELOAD=./hide_files.so bash -c 'echo /tmp/*'
## miki, nikola, petar won't appear in the expanded list
