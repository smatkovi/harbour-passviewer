// Qt 5's JSON classes for Qt 4.7, backed by QVariant.
//
// SeaPrint keeps every IPP attribute it ever sees in a QJsonObject and hands
// those objects straight to QML, so the port needs the whole shape of the API
// and not just a parser: nested objects, assignment through operator[],
// iterators with key()/value(), take/remove/find.
//
// QVariantMap/QVariantList are the storage, which has one big advantage over
// a private tree: QtDeclarative already turns them into plain JavaScript
// objects, so qml/ can keep reading printer.attrs["media-supported"] exactly
// as it does on Sailfish.
//
// Only what SeaPrint uses is implemented, but that is nearly all of it.
#ifndef QT4JSON_H
#define QT4JSON_H

#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <initializer_list>
#include <type_traits>

class QJsonArray;
class QJsonObject;
class QJsonValueRef;

class QJsonValue
{
public:
    enum Type { Null = 0x0, Bool = 0x1, Double = 0x2, String = 0x3,
                Array = 0x4, Object = 0x5, Undefined = 0x80 };

    QJsonValue() {}
    QJsonValue(const QVariant& v) : m_v(v) {}
    QJsonValue(const QString& v) : m_v(v) {}
    QJsonValue(const char* v) : m_v(QString::fromUtf8(v)) {}
    QJsonValue(const QLatin1String& v) : m_v(QString(v)) {}
    QJsonValue(const std::string& v) : m_v(QString::fromStdString(v)) {}
    inline QJsonValue(const QJsonArray& v);
    inline QJsonValue(const QJsonObject& v);
    inline QJsonValue(const QJsonValueRef& v);

    // One template instead of the bool/int/qint64/double overloads Qt 5 has:
    // the call sites pass quint8 tags and enum values, which would be an
    // ambiguous conversion between those overloads.
    template <class T>
    QJsonValue(T v, typename std::enable_if<std::is_arithmetic<T>::value ||
                                            std::is_enum<T>::value>::type* = 0)
    {
        if(std::is_same<T, bool>::value)
        {
            m_v = QVariant(bool(v));
        }
        else if(std::is_floating_point<T>::value)
        {
            m_v = QVariant(double(v));
        }
        else
        {
            m_v = QVariant(qlonglong(v));
        }
    }

    Type type() const
    {
        if(!m_v.isValid())
        {
            return Undefined;
        }
        switch(m_v.type())
        {
            case QVariant::Bool: return Bool;
            case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
            case QVariant::ULongLong: case QVariant::Double: return Double;
            case QVariant::String: return String;
            case QVariant::List: case QVariant::StringList: return Array;
            case QVariant::Map: return Object;
            default: return m_v.isNull() ? Null : String;
        }
    }

    bool isUndefined() const { return !m_v.isValid(); }
    bool isNull() const { return !m_v.isValid() || m_v.isNull(); }
    bool isBool() const { return m_v.type() == QVariant::Bool; }
    bool isDouble() const { return type() == Double; }
    bool isString() const { return m_v.type() == QVariant::String; }
    bool isArray() const { return type() == Array; }
    bool isObject() const { return m_v.type() == QVariant::Map; }

    QString toString(const QString& def = QString()) const
    {
        return m_v.isValid() && m_v.canConvert(QVariant::String) ? m_v.toString() : def;
    }
    int toInt(int def = 0) const { return m_v.isValid() ? m_v.toInt() : def; }
    qint64 toInteger(qint64 def = 0) const { return m_v.isValid() ? m_v.toLongLong() : def; }
    double toDouble(double def = 0) const { return m_v.isValid() ? m_v.toDouble() : def; }
    bool toBool(bool def = false) const { return m_v.isValid() ? m_v.toBool() : def; }
    QVariant toVariant() const { return m_v; }

    inline QJsonObject toObject() const;
    inline QJsonArray toArray() const;

    bool operator==(const QJsonValue& other) const
    {
        Type t = type(), ot = other.type();
        if(t == Double && ot == Double)
        {
            return toDouble() == other.toDouble();
        }
        if(t != ot)
        {
            return false;
        }
        return m_v == other.m_v;
    }
    bool operator!=(const QJsonValue& other) const { return !(*this == other); }

private:
    QVariant m_v;
};

