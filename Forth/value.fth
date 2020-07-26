
: value    constant ;

: value!   >body ! ;

: to       state @ if
               postpone ['] postpone value!
           else
               ' value!
           then ; immediate",
