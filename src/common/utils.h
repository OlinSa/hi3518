#ifndef INCLUDE_UTILS_H
#define INCLUDE_UTILS_H

#include <stdint.h>
#ifdef DEBUG
	#include <assert.h>
#endif
#ifdef DEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)   \
	do{             \
	}while(0);
#endif

#define MIN(a,b) (a) > (b) ? (b) : (a)
#define ALIGN(n,akign) (((n)+align-1) & ~((align)-1))

#ifdef __cplusplus

	#define RESET   "\033[0m"
	#define BLACK   "\033[30m"      /* Black */
	#define RED     "\033[31m"      /* Red */
	#define GREEN   "\033[32m"      /* Green */
	#define YELLOW  "\033[33m"      /* Yellow */
	#define BLUE    "\033[34m"      /* Blue */
	#define MAGENTA "\033[35m"      /* Magenta */
	#define CYAN    "\033[36m"      /* Cyan */
	#define WHITE   "\033[37m"      /* White */
	#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
	#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
	#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
	#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
	#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
	#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
	#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
	#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

#else

	#define NONE                 "\e[0m"
	#define BLACK                "\e[0;30m"
	#define L_BLACK              "\e[1;30m"
	#define RED                  "\e[0;31m"
	#define L_RED                "\e[1;31m"
	#define GREEN                "\e[0;32m"
	#define L_GREEN              "\e[1;32m"
	#define BROWN                "\e[0;33m"
	#define YELLOW               "\e[1;33m"
	#define BLUE                 "\e[0;34m"
	#define L_BLUE               "\e[1;34m"
	#define PURPLE               "\e[0;35m"
	#define L_PURPLE             "\e[1;35m"
	#define CYAN                 "\e[0;36m"
	#define L_CYAN               "\e[1;36m"
	#define GRAY                 "\e[0;37m"
	#define WHITE                "\e[1;37m"

	#define BOLD                 "\e[1m"
	#define UNDERLINE            "\e[4m"
	#define BLINK                "\e[5m"
	#define REVERSE              "\e[7m"
	#define HIDE                 "\e[8m"
	#define CLEAR                "\e[2J"
	#define CLRLINE              "\r\e[K" //or "\e[1K\r"

#endif


#endif
