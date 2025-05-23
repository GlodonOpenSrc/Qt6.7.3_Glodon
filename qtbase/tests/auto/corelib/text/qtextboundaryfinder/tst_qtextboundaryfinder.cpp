// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QScopedValueRollback>

#include <qtextboundaryfinder.h>
#include <qfile.h>
#include <qdebug.h>
#include <qlist.h>

#include <algorithm>

class tst_QTextBoundaryFinder : public QObject
{
    Q_OBJECT
private slots:
#ifdef QT_BUILD_INTERNAL
    void graphemeBoundariesDefault_data();
    void graphemeBoundariesDefault();
    void wordBoundariesDefault_data();
    void wordBoundariesDefault();
    void sentenceBoundariesDefault_data();
    void sentenceBoundariesDefault();
    void lineBoundariesDefault_data();
    void lineBoundariesDefault();
#endif

    void graphemeBoundaries_manual_data();
    void graphemeBoundaries_manual();

    void wordBoundaries_manual_data();
    void wordBoundaries_manual();
    void sentenceBoundaries_manual_data();
    void sentenceBoundaries_manual();
    void lineBoundaries_manual_data();
    void lineBoundaries_manual();

    void emptyText_data();
    void emptyText();
    void fastConstructor();
    void assignmentOperator();
    void isAtSoftHyphen_data();
    void isAtSoftHyphen();
};


QT_BEGIN_NAMESPACE
namespace QTest {

template<>
inline char *toString(const QTextBoundaryFinder::BoundaryReasons &flags)
{
    return qstrdup(QByteArray::number(int(flags)).constData());
}

template<>
inline char *toString(const QList<int> &list)
{
    QByteArray s;
    for (QList<int>::const_iterator it = list.constBegin(); it != list.constEnd(); ++it) {
        if (!s.isEmpty())
            s += ", ";
        s += QByteArray::number(*it);
    }
    s = "{ " + s + " }";
    return qstrdup(s.constData());
}

} // namespace QTest
QT_END_NAMESPACE

#ifdef QT_BUILD_INTERNAL
static void generateDataFromFile(const QString &fname)
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int> >("expectedBreakPositions");

    QString testFile = QFINDTESTDATA(fname);
    QVERIFY2(!testFile.isEmpty(), (fname.toLatin1() + QByteArray(" not found!")));
    QFile f(testFile);
    QVERIFY(f.exists());

    f.open(QIODevice::ReadOnly);

    int linenum = 0;
    while (!f.atEnd()) {
        linenum++;

        QByteArray line = f.readLine();
        if (line.startsWith('#'))
            continue;

        QString test = QString::fromUtf8(line);
        QString comments;
        int hash = test.indexOf('#');
        if (hash > 0) {
            comments = test.mid(hash + 1).simplified();
            test = test.left(hash);
        }

        QString testString;
        QList<int> expectedBreakPositions;
        const QStringList parts = test.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        for (const QString &part : parts) {
            if (part.size() == 1) {
                if (part.at(0).unicode() == 0xf7)
                    expectedBreakPositions.append(testString.size());
                else
                    QVERIFY(part.at(0).unicode() == 0xd7);
                continue;
            }
            bool ok = true;
            uint ucs4 = part.toInt(&ok, 16);
            QVERIFY(ok && ucs4 > 0);
            if (QChar::requiresSurrogates(ucs4)) {
                testString.append(QChar::highSurrogate(ucs4));
                testString.append(QChar::lowSurrogate(ucs4));
            } else {
                testString.append(QChar(ucs4));
            }
        }
        QVERIFY(!testString.isEmpty());
        QVERIFY(!expectedBreakPositions.isEmpty());

        if (!comments.isEmpty()) {
            const QStringList lst = comments.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
            comments.clear();
            for (const QString &part : lst) {
                if (part.size() == 1) {
                    if (part.at(0).unicode() == 0xf7)
                        comments += QLatin1Char('+');
                    else if (part.at(0).unicode() == 0xd7)
                        comments += QLatin1Char('x');
                    continue;
                }
                if (part.startsWith(QLatin1Char('(')) && part.endsWith(QLatin1Char(')')))
                    comments += part;
            }
        }

        const QByteArray nm = "line #" + QByteArray::number(linenum) + ": " + comments.toLatin1();
        QTest::newRow(nm.constData()) << testString << expectedBreakPositions;
    }
}
#endif

