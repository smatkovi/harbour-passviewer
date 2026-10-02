// The parser behind QJsonDocument::fromJson() for the Qt 4 build.
//
// A hand-written recursive descent pass, not QScriptEngine::evaluate(): the
// files it reads (the override table and the printer's own attribute dump)
// come from outside the app, and eval would run whatever is in them.
#include "qt4json.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)

namespace {

class Parser
{
public:
    Parser(const QByteArray& text) : m_s(QString::fromUtf8(text)), m_i(0), m_ok(true) {}

    QVariant parse()
    {
        skipWs();
        QVariant v = parseValue();
        skipWs();
        if(m_i != m_s.length())
        {
            m_ok = false;
        }
        return v;
    }

    bool ok() const { return m_ok; }
    int offset() const { return m_i; }

private:
    void skipWs()
    {
        while(m_i < m_s.length() && m_s.at(m_i).isSpace())
        {
            m_i++;
        }
    }

    bool take(QChar c)
    {
        if(m_i < m_s.length() && m_s.at(m_i) == c)
        {
            m_i++;
            return true;
        }
        return false;
    }

    bool takeWord(const char* word)
    {
        const QString w = QLatin1String(word);
        if(m_s.mid(m_i, w.length()) == w)
        {
            m_i += w.length();
            return true;
        }
        return false;
    }

    QVariant parseValue()
    {
        skipWs();
        if(m_i >= m_s.length())
        {
            m_ok = false;
            return QVariant();
        }
        const QChar c = m_s.at(m_i);
        if(c == QLatin1Char('{'))
        {
            return parseObject();
        }
        if(c == QLatin1Char('['))
        {
            return parseArray();
        }
        if(c == QLatin1Char('"'))
        {
            return parseString();
        }
        if(takeWord("true"))
        {
            return QVariant(true);
        }
        if(takeWord("false"))
        {
            return QVariant(false);
        }
        if(takeWord("null"))
        {
            return QVariant();
        }
        return parseNumber();
    }

    QVariant parseObject()
    {
        QVariantMap map;
        take(QLatin1Char('{'));
        skipWs();
        if(take(QLatin1Char('}')))
        {
            return map;
        }
        while(m_ok)
        {
            skipWs();
            if(m_i >= m_s.length() || m_s.at(m_i) != QLatin1Char('"'))
            {
                m_ok = false;
                break;
            }
            const QString key = parseString().toString();
            skipWs();
            if(!take(QLatin1Char(':')))
            {
                m_ok = false;
                break;
            }
            map.insert(key, parseValue());
            skipWs();
            if(take(QLatin1Char(',')))
            {
                continue;
            }
            if(take(QLatin1Char('}')))
            {
                break;
            }
            m_ok = false;
        }
        return map;
    }

    QVariant parseArray()
    {
        QVariantList list;
        take(QLatin1Char('['));
        skipWs();
        if(take(QLatin1Char(']')))
        {
            return list;
        }
        while(m_ok)
        {
            list.append(parseValue());
            skipWs();
            if(take(QLatin1Char(',')))
            {
                continue;
            }
            if(take(QLatin1Char(']')))
            {
                break;
            }
            m_ok = false;
        }
        return list;
    }

    QVariant parseString()
    {
        QString out;
        take(QLatin1Char('"'));
        while(m_i < m_s.length())
        {
            const QChar c = m_s.at(m_i++);
            if(c == QLatin1Char('"'))
            {
                return out;
            }
            if(c != QLatin1Char('\\'))
            {
                out += c;
                continue;
            }
            if(m_i >= m_s.length())
            {
                break;
            }
            const QChar e = m_s.at(m_i++);
            switch(e.unicode())
            {
            case 'n': out += QLatin1Char('\n'); break;
            case 't': out += QLatin1Char('\t'); break;
            case 'r': out += QLatin1Char('\r'); break;
            case 'b': out += QLatin1Char('\b'); break;
            case 'f': out += QLatin1Char('\f'); break;
            case 'u':
            {
                bool okHex = false;
                const ushort code = m_s.mid(m_i, 4).toUShort(&okHex, 16);
                if(!okHex)
                {
                    m_ok = false;
                    return out;
                }
                m_i += 4;
                out += QChar(code);
                break;
            }
            default: out += e; break;
            }
        }
        m_ok = false;
        return out;
    }

    QVariant parseNumber()
    {
        const int start = m_i;
        bool isDouble = false;
        if(m_i < m_s.length() && (m_s.at(m_i) == QLatin1Char('-') || m_s.at(m_i) == QLatin1Char('+')))
        {
            m_i++;
        }
        while(m_i < m_s.length())
        {
            const QChar c = m_s.at(m_i);
            if(c.isDigit())
            {
                m_i++;
            }
            else if(c == QLatin1Char('.') || c == QLatin1Char('e') || c == QLatin1Char('E') ||
                    c == QLatin1Char('-') || c == QLatin1Char('+'))
            {
                isDouble = true;
                m_i++;
            }
            else
            {
                break;
            }
        }
        const QString text = m_s.mid(start, m_i-start);
        if(text.isEmpty())
        {
            m_ok = false;
            return QVariant();
        }
        bool numberOk = false;
        if(!isDouble)
        {
            const qlonglong ll = text.toLongLong(&numberOk);
            if(numberOk)
            {
                return QVariant(ll);
            }
        }
        const double d = text.toDouble(&numberOk);
        if(!numberOk)
        {
            m_ok = false;
        }
        return QVariant(d);
    }

    QString m_s;
    int m_i;
    bool m_ok;
};

} // namespace

QJsonDocument QJsonDocument::fromJson(const QByteArray& text, QJsonParseError* error)
{
    Parser parser(text);
    const QVariant v = parser.parse();

    if(error)
    {
        error->error = parser.ok() ? QJsonParseError::NoError : QJsonParseError::GarbageAtEnd;
        error->offset = parser.offset();
    }
    if(!parser.ok())
    {
        return QJsonDocument();
    }
    if(v.type() == QVariant::List)
    {
        return QJsonDocument(QJsonArray(v.toList()));
    }
    if(v.type() == QVariant::Map)
    {
        return QJsonDocument(QJsonObject(v.toMap()));
    }
    return QJsonDocument();
}

#endif // QT_VERSION
