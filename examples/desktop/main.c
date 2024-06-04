#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "console.h"

enum {
	CONSOLE_RC_ERR_USER_EXIT = CONSOLE_RC_ERR_USER, // Exit from program
};

static bool console_cmds_user(char* cmd) {
	switch (console_hash(cmd)) {
		case /** + ( x1 x2 - x3) Add values: x3 = x1 + x2. **/ 0XB58E: console_binop(+); break;
		case /** - ( x1 x2 - x3) Subtract values: x3 = x1 - x2. **/ 0XB588: console_binop(-); break;
		case /** NEGATE ( d1 - d2) Negate signed value: d2 = -d1. **/ 0X7A79: console_unop(-); break;
		case /** RAISE ( i - ) Raise value as exception. **/ 0X4069: console_raise((console_rc_t)console_u_pop()); break;
		case /** EXIT ( - ?) Exit console. **/ 0XC745: console_raise(CONSOLE_RC_ERR_USER_EXIT); break;
		case /** # ( - ) Comment, rest of input ignored. **/ 0XB586: console_raise(CONSOLE_RC_STAT_IGN_EOL); break;
		default: return false;
	}
	return true;
}

/* The number & string recognisers must be before any recognisers that lookup using a hash, as numbers & strings
	can have potentially any hash value so could look like commands. */
static const console_recogniser_func RECOGNISERS[] PROGMEM = {
	console_r_number_decimal,
	console_r_number_hex,
	console_r_string,
	console_r_hex_string,
	console_cmds_builtin,
#ifdef CONSOLE_WANT_HELP
	console_cmds_help, 
#endif // CONSOLE_WANT_HELP
	console_cmds_user,
	NULL
};

static void prompt(void) {
	printf("\n>");
}

// Linux requires this to emulate TurboC getch(). Copied from Stackoverflow
#include <termios.h>
#include <unistd.h>

/* reads from keypress, doesn't echo */
static int getch(void) {
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= (tcflag_t)~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}

int main(int argc, char **argv) {
	(void)argc; (void)argv;
	consoleInit(RECOGNISERS);							// Setup console.
	printf("\n\n" 
	      "FConsole Example -- `exit' to quit.");
	prompt();

	while (1) {
		const char c = (char)getch();
		if (CONSOLE_INPUT_NEWLINE_CHAR != c)	// Don't echo newline.
			putchar(c);
		console_rc_t rc = consoleAccept(c);		// Add it to the input buffer.
		if (rc >= CONSOLE_RC_OK) {				// On newline...
			char* cmd;
			fputs(" -> ", stdout); 				// Seperator string for output.
			rc = consoleProcess(consoleAcceptBuffer(), &cmd);	// Process input string from input buffer filled by accept and record error code.
			if (CONSOLE_RC_OK != rc) {			// If all went well then we get an OK status code.
				if (CONSOLE_RC_ERR_USER_EXIT == rc) {
					puts("Bye...");
					break;
				}
				printf("Error in command `%s': %s (%d)", cmd, consoleGetErrorDescription(rc), rc);
			}
			prompt();							// In any case print a newline and prompt ready for the next line of input.
		}
	}
	return 0;
}

void consolePrint(console_small_uint_t opt, console_int_t x) {
	switch (opt & 0x7f) {
		case CONSOLE_PRINT_NEWLINE:		printf("\n"); (void)x; return; 				// No separator.
		default:						(void)x; return;							// Ignore, print nothing.
		case CONSOLE_PRINT_SIGNED:		printf("%ld", x); break;
		case CONSOLE_PRINT_UNSIGNED:	printf("+%lu", (console_uint_t)x); break;
		case CONSOLE_PRINT_HEX:			printf("$%lx", (console_uint_t)x); break;
		case CONSOLE_PRINT_STR_P:		/* Fall through... */
		case CONSOLE_PRINT_STR:			fputs((const char*)x, stdout); break;
		case CONSOLE_PRINT_CHAR:		putchar((char)x); break;
	}
	if (!(opt & CONSOLE_PRINT_NO_SEP))	putchar(' ');								// Print a space.
}

