#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <zlib.h>

#include "encodecvt.h"

// enc_convert(strA_in,strB_out,CP_UTF8,CP_ACP)			//UTF8 to ANSI
// enc_convert(strA_out, strB_in, CP_ACP, CP_UTF8)		//ANSI to UTF8
void enc_convert(const char *strIn, char *strOut, int sourceCodepage, int targetCodepage)
{
    int len = lstrlen((LPCWSTR)strIn);
    int unicodeLen = MultiByteToWideChar(sourceCodepage, 0, strIn, -1, NULL, 0);

    wchar_t* pUnicode = NULL;
    pUnicode = (wchar_t *)malloc(sizeof(wchar_t)*(unicodeLen + 1));
    //memset(pUnicode, 0, (unicodeLen + 1)*sizeof(wchar_t));
    MultiByteToWideChar(sourceCodepage, 0, strIn, -1, (LPWSTR)pUnicode, unicodeLen);

    BYTE * pTargetData = NULL;
    int targetLen = WideCharToMultiByte(targetCodepage, 0,
		(LPWSTR)pUnicode, -1, (char *)pTargetData, 0, NULL, NULL);

    pTargetData = (BYTE *)malloc(targetLen+1);
    //memset(pTargetData, 0, targetLen + 1);
    WideCharToMultiByte(targetCodepage, 0, (LPWSTR)pUnicode,
		-1, (char *)pTargetData, targetLen, NULL, NULL);
    lstrcpy((LPWSTR)strOut, (LPCWSTR)pTargetData);

    free(pUnicode);
    free(pTargetData);
}

char *utf8_to_ansi(const char *in)
{
    int unicode_len = MultiByteToWideChar(CP_UTF8, 0, in, -1, NULL, 0);
    
    wchar_t *p_unicode = NULL;
    p_unicode = (wchar_t *)malloc(sizeof(wchar_t)*(unicode_len + 1));
    MultiByteToWideChar(CP_UTF8, 0, in, -1, (LPWSTR)p_unicode, unicode_len);

    BYTE *out = NULL;
    int outlen = WideCharToMultiByte(CP_ACP, 0,
		(LPWSTR)p_unicode, -1, (char *)out, 0, NULL, NULL);

    out = (BYTE *)malloc(outlen+1);
    WideCharToMultiByte(CP_ACP, 0, (LPWSTR)p_unicode,
		-1, (char *)out, outlen, NULL, NULL);
    out[outlen] = '\0';
    
    return (char *)out;
}

// use for zlib 1.2.X
int uncompressGzip(unsigned char *pSrc, int srcSize, char **pOutDest, int *pOutBufSize)
{
    unsigned char *pBuf = pSrc + (srcSize - 1);
    uLongf len = *pBuf;
    int r;
    z_stream d_stream;

    //check gz file,rfc1952 P6
    if ((*pSrc !=0x1f)||(*(pSrc+1) != 0x8b)) {
        //ERR(("uncompressGzip non Gzip"));
        return -1;
    }

    for(int i = 0; i < 3; i++){
        pBuf--;
        len <<= 8;
        len += *pBuf;
    }

    //fortest
    if((len == 0) || (len > 1000000)) {
        //ERR(("uncompressGzip,error gzip!"));
        return -1;
    }

    char *pDesBuf = (char *)malloc(len);

    //gzipdecompression start!!!
    d_stream.zalloc = Z_NULL;
    d_stream.zfree = Z_NULL;
    d_stream.opaque = Z_NULL;
    d_stream.next_in = Z_NULL;
    d_stream.avail_in = 0;

    r = inflateInit2(&d_stream, (MAX_WBITS+16));
    if(r != Z_OK) {
        //ERR("inflateInit2 error:%d", r);
        return r;
    }

    d_stream.next_in = pSrc;
    d_stream.avail_in = srcSize;
    d_stream.next_out = (Bytef *)pDesBuf;
    d_stream.avail_out = len;
    r = inflate(&d_stream, Z_NO_FLUSH);

    switch(r) {
    case Z_NEED_DICT:
        r = Z_DATA_ERROR;
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
        (void)inflateEnd(&d_stream);
        return r;
    }
/*
	r = inflateReset2(&d_stream, (MAX_WBITS+16));
    switch(r) {
    case Z_NEED_DICT:
        r = Z_DATA_ERROR;
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
        (void)inflateEnd(&d_stream);
        return r;
    }
*/
    /*
    LOG("outlen= %d, total_in= %d, total_out= %d, avail_out= %d@@@@@@@@@@@\n",
        len, d_stream.total_in, d_stream.total_out, d_stream.avail_out);
    */
    inflateEnd(&d_stream);

    *pOutBufSize = len - 2;
    *pOutDest = pDesBuf;

    return 0;
}

