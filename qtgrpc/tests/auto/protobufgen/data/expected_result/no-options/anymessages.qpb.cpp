/* This file is autogenerated. DO NOT CHANGE. All changes will be lost */

#include "anymessages.qpb.h"
#include <QtProtobuf/qprotobufserializer.h>
#include <cmath>

namespace qtproto::tests {

class AnyMessage_QtProtobufData : public QSharedData
{
public:
    AnyMessage_QtProtobufData()
        : QSharedData()
    {
    }

    AnyMessage_QtProtobufData(const AnyMessage_QtProtobufData &other)
        : QSharedData(other),
          m_field(other.m_field)
    {
    }

    QtProtobuf::Any m_field;
};

AnyMessage::~AnyMessage() = default;

static constexpr struct {
    QtProtobufPrivate::QProtobufPropertyOrdering::Data data;
    const std::array<uint, 5> qt_protobuf_AnyMessage_uint_data;
    const char qt_protobuf_AnyMessage_char_data[32];
} qt_protobuf_AnyMessage_metadata {
    // data
    {
        0, /* = version */
        1, /* = num fields */
        2, /* = field number offset */
        3, /* = property index offset */
        4, /* = field flags offset */
        24, /* = message full name length */
    },
    // uint_data
    {
        // JSON name offsets:
        25, /* = field */
        31, /* = end-of-string-marker */
        // Field numbers:
        1, /* = field */
        // Property indices:
        0, /* = field */
        // Field flags:
        QtProtobufPrivate::NoFlags, /* = field */
    },
    // char_data
    /* metadata char_data: */
    "qtproto.tests.AnyMessage\0" /* = full message name */
    /* field char_data: */
    "field\0"
};

const QtProtobufPrivate::QProtobufPropertyOrdering AnyMessage::propertyOrdering = {
    &qt_protobuf_AnyMessage_metadata.data
};

void AnyMessage::registerTypes()
{
    qRegisterMetaType<AnyMessage>();
    qRegisterMetaType<AnyMessageRepeated>();
}

AnyMessage::AnyMessage()
    : QProtobufMessage(&AnyMessage::staticMetaObject),
      dptr(new AnyMessage_QtProtobufData)
{
}

AnyMessage::AnyMessage(const AnyMessage &other)
    : QProtobufMessage(other),
      dptr(other.dptr)
{
}
AnyMessage &AnyMessage::operator =(const AnyMessage &other)
{
    QProtobufMessage::operator=(other);
    dptr = other.dptr;
    return *this;
}
AnyMessage::AnyMessage(AnyMessage &&other) noexcept
    : QProtobufMessage(std::move(other)),
      dptr(std::move(other.dptr))
{
}
AnyMessage &AnyMessage::operator =(AnyMessage &&other) noexcept
{
    QProtobufMessage::operator=(std::move(other));
    dptr.swap(other.dptr);
    return *this;
}
bool AnyMessage::operator ==(const AnyMessage &other) const
{
    return QProtobufMessage::isEqual(*this, other)
        && dptr->m_field == other.dptr->m_field;
}

bool AnyMessage::operator !=(const AnyMessage &other) const
{
    return !this->operator ==(other);
}

QtProtobuf::Any AnyMessage::field() const
{
    return dptr->m_field;
}

void AnyMessage::setField(const QtProtobuf::Any &field)
{
    if (dptr->m_field != field) {
        dptr.detach();
        dptr->m_field = field;
    }
}


class RepeatedAnyMessage_QtProtobufData : public QSharedData
{
public:
    RepeatedAnyMessage_QtProtobufData()
        : QSharedData()
    {
    }

    RepeatedAnyMessage_QtProtobufData(const RepeatedAnyMessage_QtProtobufData &other)
        : QSharedData(other),
          m_anys(other.m_anys)
    {
    }

