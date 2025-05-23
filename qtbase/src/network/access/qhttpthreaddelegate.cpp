// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//#define QHTTPTHREADDELEGATE_DEBUG
#include "qhttpthreaddelegate_p.h"

#include <QThread>
#include <QTimer>
#include <QAuthenticator>
#include <QEventLoop>
#include <QCryptographicHash>
#include <QtCore/qscopedvaluerollback.h>

#include "private/qhttpnetworkreply_p.h"
#include "private/qnetworkaccesscache_p.h"
#include "private/qnoncontiguousbytedevice_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QNetworkReply::NetworkError statusCodeFromHttp(int httpStatusCode, const QUrl &url)
{
    QNetworkReply::NetworkError code;
    // we've got an error
    switch (httpStatusCode) {
    case 400:               // Bad Request
        code = QNetworkReply::ProtocolInvalidOperationError;
        break;

    case 401:               // Authorization required
        code = QNetworkReply::AuthenticationRequiredError;
        break;

    case 403:               // Access denied
        code = QNetworkReply::ContentAccessDenied;
        break;

    case 404:               // Not Found
        code = QNetworkReply::ContentNotFoundError;
        break;

    case 405:               // Method Not Allowed
        code = QNetworkReply::ContentOperationNotPermittedError;
        break;

    case 407:
        code = QNetworkReply::ProxyAuthenticationRequiredError;
        break;

    case 409:               // Resource Conflict
        code = QNetworkReply::ContentConflictError;
        break;

    case 410:               // Content no longer available
        code = QNetworkReply::ContentGoneError;
        break;

    case 418:               // I'm a teapot
        code = QNetworkReply::ProtocolInvalidOperationError;
        break;

    case 500:               // Internal Server Error
        code = QNetworkReply::InternalServerError;
        break;

    case 501:               // Server does not support this functionality
        code = QNetworkReply::OperationNotImplementedError;
        break;

    case 503:               // Service unavailable
        code = QNetworkReply::ServiceUnavailableError;
        break;

    default:
        if (httpStatusCode > 500) {
            // some kind of server error
            code = QNetworkReply::UnknownServerError;
        } else if (httpStatusCode >= 400) {
            // content error we did not handle above
            code = QNetworkReply::UnknownContentError;
        } else {
            qWarning("QNetworkAccess: got HTTP status code %d which is not expected from url: \"%s\"",
                     httpStatusCode, qPrintable(url.toString()));
            code = QNetworkReply::ProtocolFailure;
        }
    }

    return code;
}


static QByteArray makeCacheKey(QUrl &url, QNetworkProxy *proxy, const QString &peerVerifyName)
{
    QString result;
    QUrl copy = url;
    QString scheme = copy.scheme();
    bool isEncrypted = scheme == "https"_L1 || scheme == "preconnect-https"_L1;
    copy.setPort(copy.port(isEncrypted ? 443 : 80));
    if (scheme == "preconnect-http"_L1)
        copy.setScheme("http"_L1);
    else if (scheme == "preconnect-https"_L1)
        copy.setScheme("https"_L1);
    result = copy.toString(QUrl::RemoveUserInfo | QUrl::RemovePath |
                           QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::FullyEncoded);

#ifndef QT_NO_NETWORKPROXY
    if (proxy && proxy->type() != QNetworkProxy::NoProxy) {
        QUrl key;

        switch (proxy->type()) {
        case QNetworkProxy::Socks5Proxy:
            key.setScheme("proxy-socks5"_L1);
            break;

        case QNetworkProxy::HttpProxy:
        case QNetworkProxy::HttpCachingProxy:
            key.setScheme("proxy-http"_L1);
            break;

        default:
            break;
        }

        if (!key.scheme().isEmpty()) {
            const QByteArray obfuscatedPassword = QCryptographicHash::hash(proxy->password().toUtf8(),
                                                                           QCryptographicHash::Sha1).toHex();
            key.setUserName(proxy->user());
            key.setPassword(QString::fromUtf8(obfuscatedPassword));
            key.setHost(proxy->hostName());
            key.setPort(proxy->port());
            key.setQuery(result);
            result = key.toString(QUrl::FullyEncoded);
        }
    }
#else
    Q_UNUSED(proxy);
#endif
    if (!peerVerifyName.isEmpty())
        result += u':' + peerVerifyName;
    return "http-connection:" + std::move(result).toLatin1();
}

