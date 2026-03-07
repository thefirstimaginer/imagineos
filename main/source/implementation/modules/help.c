#include "print.h"
#include "string.h"
#include "modules.h"

void help_init() {
	print_str("");
}

void help_run(char* args) {
	help_module_main(args);
}

void help_module_main(char* args) {

	//Page 1
	if(strcmp(args, "ver") == 0){
		print_str("Help Guide Module for ImagineOS\n");
		print_str("Copyright (C) 2026 Imagine Project,");
		return;
	}
	else if (strcmp(args, "1") == 0) {
		print_str("+---------------------------------------------+\n");
		print_str("|");
		print_set_color(PRINT_COLOR_CYAN, PRINT_COLOR_BLACK);
		print_str("        ImagineOS Help Guide V2.0            ");
		print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
		print_str("|\n");
		print_str("+---------------------------------------------+\n");
		print_str("|              [SYSTEM COMMANDS]              |\n");
		print_str("|             --BASIC COMMANDS--              |\n");
		print_str("| [help] - Show Help.                         |\n");
		print_str("| [date] - Show the current time.             |\n");
		print_str("| [clear] - Clear the shell.                  |\n");
		print_str("| [history] - Command history, only the last  |\n");
		print_str("| command.                                    |\n");
		print_str("| [ver] - Show your OS version.               |\n");
		print_str("| [reboot] - Reboot the System.               |\n");
		print_str("| [halt] - Turn off the system.               |\n");
		print_str("| [calc] - Basic operations (2 NUMBERS ONLY!) |\n");
		print_str("| [halt] - Turn off the system.               |\n");
		print_str("+---------------------------------------------+");
		return;
	}
	else if(strcmp(args, "2") == 0) {
		print_str("+-------------------------------------------------+\n");
		print_str("|");
		print_set_color(PRINT_COLOR_CYAN, PRINT_COLOR_BLACK);
		print_str("          ImagineOS Help Guide V2.0              ");
		print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
		print_str("|\n");
		print_str("+-------------------------------------------------+\n");
		print_str("|                 [PROGRAMS]                      |\n");
		print_str("| [LITEINTERP] - A Lightweight Interpreter - BETA |\n");
		print_str("| [CALC] - A Calculator!                          |\n");
		print_str("| - More programs coming soon!                    |\n");
		print_str("| - To run a program just type the program        |\n");
		print_str("| name in lowerscape.                             |\n");
		print_str("+-------------------------------------------------+");
		return;
	}
	else if(strcmp(args, "3") == 0) {
		print_str("+-----------------------------------------------+\n");
		print_str("|");
		print_set_color(PRINT_COLOR_CYAN, PRINT_COLOR_BLACK);
		print_str("         ImagineOS Help Guide V2.0             ");
		print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
		print_str("|\n");
		print_str("+-----------------------------------------------+\n");
		print_str("|              [ABOUT IMAGINE OS]               |\n");
		print_str("| ImagineOS is a 64bit Operating System created |\n");
		print_str("|    by one person, i like creating operating   |\n");
		print_str("| systems, if you want to learn how the system  |\n");
		print_str("|   works or even create apps for it, read the  |\n");
		print_str("|   readme.pdf in the '/imagine/docs/' folder,  |\n");
		print_str("|    thank you for using the DEMO version of    |\n");
		print_str("|     ImagineOS R1!, i see you in the future.   |\n");
		print_str("| - The First Imaginer                          |\n");
		print_str("+-----------------------------------------------+");
		return;
	}

	print_str("To see the pages on this book, type 'help <number>'.\n");
	print_str("[SECTIONS]:\n");
	print_str("[1] - BASIC COMMANDS\n");
	print_str("[2] - PROGRAMS\n");
	print_str("[3] - ABOUT");
}