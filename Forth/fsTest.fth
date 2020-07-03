
forget tst
file tst

255 string buffer

: fsTest
    buffer 255 erase

    "/fs/numbers.txt" "r+" tst fopen if

        buffer 10 tst fread
        cr "Length :" type  . cr

        buffer 255 dump

        buffer 255 erase

        tst 0 fseek

        "What's New " buffer strcpy

        buffer dup strlen tst fwrite

        1 "%ld\r\n" buffer strform
        buffer dup strlen tst fwrite

        tst fflush

        tst fclose
    then
;