// A writable slot inside an object or an array. Qt 5 returns one of these from
// the non-const operator[], which is what makes obj["value"] = x work.
class QJsonValueRef
{
public:
    QJsonValueRef(QVariantMap* map, const QString& key) : m_map(map), m_list(0), m_key(key), m_index(-1) {}
    QJsonValueRef(QVariantList* list, int index) : m_map(0), m_list(list), m_index(index) {}

    QJsonValue value() const
    {
        if(m_map)
        {
            return QJsonValue(m_map->value(m_key));
        }
        return QJsonValue(m_list->value(m_index));
    }
    operator QJsonValue() const { return value(); }

    QJsonValueRef& operator=(const QJsonValue& v)
    {
        if(m_map)
        {
            m_map->insert(m_key, v.toVariant());
        }
        else
        {
            (*m_list)[m_index] = v.toVariant();
        }
        return *this;
    }
    QJsonValueRef& operator=(const QJsonValueRef& other) { return operator=(other.value()); }

    QJsonValue::Type type() const { return value().type(); }
    bool isUndefined() const { return value().isUndefined(); }
    bool isNull() const { return value().isNull(); }
    bool isBool() const { return value().isBool(); }
    bool isDouble() const { return value().isDouble(); }
    bool isString() const { return value().isString(); }
    bool isArray() const { return value().isArray(); }
    bool isObject() const { return value().isObject(); }
    QString toString(const QString& def = QString()) const { return value().toString(def); }
    int toInt(int def = 0) const { return value().toInt(def); }
    double toDouble(double def = 0) const { return value().toDouble(def); }
    bool toBool(bool def = false) const { return value().toBool(def); }
    QVariant toVariant() const { return value().toVariant(); }
    inline QJsonObject toObject() const;
    inline QJsonArray toArray() const;

    bool operator==(const QJsonValue& other) const { return value() == other; }
    bool operator!=(const QJsonValue& other) const { return value() != other; }

private:
    QVariantMap* m_map;
    QVariantList* m_list;
    QString m_key;
    int m_index;
};

class QJsonArray
{
public:
    QJsonArray() {}
    explicit QJsonArray(const QVariantList& list) : m_list(list) {}
    QJsonArray(std::initializer_list<QJsonValue> args)
    {
        for(const QJsonValue& v : args)
        {
            m_list.append(v.toVariant());
        }
    }

    static QJsonArray fromStringList(const QStringList& list)
    {
        QJsonArray a;
        for(const QString& s : list)
        {
            a.append(QJsonValue(s));
        }
        return a;
    }
    static QJsonArray fromVariantList(const QVariantList& list) { return QJsonArray(list); }
    QVariantList toVariantList() const { return m_list; }

    int size() const { return m_list.size(); }
    int count() const { return m_list.size(); }
    bool isEmpty() const { return m_list.isEmpty(); }
    bool empty() const { return m_list.isEmpty(); }

    QJsonValue at(int i) const { return QJsonValue(m_list.value(i)); }
    QJsonValue first() const { return at(0); }
    QJsonValue last() const { return at(m_list.size()-1); }
    QJsonValue operator[](int i) const { return at(i); }
    QJsonValueRef operator[](int i) { return QJsonValueRef(&m_list, i); }

    void append(const QJsonValue& v) { m_list.append(v.toVariant()); }
    // Qt 5 kennt auch die Schiebeform; dieser Port braucht sie, weil die
    // Anfragen so gebaut werden: locs << loc << loc.
    QJsonArray& operator<<(const QJsonValue& v) { append(v); return *this; }
    void prepend(const QJsonValue& v) { m_list.prepend(v.toVariant()); }
    void insert(int i, const QJsonValue& v) { m_list.insert(i, v.toVariant()); }
    void removeAt(int i) { m_list.removeAt(i); }
    void replace(int i, const QJsonValue& v) { m_list.replace(i, v.toVariant()); }
    void pop_front() { m_list.removeFirst(); }
    void pop_back() { m_list.removeLast(); }
    void push_back(const QJsonValue& v) { append(v); }
    QJsonValue takeAt(int i) { return QJsonValue(m_list.takeAt(i)); }
    bool contains(const QJsonValue& v) const { return m_list.contains(v.toVariant()); }

