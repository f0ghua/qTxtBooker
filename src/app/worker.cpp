
#include "worker.h"
#include "httpsession.h"
#include "QAppLogging.h"

#include <windows.h>

#include "regex_helper.h"
#include "encodecvt.h"
#include "ini.h"

#define MAX_STRLEN 256

#define CPYMATCH2STR(i, s, l)                       \
    do {                                            \
        int len = prog->endp[i] - prog->startp[i];  \
        len = (len > l)?l:len;                      \
        memcpy(s, prog->startp[i], len);            \
        (s)[len] = '\0';                            \
    } while (0);

typedef struct _page_info {
    char title[MAX_STRLEN];
    char url[MAX_STRLEN];
} page_info_t;

typedef struct _site_info {
    char domain[MAX_STRLEN];
    char title_pattern[MAX_STRLEN];
    char link_str_start[MAX_STRLEN];
    char link_str_end[MAX_STRLEN];
    char link_pattern[MAX_STRLEN];
    char content_pattern[MAX_STRLEN];
    int force_utf8;
} site_info_t;

typedef struct _book_info {
    char url[MAX_STRLEN];
    int pages;
    page_info_t pi[1024*5]; // how many pages in a book?
} book_info_t;

typedef struct _th_info {
    book_info_t bi;
    site_info_t si;
    int start;
    int end;
    HWND hwnd_pgbar;
    HWND hwnd_pagenum;
    HWND hwnd_log;
} th_info_t;

book_info_t *g_pbi;
site_info_t *g_psi;

const char g_config_fname[] 	= "./config.ini";
const char g_index_fname[] 		= "./tmp.html";
const char g_page_fname[] 		= "./page.html";

const char *g_index_url 		= "https://www.dawenxue.net/937/";
const char *g_pattern 			= "^[ \t]*<dd><a href=\"([^\"]*)\">([^<]*)<.*";
const char *g_title_pattern 	= "";
const char *g_link_str_start 	= "<div id=\"list\">";
const char *g_link_str_end 		= "</div>";
const char *g_link_pattern 		= "<dd><a href=\"([^\"]*)\">([^<]*)</a></dd>";
const char *g_content_pattern 	= "";

/*
chuangshi.qq.com

const char *g_link_str_start 	= "<div class=\"index_area\">";
const char *g_link_str_end 		= "<div class=\"footer\">";
const char *g_link_pattern 		= "<li><a href=\"([^\"]*)\"[^>]*><b class=\"title\">([^<]*)</b></a></li>";
*/

static int load_config(const char *url)
{
    ini_t *config;
    char domain[MAX_STRLEN];

    int r = regex_match_ERE(url, "https?://([^/]*).*");
    if (r != 0) {
        QLOG_ERROR() << "URL link is wrong";
        return -1;
    }

    strncpy(domain, REGEX_MATCH(1), sizeof(domain));
#ifndef F_NO_DEBUG
    QLOG_DEBUG() << "domain = " << domain;
#endif
    config = ini_load(g_config_fname);
    if (config == NULL) {
        QLOG_ERROR() << "load config file error";
        return -1;
    }

    //const char *p_tpattern = ini_get(config, domain, "title_pattern");
    const char *p_cpattern = ini_get(config, domain, "content_pattern");
    const char *p_lstart = ini_get(config, domain, "link_str_start");
    const char *p_lend = ini_get(config, domain, "link_str_end");
    const char *p_lpattern = ini_get(config, domain, "link_pattern");
    const char *p_lforceutf8 = ini_get(config, domain, "force_utf8");

    if ((p_lstart == NULL)||(p_lend == NULL)||(p_lpattern == NULL)||
        (p_cpattern == NULL)) {
        QLOG_ERROR() << "The site has not supported, please contact the author.";
        return -1;
    }

    //strncpy(g_psi->title_pattern, g_title_pattern, sizeof(g_psi->title_pattern));
    strncpy(g_psi->link_str_start, p_lstart, sizeof(g_psi->link_str_start));
    strncpy(g_psi->link_str_end, p_lend, sizeof(g_psi->link_str_end));
    strncpy(g_psi->link_pattern, p_lpattern, sizeof(g_psi->link_pattern));
    strncpy(g_psi->content_pattern, p_cpattern, sizeof(g_psi->content_pattern));

    if (p_lforceutf8 != NULL) {
        g_psi->force_utf8 = 1;
    } else {
        g_psi->force_utf8 = 0;
    }

    ini_free(config);

    return 0;
}

