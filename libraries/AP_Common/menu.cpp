// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: t -*-

//
// Simple commandline menu system.
//

#include <FastSerial.h>

#include <ctype.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "include/menu.h"

// statics
char Menu::_inbuf[MENU_COMMANDLINE_MAX];
Menu::arg Menu::_argv[MENU_ARGS_MAX + 1];

// constructor
Menu::Menu(const prog_char *prompt, const Menu::command *commands, uint8_t entries, preprompt ppfunc) :
	_prompt(prompt),
	_commands(commands),
	_entries(entries),
	_ppfunc(ppfunc)
{
}

// run the menu
void
Menu::run(void)
{
	uint8_t		len, i, ret;
	uint8_t		argc;
	int			c;
		
	// loop performing commands
	for (;;) {

		// run the pre-prompt function, if one is defined
		if ((NULL != _ppfunc) && !_ppfunc())
			return;

		// loop reading characters from the input
		len = 0;
		Serial.printf("%S] ", _prompt);
		for (;;) {
			c = Serial.read();
			if (-1 == c)
				continue;
			// carriage return -> process command
			if ('\r' == c) {
				_inbuf[len] = '\0';
				Serial.write('\r');
				Serial.write('\n');
				break;
			}
			// backspace
			if ('\b' == c) {
				if (len > 0) {
					len--;
					Serial.write('\b');
					Serial.write(' ');
					Serial.write('\b');
					continue;
				}
			}
			// printable character
			if (isprint(c) && (len < (MENU_COMMANDLINE_MAX - 1))) {
				_inbuf[len++] = c;
				Serial.write((char)c);
				continue;
			}
		}
		
		// split the input line into tokens
		argc = 0;
		_argv[argc++].str = strtok(_inbuf, " ");
		while (argc <= MENU_ARGS_MAX) {
			_argv[argc].str = strtok(NULL, " ");
			if ('\0' == _argv[argc].str)
				break;
			_argv[argc].i = atol(_argv[argc].str);
			_argv[argc].f = atof(_argv[argc].str);	// calls strtod, > 700B !
			argc++;
		}
			
		// look for a command matching the first word (note that it may be empty)
		for (i = 0; i < _entries; i++) {
			if (!strcmp_P(_argv[0].str, _commands[i].command)) {
				ret = _call(i, argc);
				if (-2 == ret)
					return;
				break;
			}
		}

		// implicit commands
		if (i == _entries) {
			if (!strcmp(_argv[0].str, "?") || (!strcmp_P(_argv[0].str, PSTR("help")))) {
				_help();
			} else if (!strcmp_P(_argv[0].str, PSTR("exit"))) {
				return;
			}
		}
	}			
}

// display the list of commands in response to the 'help' command
void
Menu::_help(void)
{
	int		i;
	
	Serial.println("Commands:");
	for (i = 0; i < _entries; i++)
		Serial.printf("  %S\n", _commands[i].command);
}

// run the n'th command in the menu
int
Menu::_call(uint8_t n, uint8_t argc)
{
	func		fn;

	fn = (func)pgm_read_word(&_commands[n].func);
	return(fn(argc, &_argv[0]));
}