    class const_iterator
    {
    public:
        const_iterator(const QVariantList* list = 0, int i = 0) : m_list(list), m_i(i) {}
        QJsonValue operator*() const { return QJsonValue(m_list->value(m_i)); }
        // Chained operator->: the proxy holds the value the caller reaches
        // through it, so attrs.begin()->toObject() stays spelled that way.
        struct Proxy
        {
            QJsonValue v;
            const QJsonValue* operator->() const { return &v; }
        };
        Proxy operator->() const { return Proxy{QJsonValue(m_list->value(m_i))}; }
        const_iterator& operator++() { ++m_i; return *this; }
        const_iterator operator++(int) { const_iterator t = *this; ++m_i; return t; }
        bool operator==(const const_iterator& o) const { return m_i == o.m_i; }
        bool operator!=(const const_iterator& o) const { return m_i != o.m_i; }
    protected:
        const QVariantList* m_list;
        int m_i;
    };

    class iterator : public const_iterator
    {
    public:
        iterator(QVariantList* list = 0, int i = 0) : const_iterator(list, i), m_mlist(list) {}
        QJsonValueRef operator*() const { return QJsonValueRef(m_mlist, m_i); }
    private:
        QVariantList* m_mlist;
    };

    iterator begin() { return iterator(&m_list, 0); }
    iterator end() { return iterator(&m_list, m_list.size()); }
    const_iterator begin() const { return const_iterator(&m_list, 0); }
    const_iterator end() const { return const_iterator(&m_list, m_list.size()); }
    const_iterator constBegin() const { return begin(); }
    const_iterator constEnd() const { return end(); }

    bool operator==(const QJsonArray& other) const { return m_list == other.m_list; }
    bool operator!=(const QJsonArray& other) const { return m_list != other.m_list; }

private:
    QVariantList m_list;
};

class QJsonObject
{
public:
    QJsonObject() {}
    explicit QJsonObject(const QVariantMap& map) : m_map(map) {}
    QJsonObject(std::initializer_list<QPair<QString, QJsonValue> > args)
    {
        for(const QPair<QString, QJsonValue>& kv : args)
        {
            m_map.insert(kv.first, kv.second.toVariant());
        }
    }

    static QJsonObject fromVariantMap(const QVariantMap& map) { return QJsonObject(map); }
    QVariantMap toVariantMap() const { return m_map; }

    int size() const { return m_map.size(); }
    int count() const { return m_map.size(); }
    bool isEmpty() const { return m_map.isEmpty(); }
    bool empty() const { return m_map.isEmpty(); }
    bool contains(const QString& key) const { return m_map.contains(key); }
    QStringList keys() const { return m_map.keys(); }

    QJsonValue value(const QString& key) const { return QJsonValue(m_map.value(key)); }
    QJsonValue operator[](const QString& key) const { return value(key); }
    QJsonValueRef operator[](const QString& key) { return QJsonValueRef(&m_map, key); }

    void insert(const QString& key, const QJsonValue& v) { m_map.insert(key, v.toVariant()); }
    void remove(const QString& key) { m_map.remove(key); }
    QJsonValue take(const QString& key)
    {
        return m_map.contains(key) ? QJsonValue(m_map.take(key)) : QJsonValue();
    }

    class const_iterator
    {
    public:
        const_iterator() {}
        const_iterator(QVariantMap::const_iterator it) : m_it(it) {}
        QString key() const { return m_it.key(); }
        QJsonValue value() const { return QJsonValue(m_it.value()); }
        QJsonValue operator*() const { return value(); }
        const_iterator& operator++() { ++m_it; return *this; }
        const_iterator operator++(int) { const_iterator t = *this; ++m_it; return t; }
        bool operator==(const const_iterator& o) const { return m_it == o.m_it; }
        bool operator!=(const const_iterator& o) const { return m_it != o.m_it; }
    protected:
        QVariantMap::const_iterator m_it;
    };

