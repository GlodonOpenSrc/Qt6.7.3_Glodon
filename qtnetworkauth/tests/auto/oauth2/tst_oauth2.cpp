// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest>

#ifndef QT_NO_SSL
#include <QSslKey>
#endif

#include <QtNetworkAuth/qabstractoauthreplyhandler.h>
#include <QtNetworkAuth/qoauth2authorizationcodeflow.h>

#include "webserver.h"
#include "tlswebserver.h"

using namespace Qt::StringLiterals;

class tst_OAuth2 : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void state();
    void getToken();
    void refreshToken();
    void getAndRefreshToken();
    void tokenRequestErrors();
    void authorizationErrors();
    void prepareRequest();
    void scope_data();
    void scope();
#ifndef QT_NO_SSL
    void setSslConfig();
    void tlsAuthentication();
#endif

private:
    QString testDataDir;
};

struct ReplyHandler : QAbstractOAuthReplyHandler
{
    QString callback() const override
    {
        return QLatin1String("test");
    }

    QAbstractOAuth::Error aTokenRequestError = QAbstractOAuth::Error::NoError;

    void networkReplyFinished(QNetworkReply *reply) override
    {
        QVariantMap data;
        const auto items = QUrlQuery(reply->readAll()).queryItems();
        for (const auto &pair : items)
            data.insert(pair.first, pair.second);

        if (aTokenRequestError == QAbstractOAuth::Error::NoError)
            emit tokensReceived(data);
        else
            emit tokenRequestErrorOccurred(aTokenRequestError, "a token request error");
    }

    void emitCallbackReceived(const QVariantMap &data)
    {
        Q_EMIT callbackReceived(data);
    }
};

void tst_OAuth2::initTestCase()
{
    // QLoggingCategory::setFilterRules(QStringLiteral("qt.networkauth* = true"));
    testDataDir = QFileInfo(QFINDTESTDATA("certs")).absolutePath();
    if (testDataDir.isEmpty())
        testDataDir = QCoreApplication::applicationDirPath();
    if (!testDataDir.endsWith(QLatin1String("/")))
        testDataDir += QLatin1String("/");
}

void tst_OAuth2::state()
{
    QOAuth2AuthorizationCodeFlow oauth2;
    oauth2.setAuthorizationUrl(QUrl{"authorizationUrl"_L1});
    oauth2.setAccessTokenUrl(QUrl{"accessTokenUrl"_L1});
    ReplyHandler replyHandler;
    oauth2.setReplyHandler(&replyHandler);
    QSignalSpy statePropertySpy(&oauth2, &QAbstractOAuth2::stateChanged);

    QString stateParameter;
    oauth2.setModifyParametersFunction(
        [&] (QAbstractOAuth::Stage, QMultiMap<QString, QVariant> *parameters) {
            stateParameter = parameters->value(u"state"_s).toString();
    });

    oauth2.grant();
    QVERIFY(!stateParameter.isEmpty()); // internally generated initial state used
    QCOMPARE(stateParameter, oauth2.state());

    // Test setting the 'state' property
    const QString simpleState = u"a_state"_s;
    oauth2.setState(simpleState);
    QCOMPARE(oauth2.state(), simpleState);
    QCOMPARE(statePropertySpy.size(), 1);
    QCOMPARE(statePropertySpy.at(0).at(0), simpleState);
    oauth2.grant();
    QCOMPARE(stateParameter, simpleState);

    // Test 'state' that requires encoding/decoding.
    // The 'state' value contains all allowed characters as defined by
    // https://datatracker.ietf.org/doc/html/rfc6749#appendix-A.5
    // state      = 1*VSCHAR
    // Where
    // VSCHAR     = %x20-7E
    const QString stateRequiringEncoding = u"! \"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"_s;
    const QString stateAsEncoded = u"%21+%22%23%24%25%26%27%28%29%2A%2B%2C-.%2F0123456789%3A%3B%3C%3D%3E%3F%40ABCDEFGHIJKLMNOPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D~"_s;
    oauth2.setState(stateRequiringEncoding);
    QCOMPARE(oauth2.state(), stateRequiringEncoding);
    oauth2.grant();
    QCOMPARE(stateParameter, stateAsEncoded);
    // Conclude authorization stage, and check that the 'state' which we returned as encoded
    // matches the original decoded state (ie. the status changes to TemporaryCredentialsReceived)
    replyHandler.emitCallbackReceived({{"code", "acode"}, {"state", stateAsEncoded}});
    QTRY_COMPARE(oauth2.status(), QAbstractOAuth::Status::TemporaryCredentialsReceived);
}