class QNetworkAccessCachedHttpConnection: public QHttpNetworkConnection,
                                      public QNetworkAccessCache::CacheableObject
{
    // Q_OBJECT
public:
    QNetworkAccessCachedHttpConnection(quint16 connectionCount, const QString &hostName, quint16 port, bool encrypt,
                                       QHttpNetworkConnection::ConnectionType connectionType)
        : QHttpNetworkConnection(connectionCount, hostName, port, encrypt, /*parent=*/nullptr, connectionType)
        ,CacheableObject(Option::Expires | Option::Shareable)
    {

    }

    virtual void dispose() override
    {
#if 0  // sample code; do this right with the API
        Q_ASSERT(!isWorking());
#endif
        delete this;
    }
};


QThreadStorage<QNetworkAccessCache *> QHttpThreadDelegate::connections;


QHttpThreadDelegate::~QHttpThreadDelegate()
{
    // It could be that the main thread has asked us to shut down, so we need to delete the HTTP reply
    if (httpReply) {
        delete httpReply;
    }

    // Get the object cache that stores our QHttpNetworkConnection objects
    // and release the entry for this QHttpNetworkConnection
    if (connections.hasLocalData() && !cacheKey.isEmpty()) {
        connections.localData()->releaseEntry(cacheKey);
    }
}


QHttpThreadDelegate::QHttpThreadDelegate(QObject *parent) :
    QObject(parent)
    , ssl(false)
    , downloadBufferMaximumSize(0)
    , readBufferMaxSize(0)
    , bytesEmitted(0)
    , pendingDownloadData()
    , pendingDownloadProgress()
    , synchronous(false)
    , connectionCacheExpiryTimeoutSeconds(-1)
    , incomingStatusCode(0)
    , isPipeliningUsed(false)
    , isHttp2Used(false)
    , incomingContentLength(-1)
    , removedContentLength(-1)
    , incomingErrorCode(QNetworkReply::NoError)
    , downloadBuffer()
    , httpConnection(nullptr)
    , httpReply(nullptr)
    , synchronousRequestLoop(nullptr)
{
}

// This is invoked as BlockingQueuedConnection from QNetworkAccessHttpBackend in the user thread
void QHttpThreadDelegate::startRequestSynchronously()
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::startRequestSynchronously() thread=" << QThread::currentThreadId();
#endif
    synchronous = true;

    QEventLoop synchronousRequestLoop;
    QScopedValueRollback<QEventLoop*> guard(this->synchronousRequestLoop, &synchronousRequestLoop);

    // Worst case timeout
    QTimer::singleShot(30*1000, this, SLOT(abortRequest()));

    QMetaObject::invokeMethod(this, "startRequest", Qt::QueuedConnection);
    synchronousRequestLoop.exec();

    connections.localData()->releaseEntry(cacheKey);
    connections.setLocalData(nullptr);

#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::startRequestSynchronously() thread=" << QThread::currentThreadId() << "finished";
#endif
}