static void doTestData(const QString &testString, const QList<int> &expectedBreakPositions,
                       QTextBoundaryFinder::BoundaryType type,
                       QTextBoundaryFinder::BoundaryReasons reasons = QTextBoundaryFinder::BreakOpportunity)
{
    QVERIFY(!testString.isEmpty());

    QTextBoundaryFinder boundaryFinder(type, testString);

    // test toNextBoundary()
    {
        QList<int> actualBreakPositions;
        do {
            QVERIFY(boundaryFinder.isAtBoundary());
            if (boundaryFinder.boundaryReasons() & reasons)
                actualBreakPositions.append(boundaryFinder.position());
        } while (boundaryFinder.toNextBoundary() != -1);
        QCOMPARE(actualBreakPositions, expectedBreakPositions);
    }
    QCOMPARE(boundaryFinder.position(), -1);
    QVERIFY(!boundaryFinder.isAtBoundary());
    QVERIFY(boundaryFinder.boundaryReasons() == QTextBoundaryFinder::NotAtBoundary);

    // test toPreviousBoundary()
    {
        QList<int> expectedBreakPositionsRev = expectedBreakPositions;
        std::sort(expectedBreakPositionsRev.begin(), expectedBreakPositionsRev.end(), std::greater<int>());

        QList<int> actualBreakPositions;
        boundaryFinder.toEnd();
        do {
            QVERIFY(boundaryFinder.isAtBoundary());
            if (boundaryFinder.boundaryReasons() & reasons)
                actualBreakPositions.append(boundaryFinder.position());
        } while (boundaryFinder.toPreviousBoundary() != -1);
        QCOMPARE(actualBreakPositions, expectedBreakPositionsRev);
    }
    QCOMPARE(boundaryFinder.position(), -1);
    QVERIFY(!boundaryFinder.isAtBoundary());
    QVERIFY(boundaryFinder.boundaryReasons() == QTextBoundaryFinder::NotAtBoundary);

    // test boundaryReasons()
    for (int i = 0; i <= testString.size(); ++i) {
        boundaryFinder.setPosition(i);
        QCOMPARE(!!(boundaryFinder.boundaryReasons() & reasons), expectedBreakPositions.contains(i));
    }
}

#ifdef QT_BUILD_INTERNAL

QT_BEGIN_NAMESPACE
extern Q_AUTOTEST_EXPORT int qt_initcharattributes_default_algorithm_only;
QT_END_NAMESPACE

void tst_QTextBoundaryFinder::graphemeBoundariesDefault_data()
{
    generateDataFromFile("data/GraphemeBreakTest.txt");
}

void tst_QTextBoundaryFinder::graphemeBoundariesDefault()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    QScopedValueRollback<int> default_algorithm(qt_initcharattributes_default_algorithm_only);
    qt_initcharattributes_default_algorithm_only++;

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Grapheme);
}

void tst_QTextBoundaryFinder::wordBoundariesDefault_data()
{
    generateDataFromFile("data/WordBreakTest.txt");
}

void tst_QTextBoundaryFinder::wordBoundariesDefault()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    QScopedValueRollback<int> default_algorithm(qt_initcharattributes_default_algorithm_only);
    qt_initcharattributes_default_algorithm_only++;

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Word);
}

void tst_QTextBoundaryFinder::sentenceBoundariesDefault_data()
{
    generateDataFromFile("data/SentenceBreakTest.txt");
}

void tst_QTextBoundaryFinder::sentenceBoundariesDefault()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    QScopedValueRollback<int> default_algorithm(qt_initcharattributes_default_algorithm_only);
    qt_initcharattributes_default_algorithm_only++;

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Sentence);
}

void tst_QTextBoundaryFinder::lineBoundariesDefault_data()
{
    generateDataFromFile("data/LineBreakTest.txt");
}