void tst_OAuth2::authorizationErrors()
{
    // This tests failures in authorization stage. For this test we don't need a web server
    // as we emit the final (failing) callbackReceived directly.
    // Helper to catch the expected warning messages:
    constexpr auto expectWarning = [](){
        static const QRegularExpression authStageWarning{"Authorization stage:.*"};
        QTest::ignoreMessage(QtWarningMsg, authStageWarning);
    };

    QOAuth2AuthorizationCodeFlow oauth2;
    oauth2.setAuthorizationUrl(QUrl{"authorization"_L1});
    oauth2.setAccessTokenUrl(QUrl{"accessToken"_L1});
    ReplyHandler replyHandler;
    oauth2.setReplyHandler(&replyHandler);

    QVariantMap callbackParameters;
    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            &oauth2, [&](const QUrl& /* url */) {
        replyHandler.emitCallbackReceived(callbackParameters);
    });

    QSignalSpy requestFailedSpy(&oauth2, &QAbstractOAuth::requestFailed);
    QSignalSpy errorSpy(&oauth2, &QAbstractOAuth2::error);
    QSignalSpy statusSpy(&oauth2, &QAbstractOAuth2::statusChanged);
    auto clearSpies = [&](){
        requestFailedSpy.clear();
        errorSpy.clear();
        statusSpy.clear();
    };

    // Test error response from the authorization server (RFC 6749 section 5.2)
    callbackParameters = {{"error"_L1, "invalid_grant"_L1},
                          {"error_description"_L1, "The error description"_L1},
                          {"error_uri"_L1, "The error URI"_L1}};
    expectWarning();
    oauth2.grant();
    QTRY_COMPARE(errorSpy.count(), 1);
    QTRY_COMPARE(requestFailedSpy.count(), 1);
    QCOMPARE(errorSpy.first().at(0).toString(), "invalid_grant"_L1);
    QCOMPARE(errorSpy.first().at(1).toString(), "The error description"_L1);
    QCOMPARE(errorSpy.first().at(2).toString(), "The error URI"_L1);
    QCOMPARE(requestFailedSpy.first().at(0).value<QAbstractOAuth::Error>(),
             QAbstractOAuth::Error::ServerError);
    QVERIFY(statusSpy.isEmpty());
    QCOMPARE(oauth2.status(), QAbstractOAuth::Status::NotAuthenticated);

    // Test not providing authorization code
    clearSpies();
    callbackParameters = {{"state"_L1, "thestate"_L1}};
    expectWarning();
    oauth2.grant();
    QTRY_COMPARE(requestFailedSpy.count(), 1);
    QCOMPARE(requestFailedSpy.first().at(0).value<QAbstractOAuth::Error>(),
             QAbstractOAuth::Error::OAuthTokenNotFoundError);
    QCOMPARE(errorSpy.count(), 0);
    QVERIFY(statusSpy.isEmpty());
    QCOMPARE(oauth2.status(), QAbstractOAuth::Status::NotAuthenticated);

    // Test not providing a state
    clearSpies();
    callbackParameters = {{"code"_L1, "thecode"_L1}};
    expectWarning();
    oauth2.grant();
    QTRY_COMPARE(requestFailedSpy.count(), 1);
    QCOMPARE(requestFailedSpy.first().at(0).value<QAbstractOAuth::Error>(),
             QAbstractOAuth::Error::ServerError);
    QCOMPARE(errorSpy.count(), 0);
    QVERIFY(statusSpy.isEmpty());
    QCOMPARE(oauth2.status(), QAbstractOAuth::Status::NotAuthenticated);

    // Test state mismatch (here we use "thestate" while the actual, expected, state is a
    // random generated string varying each run
    clearSpies();
    callbackParameters = {{"code"_L1, "thecode"_L1}, {"state"_L1, "thestate"_L1}};
    expectWarning();
    oauth2.grant();
    QTRY_COMPARE(requestFailedSpy.count(), 1);
    QCOMPARE(requestFailedSpy.first().at(0).value<QAbstractOAuth::Error>(),
             QAbstractOAuth::Error::ServerError);
    QCOMPARE(errorSpy.count(), 0);
    QVERIFY(statusSpy.isEmpty());
    QCOMPARE(oauth2.status(), QAbstractOAuth::Status::NotAuthenticated);
}