    QList<QtProtobuf::Any> m_anys;
};

RepeatedAnyMessage::~RepeatedAnyMessage() = default;

static constexpr struct {
    QtProtobufPrivate::QProtobufPropertyOrdering::Data data;
    const std::array<uint, 5> qt_protobuf_RepeatedAnyMessage_uint_data;
    const char qt_protobuf_RepeatedAnyMessage_char_data[39];
} qt_protobuf_RepeatedAnyMessage_metadata {
    // data
    {
        0, /* = version */
        1, /* = num fields */
        2, /* = field number offset */
        3, /* = property index offset */
        4, /* = field flags offset */
        32, /* = message full name length */
    },
    // uint_data
    {
        // JSON name offsets:
        33, /* = anys */
        38, /* = end-of-string-marker */
        // Field numbers:
        1, /* = anys */
        // Property indices:
        0, /* = anys */
        // Field flags:
        QtProtobufPrivate::NoFlags, /* = anys */
    },
    // char_data
    /* metadata char_data: */
    "qtproto.tests.RepeatedAnyMessage\0" /* = full message name */
    /* field char_data: */
    "anys\0"
};

const QtProtobufPrivate::QProtobufPropertyOrdering RepeatedAnyMessage::propertyOrdering = {
    &qt_protobuf_RepeatedAnyMessage_metadata.data
};

void RepeatedAnyMessage::registerTypes()
{
    qRegisterMetaType<RepeatedAnyMessage>();
    qRegisterMetaType<RepeatedAnyMessageRepeated>();
}

RepeatedAnyMessage::RepeatedAnyMessage()
    : QProtobufMessage(&RepeatedAnyMessage::staticMetaObject),
      dptr(new RepeatedAnyMessage_QtProtobufData)
{
}

RepeatedAnyMessage::RepeatedAnyMessage(const RepeatedAnyMessage &other)
    : QProtobufMessage(other),
      dptr(other.dptr)
{
}
RepeatedAnyMessage &RepeatedAnyMessage::operator =(const RepeatedAnyMessage &other)
{
    QProtobufMessage::operator=(other);
    dptr = other.dptr;
    return *this;
}
RepeatedAnyMessage::RepeatedAnyMessage(RepeatedAnyMessage &&other) noexcept
    : QProtobufMessage(std::move(other)),
      dptr(std::move(other.dptr))
{
}
RepeatedAnyMessage &RepeatedAnyMessage::operator =(RepeatedAnyMessage &&other) noexcept
{
    QProtobufMessage::operator=(std::move(other));
    dptr.swap(other.dptr);
    return *this;
}
bool RepeatedAnyMessage::operator ==(const RepeatedAnyMessage &other) const
{
    return QProtobufMessage::isEqual(*this, other)
        && QtProtobuf::repeatedValueCompare(dptr->m_anys, other.dptr->m_anys);
}

bool RepeatedAnyMessage::operator !=(const RepeatedAnyMessage &other) const
{
    return !this->operator ==(other);
}

QList<QtProtobuf::Any> RepeatedAnyMessage::anys() const
{
    return dptr->m_anys;
}

QList<QtProtobuf::Any> &RepeatedAnyMessage::anys()
{
    dptr.detach();
    return dptr->m_anys;
}

void RepeatedAnyMessage::setAnys(const QList<QtProtobuf::Any> &anys)
{
    if (dptr->m_anys != anys) {
        dptr.detach();
        dptr->m_anys = anys;
    }
}


class TwoAnyMessage_QtProtobufData : public QSharedData
{
public:
    TwoAnyMessage_QtProtobufData()
        : QSharedData()
    {
    }

    TwoAnyMessage_QtProtobufData(const TwoAnyMessage_QtProtobufData &other)
        : QSharedData(other),
          m_one(other.m_one),
          m_two(other.m_two)
    {
    }

    QtProtobuf::Any m_one;
    QtProtobuf::Any m_two;
};

TwoAnyMessage::~TwoAnyMessage() = default;

static constexpr struct {
    QtProtobufPrivate::QProtobufPropertyOrdering::Data data;
    const std::array<uint, 9> qt_protobuf_TwoAnyMessage_uint_data;
    const char qt_protobuf_TwoAnyMessage_char_data[37];
} qt_protobuf_TwoAnyMessage_metadata {
    // data
    {
        0, /* = version */
        2, /* = num fields */
        3, /* = field number offset */
        5, /* = property index offset */
        7, /* = field flags offset */
        27, /* = message full name length */
    },
    // uint_data
    {
        // JSON name offsets:
        28, /* = one */
        32, /* = two */
        36, /* = end-of-string-marker */
        // Field numbers:
        1, /* = one */
        2, /* = two */
        // Property indices:
        0, /* = one */
        1, /* = two */
        // Field flags:
        QtProtobufPrivate::NoFlags, /* = one */
        QtProtobufPrivate::NoFlags, /* = two */
    },
    // char_data
    /* metadata char_data: */
    "qtproto.tests.TwoAnyMessage\0" /* = full message name */
    /* field char_data: */
    "one\0two\0"
};

const QtProtobufPrivate::QProtobufPropertyOrdering TwoAnyMessage::propertyOrdering = {
    &qt_protobuf_TwoAnyMessage_metadata.data
};

void TwoAnyMessage::registerTypes()
{
    qRegisterMetaType<TwoAnyMessage>();
    qRegisterMetaType<TwoAnyMessageRepeated>();
}

TwoAnyMessage::TwoAnyMessage()
    : QProtobufMessage(&TwoAnyMessage::staticMetaObject),
      dptr(new TwoAnyMessage_QtProtobufData)
{
}

TwoAnyMessage::TwoAnyMessage(const TwoAnyMessage &other)
    : QProtobufMessage(other),
      dptr(other.dptr)
{
}
TwoAnyMessage &TwoAnyMessage::operator =(const TwoAnyMessage &other)
{
    QProtobufMessage::operator=(other);
    dptr = other.dptr;
    return *this;
}
TwoAnyMessage::TwoAnyMessage(TwoAnyMessage &&other) noexcept
    : QProtobufMessage(std::move(other)),
      dptr(std::move(other.dptr))
{
}
TwoAnyMessage &TwoAnyMessage::operator =(TwoAnyMessage &&other) noexcept
{
    QProtobufMessage::operator=(std::move(other));
    dptr.swap(other.dptr);
    return *this;
}
bool TwoAnyMessage::operator ==(const TwoAnyMessage &other) const
{
    return QProtobufMessage::isEqual(*this, other)
        && dptr->m_one == other.dptr->m_one
        && dptr->m_two == other.dptr->m_two;
}

bool TwoAnyMessage::operator !=(const TwoAnyMessage &other) const
{
    return !this->operator ==(other);
}

QtProtobuf::Any TwoAnyMessage::one() const
{
    return dptr->m_one;
}

QtProtobuf::Any TwoAnyMessage::two() const
{
    return dptr->m_two;
}

void TwoAnyMessage::setOne(const QtProtobuf::Any &one)
{
    if (dptr->m_one != one) {
        dptr.detach();
        dptr->m_one = one;
    }
}

void TwoAnyMessage::setTwo(const QtProtobuf::Any &two)
{
    if (dptr->m_two != two) {
        dptr.detach();
        dptr->m_two = two;
    }
}


class Example_QtProtobufData : public QSharedData
{
public:
    Example_QtProtobufData()
        : QSharedData(),
          m_i(0),
          m_j(0),
          m_h(0)
    {
    }

