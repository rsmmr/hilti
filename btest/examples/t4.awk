# @TEST-EXEC: ls -a ~ | awk -f %INPUT >dots
# @TEST-EXEC: btest-diff dots
/\.*/ { print $1 }
