#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <QString>
#include <QRegularExpression>
#include "regex_helper.h"

char match_patterns[NSUBEXP][REG_MAX_MTLEN];

int regex_match_ERE(const char *str, const char *pattern)
{
    int r, i;
    QRegularExpression regex;
    QRegularExpressionMatch match;

	for (i = 0; i < NSUBEXP; i++) {
		match_patterns[i][0] = '\0';
	}

    regex.setPattern(pattern);
    match = regex.match(str);
    if (!match.hasMatch()) {
        return -1;
    }

    for (i = 0; i < NSUBEXP; i++) {
        int len = match.captured(i).size();
        len = (len > REG_MAX_MTLEN)?REG_MAX_MTLEN:len;
        strncpy(match_patterns[i], match.captured(i).toStdString().data(), len);
        match_patterns[i][len]='\0';
    }

    return 0;
}