    class iterator
    {
    public:
        iterator() : m_map(0) {}
        iterator(QVariantMap* map, QVariantMap::iterator it) : m_map(map), m_it(it) {}
        QString key() const { return m_it.key(); }
        QJsonValueRef value() const { return QJsonValueRef(m_map, m_it.key()); }
        QJsonValueRef operator*() const { return value(); }
        iterator& operator++() { ++m_it; return *this; }
        iterator operator++(int) { iterator t = *this; ++m_it; return t; }
        bool operator==(const iterator& o) const { return m_it == o.m_it; }
        bool operator!=(const iterator& o) const { return m_it != o.m_it; }
    private:
        QVariantMap* m_map;
        QVariantMap::iterator m_it;
    };

    iterator begin() { return iterator(&m_map, m_map.begin()); }
    iterator end() { return iterator(&m_map, m_map.end()); }
    const_iterator begin() const { return const_iterator(m_map.constBegin()); }
    const_iterator end() const { return const_iterator(m_map.constEnd()); }
    const_iterator constBegin() const { return begin(); }
    const_iterator constEnd() const { return end(); }
    iterator find(const QString& key) { return iterator(&m_map, m_map.find(key)); }
    const_iterator find(const QString& key) const { return const_iterator(m_map.constFind(key)); }
    const_iterator constFind(const QString& key) const { return find(key); }

    bool operator==(const QJsonObject& other) const { return m_map == other.m_map; }
    bool operator!=(const QJsonObject& other) const { return m_map != other.m_map; }

private:
    QVariantMap m_map;
};

inline QJsonValue::QJsonValue(const QJsonArray& v) : m_v(v.toVariantList()) {}
inline QJsonValue::QJsonValue(const QJsonObject& v) : m_v(v.toVariantMap()) {}
inline QJsonValue::QJsonValue(const QJsonValueRef& v) : m_v(v.toVariant()) {}
inline QJsonObject QJsonValue::toObject() const { return QJsonObject(m_v.toMap()); }
inline QJsonArray QJsonValue::toArray() const { return QJsonArray(m_v.toList()); }
inline QJsonObject QJsonValueRef::toObject() const { return value().toObject(); }
inline QJsonArray QJsonValueRef::toArray() const { return value().toArray(); }

Q_DECLARE_METATYPE(QJsonValue)
Q_DECLARE_METATYPE(QJsonObject)
Q_DECLARE_METATYPE(QJsonArray)

class QJsonParseError
{
public:
    enum ParseError { NoError = 0, GarbageAtEnd = 9 };
    QJsonParseError() : error(NoError), offset(0) {}
    ParseError error;
    int offset;
    QString errorString() const
    {
        return error == NoError ? QLatin1String("no error") : QLatin1String("invalid json");
    }
};

class QJsonDocument
{
public:
    enum JsonFormat { Indented, Compact };

    QJsonDocument() : m_isObject(false), m_isArray(false) {}
    explicit QJsonDocument(const QJsonObject& object) : m_object(object), m_isObject(true), m_isArray(false) {}
    explicit QJsonDocument(const QJsonArray& array) : m_array(array), m_isObject(false), m_isArray(true) {}

    bool isNull() const { return !m_isObject && !m_isArray; }
    bool isEmpty() const { return isNull(); }
    bool isObject() const { return m_isObject; }
    bool isArray() const { return m_isArray; }
    QJsonObject object() const { return m_object; }
    QJsonArray array() const { return m_array; }
    void setObject(const QJsonObject& object) { m_object = object; m_isObject = true; m_isArray = false; }
    void setArray(const QJsonArray& array) { m_array = array; m_isArray = true; m_isObject = false; }

    QByteArray toJson(JsonFormat format = Indented) const
    {
        QString out;
        if(m_isArray)
        {
            writeValue(out, QJsonValue(m_array), format, 0);
        }
        else
        {
            writeValue(out, QJsonValue(m_object), format, 0);
        }
        return out.toUtf8();
    }

