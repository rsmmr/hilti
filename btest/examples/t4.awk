# @TEST-EXEC: ls -a ~ | awk -f %INPUT >dots
# @TEST-EXEC: tester-diff dots
/\.*/ { print $1 }