void tst_QTextBoundaryFinder::lineBoundariesDefault()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    QScopedValueRollback<int> default_algorithm(qt_initcharattributes_default_algorithm_only);
    qt_initcharattributes_default_algorithm_only++;

    expectedBreakPositions.prepend(0); // ### QTBF generates a boundary at start of text
    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Line);
}
#endif // QT_BUILD_INTERNAL

void tst_QTextBoundaryFinder::graphemeBoundaries_manual_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int>>("expectedBreakPositions");

    {
        // QTBUG-94951
        QChar s[] = { QChar(0x2764), // U+2764 HEAVY BLACK HEART
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                      QChar(0xD83D), QChar(0xDCF2), // U+1F4F2 MOBILE PHONE WITH RIGHTWARDS ARROW AT LEFT
                      QChar(0xD83D), QChar(0xDCE9), // U+1F4E9 ENVELOPE WITH DOWNWARDS ARROW ABOVE
                    };
        QString testString(s, sizeof(s)/sizeof(s[0]));

        QList<int> expectedBreakPositions{0, 2, 4, 6};
        QTest::newRow("+EXTPICxEXT+EXTPIC+EXTPIC+") << testString << expectedBreakPositions;
    }

    {
        QChar s[] = { QChar(0x2764), // U+2764 HEAVY BLACK HEART
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                      QChar(0x2764), // U+2764 HEAVY BLACK HEART
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                    };
        QString testString(s, sizeof(s)/sizeof(s[0]));

        QList<int> expectedBreakPositions{0, 2, 4};
        QTest::newRow("+EXTPICxEXT+EXTPICxEXT+") << testString << expectedBreakPositions;
    }

    {
        QChar s[] = { QChar(0x2764), // U+2764 HEAVY BLACK HEART
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                      QChar(0x2764), // U+2764 HEAVY BLACK HEART
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                    };
        QString testString(s, sizeof(s)/sizeof(s[0]));

        QList<int> expectedBreakPositions{0, 4, 7};
        QTest::newRow("+EXTPICxEXTxEXTxEXT+EXTPICxEXTxEXT+") << testString << expectedBreakPositions;
    }

    {
        QChar s[] = { QChar(0x2764), // U+2764 HEAVY BLACK HEART
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                      QChar(0x200D), // U+200D ZERO WIDTH JOINER
                      QChar(0xD83D), QChar(0xDCF2), // U+1F4F2 MOBILE PHONE WITH RIGHTWARDS ARROW AT LEFT
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                    };
        QString testString(s, sizeof(s)/sizeof(s[0]));

        QList<int> expectedBreakPositions{0, 7};
        QTest::newRow("+EXTPICxEXTxEXTxZWJxEXTPICxEXTxEXT+") << testString << expectedBreakPositions;
    }

    {
        QChar s[] = { QChar(0x2764), // U+2764 HEAVY BLACK HEART
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                      QChar(0x200D), // U+200D ZERO WIDTH JOINER
                      QChar(0x0041), // U+0041 LATIN CAPITAL LETTER A
                      QChar(0xD83D), QChar(0xDCF2), // U+1F4F2 MOBILE PHONE WITH RIGHTWARDS ARROW AT LEFT
                    };
        QString testString(s, sizeof(s)/sizeof(s[0]));

        QList<int> expectedBreakPositions{0, 4, 5, 7};
        QTest::newRow("+EXTPICxEXTxEXTxZWJ+Any+EXTPIC+") << testString << expectedBreakPositions;
    }

    {
        QChar s[] = { QChar(0x2764), // U+2764 HEAVY BLACK HEART
                      QChar(0xFE0F), // U+FE0F VARIATION SELECTOR-16
                      QChar(0xD83C), QChar(0xDDEA), // U+1F1EA REGIONAL INDICATOR SYMBOL LETTER E
                      QChar(0xD83C), QChar(0xDDFA), // U+1F1FA REGIONAL INDICATOR SYMBOL LETTER U
                      QChar(0xD83C), QChar(0xDDEA), // U+1F1EA REGIONAL INDICATOR SYMBOL LETTER E
                      QChar(0xD83C), QChar(0xDDFA), // U+1F1FA REGIONAL INDICATOR SYMBOL LETTER U
                      QChar(0xD83C), QChar(0xDDEA), // U+1F1EA REGIONAL INDICATOR SYMBOL LETTER E
                      QChar(0x0041), // U+0041 LATIN CAPITAL LETTER A
                    };
        QString testString(s, sizeof(s)/sizeof(s[0]));

        QList<int> expectedBreakPositions{0, 2, 6, 10, 12, 13};
        QTest::newRow("+EXTPICxEXT+RIxRI+RIxRI+RI+ANY+") << testString << expectedBreakPositions;
    }
}

