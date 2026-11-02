#pragma once

#include <QString>

// The form search compares: lower case, without accents (Ô-dachi finds o dachi, größer finds
// grosser), hyphens, slashes and middle dots as spaces, spaces simplified.
inline QString searchKey(const QString &text)
{
    QString key;
    for (const QChar c : text.normalized(QString::NormalizationForm_D)) {
        if (c.category() == QChar::Mark_NonSpacing) {
            continue;
        }
        if (c == u'ß') {
            key += QStringLiteral("ss");
        } else {
            key += (c == u'-' || c == u'/' || c == u'·') ? QChar(u' ') : c.toLower();
        }
    }
    return key.simplified();
}