// function which call this subroutine should use free to free the buffer
// later
static char *read_file_all(const char *fname, int *bufsize)
{
    char *p_content;
    char *p_ansiOut;
    FILE *fp;
    size_t len = 0;
    int size, plbuf_len;

    *bufsize = 0;

    fp = fopen(fname, "rb");
    if (fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            size = ftell(fp);
            if (size == -1) { /* Error */
                fclose(fp);
                return NULL;
            }
            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) {
                /* Handle error here */
                fclose(fp);
                return NULL;
            }
            /* Allocate our buffer to that size. */
            p_content = (char *)malloc(sizeof(char) * (size + 1));
            /* Read the entire file into memory. */
            len = fread(p_content, sizeof(char), size, fp);
#ifndef F_NO_DEBUG
            QLOG_DEBUG() << QObject::tr("read len = %1, size = %2").arg(len).arg(size);
#endif
            if (len == 0) {
                QLOG_ERROR() << "Error reading file" << stderr;
                fclose(fp);
                free(p_content);
                return NULL;
            } else {
                p_content[++len] = '\0'; /* Just to be safe. */
            }
        }

        // gzip can be >2*len, but 4*len should enough
        plbuf_len = sizeof(char) * (size + 1) * 4;
        if (((unsigned char)(*p_content) == 0x1F) &&
            ((unsigned char)*(p_content+1) == 0x8B)) { // gzip file
#ifndef F_NO_DEBUG
            QLOG_DEBUG() << ("gzip file detected, uncompress it");
#endif
            free(p_content);
            p_content = (char *)malloc(plbuf_len);
            read_gzip_file(fname, p_content, &plbuf_len);
#ifndef F_NO_DEBUG
            QLOG_DEBUG() << QObject::tr("gzip file %1 -> %2").arg(size).arg(plbuf_len);
#endif
        }

        if (g_psi->force_utf8 ||
            (((unsigned char)(*p_content) == 0xEF) &&
             ((unsigned char)*(p_content+1) == 0xBB) &&
             ((unsigned char)*(p_content+2) == 0xBF))) { // UTF8
            // convert UTF8 to GB2312(ASCII)
#ifndef F_NO_DEBUG
            QLOG_DEBUG() << "file is UTF8, convert to ASCII";
#endif
            p_ansiOut = utf8_to_ansi(p_content);
            free(p_content);
            p_content = p_ansiOut;
        }

        fclose(fp);
    }

    return p_content;
}

Worker::Worker(QObject *parent) : QObject(parent)
{

}

void Worker::run()
{
    int bufsiz;
    char *p_content;
    char *p_ansiOut;

    g_pbi = (book_info_t *)malloc(sizeof(*g_pbi));
    g_psi = (site_info_t *)malloc(sizeof(*g_psi));

    QString url = "https://www.mywenxue.com/xiaoshuo/127/127690/Index.htm";
    if (load_config(url.toStdString().data()) != 0) {
        return;
    }

    m_session = new HttpSession(this);
    m_session->requestUrl2File(QUrl(url), g_index_fname);

    memset(g_pbi, 0, sizeof(*g_pbi));
    g_pbi->pages = 0;

    p_content = read_file_all(g_index_fname, &bufsiz);
    if (p_content == NULL) {
        return;
    }

    p_ansiOut = p_content;

    if ((p_lstart = strstr(p_ansiOut, g_psi->link_str_start)) == NULL) {
        QLOG_ERROR() << ("link start is not found");
        return -1;
    }

    if ((p_lend = strstr(p_lstart, g_psi->link_str_end)) == NULL) {
        QLOG_ERROR() << ("link start is not found");
        return -1;
    }

    snprintf(pattern, sizeof(pattern), "^%s", g_psi->link_pattern);

    p = p_lstart;
#ifndef F_NO_DEBUG
    //LOG("p = %s", p);
#endif
    while (p < p_lend) {
        if ((*p == *g_psi->link_pattern) &&
            ((r = regex_match_ERE(p, pattern)) != -1)) { // match
            char stmp[MAX_STRLEN];

            snprintf(g_pbi->pi[g_pbi->pages].url,
                     sizeof(g_pbi->pi[g_pbi->pages].url), "%s%s",
                     url, REGEX_MATCH(1));

            strncpy(g_pbi->pi[g_pbi->pages].title,
                    REGEX_MATCH(2), sizeof(g_pbi->pi[g_pbi->pages].title));
            g_pbi->pages++;

            p += prog->endp[0] - prog->startp[0];

            CPYMATCH2STR(0, stmp, MAX_STRLEN);
#ifndef F_NO_DEBUG
            //LOG("prog0 = %s", stmp);
            //LOG("match p = %x, %x, %x, title = %s", p, prog->startp[0], prog->endp[0], stmp);
#endif
        } else {
            p++;
            //LOG("no match p = %x", p);
        }
    }

    free(p_content);
}
