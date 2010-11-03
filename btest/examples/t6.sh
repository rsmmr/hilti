# @TEST-EXEC: awk -f %INPUT <foo.dat >output
# @TEST-EXEC: btest-diff output

    { lines += 1; }
END { print lines; }

@TEST-START-FILE foo.dat
1
2
3
@TEST-END-FILE