static char *memcat(char *dest, size_t dest_len, const char *src, size_t src_len)
{
	memcpy(dest+dest_len, src, src_len);
	return dest;
}

// use for zlib 1.1.4
int read_gzip_file(const char *fname, char *p_plain_buf, int *buflen)
{
	char buf[1024*10];
	int err, total_len = 0;

	gzFile in = gzopen(fname, "rb");
#ifndef F_NO_DEBUG
    //LOG("gzread opened");
#endif
    for (;;) {
		memset(buf, 0, sizeof(buf));
        int len = gzread(in, buf, sizeof(buf));
#ifndef F_NO_DEBUG
        //LOG("gzread len = %d", len);
#endif
        if (len < 0) {
            //ERR("gzread err: %s", gzerror(in, &err));
			return -1;
		}
        if (len == 0) break;

		memcat(p_plain_buf, total_len, buf, len);
		total_len += len;
    }
	if (gzclose(in) != Z_OK) {
        //ERR("gzclose err: %s", gzerror(in, &err));
        return -1;
	}

    *buflen = total_len;
	return 0;
}

char *zstring_replace_str(char *str, const char *x, const char *y){
    /* to preserve the address of original pointers, tmp_ are used
     * dummy_ptr enables us to preserve the address of tmp_str when
     * a matching string pattern is found
     * */
    char *tmp_str = str;
    const char *tmp_x = x;
    const char *dummy_ptr = tmp_x;
    const char *tmp_y = y;
    int len_str=0, len_y=0, len_x=0;

    /* NULL pointer check */
    if ((*str && *x && *y)==0)
        return 0;

    /* calculating length of strings */
    for(; *tmp_y; ++len_y, ++tmp_y)
        ;

    for(; *tmp_str; ++len_str, ++tmp_str)
        ;

    for(; *tmp_x; ++len_x, ++tmp_x)
        ;

    /* Bounds check */
    if (len_y >= len_str)
        return str;

    /* reset tmp pointers */
    tmp_y = y;
    tmp_x = x;

    for (tmp_str = str ; *tmp_str; ++tmp_str)
        if(*tmp_str == *tmp_x) {
            /* save tmp_str */
            for (dummy_ptr=tmp_str; *dummy_ptr == *tmp_x; ++tmp_x, ++dummy_ptr)
                if (*(tmp_x+1) == '\0' && ((dummy_ptr-str+len_y) < len_str)){
                    /* Reached at the end of x, we got something to replace
                     * then!
                     * Copy y only if there is enough room for it
                     */
                    for(tmp_y=y; *tmp_y; ++tmp_y, ++tmp_str)
                        *tmp_str = *tmp_y;
            }
        /* reset tmp_x */
        tmp_x = x;
        }

    return str;
}

/**
 * Replace string or tokens as word from source string with given mode.
 *
 * @param mode      replacing mode
 * @param srcstr    source string
 * @param tokstr    token or string
 * @param word      target word to be replaced
 *
 * @return a pointer of malloced or source string depending on the mode if
 *         successful, otherwise returns NULL
 *
 * @note
 * The mode argument has two separated characters. First character
 * is used to decide replacing method and can be 't' or 's'.
 * The character 't' and 's' stand on [t]oken and [s]tring.
 *
 * When 't' is given each character of the token string(third argument)
 * will be compared with source string individually. If matched one
 * is found. the character will be replaced with given work.
 *
 * If 's' is given instead of 't'. Token string will be analyzed
 * only one chunk word. So the replacement will be occured when
 * the case of whole word matched.
 *
 * Second character is used to decide returning memory type and
 * can be 'n' or 'r' which are stand on [n]ew and [r]eplace.
 *
 * When 'n' is given the result will be placed into new array so
 * you should free the return string after using. Instead of this,
 * you can also use 'r' character to modify source string directly.
 * In this case, given source string should have enough space. Be
 * sure that untouchable value can not be used for source string.
 *
 * So there are four associatable modes such like below.
 *
 * Mode "tn" : [t]oken replacing & putting the result into [n]ew array.
 * Mode "tr" : [t]oken replacing & [r]eplace source string directly.
 * Mode "sn" : [s]tring replacing & putting the result into [n]ew array.
 * Mode "sr" : [s]tring replacing & [r]eplace source string directly.
 *
 * @code
 *   char srcstr[256], *retstr;
 *   char mode[4][2+1] = {"tn", "tr", "sn", "sr"};
 *
 *   for(i = 0; i < 4; i++) {
 *     strcpy(srcstr, "Welcome to The qDecoder Project.");
 *
 *     printf("before %s : srcstr = %s\n", mode[i], srcstr);
 *     retstr = qstrreplace(mode[i], srcstr, "The", "_");
 *     printf("after  %s : srcstr = %s\n", mode[i], srcstr);
 *     printf("            retstr = %s\n\n", retstr);
 *     if(mode[i][1] == 'n') free(retstr);
 *   }
 *
 *   --[Result]--
 *   before tn : srcstr = Welcome to The qDecoder Project.
 *   after  tn : srcstr = Welcome to The qDecoder Project.
 *               retstr = W_lcom_ _o ___ qD_cod_r Proj_c_.
 *
 *   before tr : srcstr = Welcome to The qDecoder Project.
 *   after  tr : srcstr = W_lcom_ _o ___ qD_cod_r Proj_c_.
 *               retstr = W_lcom_ _o ___ qD_cod_r Proj_c_.
 *
 *   before sn : srcstr = Welcome to The qDecoder Project.
 *   after  sn : srcstr = Welcome to The qDecoder Project.
 *               retstr = Welcome to _ qDecoder Project.
 *
 *   before sr : srcstr = Welcome to The qDecoder Project.
 *   after  sr : srcstr = Welcome to _ qDecoder Project.
 *               retstr = Welcome to _ qDecoder Project.
 * @endcode
 */
