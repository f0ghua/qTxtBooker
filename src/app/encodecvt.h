#ifndef _ENCODECVT_H_
#define _ENCODECVT_H_

char *utf8_to_ansi(const char *in);
int read_gzip_file(const char *fname, char *p_plain_buf, int *buflen);
char *qstrreplace(const char *mode, char *srcstr, const char *tokstr, const char *word);
void sbc_to_dbc(char *sbc, char *dbc);

#endif
