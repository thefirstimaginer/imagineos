else if (strcmp(cmd_name, "clear") == 0) {
    print_clear();
    shell_print_prompt();
    input_index = 0;
    return;
}