// This is invoked as QueuedConnection from QNetworkAccessHttpBackend in the user thread
void QHttpThreadDelegate::startRequest()
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::startRequest() thread=" << QThread::currentThreadId();
#endif
    // Check QThreadStorage for the QNetworkAccessCache
    // If not there, create this connection cache
    if (!connections.hasLocalData()) {
        connections.setLocalData(new QNetworkAccessCache());
    }

    // check if we have an open connection to this host
    QUrl urlCopy = httpRequest.url();
    urlCopy.setPort(urlCopy.port(ssl ? 443 : 80));

    QHttpNetworkConnection::ConnectionType connectionType
        = httpRequest.isHTTP2Allowed() ? QHttpNetworkConnection::ConnectionTypeHTTP2
                                       : QHttpNetworkConnection::ConnectionTypeHTTP;
    if (httpRequest.isHTTP2Direct()) {
        Q_ASSERT(!httpRequest.isHTTP2Allowed());
        connectionType = QHttpNetworkConnection::ConnectionTypeHTTP2Direct;
    }

    // Use HTTP/1.1 if h2c is not allowed and we would otherwise choose to use it
    if (!ssl && connectionType == QHttpNetworkConnection::ConnectionTypeHTTP2
        && !httpRequest.isH2cAllowed()) {
        connectionType = QHttpNetworkConnection::ConnectionTypeHTTP;
    }

#if QT_CONFIG(ssl)
    // See qnetworkreplyhttpimpl, delegate's initialization code.
    Q_ASSERT(!ssl || incomingSslConfiguration.data());
#endif // QT_CONFIG(ssl)

    const bool isH2 = httpRequest.isHTTP2Allowed() || httpRequest.isHTTP2Direct();
    if (isH2) {
#if QT_CONFIG(ssl)
        if (ssl) {
            if (!httpRequest.isHTTP2Direct()) {
                QList<QByteArray> protocols;
                protocols << QSslConfiguration::ALPNProtocolHTTP2
                          << QSslConfiguration::NextProtocolHttp1_1;
                incomingSslConfiguration->setAllowedNextProtocols(protocols);
            }
            urlCopy.setScheme(QStringLiteral("h2s"));
        } else
#endif // QT_CONFIG(ssl)
        {
            urlCopy.setScheme(QStringLiteral("h2"));
        }
    }

#ifndef QT_NO_NETWORKPROXY
    if (transparentProxy.type() != QNetworkProxy::NoProxy)
        cacheKey = makeCacheKey(urlCopy, &transparentProxy, httpRequest.peerVerifyName());
    else if (cacheProxy.type() != QNetworkProxy::NoProxy)
        cacheKey = makeCacheKey(urlCopy, &cacheProxy, httpRequest.peerVerifyName());
    else