void tst_QTextBoundaryFinder::graphemeBoundaries_manual()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Grapheme);
}

void tst_QTextBoundaryFinder::wordBoundaries_manual_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int> >("expectedBreakPositions");
    QTest::addColumn<QList<int> >("expectedStartPositions");
    QTest::addColumn<QList<int> >("expectedEndPositions");

    {
        QChar s[] = { QChar(0x000D), QChar(0x000A), QChar(0x000A) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 2 << 3;

        QTest::newRow("+CRxLF+LF+") << testString << expectedBreakPositions
                                    << expectedStartPositions << expectedEndPositions;
    }
    {
        QChar s[] = { QChar(0x000D), QChar(0x0308), QChar(0x000A), QChar(0x000A) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 1 << 2 << 3 << 4;

        QTest::newRow("+CR+FE+LF+LF+") << testString << expectedBreakPositions
                                       << expectedStartPositions << expectedEndPositions;
    }
    {
        QString testString(QString::fromUtf8("Aaa bbb ccc.\r\nDdd eee fff."));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 3 << 4 << 7 << 8 << 11 << 12 << 14 << 17 << 18 << 21 << 22 << 25 << 26;
        expectedStartPositions << 0 << 4 << 8  << 14 << 18 << 22;
        expectedEndPositions   << 3 << 7 << 11 << 17 << 21 << 25;

        QTest::newRow("words1") << testString << expectedBreakPositions
                                << expectedStartPositions << expectedEndPositions;
    }
    {
        QString testString(QString::fromUtf8("Hello (sad) world !"));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 5 << 6 << 7 << 10 << 11 << 12 << 17 << 18 << 19;
        expectedStartPositions << 0 << 7  << 12;
        expectedEndPositions   << 5 << 10 << 17;

        QTest::newRow("words2") << testString << expectedBreakPositions
                                << expectedStartPositions << expectedEndPositions;
    }
    {
        QString testString(QString::fromUtf8("mr.Hamster"));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 2 << 3 << 10;
        expectedStartPositions << 0 << 3;
        expectedEndPositions   << 2 << 10;

        QTest::newRow("words3") << testString << expectedBreakPositions
                                << expectedStartPositions << expectedEndPositions;
    }
    {
        QString testString(QString::fromUtf8("This is     a sample buffer.Please test me .     He's don't Le'Clerk."));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 4 << 5 << 7 << 12 << 13 << 14 << 20 << 21 << 27 << 28 << 34
                               << 35 << 39 << 40 << 42 << 43 << 44 << 49 << 53 << 54 << 59 << 60
                               << 68 << 69;
        expectedStartPositions << 0 << 5 << 12 << 14 << 21 << 28 << 35 << 40 << 49 << 54 << 60;
        expectedEndPositions   << 4 << 7 << 13 << 20 << 27 << 34 << 39 << 42 << 53 << 59 << 68;

        QTest::newRow("words4") << testString << expectedBreakPositions
                                << expectedStartPositions << expectedEndPositions;
    }
    {
        // text with trailing space
        QString testString(QString::fromUtf8("Please test me. Finish "));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 6 << 7 << 11 << 12 << 14 << 15 << 16 << 22 << 23;
        expectedStartPositions << 0 << 7  << 12 << 16;
        expectedEndPositions   << 6 << 11 << 14 << 22;

        QTest::newRow("qtbug6498") << testString << expectedBreakPositions
                                   << expectedStartPositions << expectedEndPositions;
    }

    // Sample Strings from WordBreakTest.html
    {
        QChar s[] = { QChar(0x0063), QChar(0x0061), QChar(0x006E), QChar(0x0027), QChar(0x0074) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 5;
        expectedStartPositions << 0;
        expectedEndPositions   << 5;

        QTest::newRow("ts 1") << testString << expectedBreakPositions
                              << expectedStartPositions << expectedEndPositions;
    }
    {
        QChar s[] = { QChar(0x0063), QChar(0x0061), QChar(0x006E), QChar(0x2019), QChar(0x0074) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 5;
        expectedStartPositions << 0;
        expectedEndPositions   << 5;

        QTest::newRow("ts 2") << testString << expectedBreakPositions
                              << expectedStartPositions << expectedEndPositions;
    }
    {
        QChar s[] = { QChar(0x0061), QChar(0x0062), QChar(0x00AD), QChar(0x0062), QChar(0x0061) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 5;
        expectedStartPositions << 0;
        expectedEndPositions   << 5;

        QTest::newRow("ts 3") << testString << expectedBreakPositions
                              << expectedStartPositions << expectedEndPositions;
    }
    {
        QChar s[] = { QChar(0x0061), QChar(0x0024), QChar(0x002D), QChar(0x0033),
                      QChar(0x0034), QChar(0x002C), QChar(0x0035), QChar(0x0036),
                      QChar(0x0037), QChar(0x002E), QChar(0x0031), QChar(0x0034),
                      QChar(0x0025), QChar(0x0062) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 1 << 2 << 3 << 12 << 13 << 14;
        expectedStartPositions << 0 << 3  << 13;
        expectedEndPositions   << 1 << 12 << 14;

        QTest::newRow("ts 4") << testString << expectedBreakPositions
                              << expectedStartPositions << expectedEndPositions;
    }
    {
        QChar s[] = { QChar(0x0033), QChar(0x0061) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 2;
        expectedStartPositions << 0;
        expectedEndPositions   << 2;

        QTest::newRow("ts 5") << testString << expectedBreakPositions
                              << expectedStartPositions << expectedEndPositions;
    }
    {
        QChar s[] = { QChar(0x2060), QChar(0x0063), QChar(0x2060), QChar(0x0061),
                      QChar(0x2060), QChar(0x006E), QChar(0x2060), QChar(0x0027),
                      QChar(0x2060), QChar(0x0074), QChar(0x2060), QChar(0x2060) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 1 << 12;
        expectedStartPositions << 1;
        expectedEndPositions   << 12;

        QTest::newRow("ts 1e") << testString << expectedBreakPositions
                               << expectedStartPositions << expectedEndPositions;
    }
    {
        QChar s[] = { QChar(0x2060), QChar(0x0063), QChar(0x2060), QChar(0x0061),
                      QChar(0x2060), QChar(0x006E), QChar(0x2060), QChar(0x2019),
                      QChar(0x2060), QChar(0x0074), QChar(0x2060), QChar(0x2060) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 1 << 12;
        expectedStartPositions << 1;
        expectedEndPositions   << 12;

        QTest::newRow("ts 2e") << testString << expectedBreakPositions
                               << expectedStartPositions << expectedEndPositions;
    }
    {
        QChar s[] = { QChar(0x2060), QChar(0x0061), QChar(0x2060), QChar(0x0062),
                      QChar(0x2060), QChar(0x00AD), QChar(0x2060), QChar(0x0062),
                      QChar(0x2060), QChar(0x0061), QChar(0x2060), QChar(0x2060) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 1 << 12;
        expectedStartPositions << 1;
        expectedEndPositions   << 12;

        QTest::newRow("ts 3e") << testString << expectedBreakPositions
                               << expectedStartPositions << expectedEndPositions;
    }
    {
        QChar s[] = { QChar(0x2060), QChar(0x0061), QChar(0x2060), QChar(0x0024),
                      QChar(0x2060), QChar(0x002D), QChar(0x2060), QChar(0x0033),
                      QChar(0x2060), QChar(0x0034), QChar(0x2060), QChar(0x002C),
                      QChar(0x2060), QChar(0x0035), QChar(0x2060), QChar(0x0036),
                      QChar(0x2060), QChar(0x0037), QChar(0x2060), QChar(0x002E),
                      QChar(0x2060), QChar(0x0031), QChar(0x2060), QChar(0x0034),
                      QChar(0x2060), QChar(0x0025), QChar(0x2060), QChar(0x0062),
                      QChar(0x2060), QChar(0x2060) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 1 << 3 << 5 << 7 << 25 << 27 << 30;
        expectedStartPositions << 1 << 7  << 27;
        expectedEndPositions   << 3 << 25 << 30;

        QTest::newRow("ts 4e") << testString << expectedBreakPositions
                               << expectedStartPositions << expectedEndPositions;
    }
    {
        QChar s[] = { QChar(0x2060), QChar(0x0033), QChar(0x2060), QChar(0x0061),
                      QChar(0x2060), QChar(0x2060) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedStartPositions, expectedEndPositions;
        expectedBreakPositions << 0 << 1 << 6;
        expectedStartPositions << 1;
        expectedEndPositions   << 6;

        QTest::newRow("ts 5e") << testString << expectedBreakPositions
                               << expectedStartPositions << expectedEndPositions;
    }
}

void tst_QTextBoundaryFinder::wordBoundaries_manual()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);
    QFETCH(QList<int>, expectedStartPositions);
    QFETCH(QList<int>, expectedEndPositions);

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Word);
    doTestData(testString, expectedStartPositions, QTextBoundaryFinder::Word, QTextBoundaryFinder::StartOfItem);
    doTestData(testString, expectedEndPositions, QTextBoundaryFinder::Word, QTextBoundaryFinder::EndOfItem);
}

void tst_QTextBoundaryFinder::sentenceBoundaries_manual_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int> >("expectedBreakPositions");

    {
        QChar s[] = { QChar(0x000D), QChar(0x000A), QChar(0x000A) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 2 << 3;

        QTest::newRow("+CRxLF+LF+") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { QChar(0x000D), QChar(0x0308), QChar(0x000A), QChar(0x000A) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 3 << 4;

        QTest::newRow("+CR+FExLF+LF+") << testString << expectedBreakPositions;
    }
    {
        QString testString(QString::fromUtf8("Aaa bbb ccc.\r\nDdd eee fff."));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 14 << 26;

        QTest::newRow("data1") << testString << expectedBreakPositions;
    }
    {
        QString testString(QString::fromUtf8("Diga-nos qualé a sua opinião"));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 28;

        QTest::newRow("data2") << testString << expectedBreakPositions;
    }
    {
        QString testString(QString::fromUtf8("mr.Hamster"));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 3 << 10;

        QTest::newRow("data3") << testString << expectedBreakPositions;
    }
    {
        QString testString(QString::fromUtf8("Doing TEST, doing another test."));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 31;

        QTest::newRow("data4") << testString << expectedBreakPositions;
    }
}

void tst_QTextBoundaryFinder::sentenceBoundaries_manual()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    QVERIFY(expectedBreakPositions.size() >= 2);
    QList<int> expectedStartPositions = expectedBreakPositions; expectedStartPositions.removeLast();
    QList<int> expectedEndPositions = expectedBreakPositions; expectedEndPositions.removeFirst();

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Sentence);
    doTestData(testString, expectedStartPositions, QTextBoundaryFinder::Sentence, QTextBoundaryFinder::StartOfItem);
    doTestData(testString, expectedEndPositions, QTextBoundaryFinder::Sentence, QTextBoundaryFinder::EndOfItem);
}

void tst_QTextBoundaryFinder::lineBoundaries_manual_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int> >("expectedBreakPositions");
    QTest::addColumn<QList<int> >("expectedMandatoryBreakPositions");

    {
        QString testString(QString::fromUtf8("Aaa bbb ccc.\r\nDdd eee fff."));
        QList<int> expectedBreakPositions, expectedMandatoryBreakPositions;
        expectedBreakPositions << 0 << 4 << 8 << 14 << 18 << 22 << 26;
        expectedMandatoryBreakPositions << 0 << 14 << 26;

        QTest::newRow("data1") << testString << expectedBreakPositions
                               << expectedMandatoryBreakPositions;
    }
    {
        QString testString(QString::fromUtf8("Diga-nos qualé a sua opinião"));
        QList<int> expectedBreakPositions, expectedMandatoryBreakPositions;
        expectedBreakPositions << 0 << 5 << 9 << 15 << 17 << 21 << 28;
        expectedMandatoryBreakPositions << 0 << 28;

        QTest::newRow("data2") << testString << expectedBreakPositions
                               << expectedMandatoryBreakPositions;
    }

    {
        QChar s[] = { QChar(0x000D), QChar(0x0308), QChar(0x000A), QChar(0x000A), QChar(0x0020) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedMandatoryBreakPositions;
        expectedBreakPositions << 0 << 1 << 3 << 4 << 5;
        expectedMandatoryBreakPositions << 0 << 1 << 3 << 4 << 5;

        QTest::newRow("x(CR)+(FE)x(LF)+(LF)+(SP)+") << testString << expectedBreakPositions
                                                    << expectedMandatoryBreakPositions;
    }
    {
        QChar s[] = { QChar(0x000A), QChar(0x2E80), QChar(0x0308), QChar(0x0023), QChar(0x0023) };
        QString testString(s, sizeof(s)/sizeof(QChar));
        QList<int> expectedBreakPositions, expectedMandatoryBreakPositions;
        expectedBreakPositions << 0 << 1 << 3 << 5;
        expectedMandatoryBreakPositions << 0 << 1 << 5;

        QTest::newRow("x(LF)+(ID)x(CM)+(AL)x(AL)+") << testString << expectedBreakPositions
                                                    << expectedMandatoryBreakPositions;
    }
    {
        QChar s[] = { QChar(0x000A), QChar(0x0308), QChar(0x0023), QChar(0x0023) };
        QString testString(s, sizeof(s)/sizeof(QChar));
        QList<int> expectedBreakPositions, expectedMandatoryBreakPositions;
        expectedBreakPositions << 0 << 1 << 4;
        expectedMandatoryBreakPositions << 0 << 1 << 4;

        QTest::newRow("x(LF)+(CM)x(AL)x(AL)+") << testString << expectedBreakPositions
                                               << expectedMandatoryBreakPositions;
    }

    {
        QChar s[] = { QChar(0x0061), QChar(0x00AD), QChar(0x0062), QChar(0x0009), QChar(0x0063), QChar(0x0064) };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions, expectedMandatoryBreakPositions;
        expectedBreakPositions << 0 << 2 << 4 << 6;
        expectedMandatoryBreakPositions << 0 << 6;

        QTest::newRow("x(AL)x(BA)+(AL)x(BA)+(AL)x(AL)+") << testString << expectedBreakPositions
                                                         << expectedMandatoryBreakPositions;
    }
}

void tst_QTextBoundaryFinder::lineBoundaries_manual()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);
    QFETCH(QList<int>, expectedMandatoryBreakPositions);

    QVERIFY(expectedMandatoryBreakPositions.size() >= 2);
    QList<int> expectedStartPositions = expectedMandatoryBreakPositions; expectedStartPositions.removeLast();
    QList<int> expectedEndPositions = expectedMandatoryBreakPositions; expectedEndPositions.removeFirst();

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Line);
    doTestData(testString, expectedMandatoryBreakPositions, QTextBoundaryFinder::Line, QTextBoundaryFinder::MandatoryBreak);
    doTestData(testString, expectedStartPositions, QTextBoundaryFinder::Line, QTextBoundaryFinder::StartOfItem);
    doTestData(testString, expectedEndPositions, QTextBoundaryFinder::Line, QTextBoundaryFinder::EndOfItem);
}

Q_DECLARE_METATYPE(QTextBoundaryFinder)

void tst_QTextBoundaryFinder::emptyText_data()
{
    QTest::addColumn<QTextBoundaryFinder>("boundaryFinder");

    QString empty;
    QString notEmpty(QLatin1String("not empty"));
    uchar attrs[11];

    QTextBoundaryFinder invalidFinder(QTextBoundaryFinder::Word, empty);
    QTest::newRow("empty1") << invalidFinder;
    QTextBoundaryFinder finder(invalidFinder);
    QTest::newRow("empty2") << finder;
    finder = QTextBoundaryFinder(QTextBoundaryFinder::Grapheme, notEmpty);
    finder = invalidFinder;
    QTest::newRow("empty3") << finder;
    QTest::newRow("empty4") << QTextBoundaryFinder(QTextBoundaryFinder::Word, notEmpty.constData(), 0, 0, 0);
    QTest::newRow("empty5") << QTextBoundaryFinder(QTextBoundaryFinder::Word, notEmpty.constData(), 0, attrs, 11);
}

void tst_QTextBoundaryFinder::emptyText()
{
    QFETCH(QTextBoundaryFinder, boundaryFinder);

    QCOMPARE(boundaryFinder.position(), 0);
    QCOMPARE(boundaryFinder.boundaryReasons(), QTextBoundaryFinder::NotAtBoundary);

    boundaryFinder.toNextBoundary();
    QCOMPARE(boundaryFinder.position(), -1);
    QCOMPARE(boundaryFinder.boundaryReasons(), QTextBoundaryFinder::NotAtBoundary);
}

void tst_QTextBoundaryFinder::fastConstructor()
{
    QString text("Hello World");
    QTextBoundaryFinder finder(QTextBoundaryFinder::Word, text.constData(), text.size(), /*buffer*/0, /*buffer size*/0);

    QCOMPARE(finder.position(), 0);
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::StartOfItem);

    finder.toNextBoundary();
    QCOMPARE(finder.position(), 5);
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::EndOfItem);

    finder.toNextBoundary();
    QCOMPARE(finder.position(), 6);
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::StartOfItem);

    finder.toNextBoundary();
    QCOMPARE(finder.position(), text.size());
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::EndOfItem);

    finder.toNextBoundary();
    QCOMPARE(finder.position(), -1);
    QCOMPARE(finder.boundaryReasons(), QTextBoundaryFinder::NotAtBoundary);
}