void tst_OAuth2::getToken()
{
    WebServer webServer([](const WebServer::HttpRequest &request, QTcpSocket *socket) {
        if (request.url.path() == QLatin1String("/accessToken")) {
            const QString text = "access_token=token&token_type=bearer";
            const QByteArray replyMessage {
                "HTTP/1.0 200 OK\r\n"
                "Content-Type: application/x-www-form-urlencoded; charset=\"utf-8\"\r\n"
                "Content-Length: " + QByteArray::number(text.size()) + "\r\n\r\n"
                + text.toUtf8()
            };
            socket->write(replyMessage);
        }
    });
    QOAuth2AuthorizationCodeFlow oauth2;
    oauth2.setAuthorizationUrl(webServer.url(QLatin1String("authorization")));
    oauth2.setAccessTokenUrl(webServer.url(QLatin1String("accessToken")));
    ReplyHandler replyHandler;
    oauth2.setReplyHandler(&replyHandler);
    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            this, [&](const QUrl &url) {
        const QUrlQuery query(url.query());
        replyHandler.emitCallbackReceived(QVariantMap {
                                               { QLatin1String("code"), QLatin1String("test") },
                                               { QLatin1String("state"),
                                                 query.queryItemValue(QLatin1String("state")) }
                                           });
    });
    QSignalSpy grantedSpy(&oauth2, &QOAuth2AuthorizationCodeFlow::granted);
    oauth2.grant();
    QTRY_COMPARE(grantedSpy.size(), 1);
    QCOMPARE(oauth2.token(), QLatin1String("token"));
}

void tst_OAuth2::refreshToken()
{
    WebServer webServer([](const WebServer::HttpRequest &request, QTcpSocket *socket) {
        if (request.url.path() == QLatin1String("/accessToken")) {
            const QString text = "access_token=token&token_type=bearer";
            const QByteArray replyMessage {
                "HTTP/1.0 200 OK\r\n"
                "Content-Type: application/x-www-form-urlencoded; charset=\"utf-8\"\r\n"
                "Content-Length: " + QByteArray::number(text.size()) + "\r\n\r\n"
                + text.toUtf8()
            };
            socket->write(replyMessage);
        }
    });
    QOAuth2AuthorizationCodeFlow oauth2;
    oauth2.setAccessTokenUrl(webServer.url(QLatin1String("accessToken")));
    ReplyHandler replyHandler;
    oauth2.setReplyHandler(&replyHandler);
    oauth2.setRefreshToken(QLatin1String("refresh_token"));
    QSignalSpy grantedSpy(&oauth2, &QOAuth2AuthorizationCodeFlow::granted);
    oauth2.refreshAccessToken();
    QTRY_COMPARE(grantedSpy.size(), 1);
    QCOMPARE(oauth2.token(), QLatin1String("token"));
}

