#include "str_util.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

BOOL str_compare(char* a, char* b) {
	int i;
	//compare each char
	for(i = 0; a[i] != '\0' && b[i] != '\0'; ++i) {
		if(a[i] != b[i]) {
			return FALSE;
		}
	}
	if(a[i] != b[i]) {
		return FALSE;
	} else {
		return TRUE;
	}
}

//caller is responsible for making sure that b is at least the same size as a
void str_copy(char*a, char* b) {
	int i;
	for(i = 0; a[i] != '\0'; ++i) {
		b[i] = a[i];
	}
	b[i] = a[i];
}

int str_to_int(char* a) {
	int i = 0;
	int result = 0;
	char ASCII0 = '0';
	//check each char
	while ('0' <= a[i] && '9' >= a[i]) {
		result *=10;
		result += a[i] - ASCII0;
		++i;
	}
	return result;
}
