


forget tst
file tst

255 string buffer

 : creat
    buffer 255 erase

    "This is a test" buffer strcpy

    buffer 32 dump

    "/fs/numbers.txt" "w+" tst fopen if
        buffer dup strlen tst fwrite

        tst fclose
    else
            "Failed" type cr
    then

 ;


 creat