void tst_OAuth2::getAndRefreshToken()
{
    // In this test we use the grant_type as a token to be able to
    // identify the token request from the token refresh.
    WebServer webServer([](const WebServer::HttpRequest &request, QTcpSocket *socket) {
        if (request.url.path() == QLatin1String("/accessToken")) {
            const QUrlQuery query(request.body);
            const QString format = QStringLiteral("access_token=%1&token_type=bearer&expires_in=1&"
                                                  "refresh_token=refresh_token");
            const auto text = format.arg(query.queryItemValue(QLatin1String("grant_type")));
            const QByteArray replyMessage {
                "HTTP/1.0 200 OK\r\n"
                "Content-Type: application/x-www-form-urlencoded; charset=\"utf-8\"\r\n"
                "Content-Length: " + QByteArray::number(text.size()) + "\r\n\r\n"
                + text.toUtf8()
            };
            socket->write(replyMessage);
        }
    });
    QOAuth2AuthorizationCodeFlow oauth2;
    oauth2.setAuthorizationUrl(webServer.url(QLatin1String("authorization")));
    oauth2.setAccessTokenUrl(webServer.url(QLatin1String("accessToken")));
    ReplyHandler replyHandler;
    oauth2.setReplyHandler(&replyHandler);
    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            this, [&](const QUrl &url) {
        const QUrlQuery query(url.query());
        replyHandler.emitCallbackReceived(QVariantMap {
                                              { QLatin1String("code"), QLatin1String("test") },
                                              { QLatin1String("state"),
                                                query.queryItemValue(QLatin1String("state")) }
                                          });
    });
    QSignalSpy grantedSpy(&oauth2, &QOAuth2AuthorizationCodeFlow::granted);
    oauth2.grant();
    QTRY_COMPARE(grantedSpy.size(), 1);
    QCOMPARE(oauth2.token(), QLatin1String("authorization_code"));
    grantedSpy.clear();
    oauth2.refreshAccessToken();
    QTRY_COMPARE(grantedSpy.size(), 1);
    QCOMPARE(oauth2.token(), QLatin1String("refresh_token"));
}

