#ifndef HTMLENTITYPARSER_H
#define HTMLENTITYPARSER_H

#include <QString>

class HtmlEntityParser
{
public:
    HtmlEntityParser();
    QString parseText(const QString &text);

private:
    QString parseEntity();

    QString txt;
    int pos, len;

};

#endif // HTMLENTITYPARSER_H
