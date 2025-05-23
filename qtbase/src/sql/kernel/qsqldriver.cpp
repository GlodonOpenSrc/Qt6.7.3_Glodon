// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsqldriver.h"

#include "qdatetime.h"
#include "qsqlerror.h"
#include "qsqlfield.h"
#include "qsqlindex.h"
#include "private/qsqldriver_p.h"
#include "private/qtools_p.h"

#include <limits.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QString prepareIdentifier(const QString &identifier,
        QSqlDriver::IdentifierType type, const QSqlDriver *driver)
{
    Q_ASSERT(driver != nullptr);
    QString ret = identifier;
    if (!driver->isIdentifierEscaped(identifier, type))
        ret = driver->escapeIdentifier(identifier, type);
    return ret;
}

/*!
    \class QSqlDriver
    \brief The QSqlDriver class is an abstract base class for accessing
    specific SQL databases.

    \ingroup database
    \inmodule QtSql

    This class should not be used directly. Use QSqlDatabase instead.

    If you want to create your own SQL drivers, you can subclass this
    class and reimplement its pure virtual functions and those
    virtual functions that you need. See \l{How to Write Your Own
    Database Driver} for more information.

    \sa QSqlDatabase, QSqlResult
*/

/*!
    Constructs a new driver with the given \a parent.
*/

QSqlDriver::QSqlDriver(QObject *parent)
    : QObject(*new QSqlDriverPrivate, parent)
{
}

