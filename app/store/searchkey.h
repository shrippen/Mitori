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
        const QChar::Category category = QChar::category(c);
        // Combining marks (accents, variation selectors of emoji) and invisible characters go;
        // the kana voicing marks stay (NFC joins them again: が).
        if ((category == QChar::Mark_NonSpacing && c != 0x3099 && c != 0x309A) || category == QChar::Other_Format
                || isApostrophe(c)) {
            continue;
        }
        if (category == QChar::Mark_Enclosing) {
            key += QChar(u' ');     // a keycap: 1️⃣ is a symbol
            continue;
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

// By code point: a kanji beyond the BMP (𠮟) is two UTF-16 units.
inline bool hasKanji(const QString &text)
{
    const QList<uint> points = text.toUcs4();
    return std::any_of(points.cbegin(), points.cend(), [](uint c) { return QChar::script(char32_t(c)) == QChar::Script_Han; });
}

inline QString withoutHiragana(const QString &text)
{
    QList<uint> points = text.toUcs4();
    points.removeIf([](uint c) { return QChar::script(char32_t(c)) == QChar::Script_Hiragana; });
    return QString::fromUcs4(reinterpret_cast<const char32_t *>(points.constData()), points.size());
}

// A text asked whether it names something (a technique in a note): the name's words joined must be
// a run of whole words of the text, so that spaces and hyphens do not matter ("Shihô-nage" in "shiho
// nage aus" and "Shihonage,") but word ends do (not in "Shihonagen"). With kanji anywhere: Japanese
// has no spaces, and okurigana may differ.
class NamedIn
{
public:
    explicit NamedIn(const QString &text)
        : m_words(searchKey(text).split(u' ', Qt::SkipEmptyParts))
    {
    }

    // Where a name stands: from word `first` to word `last` of the text.
    struct Span {
        qsizetype first = 0;
        qsizetype last = 0;

        bool inside(const Span &other) const
        {
            return other.first <= first && last <= other.last && (other.first < first || last < other.last);
        }
    };

    // Every place of a name; none for a name with kanji (those have no word ends).
    QList<Span> spans(const QString &name) const
    {
        QList<Span> found;
        const QString joined = searchKey(name).remove(u' ');
        if (joined.isEmpty() || hasKanji(joined)) {
            return found;
        }
        for (qsizetype i = 0; i < m_words.size(); ++i) {
            QString run;
            qsizetype j = i;
            for (; j < m_words.size() && run.size() < joined.size(); ++j) {
                run += m_words.at(j);
            }
            if (run == joined) {
                found << Span{i, j - 1};
            }
        }
        return found;
    }

    bool names(const QString &name) const
    {
        const QString joined = searchKey(name).remove(u' ');
        if (joined.isEmpty() || !hasKanji(joined)) {
            return !spans(name).isEmpty();
        }
        const QString bare = withoutHiragana(joined);
        return !bare.isEmpty() && withoutHiragana(m_words.join(QString())).contains(bare);
    }

private:
    QStringList m_words;
};

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
        if (!hasKanji(w)) {
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