#endif
        cacheKey = makeCacheKey(urlCopy, nullptr, httpRequest.peerVerifyName());

    // the http object is actually a QHttpNetworkConnection
    httpConnection = static_cast<QNetworkAccessCachedHttpConnection *>(connections.localData()->requestEntryNow(cacheKey));
    if (!httpConnection) {
        // no entry in cache; create an object
        // the http object is actually a QHttpNetworkConnection
        httpConnection = new QNetworkAccessCachedHttpConnection(http1Parameters.numberOfConnectionsPerHost(), urlCopy.host(), urlCopy.port(), ssl,
                                                                connectionType);
        if (connectionType == QHttpNetworkConnection::ConnectionTypeHTTP2
            || connectionType == QHttpNetworkConnection::ConnectionTypeHTTP2Direct) {
            httpConnection->setHttp2Parameters(http2Parameters);
        }
#ifndef QT_NO_SSL
        // Set the QSslConfiguration from this QNetworkRequest.
        if (ssl)
            httpConnection->setSslConfiguration(*incomingSslConfiguration);
#endif

#ifndef QT_NO_NETWORKPROXY
        httpConnection->setTransparentProxy(transparentProxy);
        httpConnection->setCacheProxy(cacheProxy);
#endif
        httpConnection->setPeerVerifyName(httpRequest.peerVerifyName());
        // cache the QHttpNetworkConnection corresponding to this cache key
        connections.localData()->addEntry(cacheKey, httpConnection, connectionCacheExpiryTimeoutSeconds);
    } else {
        if (httpRequest.withCredentials()) {
            QNetworkAuthenticationCredential credential = authenticationManager->fetchCachedCredentials(httpRequest.url(), nullptr);
            if (!credential.user.isEmpty() && !credential.password.isEmpty()) {
                QAuthenticator auth;
                auth.setUser(credential.user);
                auth.setPassword(credential.password);
                httpConnection->d_func()->copyCredentials(-1, &auth, false);
            }
        }
    }

    // Send the request to the connection
    httpReply = httpConnection->sendRequest(httpRequest);
    httpReply->setParent(this);

    // Connect the reply signals that we need to handle and then forward
    if (synchronous) {
        connect(httpReply,SIGNAL(headerChanged()), this, SLOT(synchronousHeaderChangedSlot()));
        connect(httpReply,SIGNAL(finished()), this, SLOT(synchronousFinishedSlot()));
        connect(httpReply,SIGNAL(finishedWithError(QNetworkReply::NetworkError,QString)),
                this, SLOT(synchronousFinishedWithErrorSlot(QNetworkReply::NetworkError,QString)));

        connect(httpReply, SIGNAL(authenticationRequired(QHttpNetworkRequest,QAuthenticator*)),
                this, SLOT(synchronousAuthenticationRequiredSlot(QHttpNetworkRequest,QAuthenticator*)));
#ifndef QT_NO_NETWORKPROXY
        connect(httpReply, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                this, SLOT(synchronousProxyAuthenticationRequiredSlot(QNetworkProxy,QAuthenticator*)));
#endif

        // Don't care about ignored SSL errors for now in the synchronous HTTP case.
    } else if (!synchronous) {
        connect(httpReply,SIGNAL(socketStartedConnecting()), this, SIGNAL(socketStartedConnecting()));
        connect(httpReply,SIGNAL(requestSent()), this, SIGNAL(requestSent()));
        connect(httpReply,SIGNAL(headerChanged()), this, SLOT(headerChangedSlot()));
        connect(httpReply,SIGNAL(finished()), this, SLOT(finishedSlot()));
        connect(httpReply,SIGNAL(finishedWithError(QNetworkReply::NetworkError,QString)),
                this, SLOT(finishedWithErrorSlot(QNetworkReply::NetworkError,QString)));
        // some signals are only interesting when normal asynchronous style is used
        connect(httpReply,SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
        connect(httpReply,SIGNAL(dataReadProgress(qint64,qint64)), this, SLOT(dataReadProgressSlot(qint64,qint64)));
#ifndef QT_NO_SSL
        connect(httpReply,SIGNAL(encrypted()), this, SLOT(encryptedSlot()));
        connect(httpReply,SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrorsSlot(QList<QSslError>)));
        connect(httpReply,SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)),
                this, SLOT(preSharedKeyAuthenticationRequiredSlot(QSslPreSharedKeyAuthenticator*)));
#endif

        // In the asynchronous HTTP case we can just forward those signals
        // Connect the reply signals that we can directly forward
        connect(httpReply, SIGNAL(authenticationRequired(QHttpNetworkRequest,QAuthenticator*)),
                this, SIGNAL(authenticationRequired(QHttpNetworkRequest,QAuthenticator*)));
#ifndef QT_NO_NETWORKPROXY
        connect(httpReply, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                this, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
#endif
    }

    connect(httpReply, SIGNAL(cacheCredentials(QHttpNetworkRequest,QAuthenticator*)),
            this, SLOT(cacheCredentialsSlot(QHttpNetworkRequest,QAuthenticator*)));
    if (httpReply->errorCode() != QNetworkReply::NoError) {
        if (synchronous)
            synchronousFinishedWithErrorSlot(httpReply->errorCode(), httpReply->errorString());
        else
            finishedWithErrorSlot(httpReply->errorCode(), httpReply->errorString());
    }
}