void tst_QTextBoundaryFinder::assignmentOperator()
{
    QString text(QLatin1String("Hello World"));

    QTextBoundaryFinder invalidFinder;
    QVERIFY(!invalidFinder.isValid());
    QCOMPARE(invalidFinder.string(), QString());

    QTextBoundaryFinder validFinder(QTextBoundaryFinder::Word, text);
    QVERIFY(validFinder.isValid());
    QCOMPARE(validFinder.string(), text);

    QTextBoundaryFinder finder(QTextBoundaryFinder::Line, QLatin1String("dummy"));
    QVERIFY(finder.isValid());

    finder = invalidFinder;
    QVERIFY(!finder.isValid());
    QCOMPARE(finder.string(), QString());

    finder = validFinder;
    QVERIFY(finder.isValid());
    QCOMPARE(finder.string(), text);
}

void tst_QTextBoundaryFinder::isAtSoftHyphen_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int> >("expectedBreakPositions");
    QTest::addColumn<QList<int> >("expectedSoftHyphenPositions");

    {
        QString testString = QString::fromUtf8("I a-m break-able");
        testString.replace(QLatin1Char('-'), QChar(QChar::SoftHyphen));
        QList<int> expectedBreakPositions, expectedSoftHyphenPositions;
        expectedBreakPositions << 0 << 2 << 4 << 6 << 12 << 16;
        expectedSoftHyphenPositions << 4 << 12;

        QTest::newRow("Soft Hyphen") << testString << expectedBreakPositions
                                     << expectedSoftHyphenPositions;
    }
}

void tst_QTextBoundaryFinder::isAtSoftHyphen()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);
    QFETCH(QList<int>, expectedSoftHyphenPositions);

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Line);
    doTestData(testString, expectedSoftHyphenPositions, QTextBoundaryFinder::Line, QTextBoundaryFinder::SoftHyphen);
}

QTEST_MAIN(tst_QTextBoundaryFinder)
#include "tst_qtextboundaryfinder.moc"