    Example_QtProtobufData(const Example_QtProtobufData &other)
        : QSharedData(other),
          m_str(other.m_str),
          m_i(other.m_i),
          m_j(other.m_j),
          m_h(other.m_h),
          m_str2(other.m_str2)
    {
    }

    QString m_str;
    QtProtobuf::sint32 m_i;
    QtProtobuf::sint32 m_j;
    QtProtobuf::sint32 m_h;
    QString m_str2;
};

Example::~Example() = default;

static constexpr struct {
    QtProtobufPrivate::QProtobufPropertyOrdering::Data data;
    const std::array<uint, 21> qt_protobuf_Example_uint_data;
    const char qt_protobuf_Example_char_data[38];
} qt_protobuf_Example_metadata {
    // data
    {
        0, /* = version */
        5, /* = num fields */
        6, /* = field number offset */
        11, /* = property index offset */
        16, /* = field flags offset */
        21, /* = message full name length */
    },
    // uint_data
    {
        // JSON name offsets:
        22, /* = str */
        26, /* = i */
        28, /* = j */
        30, /* = h */
        32, /* = str2 */
        37, /* = end-of-string-marker */
        // Field numbers:
        1, /* = str */
        2, /* = i */
        3, /* = j */
        4, /* = h */
        5, /* = str2 */
        // Property indices:
        0, /* = str */
        1, /* = i */
        2, /* = j */
        3, /* = h */
        4, /* = str2 */
        // Field flags:
        QtProtobufPrivate::NoFlags, /* = str */
        QtProtobufPrivate::NoFlags, /* = i */
        QtProtobufPrivate::NoFlags, /* = j */
        QtProtobufPrivate::NoFlags, /* = h */
        QtProtobufPrivate::NoFlags, /* = str2 */
    },
    // char_data
    /* metadata char_data: */
    "qtproto.tests.Example\0" /* = full message name */
    /* field char_data: */
    "str\0i\0j\0h\0str2\0"
};

const QtProtobufPrivate::QProtobufPropertyOrdering Example::propertyOrdering = {
    &qt_protobuf_Example_metadata.data
};

void Example::registerTypes()
{
    qRegisterMetaType<Example>();
    qRegisterMetaType<ExampleRepeated>();
}

Example::Example()
    : QProtobufMessage(&Example::staticMetaObject),
      dptr(new Example_QtProtobufData)
{
}

Example::Example(const Example &other)
    : QProtobufMessage(other),
      dptr(other.dptr)
{
}
Example &Example::operator =(const Example &other)
{
    QProtobufMessage::operator=(other);
    dptr = other.dptr;
    return *this;
}
Example::Example(Example &&other) noexcept
    : QProtobufMessage(std::move(other)),
      dptr(std::move(other.dptr))
{
}
Example &Example::operator =(Example &&other) noexcept
{
    QProtobufMessage::operator=(std::move(other));
    dptr.swap(other.dptr);
    return *this;
}
bool Example::operator ==(const Example &other) const
{
    return QProtobufMessage::isEqual(*this, other)
        && dptr->m_str == other.dptr->m_str
        && dptr->m_i == other.dptr->m_i
        && dptr->m_j == other.dptr->m_j
        && dptr->m_h == other.dptr->m_h
        && dptr->m_str2 == other.dptr->m_str2;
}

bool Example::operator !=(const Example &other) const
{
    return !this->operator ==(other);
}

QString Example::str() const
{
    return dptr->m_str;
}

QtProtobuf::sint32 Example::i() const
{
    return dptr->m_i;
}

QtProtobuf::sint32 Example::j() const
{
    return dptr->m_j;
}

QtProtobuf::sint32 Example::h() const
{
    return dptr->m_h;
}

QString Example::str2() const
{
    return dptr->m_str2;
}

void Example::setStr(const QString &str)
{
    if (dptr->m_str != str) {
        dptr.detach();
        dptr->m_str = str;
    }
}

void Example::setI(const QtProtobuf::sint32 &i)
{
    if (dptr->m_i != i) {
        dptr.detach();
        dptr->m_i = i;
    }
}

void Example::setJ(const QtProtobuf::sint32 &j)
{
    if (dptr->m_j != j) {
        dptr.detach();
        dptr->m_j = j;
    }
}

void Example::setH(const QtProtobuf::sint32 &h)
{
    if (dptr->m_h != h) {
        dptr.detach();
        dptr->m_h = h;
    }
}

void Example::setStr2(const QString &str2)
{
    if (dptr->m_str2 != str2) {
        dptr.detach();
        dptr->m_str2 = str2;
    }
}


class SimpleMessage_QtProtobufData : public QSharedData
{
public:
    SimpleMessage_QtProtobufData()
        : QSharedData(),
          m_i(0)
    {
    }

