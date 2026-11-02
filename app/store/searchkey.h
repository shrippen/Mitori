#pragma once

#include <QString>
#include <QList>
#include <QStringList>

#include <algorithm>

inline bool isDash(char32_t c)
{
    return c == u'-' || c == u'‐' || c == u'‑' || c == u'‒' || c == u'–' || c == u'—' || c == u'―' || c == u'⸺' || c == u'−';
}

inline bool isApostrophe(char32_t c)
{
    return c == u'\'' || c == u'’' || c == u'‘' || c == u'ʼ' || c == u'ʻ' || c == u'´' || c == u'`';
}

// The form search compares: lower case, without accents (Ô-dachi finds o dachi, größer finds
// grosser; kana keep their dakuten), full-width letters folded, apostrophes and invisible
// characters (soft hyphen) left out (San'nin as sannin), dashes, other punctuation, symbols and
// emoji as spaces ("(Shiho-nage)," starts and ends words) except a point or comma between digits
// (3,5), spaces simplified. `keepHyphens` writes dashes as "-" instead. By code point, so that an
// emoji beyond the BMP counts as a symbol.
inline QString searchKey(const QString &text, bool keepHyphens = false)
{
    // Apostrophes go before the decomposition, which makes ´ a space and an accent.
    QString plain = text;
    plain.removeIf([](QChar c) { return isApostrophe(c.unicode()); });
    const QList<uint> points = plain.normalized(QString::NormalizationForm_KD).toUcs4();
    const auto digit = [&points](qsizetype i) {
        return i >= 0 && i < points.size() && QChar::isDigit(char32_t(points[i]));
    };
    QString key;
    for (qsizetype i = 0; i < points.size(); ++i) {
        const char32_t c = points[i];
        if ((c >= 0x0300 && c <= 0x036F) || QChar::category(c) == QChar::Other_Format) {
            continue;   // combining accents of Latin letters, invisible characters
        }
        const char32_t lower = QChar::toLower(c);
        if (lower == u'ß') {
            key += QStringLiteral("ss");
        } else if (isDash(c)) {
            key += keepHyphens ? QChar(u'-') : QChar(u' ');
        } else if ((c == u'.' || c == u',') && digit(i - 1) && digit(i + 1)) {
            key += QChar(c);
        } else if (QChar::isPunct(c) || QChar::isSymbol(c)) {
            key += QChar(u' ');
        } else {
            key += QString::fromUcs4(&lower, 1);
        }
    }
    return key.simplified().normalized(QString::NormalizationForm_C);   // kana with dakuten as one
}

// The words of a query; a word may keep its hyphens (shiho-nage, see SearchText).
inline QStringList searchWords(const QString &query)
{
    QStringList words;
    for (const QString &w : searchKey(query, true).split(u' ', Qt::SkipEmptyParts)) {
        if (!QString(w).remove(u'-').isEmpty()) {
            words << w;
        }
    }
    return words;
}

inline bool isKanji(QChar c)
{
    return c.script() == QChar::Script_Han;
}

inline QString withoutHiragana(QString text)
{
    text.removeIf([](QChar c) { return c.script() == QChar::Script_Hiragana; });
    return text;
}

// A text to search (a technique's fields, a note): whether every word of a query occurs in it.
// A word matches inside a word, or joined from the start of a word across the following
// hyphens and spaces: across a hyphen it may end anywhere (shihon finds Shiho-nage while
// typing), across a space only where a word ends (nurein finds "nur ein", mitte not "mit
// Tenkan"). A hyphenated word matches joined, or each part from the start of a word
// (ikkyo-irimi as ikkyo irimi; ai-hanmi not in Kaiten-nage). A word with kanji leaves the
// okurigana out on both sides (切り落とし finds 切り落し).
class SearchText
{
public:
    explicit SearchText(const QString &text)
    {
        const QString key = searchKey(text, true);
        m_key = QString(key).replace(u'-', u' ');
        QChar before = u' ';
        for (const QChar c : key) {
            if (c == u' ' || c == u'-') {
                if (!m_joined.isEmpty()) {
                    m_ends << m_joined.size();
                    if (c == u' ') {
                        m_spaces << m_joined.size();
                    }
                }
            } else {
                if (before == u' ' || before == u'-') {
                    m_starts << m_joined.size();
                }
                m_joined += c;
            }
            before = c;
        }
        m_ends << m_joined.size();
    }

    bool matches(const QStringList &words) const
    {
        return std::all_of(words.cbegin(), words.cend(), [this](const QString &w) {
            if (!w.contains(u'-')) {
                return m_key.contains(w) || fromWordStart(w);
            }
            if (fromWordStart(QString(w).remove(u'-'))) {
                return true;
            }
            const QStringList parts = w.split(u'-', Qt::SkipEmptyParts);
            return std::all_of(parts.cbegin(), parts.cend(), [this](const QString &p) { return fromWordStart(p); });
        });
    }

private:
    bool fromWordStart(const QString &w) const
    {
        const QStringView joined(m_joined);
        if (joined.contains(w)) {
            for (const qsizetype at : m_starts) {
                if (!joined.sliced(at).startsWith(w)) {
                    continue;
                }
                const qsizetype end = at + w.size();
                const auto space = std::upper_bound(m_spaces.cbegin(), m_spaces.cend(), at);
                const bool acrossSpace = space != m_spaces.cend() && *space < end;
                if (!acrossSpace || std::binary_search(m_ends.cbegin(), m_ends.cend(), end)) {
                    return true;
                }
            }
        }
        if (!std::any_of(w.cbegin(), w.cend(), isKanji)) {
            return false;
        }
        if (m_kanji.isNull()) {
            m_kanji = withoutHiragana(m_joined);
        }
        const QString bare = withoutHiragana(w);
        return !bare.isEmpty() && m_kanji.contains(bare);
    }

    QString m_key;                  // as searchKey gives it
    QString m_joined;               // without spaces and hyphens
    QList<qsizetype> m_starts;      // where words begin in m_joined
    QList<qsizetype> m_ends;        // where words end
    QList<qsizetype> m_spaces;      // where a space (not a hyphen) was
    mutable QString m_kanji;
};