/*!  \internal
*/
QSqlDriver::QSqlDriver(QSqlDriverPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    Destroys the object and frees any allocated resources.
*/

QSqlDriver::~QSqlDriver()
{
}

/*!
    \since 5.0

    \fn QSqlDriver::notification(const QString &name, QSqlDriver::NotificationSource source, const QVariant & payload)

    This signal is emitted when the database posts an event notification
    that the driver subscribes to. \a name identifies the event notification, \a source indicates the signal source,
    \a payload holds the extra data optionally delivered with the notification.

    \sa subscribeToNotification()
*/

/*!
    \fn bool QSqlDriver::open(const QString &db, const QString &user, const QString& password,
                              const QString &host, int port, const QString &options)

    Derived classes must reimplement this pure virtual function to
    open a database connection on database \a db, using user name \a
    user, password \a password, host \a host, port \a port and
    connection options \a options.

    The function must return true on success and false on failure.

    \sa setOpen()
*/

/*!
    \fn bool QSqlDriver::close()

    Derived classes must reimplement this pure virtual function in
    order to close the database connection. Return true on success,
    false on failure.

    \sa open(), setOpen()
*/

/*!
    \fn QSqlResult *QSqlDriver::createResult() const

    Creates an empty SQL result on the database. Derived classes must
    reimplement this function and return a QSqlResult object
    appropriate for their database to the caller.
*/

/*!
    Returns \c true if the database connection is open; otherwise returns
    false.
*/

bool QSqlDriver::isOpen() const
{
    Q_D(const QSqlDriver);
    return d->isOpen;
}

/*!
    Returns \c true if the there was an error opening the database
    connection; otherwise returns \c false.
*/

bool QSqlDriver::isOpenError() const
{
    Q_D(const QSqlDriver);
    return d->isOpenError;
}

/*!
    \enum QSqlDriver::DriverFeature

    This enum contains a list of features a driver might support. Use
    hasFeature() to query whether a feature is supported or not. Some features
    depend on the database server so they can only properly determined after
    the database connection is successfully opened with QSqlDatabase::open().

    \value Transactions  Whether the driver supports SQL transactions.
    \value QuerySize  Whether the database is capable of reporting the size
    of a query. Note that some databases do not support returning the size
    (i.e. number of rows returned) of a query, in which case
    QSqlQuery::size() will return -1.
    \value BLOB  Whether the driver supports Binary Large Object fields.
    \value Unicode  Whether the driver supports Unicode strings if the
    database server does.
    \value PreparedQueries  Whether the driver supports prepared query execution.
    \value NamedPlaceholders  Whether the driver supports the use of named placeholders.
    \value PositionalPlaceholders  Whether the driver supports the use of positional placeholders.
    \value LastInsertId  Whether the driver supports returning the Id of the last touched row.
    \value BatchOperations  Whether the driver supports batched operations, see QSqlQuery::execBatch()
    \value SimpleLocking  Whether the driver disallows a write lock on a table while other queries have a read lock on it.
    \value LowPrecisionNumbers  Whether the driver allows fetching numerical values with low precision.
    \value EventNotifications Whether the driver supports database event notifications.
    \value FinishQuery Whether the driver can do any low-level resource cleanup when QSqlQuery::finish() is called.
    \value MultipleResultSets Whether the driver can access multiple result sets returned from batched statements or stored procedures.
    \value CancelQuery Whether the driver allows cancelling a running query.

    More information about supported features can be found in the
    \l{sql-driver.html}{Qt SQL driver} documentation.

    \sa hasFeature()
*/

/*!
    \enum QSqlDriver::StatementType

    This enum contains a list of SQL statement (or clause) types the
    driver can create.

    \value WhereStatement  An SQL \c WHERE statement (e.g., \c{WHERE f = 5}).
    \value SelectStatement An SQL \c SELECT statement (e.g., \c{SELECT f FROM t}).
    \value UpdateStatement An SQL \c UPDATE statement (e.g., \c{UPDATE TABLE t set f = 1}).
    \value InsertStatement An SQL \c INSERT statement (e.g., \c{INSERT INTO t (f) values (1)}).
    \value DeleteStatement An SQL \c DELETE statement (e.g., \c{DELETE FROM t}).

    \sa sqlStatement()
*/

/*!
    \enum QSqlDriver::IdentifierType

    This enum contains a list of SQL identifier types.

    \value FieldName A SQL field name
    \value TableName A SQL table name
*/

/*!
    \enum QSqlDriver::NotificationSource

    This enum contains a list of SQL notification sources.

    \value UnknownSource The notification source is unknown
    \value SelfSource The notification source is this connection
    \value OtherSource The notification source is another connection
*/

/*!
    \enum QSqlDriver::DbmsType
    \internal

    This enum contains DBMS types.

    \value UnknownDbms
    \value MSSqlServer
    \value MySqlServer
    \value PostgreSQL
    \value Oracle
    \value Sybase
    \value SQLite
    \value Interbase
    \value DB2
    \value [since 6.6] MimerSQL
*/

/*!
    \fn bool QSqlDriver::hasFeature(DriverFeature feature) const

    Returns \c true if the driver supports feature \a feature; otherwise
    returns \c false.

    Note that some databases need to be open() before this can be
    determined.

    \sa DriverFeature
*/

/*!
    This function sets the open state of the database to \a open.
    Derived classes can use this function to report the status of
    open().

    \sa open(), setOpenError()
*/

void QSqlDriver::setOpen(bool open)
{
    Q_D(QSqlDriver);
    d->isOpen = open;
}

/*!
    This function sets the open error state of the database to \a
    error. Derived classes can use this function to report the status
    of open(). Note that if \a error is true the open state of the
    database is set to closed (i.e., isOpen() returns \c false).

    \sa open(), setOpen()
*/

void QSqlDriver::setOpenError(bool error)
{
    Q_D(QSqlDriver);
    d->isOpenError = error;
    if (error)
        d->isOpen = false;
}

/*!
    This function is called to begin a transaction. If successful,
    return true, otherwise return false. The default implementation
    does nothing and returns \c false.

    \sa commitTransaction(), rollbackTransaction()
*/

bool QSqlDriver::beginTransaction()
{
    return false;
}

/*!
    This function is called to commit a transaction. If successful,
    return true, otherwise return false. The default implementation
    does nothing and returns \c false.

    \sa beginTransaction(), rollbackTransaction()
*/

bool QSqlDriver::commitTransaction()
{
    return false;
}

/*!
    This function is called to rollback a transaction. If successful,
    return true, otherwise return false. The default implementation
    does nothing and returns \c false.

    \sa beginTransaction(), commitTransaction()
*/

bool QSqlDriver::rollbackTransaction()
{
    return false;
}

/*!
    This function is used to set the value of the last error, \a error,
    that occurred on the database.

    \sa lastError()
*/

void QSqlDriver::setLastError(const QSqlError &error)
{
    Q_D(QSqlDriver);
    d->error = error;
}

/*!
    Returns a QSqlError object which contains information about the
    last error that occurred on the database.
*/

QSqlError QSqlDriver::lastError() const
{
    Q_D(const QSqlDriver);
    return d->error;
}

/*!
    Returns a list of the names of the tables in the database. The
    default implementation returns an empty list.

    The \a tableType argument describes what types of tables
    should be returned. Due to binary compatibility, the string
    contains the value of the enum QSql::TableTypes as text.
    An empty string should be treated as QSql::Tables for
    backward compatibility.
*/

QStringList QSqlDriver::tables(QSql::TableType) const
{
    return QStringList();
}

/*!
    Returns the primary index for table \a tableName. Returns an empty
    QSqlIndex if the table doesn't have a primary index. The default
    implementation returns an empty index.
*/

QSqlIndex QSqlDriver::primaryIndex(const QString&) const
{
    return QSqlIndex();
}


/*!
    Returns a QSqlRecord populated with the names of the fields in
    table \a tableName. If no such table exists, an empty record is
    returned. The default implementation returns an empty record.
*/

QSqlRecord QSqlDriver::record(const QString & /* tableName */) const
{
    return QSqlRecord();
}

/*!
    Returns the \a identifier escaped according to the database rules.
    \a identifier can either be a table name or field name, dependent
    on \a type.

    The default implementation does nothing.
    \sa isIdentifierEscaped()
 */
QString QSqlDriver::escapeIdentifier(const QString &identifier, IdentifierType) const
{
    return identifier;
}

/*!
    Returns whether \a identifier is escaped according to the database rules.
    \a identifier can either be a table name or field name, dependent
    on \a type.

    Reimplement this function if you want to provide your own implementation in your
    QSqlDriver subclass,

    \sa stripDelimiters(), escapeIdentifier()
 */
bool QSqlDriver::isIdentifierEscaped(const QString &identifier, IdentifierType type) const
{
    Q_UNUSED(type);
    return identifier.size() > 2
        && identifier.startsWith(u'"') //left delimited
        && identifier.endsWith(u'"'); //right delimited
}

/*!
    Returns the \a identifier with the leading and trailing delimiters removed,
    \a identifier can either be a table name or field name,
    dependent on \a type.  If \a identifier does not have leading
    and trailing delimiter characters, \a identifier is returned without
    modification.

    Reimplement this function if you want to provide your own implementation in your
    QSqlDriver subclass,

    \since 4.5
    \sa isIdentifierEscaped()
 */
QString QSqlDriver::stripDelimiters(const QString &identifier, IdentifierType type) const
{
    QString ret;
    if (isIdentifierEscaped(identifier, type)) {
        ret = identifier.mid(1);
        ret.chop(1);
    } else {
        ret = identifier;
    }
    return ret;
}

/*!
    Returns a SQL statement of type \a type for the table \a tableName
    with the values from \a rec. If \a preparedStatement is true, the
    string will contain placeholders instead of values.

    The generated flag in each field of \a rec determines whether the
    field is included in the generated statement.

    This method can be used to manipulate tables without having to worry
    about database-dependent SQL dialects. For non-prepared statements,
    the values will be properly escaped.

    In the WHERE statement, each non-null field of \a rec specifies a
    filter condition of equality to the field value, or if prepared, a
    placeholder. However, prepared or not, a null field specifies the
    condition IS NULL and never introduces a placeholder. The
    application must not attempt to bind data for the null field during
    execution. The field must be set to some non-null value if a
    placeholder is desired. Furthermore, since non-null fields specify
    equality conditions and SQL NULL is not equal to anything, even
    itself, it is generally not useful to bind a null to a placeholder.

*/
QString QSqlDriver::sqlStatement(StatementType type, const QString &tableName,
                                 const QSqlRecord &rec, bool preparedStatement) const
{
    const auto tableNameString = tableName.isEmpty() ? QString()
                                    : prepareIdentifier(tableName, QSqlDriver::TableName, this);
    QString s;
    s.reserve(128);
    switch (type) {
    case SelectStatement:
        for (qsizetype i = 0; i < rec.count(); ++i) {
            if (rec.isGenerated(i))
                s.append(prepareIdentifier(rec.fieldName(i), QSqlDriver::FieldName, this)).append(", "_L1);
        }
        if (s.isEmpty())
            return s;
        s.chop(2);
        s = "SELECT "_L1 + s + " FROM "_L1 + tableNameString;
        break;
    case WhereStatement:
    {
        const QString tableNamePrefix = tableNameString.isEmpty()
                                            ? QString() : tableNameString + u'.';
        for (qsizetype i = 0; i < rec.count(); ++i) {
            if (!rec.isGenerated(i))
                continue;
            s.append(s.isEmpty() ? "WHERE "_L1 : " AND "_L1);
            s.append(tableNamePrefix);
            s.append(prepareIdentifier(rec.fieldName(i), QSqlDriver::FieldName, this));
            if (rec.isNull(i))
                s.append(" IS NULL"_L1);
            else if (preparedStatement)
                s.append(" = ?"_L1);
            else
                s.append(" = "_L1).append(formatValue(rec.field(i)));
        }
        break;
    }
    case UpdateStatement:
        s = s + "UPDATE "_L1 + tableNameString + " SET "_L1;
        for (qsizetype i = 0; i < rec.count(); ++i) {
            if (!rec.isGenerated(i))
                continue;
            s.append(prepareIdentifier(rec.fieldName(i), QSqlDriver::FieldName, this)).append(u'=');
            if (preparedStatement)
                s.append(u'?');
            else
                s.append(formatValue(rec.field(i)));
            s.append(", "_L1);
        }
        if (s.endsWith(", "_L1))
            s.chop(2);
        else
            s.clear();
        break;
    case DeleteStatement:
        s = s + "DELETE FROM "_L1 + tableNameString;
        break;
    case InsertStatement: {
        s = s + "INSERT INTO "_L1 + tableNameString + " ("_L1;
        QString vals;
        for (qsizetype i = 0; i < rec.count(); ++i) {
            if (!rec.isGenerated(i))
                continue;
            s.append(prepareIdentifier(rec.fieldName(i), QSqlDriver::FieldName, this)).append(", "_L1);
            if (preparedStatement)
                vals.append(u'?');
            else
                vals.append(formatValue(rec.field(i)));
            vals.append(", "_L1);
        }
        if (vals.isEmpty()) {
            s.clear();
        } else {
            vals.chop(2); // remove trailing comma
            s[s.size() - 2] = u')';
            s.append("VALUES ("_L1).append(vals).append(u')');
        }
        break; }
    }
    return s;
}

/*!
    Returns a string representation of the \a field value for the
    database. This is used, for example, when constructing INSERT and
    UPDATE statements.

    The default implementation returns the value formatted as a string
    according to the following rules:

    \list

    \li If \a field is character data, the value is returned enclosed
    in single quotation marks, which is appropriate for many SQL
    databases. Any embedded single-quote characters are escaped
    (replaced with two single-quote characters). If \a trimStrings is
    true (the default is false), all trailing whitespace is trimmed
    from the field.

    \li If \a field is date/time data, the value is formatted in ISO
    format and enclosed in single quotation marks. If the date/time
    data is invalid, "NULL" is returned.

    \li If \a field is \l{QByteArray}{bytearray} data, and the
    driver can edit binary fields, the value is formatted as a
    hexadecimal string.

    \li For any other field type, toString() is called on its value
    and the result of this is returned.

    \endlist

    \sa QVariant::toString()

*/
QString QSqlDriver::formatValue(const QSqlField &field, bool trimStrings) const
{
    const auto nullTxt = "NULL"_L1;

    QString r;
    if (field.isNull())
        r = nullTxt;
    else {
        switch (field.metaType().id()) {
        case QMetaType::Int:
        case QMetaType::UInt:
            if (field.value().userType() == QMetaType::Bool)
                r = field.value().toBool() ? "1"_L1 : "0"_L1;
            else
                r = field.value().toString();
            break;
#if QT_CONFIG(datestring)
        case QMetaType::QDate:
            if (field.value().toDate().isValid())
                r = u'\'' + field.value().toDate().toString(Qt::ISODate) + u'\'';
            else
                r = nullTxt;
            break;
        case QMetaType::QTime:
            if (field.value().toTime().isValid())
                r =  u'\'' + field.value().toTime().toString(Qt::ISODate) + u'\'';
            else
                r = nullTxt;
            break;
        case QMetaType::QDateTime:
            if (field.value().toDateTime().isValid())
                r = u'\'' + field.value().toDateTime().toString(Qt::ISODate) + u'\'';
            else
                r = nullTxt;
            break;
#endif
        case QMetaType::QString:
        case QMetaType::QChar:
        {
            QString result = field.value().toString();
            if (trimStrings) {
                int end = result.size();
                while (end && result.at(end-1).isSpace()) /* skip white space from end */
                    end--;
                result.truncate(end);
            }
            /* escape the "'" character */
            result.replace(u'\'', "''"_L1);
            r = u'\'' + result + u'\'';
            break;
        }
        case QMetaType::Bool:
            r = QString::number(field.value().toBool());
            break;
        case QMetaType::QByteArray : {
            if (hasFeature(BLOB)) {
                const QByteArray ba = field.value().toByteArray();
                r.reserve((ba.size() + 1) * 2);
                r += u'\'';
                for (const char c : ba) {
                    const uchar s = uchar(c);
                    r += QLatin1Char(QtMiscUtils::toHexLower(s >> 4));
                    r += QLatin1Char(QtMiscUtils::toHexLower(s & 0x0f));
                }
                r += u'\'';
                break;
            }
        }
            Q_FALLTHROUGH();
        default:
            r = field.value().toString();
            break;
        }
    }
    return r;
}

/*!
    Returns the low-level database handle wrapped in a QVariant or an
    invalid variant if there is no handle.

    \warning Use this with uttermost care and only if you know what you're doing.

    \warning The handle returned here can become a stale pointer if the connection
    is modified (for example, if you close the connection).

    \warning The handle can be NULL if the connection is not open yet.

    The handle returned here is database-dependent, you should query the type
    name of the variant before accessing it.

    This example retrieves the handle for a connection to sqlite:

    \snippet code/src_sql_kernel_qsqldriver.cpp 0

    This snippet returns the handle for PostgreSQL or MySQL:

    \snippet code/src_sql_kernel_qsqldriver.cpp 1

    \sa QSqlResult::handle()
*/
QVariant QSqlDriver::handle() const
{
    return QVariant();
}

/*!
    This function is called to subscribe to event notifications from the database.
    \a name identifies the event notification.

    If successful, return true, otherwise return false.

    The database must be open when this function is called. When the database is closed
    by calling close() all subscribed event notifications are automatically unsubscribed.
    Note that calling open() on an already open database may implicitly cause close() to
    be called, which will cause the driver to unsubscribe from all event notifications.

    When an event notification identified by \a name is posted by the database the
    notification() signal is emitted.

    Reimplement this function if you want to provide event notification support in your
    own QSqlDriver subclass,

    \since 4.4
    \sa unsubscribeFromNotification(), subscribedToNotifications(), QSqlDriver::hasFeature()
*/
bool QSqlDriver::subscribeToNotification(const QString &name)
{
    Q_UNUSED(name);
    return false;
}

/*!
    This function is called to unsubscribe from event notifications from the database.
    \a name identifies the event notification.

    If successful, return true, otherwise return false.

    The database must be open when this function is called. All subscribed event
    notifications are automatically unsubscribed from when the close() function is called.

    After calling \e this function the notification() signal will no longer be emitted
    when an event notification identified by \a name is posted by the database.

    Reimplement this function if you want to provide event notification support in your
    own QSqlDriver subclass,

    \since 4.4
    \sa subscribeToNotification(), subscribedToNotifications()
*/
bool QSqlDriver::unsubscribeFromNotification(const QString &name)
{
    Q_UNUSED(name);
    return false;
}

/*!
    Returns a list of the names of the event notifications that are currently subscribed to.

    Reimplement this function if you want to provide event notification support in your
    own QSqlDriver subclass,

    \since 4.4
    \sa subscribeToNotification(), unsubscribeFromNotification()
*/
QStringList QSqlDriver::subscribedToNotifications() const
{
    return QStringList();
}

/*!
    \since 4.6

    Sets the default numerical precision policy used by queries created
    by this driver to \a precisionPolicy.

    Note: Setting the default precision policy to \a precisionPolicy
    doesn't affect any currently active queries.

    \sa QSql::NumericalPrecisionPolicy, numericalPrecisionPolicy(),
    QSqlQuery::setNumericalPrecisionPolicy(), QSqlQuery::numericalPrecisionPolicy()
*/
void QSqlDriver::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy)
{
    Q_D(QSqlDriver);
    d->precisionPolicy = precisionPolicy;
}