// This gets called from the user thread or by the synchronous HTTP timeout timer
void QHttpThreadDelegate::abortRequest()
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::abortRequest() thread=" << QThread::currentThreadId() << "sync=" << synchronous;
#endif
    if (httpReply) {
        httpReply->abort();
        delete httpReply;
        httpReply = nullptr;
    }

    // Got aborted by the timeout timer
    if (synchronous) {
        incomingErrorCode = QNetworkReply::TimeoutError;
        QMetaObject::invokeMethod(synchronousRequestLoop, "quit", Qt::QueuedConnection);
    } else {
        //only delete this for asynchronous mode or QNetworkAccessHttpBackend will crash - see QNetworkAccessHttpBackend::postRequest()
        this->deleteLater();
    }
}

void QHttpThreadDelegate::readBufferSizeChanged(qint64 size)
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::readBufferSizeChanged() size " << size;
#endif
    if (httpReply) {
        httpReply->setDownstreamLimited(size > 0);
        httpReply->setReadBufferSize(size);
        readBufferMaxSize = size;
    }
}

void QHttpThreadDelegate::readBufferFreed(qint64 size)
{
    if (readBufferMaxSize) {
        bytesEmitted -= size;

        QMetaObject::invokeMethod(this, "readyReadSlot", Qt::QueuedConnection);
    }
}

void QHttpThreadDelegate::readyReadSlot()
{
    if (!httpReply)
        return;

    // Don't do in zerocopy case
    if (!downloadBuffer.isNull())
        return;

    if (readBufferMaxSize) {
        if (bytesEmitted < readBufferMaxSize) {
            qint64 sizeEmitted = 0;
            while (httpReply->readAnyAvailable() && (sizeEmitted < (readBufferMaxSize-bytesEmitted))) {
                if (httpReply->sizeNextBlock() > (readBufferMaxSize-bytesEmitted)) {
                    sizeEmitted = readBufferMaxSize-bytesEmitted;
                    bytesEmitted += sizeEmitted;
                    pendingDownloadData->fetchAndAddRelease(1);
                    emit downloadData(httpReply->read(sizeEmitted));
                } else {
                    sizeEmitted = httpReply->sizeNextBlock();
                    bytesEmitted += sizeEmitted;
                    pendingDownloadData->fetchAndAddRelease(1);
                    emit downloadData(httpReply->readAny());
                }
            }
        } else {
            // We need to wait until we empty data from the read buffer in the reply.
        }

    } else {
        while (httpReply->readAnyAvailable()) {
            pendingDownloadData->fetchAndAddRelease(1);
            emit downloadData(httpReply->readAny());
        }
    }
}

