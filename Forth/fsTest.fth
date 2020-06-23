
forget tst
file tst

255 string buffer

: fsTest
    buffer 255 erase

    "/fs/numbers.txt" "r+" tst fopen if

\        buffer 10 tst fread

        tst buffer fgetline
        cr "Length :" type  . cr

        buffer 255 dump

        buffer 255 erase

        tst 0 fseek

        "Hello There" buffer strcpy

        buffer dup strlen tst fwrite

        tst fflush

        tst fclose
    then
;

fsTest