char *qstrreplace(const char *mode, char *srcstr, const char *tokstr,
                  const char *word)
{
    if (mode == NULL || strlen(mode) != 2|| srcstr == NULL || tokstr == NULL
    || word == NULL) {
        //DEBUG("Unknown mode \"%s\".", mode);
        return NULL;
    }

    char *newstr, *newp, *srcp, *tokenp, *retp;
    newstr = newp = srcp = tokenp = retp = NULL;

    char method = mode[0], memuse = mode[1];
    int maxstrlen, tokstrlen;

    /* Put replaced string into malloced 'newstr' */
    if (method == 't') { /* Token replace */
        maxstrlen = strlen(srcstr) * ((strlen(word) > 0) ? strlen(word) : 1);
        newstr = (char *) malloc(maxstrlen + 1);

        for (srcp = (char *) srcstr, newp = newstr; *srcp; srcp++) {
            for (tokenp = (char *) tokstr; *tokenp; tokenp++) {
                if (*srcp == *tokenp) {
                    char *wordp;
                    for (wordp = (char *) word; *wordp; wordp++) {
                        *newp++ = *wordp;
                    }
                    break;
                }
            }
            if (!*tokenp)
                *newp++ = *srcp;
        }
        *newp = '\0';
    } else if (method == 's') { /* String replace */
        if (strlen(word) > strlen(tokstr)) {
            maxstrlen = ((strlen(srcstr) / strlen(tokstr)) * strlen(word))
                    + (strlen(srcstr) % strlen(tokstr));
        } else {
            maxstrlen = strlen(srcstr);
        }
        newstr = (char *) malloc(maxstrlen + 1);
        tokstrlen = strlen(tokstr);

        for (srcp = srcstr, newp = newstr; *srcp; srcp++) {
            if (!strncmp(srcp, tokstr, tokstrlen)) {
                char *wordp;
                for (wordp = (char *) word; *wordp; wordp++)
                    *newp++ = *wordp;
                srcp += tokstrlen - 1;
            } else
                *newp++ = *srcp;
        }
        *newp = '\0';
    } else {
        //DEBUG("Unknown mode \"%s\".", mode);
        return NULL;
    }

    /* decide whether newing the memory or replacing into exist one */
    if (memuse == 'n')
        retp = newstr;
    else if (memuse == 'r') {
        strcpy(srcstr, newstr);
        free(newstr);
        retp = srcstr;
    } else {
        //DEBUG("Unknown mode \"%s\".", mode);
        free(newstr);
        return NULL;
    }

    return retp;
}

// change full width to half width (gbk need)
void sbc_to_dbc(char *sbc, char *dbc)
{
    for(; *sbc; ++sbc) {
        if((*sbc & 0xff) == 0xA1 && (*(sbc + 1) & 0xff) == 0xA1) {
            *dbc++ = 0x20;
            ++sbc;
        } else if((*sbc & 0xff) == 0xA3 && 
                  (*(sbc + 1) & 0xff) >= 0xA1 && 
                  (*(sbc + 1) & 0xff) <= 0xFE) {
            *dbc++ = *++sbc - 0x80;
		} else {
            if(*sbc < 0)
                *dbc++ = *sbc++;
            *dbc++ = *sbc;
        }
    }
    *dbc = 0;
}