/*!
    \since 4.6

    Returns the current default precision policy for the database connection.

    \sa QSql::NumericalPrecisionPolicy, setNumericalPrecisionPolicy(),
    QSqlQuery::numericalPrecisionPolicy(), QSqlQuery::setNumericalPrecisionPolicy()
*/
QSql::NumericalPrecisionPolicy QSqlDriver::numericalPrecisionPolicy() const
{
    Q_D(const QSqlDriver);
    return d->precisionPolicy;
}

/*!
    \since 5.4
    \internal

    Returns the current DBMS type for the database connection.
*/
QSqlDriver::DbmsType QSqlDriver::dbmsType() const
{
    Q_D(const QSqlDriver);
    return d->dbmsType;
}

/*!
    \since 5.0
    \internal

    Tries to cancel the running query, if the underlying driver has the
    capability to cancel queries. Returns \c true on success, otherwise false.

    This function can be called from a different thread.

    If you use this function as a slot, you need to use a Qt::DirectConnection
    from a different thread.

    Reimplement this function to support canceling running queries in
    your own QSqlDriver subclass. It must be implemented in a thread-safe
    manner.

    \sa QSqlDriver::hasFeature()
*/
bool QSqlDriver::cancelQuery()
{
    return false;
}

/*!
    \since 6.0

    Returns the maximum length for the identifier \a type according to the database settings. Returns
    INT_MAX by default if the is no maximum for the database.
*/

int QSqlDriver::maximumIdentifierLength(QSqlDriver::IdentifierType type) const
{
    Q_UNUSED(type);
    return INT_MAX;
}

QT_END_NAMESPACE

#include "moc_qsqldriver.cpp"
