set(SOURCE_FILES main.c driver.c lexer.c parser.c
        arena.c ir_gen.c pretty_print.c
        codegen.c
        semantic_analysis.h
        semantic_analysis.c)

set(EXECUTABLE_OUTPUT_PATH ..)

add_executable(bcc ${SOURCE_FILES})

add_executable(semantic_analysis_test semantic_analysis_test.c semantic_analysis.c semantic_analysis.h)
add_test(NAME SemanticAnalysisTest COMMAND semantic_analysis_test)