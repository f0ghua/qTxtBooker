#ifndef _REGEX_HELPER_H_
#define _REGEX_HELPER_H_

#define NSUBEXP         128
#define REG_MAX_MTLEN   (1024*100) // 100K
#define REGEX_MATCH(i) match_patterns[i]

extern char match_patterns[NSUBEXP][REG_MAX_MTLEN];

int regex_match_ERE(const char *str, const char *pattern);

#endif
