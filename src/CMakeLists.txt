set(SOURCE_FILES main.c driver.c lexer.c parser.c
        arena.c
        codegen.c)

set(EXECUTABLE_OUTPUT_PATH ..)

add_executable(bcc ${SOURCE_FILES}) 