void tst_OAuth2::tokenRequestErrors()
{
    // This test tests the token acquisition and refreshing errors.
    // Helper to catch the expected warning messages:
    constexpr auto expectWarning = [](){
        static const QRegularExpression tokenWarning{"Token request failed:.*"};
        QTest::ignoreMessage(QtWarningMsg, tokenWarning);
    };

    QByteArray accessTokenResponse; // Varying reply for the auth server
    WebServer authServer([&](const WebServer::HttpRequest &request, QTcpSocket *socket) {
        if (request.url.path() == QLatin1String("/accessToken"))
            socket->write(accessTokenResponse);
    });

    QOAuth2AuthorizationCodeFlow oauth2;
    oauth2.setAuthorizationUrl(authServer.url(QLatin1String("authorization")));
    oauth2.setAccessTokenUrl(authServer.url(QLatin1String("accessToken")));

    ReplyHandler replyHandler;
    oauth2.setReplyHandler(&replyHandler);

    QSignalSpy requestFailedSpy(&oauth2, &QAbstractOAuth::requestFailed);
    QSignalSpy grantedSpy(&oauth2, &QOAuth2AuthorizationCodeFlow::granted);
    QSignalSpy statusSpy(&oauth2, &QAbstractOAuth2::statusChanged);
    auto clearSpies = [&](){
        requestFailedSpy.clear();
        grantedSpy.clear();
        statusSpy.clear();
    };

    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            &oauth2, [&](const QUrl &url) {
        // Successful authorization stage, after which we can test token requests.
        // For clarity: in these tests we omit browser interaction by directly triggering
        // the emission of replyhandler::callbackReceived() signal
        const QUrlQuery query(url.query());
        replyHandler.emitCallbackReceived(QVariantMap {
            { QLatin1String("code"), QLatin1String("test") },
            { QLatin1String("state"),
             query.queryItemValue(QLatin1String("state")) }
        });
    });

    // Check the initial state
    QVERIFY(requestFailedSpy.isEmpty());
    QVERIFY(grantedSpy.isEmpty());
    QVERIFY(statusSpy.isEmpty());
    QCOMPARE(oauth2.status(), QAbstractOAuth::Status::NotAuthenticated);

    // Try to get an access token with an invalid response
    accessTokenResponse = "an invalid response"_ba;
    expectWarning();
    oauth2.grant();
    QTRY_COMPARE(requestFailedSpy.size(), 1);
    QVERIFY(grantedSpy.isEmpty());
    QCOMPARE(statusSpy.size(), 1); // Authorization was successful so we get one signal
    QCOMPARE(oauth2.status(), QAbstractOAuth::Status::TemporaryCredentialsReceived);

    // Try to get an access token, but replyhandler indicates an error
    clearSpies();
    replyHandler.aTokenRequestError = QAbstractOAuth::Error::NetworkError;
    expectWarning();
    oauth2.grant();
    QTRY_COMPARE(requestFailedSpy.size(), 1);
    QVERIFY(grantedSpy.isEmpty());
    QCOMPARE(oauth2.status(), QAbstractOAuth::Status::TemporaryCredentialsReceived);

    // Make a successful access & refresh token acquisition
    replyHandler.aTokenRequestError = QAbstractOAuth::Error::NoError;
    clearSpies();
    accessTokenResponse =
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: application/x-www-form-urlencoded; charset=\"utf-8\"\r\n"
        "\r\n"
        "access_token=the_access_token&token_type=bearer&refresh_token=the_refresh_token"_ba;
    oauth2.grant();
    QTRY_COMPARE(grantedSpy.size(), 1);
    QCOMPARE(statusSpy.size(), 3);
    // First status change is going from TempCred back to NotAuthenticated
    QCOMPARE(statusSpy.takeFirst().at(0).value<QAbstractOAuth::Status>(),
             QAbstractOAuth::Status::NotAuthenticated);
    QCOMPARE(statusSpy.takeFirst().at(0).value<QAbstractOAuth::Status>(),
             QAbstractOAuth::Status::TemporaryCredentialsReceived);
    QCOMPARE(statusSpy.takeFirst().at(0).value<QAbstractOAuth::Status>(),
             QAbstractOAuth::Status::Granted);
    QVERIFY(requestFailedSpy.isEmpty());
    QCOMPARE(oauth2.status(), QAbstractOAuth::Status::Granted);
    QCOMPARE(oauth2.token(), u"the_access_token"_s);
    QCOMPARE(oauth2.refreshToken(), u"the_refresh_token"_s);

    // Successfully refresh access token
    clearSpies();
    oauth2.refreshAccessToken();
    QTRY_COMPARE(statusSpy.size(), 2);
    QCOMPARE(statusSpy.takeFirst().at(0).value<QAbstractOAuth::Status>(),
             QAbstractOAuth::Status::RefreshingToken);
    QCOMPARE(statusSpy.takeFirst().at(0).value<QAbstractOAuth::Status>(),
             QAbstractOAuth::Status::Granted);
    QCOMPARE(oauth2.status(), QAbstractOAuth::Status::Granted);
    QVERIFY(requestFailedSpy.isEmpty());

    // Failed access token refresh
    clearSpies();
    replyHandler.aTokenRequestError = QAbstractOAuth::Error::ServerError;
    expectWarning();
    oauth2.refreshAccessToken();
    QTRY_COMPARE(statusSpy.size(), 2);
    QCOMPARE(statusSpy.takeFirst().at(0).value<QAbstractOAuth::Status>(),
             QAbstractOAuth::Status::RefreshingToken);
    QCOMPARE(statusSpy.takeFirst().at(0).value<QAbstractOAuth::Status>(),
             QAbstractOAuth::Status::Granted); // back to granted since we have an access token
    QCOMPARE(requestFailedSpy.size(), 1);
    QCOMPARE(oauth2.status(), QAbstractOAuth::Status::Granted);
}

void tst_OAuth2::prepareRequest()
{
    QOAuth2AuthorizationCodeFlow oauth2;
    oauth2.setToken(QStringLiteral("access_token"));

    QNetworkRequest request(QUrl("http://localhost"));
    oauth2.prepareRequest(&request, QByteArray());
    QCOMPARE(request.rawHeader("Authorization"), QByteArray("Bearer access_token"));
}