    SimpleMessage_QtProtobufData(const SimpleMessage_QtProtobufData &other)
        : QSharedData(other),
          m_i(other.m_i)
    {
    }

    QtProtobuf::int32 m_i;
};

SimpleMessage::~SimpleMessage() = default;

static constexpr struct {
    QtProtobufPrivate::QProtobufPropertyOrdering::Data data;
    const std::array<uint, 5> qt_protobuf_SimpleMessage_uint_data;
    const char qt_protobuf_SimpleMessage_char_data[31];
} qt_protobuf_SimpleMessage_metadata {
    // data
    {
        0, /* = version */
        1, /* = num fields */
        2, /* = field number offset */
        3, /* = property index offset */
        4, /* = field flags offset */
        27, /* = message full name length */
    },
    // uint_data
    {
        // JSON name offsets:
        28, /* = i */
        30, /* = end-of-string-marker */
        // Field numbers:
        500000, /* = i */
        // Property indices:
        0, /* = i */
        // Field flags:
        QtProtobufPrivate::NoFlags, /* = i */
    },
    // char_data
    /* metadata char_data: */
    "qtproto.tests.SimpleMessage\0" /* = full message name */
    /* field char_data: */
    "i\0"
};

const QtProtobufPrivate::QProtobufPropertyOrdering SimpleMessage::propertyOrdering = {
    &qt_protobuf_SimpleMessage_metadata.data
};

void SimpleMessage::registerTypes()
{
    qRegisterMetaType<SimpleMessage>();
    qRegisterMetaType<SimpleMessageRepeated>();
}

SimpleMessage::SimpleMessage()
    : QProtobufMessage(&SimpleMessage::staticMetaObject),
      dptr(new SimpleMessage_QtProtobufData)
{
}

SimpleMessage::SimpleMessage(const SimpleMessage &other)
    : QProtobufMessage(other),
      dptr(other.dptr)
{
}
SimpleMessage &SimpleMessage::operator =(const SimpleMessage &other)
{
    QProtobufMessage::operator=(other);
    dptr = other.dptr;
    return *this;
}
SimpleMessage::SimpleMessage(SimpleMessage &&other) noexcept
    : QProtobufMessage(std::move(other)),
      dptr(std::move(other.dptr))
{
}
SimpleMessage &SimpleMessage::operator =(SimpleMessage &&other) noexcept
{
    QProtobufMessage::operator=(std::move(other));
    dptr.swap(other.dptr);
    return *this;
}
bool SimpleMessage::operator ==(const SimpleMessage &other) const
{
    return QProtobufMessage::isEqual(*this, other)
        && dptr->m_i == other.dptr->m_i;
}

bool SimpleMessage::operator !=(const SimpleMessage &other) const
{
    return !this->operator ==(other);
}

QtProtobuf::int32 SimpleMessage::i() const
{
    return dptr->m_i;
}

void SimpleMessage::setI(const QtProtobuf::int32 &i)
{
    if (dptr->m_i != i) {
        dptr.detach();
        dptr->m_i = i;
    }
}

} // namespace qtproto::tests

#include "moc_anymessages.qpb.cpp"