void QHttpThreadDelegate::finishedSlot()
{
    if (!httpReply)
        return;

#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::finishedSlot() thread=" << QThread::currentThreadId() << "result=" << httpReply->statusCode();
#endif

    // If there is still some data left emit that now
    while (httpReply->readAnyAvailable()) {
        pendingDownloadData->fetchAndAddRelease(1);
        emit downloadData(httpReply->readAny());
    }

#ifndef QT_NO_SSL
    if (ssl)
        emit sslConfigurationChanged(httpReply->sslConfiguration());
#endif

    if (httpReply->statusCode() >= 400) {
            // it's an error reply
            QString msg = QLatin1StringView(QT_TRANSLATE_NOOP("QNetworkReply",
                                                              "Error transferring %1 - server replied: %2"));
            msg = msg.arg(httpRequest.url().toString(), httpReply->reasonPhrase());
            emit error(statusCodeFromHttp(httpReply->statusCode(), httpRequest.url()), msg);
        }

    if (httpRequest.isFollowRedirects() && httpReply->isRedirecting())
        emit redirected(httpReply->redirectUrl(), httpReply->statusCode(), httpReply->request().redirectCount() - 1);

    emit downloadFinished();

    QMetaObject::invokeMethod(httpReply, "deleteLater", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
    httpReply = nullptr;
}

void QHttpThreadDelegate::synchronousFinishedSlot()
{
    if (!httpReply)
        return;

#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::synchronousFinishedSlot() thread=" << QThread::currentThreadId() << "result=" << httpReply->statusCode();
#endif
    if (httpReply->statusCode() >= 400) {
            // it's an error reply
            QString msg = QLatin1StringView(QT_TRANSLATE_NOOP("QNetworkReply",
                                                              "Error transferring %1 - server replied: %2"));
            incomingErrorDetail = msg.arg(httpRequest.url().toString(), httpReply->reasonPhrase());
            incomingErrorCode = statusCodeFromHttp(httpReply->statusCode(), httpRequest.url());
    }

    isCompressed = httpReply->isCompressed();
    synchronousDownloadData = httpReply->readAll();

    QMetaObject::invokeMethod(httpReply, "deleteLater", Qt::QueuedConnection);
    QMetaObject::invokeMethod(synchronousRequestLoop, "quit", Qt::QueuedConnection);
    httpReply = nullptr;
}

void QHttpThreadDelegate::finishedWithErrorSlot(QNetworkReply::NetworkError errorCode, const QString &detail)
{
    if (!httpReply)
        return;

#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::finishedWithErrorSlot() thread=" << QThread::currentThreadId() << "error=" << errorCode << detail;
#endif

#ifndef QT_NO_SSL
    if (ssl)
        emit sslConfigurationChanged(httpReply->sslConfiguration());
#endif
    emit error(errorCode,detail);
    emit downloadFinished();


    QMetaObject::invokeMethod(httpReply, "deleteLater", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
    httpReply = nullptr;
}


void QHttpThreadDelegate::synchronousFinishedWithErrorSlot(QNetworkReply::NetworkError errorCode, const QString &detail)
{
    if (!httpReply)
        return;

#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::synchronousFinishedWithErrorSlot() thread=" << QThread::currentThreadId() << "error=" << errorCode << detail;
#endif
    incomingErrorCode = errorCode;
    incomingErrorDetail = detail;

    synchronousDownloadData = httpReply->readAll();

    QMetaObject::invokeMethod(httpReply, "deleteLater", Qt::QueuedConnection);
    QMetaObject::invokeMethod(synchronousRequestLoop, "quit", Qt::QueuedConnection);
    httpReply = nullptr;
}

void QHttpThreadDelegate::headerChangedSlot()
{
    if (!httpReply)
        return;

#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::headerChangedSlot() thread=" << QThread::currentThreadId();
#endif

#ifndef QT_NO_SSL
    if (ssl)
        emit sslConfigurationChanged(httpReply->sslConfiguration());
#endif

    // Is using a zerocopy buffer allowed by user and possible with this reply?
    if (httpReply->supportsUserProvidedDownloadBuffer()
        && (downloadBufferMaximumSize > 0) && (httpReply->contentLength() <= downloadBufferMaximumSize)) {
        char *buf = new (std::nothrow) char[httpReply->contentLength()];
        // in out of memory situations, don't use downloadBuffer.
        if (buf) {
            downloadBuffer = QSharedPointer<char>(buf, [](auto p) { delete[] p; });
            httpReply->setUserProvidedDownloadBuffer(buf);
        }
    }

    // We fetch this into our own
    incomingHeaders = httpReply->header();
    incomingStatusCode = httpReply->statusCode();
    incomingReasonPhrase = httpReply->reasonPhrase();
    isPipeliningUsed = httpReply->isPipeliningUsed();
    incomingContentLength = httpReply->contentLength();
    removedContentLength = httpReply->removedContentLength();
    isHttp2Used = httpReply->isHttp2Used();
    isCompressed = httpReply->isCompressed();

    emit downloadMetaData(incomingHeaders,
                          incomingStatusCode,
                          incomingReasonPhrase,
                          isPipeliningUsed,
                          downloadBuffer,
                          incomingContentLength,
                          removedContentLength,
                          isHttp2Used,
                          isCompressed);
}

void QHttpThreadDelegate::synchronousHeaderChangedSlot()
{
    if (!httpReply)
        return;

#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::synchronousHeaderChangedSlot() thread=" << QThread::currentThreadId();
#endif
    // Store the information we need in this object, the QNetworkAccessHttpBackend will later read it
    incomingHeaders = httpReply->header();
    incomingStatusCode = httpReply->statusCode();
    incomingReasonPhrase = httpReply->reasonPhrase();
    isPipeliningUsed = httpReply->isPipeliningUsed();
    isHttp2Used = httpReply->isHttp2Used();
    incomingContentLength = httpReply->contentLength();
}


void QHttpThreadDelegate::dataReadProgressSlot(qint64 done, qint64 total)
{
    // If we don't have a download buffer don't attempt to go this codepath
    // It is not used by QNetworkAccessHttpBackend
    if (downloadBuffer.isNull())
        return;

    pendingDownloadProgress->fetchAndAddRelease(1);
    emit downloadProgress(done, total);
}

void QHttpThreadDelegate::cacheCredentialsSlot(const QHttpNetworkRequest &request, QAuthenticator *authenticator)
{
    authenticationManager->cacheCredentials(request.url(), authenticator);
}


#ifndef QT_NO_SSL
void QHttpThreadDelegate::encryptedSlot()
{
    if (!httpReply)
        return;

    emit sslConfigurationChanged(httpReply->sslConfiguration());
    emit encrypted();
}

void QHttpThreadDelegate::sslErrorsSlot(const QList<QSslError> &errors)
{
    if (!httpReply)
        return;

    emit sslConfigurationChanged(httpReply->sslConfiguration());

    bool ignoreAll = false;
    QList<QSslError> specificErrors;
    emit sslErrors(errors, &ignoreAll, &specificErrors);
    if (ignoreAll)
        httpReply->ignoreSslErrors();
    if (!specificErrors.isEmpty())
        httpReply->ignoreSslErrors(specificErrors);
}

void QHttpThreadDelegate::preSharedKeyAuthenticationRequiredSlot(QSslPreSharedKeyAuthenticator *authenticator)
{
    if (!httpReply)
        return;

    emit preSharedKeyAuthenticationRequired(authenticator);
}
#endif

void QHttpThreadDelegate::synchronousAuthenticationRequiredSlot(const QHttpNetworkRequest &request, QAuthenticator *a)
{
    if (!httpReply)
        return;

    Q_UNUSED(request);
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::synchronousAuthenticationRequiredSlot() thread=" << QThread::currentThreadId();
#endif

    // Ask the credential cache
    QNetworkAuthenticationCredential credential = authenticationManager->fetchCachedCredentials(httpRequest.url(), a);
    if (!credential.isNull()) {
        a->setUser(credential.user);
        a->setPassword(credential.password);
    }

    // Disconnect this connection now since we only want to ask the authentication cache once.
    QObject::disconnect(httpReply, SIGNAL(authenticationRequired(QHttpNetworkRequest,QAuthenticator*)),
        this, SLOT(synchronousAuthenticationRequiredSlot(QHttpNetworkRequest,QAuthenticator*)));
}

#ifndef QT_NO_NETWORKPROXY
void  QHttpThreadDelegate::synchronousProxyAuthenticationRequiredSlot(const QNetworkProxy &p, QAuthenticator *a)
{
    if (!httpReply)
        return;

#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::synchronousProxyAuthenticationRequiredSlot() thread=" << QThread::currentThreadId();
#endif
    // Ask the credential cache
    QNetworkAuthenticationCredential credential = authenticationManager->fetchCachedProxyCredentials(p, a);
    if (!credential.isNull()) {
        a->setUser(credential.user);
        a->setPassword(credential.password);
    }

#ifndef QT_NO_NETWORKPROXY
    // Disconnect this connection now since we only want to ask the authentication cache once.
    QObject::disconnect(httpReply, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
        this, SLOT(synchronousProxyAuthenticationRequiredSlot(QNetworkProxy,QAuthenticator*)));
#endif
}

#endif

QT_END_NAMESPACE

#include "moc_qhttpthreaddelegate_p.cpp"
