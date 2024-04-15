>+>>>>>
++>+>>+++>+>>+

>+[                      set up; for each subarray:
    --[+<<<-]<[                         find the subarray; if it exists:
        [<+>-]<[                        S=pivot; while pivot is in S:
            <[                          if not at end of subarray
                ->[<<<+>>>>+<-]         move pivot left (and copy it) 
                <<[>>+>[->]<<[<]<-]>    move value to S and compare with pivot
            ]>>>+<[[-]<[>+<-]<]>[       if pivot greater then set V=S; else:
                [>>>]+<<<-<[<<[<<<]>>+>[>>>]<-]     swap smaller value into V
                <<[<<<]>[>>[>>>]<+<<[<<<]>-]        swap S into its place
            ]+<<<                       end else and set S=1 for return path
        ]                               subarray done (pivot was swapped in)
    ]+[->>>]>>                          end "if subarray exists"; go to right
]>[brainfuck.org>>>]                    done sorting whole array; output it