void tst_OAuth2::scope_data()
{
    static const auto requestedScope = u"requested"_s;
    QTest::addColumn<QString>("scope");
    QTest::addColumn<QString>("granted_scope");
    QTest::addColumn<QString>("expected_scope");

    QTest::addRow("scope_returned") << requestedScope << requestedScope << requestedScope;
    QTest::addRow("differing_scope_returned") << requestedScope << u"granted"_s << u"granted"_s;
    QTest::addRow("empty_scope_returned") << requestedScope << u""_s << requestedScope;
}

void tst_OAuth2::scope()
{
    QFETCH(QString, scope);
    QFETCH(QString, granted_scope);
    QFETCH(QString, expected_scope);

    QOAuth2AuthorizationCodeFlow oauth2;
    QVERIFY(oauth2.scope().isEmpty());

    // Set the requested scope and verify it changes
    QSignalSpy scopeSpy(&oauth2, &QAbstractOAuth2::scopeChanged);
    oauth2.setScope(scope);
    QCOMPARE(scopeSpy.size(), 1);
    QCOMPARE(oauth2.scope(), scope);
    QCOMPARE(scopeSpy.at(0).at(0).toString(), scope);

    // Verify that empty authorization server 'scope' response doesn't overwrite the
    // requested scope, whereas a returned scope value does
    WebServer webServer([granted_scope](const WebServer::HttpRequest &request, QTcpSocket *socket) {
        if (request.url.path() == "/accessTokenUrl"_L1) {
            QString accessTokenResponseParams;
            accessTokenResponseParams += u"access_token=token&token_type=bearer"_s;
            if (!granted_scope.isEmpty())
                accessTokenResponseParams += u"&scope="_s + granted_scope;
            const QByteArray replyMessage {
                "HTTP/1.0 200 OK\r\n"
                "Content-Type: application/x-www-form-urlencoded; charset=\"utf-8\"\r\n"
                "Content-Length: "
                + QByteArray::number(accessTokenResponseParams.size()) + "\r\n\r\n"
                + accessTokenResponseParams.toUtf8()
            };
            socket->write(replyMessage);
        }
    });
    oauth2.setAuthorizationUrl(webServer.url("authorizationUrl"_L1));
    oauth2.setAccessTokenUrl(webServer.url("accessTokenUrl"_L1));
    oauth2.setState("a_state"_L1);
    ReplyHandler replyHandler;
    oauth2.setReplyHandler(&replyHandler);
    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            this, [&](const QUrl &) {
                replyHandler.emitCallbackReceived(QVariantMap {
                    { "code"_L1, "a_code"_L1 }, { "state"_L1, "a_state"_L1 },
        });
    });
    oauth2.grant();

    QTRY_COMPARE(oauth2.status(), QAbstractOAuth::Status::Granted);
    QCOMPARE(oauth2.scope(), expected_scope);
    if (!granted_scope.isEmpty() && (granted_scope != scope)) {
        QCOMPARE(scopeSpy.size(), 2);
        QCOMPARE(scopeSpy.at(1).at(0).toString(), expected_scope);
    } else {
        QCOMPARE(scopeSpy.size(), 1);
    }
}