    static QJsonDocument fromJson(const QByteArray& text, QJsonParseError* error = 0);
    static QJsonDocument fromVariant(const QVariant& v)
    {
        if(v.type() == QVariant::List)
        {
            return QJsonDocument(QJsonArray(v.toList()));
        }
        return QJsonDocument(QJsonObject(v.toMap()));
    }

private:
    static void writeString(QString& out, const QString& s)
    {
        out += QLatin1Char('"');
        for(int i = 0; i < s.length(); i++)
        {
            const QChar c = s.at(i);
            if(c == QLatin1Char('"') || c == QLatin1Char('\\'))
            {
                out += QLatin1Char('\\');
                out += c;
            }
            else if(c == QLatin1Char('\n'))
            {
                out += QLatin1String("\\n");
            }
            else if(c == QLatin1Char('\t'))
            {
                out += QLatin1String("\\t");
            }
            else if(c.unicode() < 0x20)
            {
                out += QString("\\u%1").arg(c.unicode(), 4, 16, QLatin1Char('0'));
            }
            else
            {
                out += c;
            }
        }
        out += QLatin1Char('"');
    }

    static void writeValue(QString& out, const QJsonValue& v, JsonFormat format, int depth)
    {
        const QString nl = format == Indented ? QLatin1String("\n") : QLatin1String("");
        const QString pad = format == Indented ? QString(4*(depth+1), QLatin1Char(' ')) : QString();
        const QString padEnd = format == Indented ? QString(4*depth, QLatin1Char(' ')) : QString();

        switch(v.type())
        {
        case QJsonValue::Object:
        {
            QJsonObject o = v.toObject();
            if(o.isEmpty())
            {
                out += QLatin1String("{}");
                return;
            }
            out += QLatin1Char('{') + nl;
            QStringList keys = o.keys();
            for(int i = 0; i < keys.size(); i++)
            {
                out += pad;
                writeString(out, keys.at(i));
                out += QLatin1Char(':');
                if(format == Indented)
                {
                    out += QLatin1Char(' ');
                }
                writeValue(out, o.value(keys.at(i)), format, depth+1);
                if(i != keys.size()-1)
                {
                    out += QLatin1Char(',');
                }
                out += nl;
            }
            out += padEnd + QLatin1Char('}');
            break;
        }
        case QJsonValue::Array:
        {
            QJsonArray a = v.toArray();
            if(a.isEmpty())
            {
                out += QLatin1String("[]");
                return;
            }
            out += QLatin1Char('[') + nl;
            for(int i = 0; i < a.size(); i++)
            {
                out += pad;
                writeValue(out, a.at(i), format, depth+1);
                if(i != a.size()-1)
                {
                    out += QLatin1Char(',');
                }
                out += nl;
            }
            out += padEnd + QLatin1Char(']');
            break;
        }
        case QJsonValue::Bool:
            out += v.toBool() ? QLatin1String("true") : QLatin1String("false");
            break;
        case QJsonValue::Double:
        {
            double d = v.toDouble();
            if(d == qint64(d))
            {
                out += QString::number(qint64(d));
            }
            else
            {
                out += QString::number(d, 'g', 16);
            }
            break;
        }
        case QJsonValue::String:
            writeString(out, v.toString());
            break;
        default:
            out += QLatin1String("null");
            break;
        }
    }

    QJsonObject m_object;
    QJsonArray m_array;
    bool m_isObject;
    bool m_isArray;
};

inline QDebug operator<<(QDebug dbg, const QJsonValue& v)
{
    dbg.nospace() << v.toVariant();
    return dbg.space();
}
inline QDebug operator<<(QDebug dbg, const QJsonObject& o)
{
    dbg.nospace() << "QJsonObject(" << o.toVariantMap() << ")";
    return dbg.space();
}
inline QDebug operator<<(QDebug dbg, const QJsonArray& a)
{
    dbg.nospace() << "QJsonArray(" << a.toVariantList() << ")";
    return dbg.space();
}

#endif // QT_VERSION
#endif // QT4JSON_H