#ifndef QT_NO_SSL
static QSslConfiguration createSslConfiguration(QString keyFileName, QString certificateFileName)
{
    QSslConfiguration configuration(QSslConfiguration::defaultConfiguration());

    QFile keyFile(keyFileName);
    if (keyFile.open(QIODevice::ReadOnly)) {
        QSslKey key(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        if (!key.isNull()) {
            configuration.setPrivateKey(key);
        } else {
            qCritical() << "Could not parse key: " << keyFileName;
        }
    } else {
        qCritical() << "Could not find key: " << keyFileName;
    }

    QList<QSslCertificate> localCert = QSslCertificate::fromPath(certificateFileName);
    if (!localCert.isEmpty() && !localCert.first().isNull()) {
        configuration.setLocalCertificate(localCert.first());
    } else {
        qCritical() << "Could not find certificate: " << certificateFileName;
    }

    configuration.setPeerVerifyMode(QSslSocket::VerifyPeer);

    return configuration;
}

void tst_OAuth2::setSslConfig()
{
    QOAuth2AuthorizationCodeFlow oauth2;
    QSignalSpy sslConfigSpy(&oauth2, &QAbstractOAuth2::sslConfigurationChanged);

    QVERIFY(sslConfigSpy.isValid());
    QCOMPARE(oauth2.sslConfiguration(), QSslConfiguration());
    QCOMPARE(sslConfigSpy.size(), 0);

    auto config = createSslConfiguration(testDataDir + "certs/selfsigned-server.key",
                                         testDataDir + "certs/selfsigned-server.crt");
    oauth2.setSslConfiguration(config);

    QCOMPARE(oauth2.sslConfiguration(), config);
    QCOMPARE(sslConfigSpy.size(), 1);

    // set same config - nothing happens
    oauth2.setSslConfiguration(config);
    QCOMPARE(sslConfigSpy.size(), 1);

    // change config
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    oauth2.setSslConfiguration(config);
    QCOMPARE(oauth2.sslConfiguration(), config);
    QCOMPARE(sslConfigSpy.size(), 2);
}

void tst_OAuth2::tlsAuthentication()
{
    if (!QSslSocket::supportsSsl())
        QSKIP("This test will fail because the backend does not support TLS");

    // erros may vary, depending on backend
    const QSet<QSslError::SslError> expectedErrors{ QSslError::SelfSignedCertificate,
                                                    QSslError::CertificateUntrusted,
                                                    QSslError::HostNameMismatch };
    auto serverConfig = createSslConfiguration(testDataDir + "certs/selfsigned-server.key",
                                               testDataDir + "certs/selfsigned-server.crt");
    TlsWebServer tlsServer([](const WebServer::HttpRequest &request, QTcpSocket *socket) {
        if (request.url.path() == QLatin1String("/accessToken")) {
            const QString text = "access_token=token&token_type=bearer";
            const QByteArray replyMessage {
                "HTTP/1.0 200 OK\r\n"
                "Content-Type: application/x-www-form-urlencoded; charset=\"utf-8\"\r\n"
                "Content-Length: " + QByteArray::number(text.size()) + "\r\n\r\n"
                + text.toUtf8()
            };
            socket->write(replyMessage);
        }
    }, serverConfig);
    tlsServer.setExpectedSslErrors(expectedErrors);

    auto clientConfig = createSslConfiguration(testDataDir + "certs/selfsigned-client.key",
                                               testDataDir + "certs/selfsigned-client.crt");
    QNetworkAccessManager nam;
    QOAuth2AuthorizationCodeFlow oauth2;
    oauth2.setNetworkAccessManager(&nam);
    oauth2.setSslConfiguration(clientConfig);
    oauth2.setAuthorizationUrl(tlsServer.url(QLatin1String("authorization")));
    oauth2.setAccessTokenUrl(tlsServer.url(QLatin1String("accessToken")));
    ReplyHandler replyHandler;
    oauth2.setReplyHandler(&replyHandler);
    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            this, [&](const QUrl &url) {
        const QUrlQuery query(url.query());
        replyHandler.emitCallbackReceived(QVariantMap {
                                               { QLatin1String("code"), QLatin1String("test") },
                                               { QLatin1String("state"),
                                                 query.queryItemValue(QLatin1String("state")) }
                                           });
    });
    connect(&nam, &QNetworkAccessManager::sslErrors, this,
        [&expectedErrors](QNetworkReply *r, const QList<QSslError> &errors) {
            QCOMPARE(errors.size(), 2);
            for (const auto &err : errors)
                QVERIFY(expectedErrors.contains(err.error()));
            r->ignoreSslErrors();
        });

    QSignalSpy grantedSpy(&oauth2, &QOAuth2AuthorizationCodeFlow::granted);
    oauth2.grant();
    QTRY_COMPARE(grantedSpy.size(), 1);
    QCOMPARE(oauth2.token(), QLatin1String("token"));
}
#endif // !QT_NO_SSL

QTEST_MAIN(tst_OAuth2)
#include "tst_oauth2.moc"
