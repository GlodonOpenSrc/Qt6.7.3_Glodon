// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtGui>
#include <QtWidgets>
#include <QtDebug>
#include <QList>
#include <QPointer>
#include <QSignalSpy>

#include <QtTest/private/qtesthelpers_p.h>

#include "../../../../shared/filesystem.h"

#include <memory>

#include <QtWidgets/private/qapplication_p.h>

Q_DECLARE_METATYPE(QCompleter::CompletionMode)

using namespace QTestPrivate;

class CsvCompleter : public QCompleter
{
    Q_OBJECT
public:
    using QCompleter::QCompleter;

    QString pathFromIndex(const QModelIndex& sourceIndex) const override;

    void setCsvCompletion(bool set) { csv = set; }

protected:
    QStringList splitPath(const QString &path) const override
    {
        return csv ? path.split(QLatin1Char(',')) : QCompleter::splitPath(path);
    }

private:
    bool csv = true;
};

QString CsvCompleter::pathFromIndex(const QModelIndex &sourceIndex) const
{
    if (!csv)
        return QCompleter::pathFromIndex(sourceIndex);

    if (!sourceIndex.isValid())
        return QString();

    QModelIndex idx = sourceIndex;
    QStringList list;
    do {
        QString t = model()->data(idx, completionRole()).toString();
        list.prepend(t);
        QModelIndex parent = idx.parent();
        idx = parent.sibling(parent.row(), sourceIndex.column());
    } while (idx.isValid());

    return list.size() == 1 ? list.constFirst() : list.join(QLatin1Char(','));
}

class tst_QCompleter : public QObject
{
    Q_OBJECT
public:
    tst_QCompleter();
    ~tst_QCompleter();

private slots:
    void getSetCheck();

    void multipleWidgets();
    void focusIn();

    void csMatchingOnCsSortedModel_data();
    void csMatchingOnCsSortedModel();
    void ciMatchingOnCiSortedModel_data();
    void ciMatchingOnCiSortedModel();

    void ciMatchingOnCsSortedModel_data();
    void ciMatchingOnCsSortedModel();
    void csMatchingOnCiSortedModel_data();
    void csMatchingOnCiSortedModel();

    void fileSystemModel_data();
    void fileSystemModel();
    void fileDialog_data();
    void fileDialog();

    void changingModel_data();
    void changingModel();

    void sortedEngineRowCount_data();
    void sortedEngineRowCount();
    void unsortedEngineRowCount_data();
    void unsortedEngineRowCount();

    void currentRow();
    void sortedEngineMapFromSource();
    void unsortedEngineMapFromSource();

    void historySearch();

    void modelDeletion();
    void setters();

    void dynamicSortOrder();
    void disabledItems();

    void hideWidget();

    // task-specific tests below me
    void task178797_activatedOnReturn();
    void task189564_omitNonSelectableItems();
    void task246056_setCompletionPrefix();
    void task250064_lostFocus();

    void task253125_lineEditCompletion_data();
    void task253125_lineEditCompletion();
    void task247560_keyboardNavigation();
    void QTBUG_14292_filesystem();
    void QTBUG_52028_tabAutoCompletes();
    void QTBUG_51889_activatedSentTwice();
    void showPopupInGraphicsView();
    void inheritedEventFilter();

private:
    void filter(bool assync = false);
    void testRowCount();
    enum ModelType {
        CASE_SENSITIVELY_SORTED_MODEL,
        CASE_INSENSITIVELY_SORTED_MODEL,
        HISTORY_MODEL,
        FILESYSTEM_MODEL
    };
    void setSourceModel(ModelType);

    CsvCompleter *completer = nullptr;
    QTreeWidget *treeWidget;
    const int completionColumn = 0;
    const int columnCount = 3;
};

tst_QCompleter::tst_QCompleter() : treeWidget(new QTreeWidget)
{
    treeWidget->move(100, 100);
    treeWidget->setColumnCount(columnCount);
}

tst_QCompleter::~tst_QCompleter()
{
    delete treeWidget;
    delete completer;
}

#ifdef Q_OS_ANDROID
static QString androidHomePath()
{
    const auto homePaths = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    QDir dir = homePaths.isEmpty() ? QDir() : homePaths.first();
    dir.cdUp();
    return dir.path();
}
#endif

void tst_QCompleter::setSourceModel(ModelType type)
{
    QTreeWidgetItem *parent, *child;
    treeWidget->clear();
    switch(type) {
    case CASE_SENSITIVELY_SORTED_MODEL:
        // Creates a tree model with top level items P0, P1, .., p0, p1,..
        // Each of these items parents have children (for P0 - c0P0, c1P0,...)
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 5; j++) {
                parent = new QTreeWidgetItem(treeWidget);
                const QString text = QLatin1Char(i == 0 ? 'P' : 'p') + QString::number(j);
                parent->setText(completionColumn, text);
                for (int k = 0; k < 5; k++) {
                    child = new QTreeWidgetItem(parent);
                    QString t = QLatin1Char('c') + QString::number(k) + text;
                    child->setText(completionColumn, t);
                }
            }
        }
        completer->setModel(treeWidget->model());
        completer->setCompletionColumn(completionColumn);
        break;
    case CASE_INSENSITIVELY_SORTED_MODEL:
    case HISTORY_MODEL:
        // Creates a tree model with top level items P0, p0, P1, p1,...
        // Each of these items have children c0p0, c1p0,..
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 2; j++) {
                parent = new QTreeWidgetItem(treeWidget);
                const QString text = QLatin1Char(j == 0 ? 'P' : 'p') + QString::number(i);
                parent->setText(completionColumn, text);
                for (int k = 0; k < 5; k++) {
                    child = new QTreeWidgetItem(parent);
                    QString t = QLatin1Char('c') + QString::number(k) + text;
                    child->setText(completionColumn, t);
                }
            }
        }
        completer->setModel(treeWidget->model());
        completer->setCompletionColumn(completionColumn);
        if (type == CASE_INSENSITIVELY_SORTED_MODEL)
            break;
        parent = new QTreeWidgetItem(treeWidget);
        parent->setText(completionColumn, QLatin1String("p3,c3p3"));
        parent = new QTreeWidgetItem(treeWidget);
        parent->setText(completionColumn, QLatin1String("p2,c4p2"));
        break;
    case FILESYSTEM_MODEL:
        completer->setCsvCompletion(false);
        {
            auto m = new QFileSystemModel(completer);
#ifdef Q_OS_ANDROID
            // Android 11 and above doesn't allow accessing root filesystem as before,
            // so let's opt int for the app's home.
            m->setRootPath(androidHomePath());
#else
            m->setRootPath("/");
#endif
            completer->setModel(m);
        }
        completer->setCompletionColumn(0);
        break;
    default:
        qDebug() << "Invalid type";
        break;
    }
}

void tst_QCompleter::filter(bool assync)
{
    QFETCH(QString, filterText);
    QFETCH(const QString, step);
    QFETCH(QString, completion);
    QFETCH(QString, completionText);

    if (filterText.compare("FILTERING_OFF", Qt::CaseInsensitive) == 0) {
        completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        return;
    }

    int result = -1;
    const int attempts = assync ? 10 : 1;
    for (int times = 0; times < attempts; ++times) {
        completer->setCompletionPrefix(filterText);

        for (QChar s : step) {
            int row = completer->currentRow();
            switch (s.toUpper().toLatin1()) {
            case 'P':
                --row;
                break;
            case 'N':
                ++row;
                break;
            case 'L':
                row = completer->completionCount() - 1;
                break;
            case 'F':
                row = 0;
                break;
            default:
                QFAIL(qPrintable(QString(
                                     "Problem with 'step' value in test data: %1 (only P, N, L and F are allowed)."
                                     ).arg(s)));
            }
            completer->setCurrentRow(row);
        }

        result = QString::compare(completer->currentCompletion(), completionText,
                                  completer->caseSensitivity());
        if (result == 0)
            break;
        if (assync)
            QTest::qWait(50 * times);
    }

    QCOMPARE(result, 0);
}

// Testing get/set functions
void tst_QCompleter::getSetCheck()
{
    QStandardItemModel standardItemModel(3,3);
    QCompleter completer(&standardItemModel);

    // QString QCompleter::completionPrefix()
    // void QCompleter::setCompletionPrefix(QString)
    completer.setCompletionPrefix(QString("te"));
    QCOMPARE(completer.completionPrefix(), QString("te"));
    completer.setCompletionPrefix(QString());
    QCOMPARE(completer.completionPrefix(), QString());

    // ModelSorting QCompleter::modelSorting()
    // void QCompleter::setModelSorting(ModelSorting)
    completer.setModelSorting(QCompleter::CaseSensitivelySortedModel);
    QCOMPARE(completer.modelSorting(), QCompleter::CaseSensitivelySortedModel);
    completer.setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    QCOMPARE(completer.modelSorting(), QCompleter::CaseInsensitivelySortedModel);
    completer.setModelSorting(QCompleter::UnsortedModel);
    QCOMPARE(completer.modelSorting(), QCompleter::UnsortedModel);

    // CompletionMode QCompleter::completionMode()
    // void QCompleter::setCompletionMode(CompletionMode)
    QCOMPARE(completer.completionMode(), QCompleter::PopupCompletion); // default value
    completer.setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    QCOMPARE(completer.completionMode(), QCompleter::UnfilteredPopupCompletion);
    completer.setCompletionMode(QCompleter::InlineCompletion);
    QCOMPARE(completer.completionMode(), QCompleter::InlineCompletion);

    // int QCompleter::completionColumn()
    // void QCompleter::setCompletionColumn(int)
    completer.setCompletionColumn(2);
    QCOMPARE(completer.completionColumn(), 2);
    completer.setCompletionColumn(1);
    QCOMPARE(completer.completionColumn(), 1);

    // int QCompleter::completionRole()
    // void QCompleter::setCompletionRole(int)
    QCOMPARE(completer.completionRole(), static_cast<int>(Qt::EditRole)); // default value
    completer.setCompletionRole(Qt::DisplayRole);
    QCOMPARE(completer.completionRole(), static_cast<int>(Qt::DisplayRole));

    // int QCompleter::maxVisibleItems()
    // void QCompleter::setMaxVisibleItems(int)
    QCOMPARE(completer.maxVisibleItems(), 7); // default value
    completer.setMaxVisibleItems(10);
    QCOMPARE(completer.maxVisibleItems(), 10);
    QTest::ignoreMessage(QtWarningMsg, "QCompleter::setMaxVisibleItems: "
                         "Invalid max visible items (-2147483648) must be >= 0");
    completer.setMaxVisibleItems(INT_MIN);
    QCOMPARE(completer.maxVisibleItems(), 10); // Cannot be set to something negative => old value

    // Qt::CaseSensitivity QCompleter::caseSensitivity()
    // void QCompleter::setCaseSensitivity(Qt::CaseSensitivity)
    QCOMPARE(completer.caseSensitivity(), Qt::CaseSensitive); // default value
    completer.setCaseSensitivity(Qt::CaseInsensitive);
    QCOMPARE(completer.caseSensitivity(), Qt::CaseInsensitive);

    // bool QCompleter::wrapAround()
    // void QCompleter::setWrapAround(bool)
    QCOMPARE(completer.wrapAround(), true); // default value
    completer.setWrapAround(false);
    QCOMPARE(completer.wrapAround(), false);

#if QT_CONFIG(filesystemmodel)
    // QTBUG-54642, changing from QFileSystemModel to another model should restore role.
    completer.setCompletionRole(Qt::EditRole);
    QCOMPARE(completer.completionRole(), static_cast<int>(Qt::EditRole)); // default value
    QFileSystemModel fileSystemModel;
    completer.setModel(&fileSystemModel);
    QCOMPARE(completer.completionRole(), static_cast<int>(QFileSystemModel::FileNameRole));
    completer.setModel(&standardItemModel);
    QCOMPARE(completer.completionRole(), static_cast<int>(Qt::EditRole));
    completer.setCompletionRole(Qt::ToolTipRole);
    QStandardItemModel standardItemModel2(2, 2); // Do not clobber a custom role when changing models
    completer.setModel(&standardItemModel2);
    QCOMPARE(completer.completionRole(), static_cast<int>(Qt::ToolTipRole));
#endif // QT_CONFIG(filesystemmodel)
}

void tst_QCompleter::csMatchingOnCsSortedModel_data()
{
    delete completer;
    completer = new CsvCompleter;
    completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    setSourceModel(CASE_SENSITIVELY_SORTED_MODEL);

    QTest::addColumn<QString>("filterText");
    QTest::addColumn<QString>("step");
    QTest::addColumn<QString>("completion");
    QTest::addColumn<QString>("completionText");

#define ROWNAME(name) ((QByteArray(name) + ' ' + QByteArray::number(i)).constData())

    for (int i = 0; i < 2; i++) {
         if (i == 1)
             QTest::newRow("FILTERING_OFF") << "FILTERING_OFF" << "" << "" << "";

         // Plain text filter
         QTest::newRow(ROWNAME("()")) << "" << "" << "P0" << "P0";
         QTest::newRow(ROWNAME("()F")) << "" << "F" << "P0" << "P0";
         QTest::newRow(ROWNAME("()L")) << "" << "L" << "p4" << "p4";
         QTest::newRow(ROWNAME("()N")) << "" << "N" << "P1" << "P1";
         QTest::newRow(ROWNAME("(P)")) << "P" << "" << "P0" << "P0";
         QTest::newRow(ROWNAME("(P)F")) << "P" << "" << "P0" << "P0";
         QTest::newRow(ROWNAME("(P)L")) << "P" << "L" << "P4" << "P4";
         QTest::newRow(ROWNAME("(p)")) << "p" << "" << "p0" << "p0";
         QTest::newRow(ROWNAME("(p)N")) << "p" << "N" << "p1" << "p1";
         QTest::newRow(ROWNAME("(p)NN")) << "p" << "NN" << "p2" << "p2";
         QTest::newRow(ROWNAME("(p)NNN")) << "p" << "NNN" << "p3" << "p3";
         QTest::newRow(ROWNAME("(p)NNNN")) << "p" << "NNNN" << "p4" << "p4";
         QTest::newRow(ROWNAME("(p1)")) << "p1" << "" << "p1" << "p1";
         QTest::newRow(ROWNAME("(p11)")) << "p11" << "" << "" << "";

         // Tree filter
         QTest::newRow(ROWNAME("(P0,)")) << "P0," << "" << "c0P0" << "P0,c0P0";
         QTest::newRow(ROWNAME("(P0,c)")) << "P0,c" << "" << "c0P0" << "P0,c0P0";
         QTest::newRow(ROWNAME("(P0,c1)")) << "P0,c1" << "" << "c1P0" << "P0,c1P0";
         QTest::newRow(ROWNAME("(P0,c3P0)")) << "P0,c3P0" << "" << "c3P0" << "P0,c3P0";
         QTest::newRow(ROWNAME("(P3,c)F")) << "P3,c" << "F" << "c0P3" << "P3,c0P3";
         QTest::newRow(ROWNAME("(P3,c)L")) << "P3,c" << "L" << "c4P3" << "P3,c4P3";
         QTest::newRow(ROWNAME("(P3,c)N")) << "P3,c" << "N" << "c1P3" << "P3,c1P3";
         QTest::newRow(ROWNAME("(P3,c)NN")) << "P3,c" << "NN" << "c2P3" << "P3,c2P3";
         QTest::newRow(ROWNAME("(P3,,c)")) << "P3,,c" << "" << "" << "";
         QTest::newRow(ROWNAME("(P3,c0P3,)")) << "P3,c0P3," << "" << "" << "";
         QTest::newRow(ROWNAME("(P,)")) << "P," << "" << "" << "";
     }
#undef ROWNAME
}

void tst_QCompleter::csMatchingOnCsSortedModel()
{
    filter();
}

void tst_QCompleter::ciMatchingOnCiSortedModel_data()
{
    delete completer;
    completer = new CsvCompleter;
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    setSourceModel(CASE_INSENSITIVELY_SORTED_MODEL);

    QTest::addColumn<QString>("filterText");
    QTest::addColumn<QString>("step");
    QTest::addColumn<QString>("completion");
    QTest::addColumn<QString>("completionText");

    for (int i = 0; i < 2; i++) {
        if (i == 1)
            QTest::newRow("FILTERING_OFF") << "FILTERING_OFF" << "" << "" << "";

        // Plain text filter
        QTest::newRow("()") << "" << "" << "P0" << "P0";
        QTest::newRow("()F") << "" << "F" << "P0" << "P0";
        QTest::newRow("()L") << "" << "L" << "p4" << "p4";
        QTest::newRow("()N") << "" << "N" << "p0" << "p0";
        QTest::newRow("(P)") << "P" << "" << "P0" << "P0";
        QTest::newRow("(P)F") << "P" << "" << "P0" << "P0";
        QTest::newRow("(P)L") << "P" << "L" << "p4" << "p4";
        QTest::newRow("(p)") << "p" << "" << "P0" << "P0";
        QTest::newRow("(p)N") << "p" << "N" << "p0" << "p0";
        QTest::newRow("(p)NN") << "p" << "NN" << "P1" << "P1";
        QTest::newRow("(p)NNN") << "p" << "NNN" << "p1" << "p1";
        QTest::newRow("(p1)") << "p1" << "" << "P1" << "P1";
        QTest::newRow("(p1)N") << "p1" << "N" << "p1" << "p1";
        QTest::newRow("(p11)") << "p11" << "" << "" << "";

        //// Tree filter
        QTest::newRow("(p0,)") << "p0," << "" << "c0P0" << "P0,c0P0";
        QTest::newRow("(p0,c)") << "p0,c" << "" << "c0P0" << "P0,c0P0";
        QTest::newRow("(p0,c1)") << "p0,c1" << "" << "c1P0" << "P0,c1P0";
        QTest::newRow("(p0,c3P0)") << "p0,c3P0" << "" << "c3P0" << "P0,c3P0";
        QTest::newRow("(p3,c)F") << "p3,c" << "F" << "c0P3" << "P3,c0P3";
        QTest::newRow("(p3,c)L") << "p3,c" << "L" << "c4P3" << "P3,c4P3";
        QTest::newRow("(p3,c)N") << "p3,c" << "N" << "c1P3" << "P3,c1P3";
        QTest::newRow("(p3,c)NN") << "p3,c" << "NN" << "c2P3" << "P3,c2P3";
        QTest::newRow("(p3,,c)") << "p3,,c" << "" << "" << "";
        QTest::newRow("(p3,c0P3,)") << "p3,c0P3," << "" << "" << "";
        QTest::newRow("(p,)") << "p," << "" << "" << "";
    }
}

void tst_QCompleter::ciMatchingOnCiSortedModel()
{
    filter();
}

void tst_QCompleter::ciMatchingOnCsSortedModel_data()
{
    delete completer;
    completer = new CsvCompleter;
    completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    setSourceModel(CASE_SENSITIVELY_SORTED_MODEL);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    QTest::addColumn<QString>("filterText");
    QTest::addColumn<QString>("step");
    QTest::addColumn<QString>("completion");
    QTest::addColumn<QString>("completionText");

    for (int i = 0; i < 2; i++) {
        if (i == 1)
            QTest::newRow("FILTERING_OFF") << "FILTERING_OFF" << "" << "" << "";

        // Plain text filter
        QTest::newRow("()") << "" << "" << "P0" << "P0";
        QTest::newRow("()F") << "" << "F" << "P0" << "P0";
        QTest::newRow("()L") << "" << "L" << "p4" << "p4";
        QTest::newRow("(P)") << "P" << "" << "P0" << "P0";
        QTest::newRow("(P)F") << "P" << "" << "P0" << "P0";
        QTest::newRow("(P)L") << "P" << "L" << "p4" << "p4";
        QTest::newRow("(p)") << "p" << "" << "P0" << "P0";
        QTest::newRow("(p)N") << "p" << "N" << "P1" << "P1";
        QTest::newRow("(p)NN") << "p" << "NN" << "P2" << "P2";
        QTest::newRow("(p)NNN") << "p" << "NNN" << "P3" << "P3";
        QTest::newRow("(p1)") << "p1" << "" << "P1" << "P1";
        QTest::newRow("(p1)N") << "p1" << "N" << "p1" << "p1";
        QTest::newRow("(p11)") << "p11" << "" << "" << "";

        // Tree filter
        QTest::newRow("(p0,)") << "p0," << "" << "c0P0" << "P0,c0P0";
        QTest::newRow("(p0,c)") << "p0,c" << "" << "c0P0" << "P0,c0P0";
        QTest::newRow("(p0,c1)") << "p0,c1" << "" << "c1P0" << "P0,c1P0";
        QTest::newRow("(p0,c3P0)") << "p0,c3P0" << "" << "c3P0" << "P0,c3P0";
        QTest::newRow("(p3,c)F") << "p3,c" << "F" << "c0P3" << "P3,c0P3";
        QTest::newRow("(p3,c)L") << "p3,c" << "L" << "c4P3" << "P3,c4P3";
        QTest::newRow("(p3,c)N") << "p3,c" << "N" << "c1P3" << "P3,c1P3";
        QTest::newRow("(p3,c)NN") << "p3,c" << "NN" << "c2P3" << "P3,c2P3";
        QTest::newRow("(p3,,c)") << "p3,,c" << "" << "" << "";
        QTest::newRow("(p3,c0P3,)") << "p3,c0P3," << "" << "" << "";
        QTest::newRow("(p,)") << "p," << "" << "" << "";
    }
}

void tst_QCompleter::ciMatchingOnCsSortedModel()
{
    filter();
}

void tst_QCompleter::csMatchingOnCiSortedModel_data()
{
    delete completer;
    completer = new CsvCompleter;
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    setSourceModel(CASE_INSENSITIVELY_SORTED_MODEL);
    completer->setCaseSensitivity(Qt::CaseSensitive);

    QTest::addColumn<QString>("filterText");
    QTest::addColumn<QString>("step");
    QTest::addColumn<QString>("completion");
    QTest::addColumn<QString>("completionText");

    for (int i = 0; i < 2; i++) {
        if (i == 1)
            QTest::newRow("FILTERING_OFF") << "FILTERING_OFF" << "" << "" << "";

        // Plain text filter
        QTest::newRow("()") << "" << "" << "P0" << "P0";
        QTest::newRow("()F") << "" << "F" << "P0" << "P0";
        QTest::newRow("()L") << "" << "L" << "p4" << "p4";
        QTest::newRow("()N") << "" << "N" << "p0" << "p0";
        QTest::newRow("(P)") << "P" << "" << "P0" << "P0";
        QTest::newRow("(P)F") << "P" << "" << "P0" << "P0";
        QTest::newRow("(P)L") << "P" << "L" << "P4" << "P4";
        QTest::newRow("(p)") << "p" << "" << "p0" << "p0";
        QTest::newRow("(p)N") << "p" << "N" << "p1" << "p1";
        QTest::newRow("(p)NN") << "p" << "NN" << "p2" << "p2";
        QTest::newRow("(p)NNN") << "p" << "NNN" << "p3" << "p3";
        QTest::newRow("(p1)") << "p1" << "" << "p1" << "p1";
        QTest::newRow("(p11)") << "p11" << "" << "" << "";

        //// Tree filter
        QTest::newRow("(p0,)") << "p0," << "" << "c0p0" << "p0,c0p0";
        QTest::newRow("(p0,c)") << "p0,c" << "" << "c0p0" << "p0,c0p0";
        QTest::newRow("(p0,c1)") << "p0,c1" << "" << "c1p0" << "p0,c1p0";
        QTest::newRow("(p0,c3P0)") << "p0,c3p0" << "" << "c3p0" << "p0,c3p0";
        QTest::newRow("(p3,c)F") << "p3,c" << "F" << "c0p3" << "p3,c0p3";
        QTest::newRow("(p3,c)L") << "p3,c" << "L" << "c4p3" << "p3,c4p3";
        QTest::newRow("(p3,c)N") << "p3,c" << "N" << "c1p3" << "p3,c1p3";
        QTest::newRow("(p3,c)NN") << "p3,c" << "NN" << "c2p3" << "p3,c2p3";
        QTest::newRow("(p3,,c)") << "p3,,c" << "" << "" << "";
        QTest::newRow("(p3,c0P3,)") << "p3,c0P3," << "" << "" << "";
        QTest::newRow("(p,)") << "p," << "" << "" << "";

        QTest::newRow("FILTERING_OFF") << "FILTERING_OFF" << "" << "" << "";
    }
}

void tst_QCompleter::csMatchingOnCiSortedModel()
{
    filter();
}

void tst_QCompleter::fileSystemModel_data()
{
    delete completer;
    completer = new CsvCompleter;
    completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    setSourceModel(FILESYSTEM_MODEL);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    QTest::addColumn<QString>("filterText");
    QTest::addColumn<QString>("step");
    QTest::addColumn<QString>("completion");
    QTest::addColumn<QString>("completionText");

    // NOTE: Add tests carefully, ensurely the paths exist on all systems
    // Output is the sourceText; currentCompletionText()

    for (int i = 0; i < 2; i++) {
        if (i == 1)
            QTest::newRow("FILTERING_OFF") << "FILTERING_OFF" << "" << "" << "";

#if defined(Q_OS_WIN)
        QTest::newRow("()") << "C" << "" << "C:" << "C:";
        QTest::newRow("()") << "C:\\Program" << "" << "Program Files" << "C:\\Program Files";
#elif defined (Q_OS_MAC)
        QTest::newRow("()") << "" << "" << "/" << "/";
        QTest::newRow("(/a)") << "/a" << "" << "Applications" << "/Applications";
//        QTest::newRow("(/d)") << "/d" << "" << "Developer" << "/Developer";
#elif defined(Q_OS_ANDROID)
        QTest::newRow("()") << "" << "" << "/" << "/";
        const QString androidDir = androidHomePath();
        const QString tag = QStringLiteral("%1/fil").arg(androidDir);
        QTest::newRow(tag.toUtf8().data()) << tag << "" << "files" << androidDir + "/files";
#elif defined(Q_OS_VXWORKS)
        QTest::newRow("()") << "" << "" << "/" << "/";
        QTest::newRow("(/tm)") << "/tm" << "" << "tmp" << "/tmp";
#else
        QTest::newRow("()") << "" << "" << "/" << "/";
#if !defined(Q_OS_AIX) && !defined(Q_OS_HPUX) && !defined(Q_OS_QNX)
        QTest::newRow("(/h)") << "/h" << "" << "home" << "/home";
#endif
        QTest::newRow("(/et)") << "/et" << "" << "etc" << "/etc";
        QTest::newRow("(/etc/passw)") << "/etc/passw" << "" << "passwd" << "/etc/passwd";
#endif
    }
}

void tst_QCompleter::fileSystemModel()
{
    //QFileSystemModel is async.
    filter(true);
}

/*!
    In the file dialog, the completer uses the EditRole.
    See QTBUG-94799
*/
void tst_QCompleter::fileDialog_data()
{
    fileSystemModel_data();
    completer->setCompletionRole(Qt::EditRole);
}

void tst_QCompleter::fileDialog()
{
    //QFileSystemModel is async.
    filter(true);
}


void tst_QCompleter::changingModel_data()
{
}

void tst_QCompleter::changingModel()
{
    for (int i = 0; i < 2; i++) {
        delete completer;
        completer = new CsvCompleter;
        completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
        completer->setCaseSensitivity(Qt::CaseSensitive);
        setSourceModel(CASE_SENSITIVELY_SORTED_MODEL);

        if (i == 1) {
            completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        }

        completer->setCompletionPrefix("p");
        completer->setCurrentRow(completer->completionCount() - 1);
        QCOMPARE(completer->currentCompletion(), QString("p4"));

        // Test addition of data
        QTreeWidgetItem p5item;
        p5item.setText(completionColumn, "p5");
        treeWidget->addTopLevelItem(&p5item);
        completer->setCompletionPrefix("p5");
        QCOMPARE(completer->currentCompletion(), QString("p5"));

        // Test removal of data
        int p5index = treeWidget->indexOfTopLevelItem(&p5item);
        treeWidget->takeTopLevelItem(p5index);
        QCOMPARE(completer->currentCompletion(), QString(""));

        // Test clear
        treeWidget->clear();
        QCOMPARE(completer->currentIndex(), QModelIndex());
    }
}

void tst_QCompleter::testRowCount()
{
    QFETCH(QString, filterText);
    QFETCH(bool, hasChildren);
    QFETCH(int, rowCount);
    QFETCH(int, completionCount);

    if (filterText.compare("FILTERING_OFF", Qt::CaseInsensitive) == 0) {
        completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        return;
    }

    completer->setCompletionPrefix(filterText);
    const QAbstractItemModel *completionModel = completer->completionModel();
    QCOMPARE(completionModel->rowCount(), rowCount);
    QCOMPARE(completionCount, completionCount);
    QCOMPARE(completionModel->hasChildren(), hasChildren);
    QCOMPARE(completionModel->columnCount(), columnCount);
}

void tst_QCompleter::sortedEngineRowCount_data()
{
    delete completer;
    completer = new CsvCompleter;
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    setSourceModel(CASE_INSENSITIVELY_SORTED_MODEL);

    QTest::addColumn<QString>("filterText");
    QTest::addColumn<bool>("hasChildren");
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("completionCount");

    QTest::newRow("whatever") << "whatever" << false << 0 << 0;
    QTest::newRow("p") << "p" << true << 10 << 10;
    QTest::newRow("p1") << "p1" << true << 2 << 2;
    QTest::newRow("P1,") << "P1," << true << 5 << 5;
    QTest::newRow("P1,c") << "P1,c" << true << 5 << 5;
    QTest::newRow("P1,cc") << "P1,cc" << false << 0 << 0;

    QTest::newRow("FILTERING_OFF") << "FILTERING_OFF" << false << 0 << 0;

    QTest::newRow("whatever(filter off)") << "whatever" << true << 10 << 0;
    QTest::newRow("p1(filter off)") << "p1" << true << 10 << 2;
    QTest::newRow("p1,(filter off)") << "p1," << true << 5 << 5;
    QTest::newRow("p1,c(filter off)") << "p1,c" << true << 5 << 5;
    QTest::newRow("P1,cc(filter off)") << "P1,cc" << true << 5 << 0;
}

void tst_QCompleter::sortedEngineRowCount()
{
    testRowCount();
}

void tst_QCompleter::unsortedEngineRowCount_data()
{
    delete completer;
    completer = new CsvCompleter;
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    setSourceModel(CASE_INSENSITIVELY_SORTED_MODEL);

    QTest::addColumn<QString>("filterText");
    QTest::addColumn<bool>("hasChildren");
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("completionCount");

    QTest::newRow("whatever") << "whatever" << false << 0 << 0;
    QTest::newRow("p") << "p" << true << 5 << 5;
    QTest::newRow("p1") << "p1" << true << 1 << 1;
    QTest::newRow("P1,") << "P1," << true << 5 << 5;
    QTest::newRow("P1,c") << "P1,c" << true << 5 << 5;
    QTest::newRow("P1,cc") << "P1,cc" << false << 0 << 0;

    QTest::newRow("FILTERING_OFF") << "FILTERING_OFF" << false << 0 << 0;

    QTest::newRow("whatever(filter off)") << "whatever" << true << 10 << 0;
    QTest::newRow("p1(filter off)") << "p1" << true << 10 << 1;
    QTest::newRow("p1,(filter off)") << "p1," << true << 5 << 5;
    QTest::newRow("p1,c(filter off)") << "p1,c" << true << 5 << 5;
    QTest::newRow("P1,cc(filter off)") << "P1,cc" << true << 5 << 0;
}

void tst_QCompleter::unsortedEngineRowCount()
{
    testRowCount();
}

void tst_QCompleter::currentRow()
{
    delete completer;
    completer = new CsvCompleter;
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    setSourceModel(CASE_INSENSITIVELY_SORTED_MODEL);

    // blank text
    completer->setCompletionPrefix("");
    QCOMPARE(completer->currentRow(), 0);
    QVERIFY(completer->setCurrentRow(4));
    QCOMPARE(completer->currentRow(), 4);
    QVERIFY(!completer->setCurrentRow(13));
    QVERIFY(completer->setCurrentRow(4));

    // some text
     completer->setCompletionPrefix("p1");
    QCOMPARE(completer->currentRow(), 0);
    QVERIFY(completer->setCurrentRow(1));
    QCOMPARE(completer->currentRow(), 1);
    QVERIFY(!completer->setCurrentRow(2));
    QCOMPARE(completer->currentRow(), 1);

    // invalid text
    completer->setCompletionPrefix("well");
    QCOMPARE(completer->currentRow(), -1);
}

void tst_QCompleter::sortedEngineMapFromSource()
{
    delete completer;
    completer = new CsvCompleter;
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    setSourceModel(CASE_INSENSITIVELY_SORTED_MODEL);

    QModelIndex si1, si2, pi;
    QAbstractItemModel *sourceModel = completer->model();
    auto completionModel = qobject_cast<const QAbstractProxyModel *>(completer->completionModel());

    // Fitering ON
    // empty
    si1 = sourceModel->index(4, completionColumn); // "P2"
    si2 = sourceModel->index(2, 0, si1); // "P2,c0P2"
    pi = completionModel->mapFromSource(si1);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P2"));
    pi = completionModel->mapFromSource(si2);
    QCOMPARE(pi.isValid(), false);

    // some text
    completer->setCompletionPrefix("p");
    pi = completionModel->mapFromSource(si1);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P2"));
    pi = completionModel->mapFromSource(si2);
    QCOMPARE(pi.isValid(), false);

    // more text
    completer->setCompletionPrefix("p2");
    pi = completionModel->mapFromSource(si1);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P2"));
    pi = completionModel->mapFromSource(si2);
    QCOMPARE(pi.isValid(), false);

    // invalid text
    completer->setCompletionPrefix("whatever");
    pi = completionModel->mapFromSource(si1);
    QVERIFY(!pi.isValid());

    // Fitering OFF
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    // empty
    si1 = sourceModel->index(4, completionColumn); // "P2"
    si2 = sourceModel->index(2, 0, si1); // "P2,c0P2"
    pi = completionModel->mapFromSource(si1);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P2"));
    pi = completionModel->mapFromSource(si2);
    QCOMPARE(pi.isValid(), false);

    // some text
    completer->setCompletionPrefix("p");
    pi = completionModel->mapFromSource(si1);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P2"));
    pi = completionModel->mapFromSource(si2);
    QCOMPARE(pi.isValid(), false);

    // more text
    completer->setCompletionPrefix("p2");
    pi = completionModel->mapFromSource(si1);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P2"));
    pi = completionModel->mapFromSource(si2);
    QCOMPARE(pi.isValid(), false);

    // invalid text
    completer->setCompletionPrefix("whatever");
    pi = completionModel->mapFromSource(si1);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P2"));
}

void tst_QCompleter::unsortedEngineMapFromSource()
{
    delete completer;
    completer = new CsvCompleter;
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    setSourceModel(HISTORY_MODEL); // case insensitively sorted model
    completer->setModelSorting(QCompleter::UnsortedModel);

    QModelIndex si, si2, si3, pi;
    QAbstractItemModel *sourceModel = completer->model();
    auto completionModel = qobject_cast<const QAbstractProxyModel *>(completer->completionModel());

    si = sourceModel->index(6, completionColumn); // "P3"
    QCOMPARE(si.data().toString(), QLatin1String("P3"));
    si2 = sourceModel->index(3, completionColumn, sourceModel->index(0, completionColumn)); // "P0,c3P0"
    QCOMPARE(si2.data().toString(), QLatin1String("c3P0"));
    si3 = sourceModel->index(10, completionColumn); // "p3,c3p3" (history)
    QCOMPARE(si3.data().toString(), QLatin1String("p3,c3p3"));

    // FILTERING ON
    // empty
    pi = completionModel->mapFromSource(si);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P3"));
    pi = completionModel->mapFromSource(si2);
    QCOMPARE(pi.isValid(), false);
    pi = completionModel->mapFromSource(si3);
   QCOMPARE(completionModel->data(pi).toString(), QLatin1String("p3,c3p3"));

    // some text
    completer->setCompletionPrefix("P");
    pi = completionModel->mapFromSource(si);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P3"));
    pi = completionModel->mapFromSource(si2);
    QCOMPARE(pi.isValid(), false);
    pi = completionModel->mapFromSource(si3);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("p3,c3p3"));

    // invalid text
    completer->setCompletionPrefix("whatever");
    pi = completionModel->mapFromSource(si);
    QVERIFY(!pi.isValid());
    pi = completionModel->mapFromSource(si2);
    QVERIFY(!pi.isValid());

    // tree matching
    completer->setCompletionPrefix("P0,c");
    pi = completionModel->mapFromSource(si);
    QVERIFY(!pi.isValid());
    pi = completionModel->mapFromSource(si2);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("c3P0"));
    pi = completionModel->mapFromSource(si3);
    QCOMPARE(pi.isValid(), false);

    // more tree matching
    completer->setCompletionPrefix("p3,");
    pi = completionModel->mapFromSource(si2);
    QVERIFY(!pi.isValid());
    pi = completionModel->mapFromSource(si3);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("p3,c3p3"));

    // FILTERING OFF
    // empty
    completer->setCompletionPrefix("");
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    pi = completionModel->mapFromSource(si);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P3"));

    // some text
    completer->setCompletionPrefix("P");
    pi = completionModel->mapFromSource(si);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P3"));

    // more text
    completer->setCompletionPrefix("P3");
    pi = completionModel->mapFromSource(si);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P3"));

    // invalid text
    completer->setCompletionPrefix("whatever");
    pi = completionModel->mapFromSource(si);
    QCOMPARE(completionModel->data(pi).toString(), QLatin1String("P3"));
}

void tst_QCompleter::historySearch()
{
    delete completer;
    completer = new CsvCompleter;
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    setSourceModel(HISTORY_MODEL);

    auto completionModel = qobject_cast<const QAbstractProxyModel *>(completer->completionModel());

    // "p3,c3p3" and "p2,c4p2" are added in the tree root

    // FILTERING ON
    // empty
    completer->setCurrentRow(10);
    QCOMPARE(completer->currentCompletion(), QLatin1String("p3,c3p3"));

    // more text
    completer->setCompletionPrefix("p2");
    completer->setCurrentRow(1);
    QCOMPARE(completer->currentCompletion(), QLatin1String("p2,c4p2"));

    // comma separated text
    completer->setCompletionPrefix("p2,c4");
    completer->setCurrentRow(1);
    QCOMPARE(completionModel->rowCount(), 2);
    QCOMPARE(completer->currentCompletion(), QLatin1String("p2,c4p2"));

    // invalid text
    completer->setCompletionPrefix("whatever");
    QCOMPARE(completer->currentCompletion(), QString());

    // FILTERING OFF
    completer->setCompletionPrefix("");
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    completer->setCurrentRow(10);
    QCOMPARE(completer->currentCompletion(), QLatin1String("p3,c3p3"));

    // more text
    completer->setCompletionPrefix("p2");
    completer->setCurrentRow(1);
    QCOMPARE(completer->currentCompletion(), QLatin1String("p2,c4p2"));

    // comma separated text
    completer->setCompletionPrefix("p2,c4");
    QCOMPARE(completionModel->rowCount(), 5);

    // invalid text
    completer->setCompletionPrefix("whatever");
    QCOMPARE(completer->currentCompletion(), QString());
}

void tst_QCompleter::setters()
{
    delete completer;
    completer = new CsvCompleter;
    QVERIFY(completer->popup() != nullptr);
    QPointer<QStandardItemModel> itemModel(new QStandardItemModel(1, 0, completer));
    QAbstractItemModel *oldModel = completer->model();
    completer->setModel(itemModel.data());
    QVERIFY(completer->popup()->model() != oldModel);
    QCOMPARE(completer->popup()->model(), completer->completionModel());
    completer->setPopup(new QListView);
    QCOMPARE(completer->popup()->model(), completer->completionModel());
    completer->setModel(new QStringListModel(completer));
    QVERIFY(itemModel.isNull()); // must have been deleted

    completer->setModel(nullptr);
    completer->setWidget(nullptr);
}

void tst_QCompleter::modelDeletion()
{
    delete completer;
    completer = new CsvCompleter;
    const QStringList list = {"item1", "item2", "item3"};
    std::unique_ptr<QStringListModel> listModel(new QStringListModel(list));
    completer->setCompletionPrefix("i");
    completer->setModel(listModel.get());
    QCOMPARE(completer->completionCount(), 3);
    std::unique_ptr<QListView> view(new QListView);
    view->setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    view->setModel(completer->completionModel());
    listModel.reset();
    view->move(200, 200);
    view->show();
    QCoreApplication::processEvents();
    view.reset();
    QCOMPARE(completer->completionCount(), 0);
    QCOMPARE(completer->currentRow(), -1);
}

void tst_QCompleter::multipleWidgets()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QStringList list;
    list << "item1" << "item2" << "item2";
    QCompleter completer(list);
    completer.setCompletionMode(QCompleter::InlineCompletion);

    QWidget window;
    window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    window.move(200, 200);
    window.show();
    QApplicationPrivate::setActiveWindow(&window);
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QFocusEvent focusIn(QEvent::FocusIn);
    QFocusEvent focusOut(QEvent::FocusOut);

    auto comboBox = new QComboBox(&window);
    comboBox->setEditable(true);
    comboBox->setCompleter(&completer);
    comboBox->setFocus();
    comboBox->show();
    window.activateWindow();
    QApplicationPrivate::setActiveWindow(&window);
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QCOMPARE(QApplication::focusWidget(), comboBox);
    comboBox->lineEdit()->setText("it");
    QCOMPARE(comboBox->currentText(), QString("it")); // should not complete with setText
    QTest::keyPress(comboBox, 'e');
    QCOMPARE(comboBox->currentText(), QString("item1"));
    comboBox->clearEditText();
    QCOMPARE(comboBox->currentText(), QString("")); // combo box text must not change!

    auto lineEdit = new QLineEdit(&window);
    lineEdit->setCompleter(&completer);
    lineEdit->show();
    lineEdit->setFocus();
    QTRY_COMPARE(QApplication::focusWidget(), lineEdit);
    lineEdit->setText("it");
    QCOMPARE(lineEdit->text(), QString("it")); // should not completer with setText
    QCOMPARE(comboBox->currentText(), QString("")); // combo box text must not change!
    QTest::keyPress(lineEdit, 'e');
    QCOMPARE(lineEdit->text(), QString("item1"));
    QCOMPARE(comboBox->currentText(), QString("")); // combo box text must not change!
}

void tst_QCompleter::focusIn()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QCompleter completer({"item1", "item2", "item2"});

    QWidget window;
    window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    window.move(200, 200);
    window.show();
    window.activateWindow();
    QApplicationPrivate::setActiveWindow(&window);
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto comboBox = new QComboBox(&window);
    comboBox->setEditable(true);
    comboBox->setCompleter(&completer);
    comboBox->show();
    comboBox->lineEdit()->setText("it");

    auto lineEdit = new QLineEdit(&window);
    lineEdit->setCompleter(&completer);
    lineEdit->setText("it");
    lineEdit->show();

    auto lineEdit2 = new QLineEdit(&window); // has no completer!
    lineEdit2->show();

    comboBox->setFocus();
    QTRY_COMPARE(completer.widget(), comboBox);
    lineEdit->setFocus();
    QTRY_COMPARE(completer.widget(), lineEdit);
    comboBox->setFocus();
    QTRY_COMPARE(completer.widget(), comboBox);
    lineEdit2->setFocus();
    QTRY_COMPARE(completer.widget(), comboBox);
}

void tst_QCompleter::dynamicSortOrder()
{
    QStandardItemModel model;
    QCompleter completer(&model);
    completer.setModelSorting(QCompleter::CaseSensitivelySortedModel);
    QStandardItem *root = model.invisibleRootItem();
    for (int i = 0; i < 20; i++) {
        root->appendRow(new QStandardItem(QString::number(i)));
    }
    root->appendRow(new QStandardItem("13"));
    root->sortChildren(0, Qt::AscendingOrder);
    completer.setCompletionPrefix("1");
    QCOMPARE(completer.completionCount(), 12);
    completer.setCompletionPrefix("13");
    QCOMPARE(completer.completionCount(), 2);

    root->sortChildren(0, Qt::DescendingOrder);
    completer.setCompletionPrefix("13");
    QCOMPARE(completer.completionCount(), 2);
    completer.setCompletionPrefix("1");
    QCOMPARE(completer.completionCount(), 12);
}

void tst_QCompleter::disabledItems()
{
    QLineEdit lineEdit;
    lineEdit.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    auto model = new QStandardItemModel(&lineEdit);
    QStandardItem *suggestions = new QStandardItem("suggestions");
    suggestions->setEnabled(false);
    model->appendRow(suggestions);
    model->appendRow(new QStandardItem("suggestions Enabled"));
    auto completer = new QCompleter(model, &lineEdit);
    QSignalSpy spy(completer, QOverload<const QString &>::of(&QCompleter::activated));
    lineEdit.setCompleter(completer);
    lineEdit.move(200, 200);
    lineEdit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&lineEdit));

    QTest::keyPress(&lineEdit, Qt::Key_S);
    QTest::keyPress(&lineEdit, Qt::Key_U);
    QAbstractItemView *view = lineEdit.completer()->popup();
    QVERIFY(view->isVisible());
    QTest::mouseClick(view->viewport(), Qt::LeftButton, {}, view->visualRect(view->model()->index(0, 0)).center());
    QCOMPARE(spy.size(), 0);
    QVERIFY(view->isVisible());
    QTest::mouseClick(view->viewport(), Qt::LeftButton, {}, view->visualRect(view->model()->index(1, 0)).center());
    QCOMPARE(spy.size(), 1);
    QVERIFY(!view->isVisible());
}

void tst_QCompleter::hideWidget()
{
    // hiding the widget should hide/close the popup
    QWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.setLayout(new QVBoxLayout);

    QLineEdit edit;
    edit.setCompleter(new QCompleter({ "foo", "bar" }));

    w.layout()->addWidget(&edit);

    const auto pos = w.screen()->availableGeometry().topLeft() + QPoint(200, 200);
    w.move(pos);
    w.show();
    QApplicationPrivate::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    // activate the completer
    QTest::keyClick(&edit, Qt::Key_F);
    QVERIFY(edit.completer()->popup());
    QTRY_VERIFY(edit.completer()->popup()->isVisible());
    edit.hide();
    QVERIFY(!edit.completer()->popup()->isVisible());
}

void tst_QCompleter::task178797_activatedOnReturn()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QLineEdit ledit;
    setFrameless(&ledit);
    auto completer = new QCompleter({"foobar1", "foobar2"}, &ledit);
    ledit.setCompleter(completer);
    QSignalSpy spy(completer, QOverload<const QString &>::of(&QCompleter::activated));
    QCOMPARE(spy.size(), 0);
    ledit.move(200, 200);
    ledit.show();
    QApplicationPrivate::setActiveWindow(&ledit);
    QVERIFY(QTest::qWaitForWindowActive(&ledit));
    QTest::keyClick(&ledit, Qt::Key_F);
    QCoreApplication::processEvents();
    QTRY_VERIFY(QApplication::activePopupWidget());
    QTest::keyClick(QApplication::activePopupWidget(), Qt::Key_Down);
    QCoreApplication::processEvents();
    QTest::keyClick(QApplication::activePopupWidget(), Qt::Key_Return);
    QCoreApplication::processEvents();
    QCOMPARE(spy.size(), 1);
}

class task189564_StringListModel : public QStringListModel
{
    const QString omitString;
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        Qt::ItemFlags flags = Qt::ItemIsEnabled;
        if (data(index, Qt::DisplayRole).toString() != omitString)
            flags |= Qt::ItemIsSelectable;
        return flags;
    }
public:
    explicit task189564_StringListModel(const QString &omitString, QObject *parent = nullptr)
        : QStringListModel(parent)
        , omitString(omitString)
    {
    }
};

void tst_QCompleter::task189564_omitNonSelectableItems()
{
    const QString prefix("a");
    const int n = 5;

    QStringList strings;
    for (int i = 0; i < n; ++i)
        strings << prefix + QString::number(i);
    const QString omitString(strings.at(n / 2));
    task189564_StringListModel model(omitString);
    model.setStringList(strings);
    QCompleter completer_(&model);
    completer_.setCompletionPrefix(prefix);

    QAbstractItemModel *completionModel = completer_.completionModel();
    QModelIndexList matches1 =
        completionModel->match(completionModel->index(0, 0), Qt::DisplayRole, prefix, -1);
    QCOMPARE(matches1.size(), n - 1);
    QModelIndexList matches2 =
        completionModel->match(completionModel->index(0, 0), Qt::DisplayRole, omitString);
    QVERIFY(matches2.isEmpty());
}

class task246056_ComboBox : public QComboBox
{
    Q_OBJECT
public:
    task246056_ComboBox()
    {
        setEditable(true);
        setInsertPolicy(NoInsert);
        if (completer()) {
            completer()->setCompletionMode(QCompleter::PopupCompletion);
            completer()->setCompletionRole(Qt::DisplayRole);
            connect(lineEdit(), &QLineEdit::editingFinished, this, &task246056_ComboBox::setCompletionPrefix);
        }
    }
private slots:
    void setCompletionPrefix() { completer()->setCompletionPrefix(lineEdit()->text()); }
};

void tst_QCompleter::task246056_setCompletionPrefix()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    task246056_ComboBox comboBox;
    setFrameless(&comboBox);
    QVERIFY(comboBox.completer());
    comboBox.addItem("");
    comboBox.addItem("a1");
    comboBox.addItem("a2");
    comboBox.move(200, 200);
    comboBox.show();
    QApplicationPrivate::setActiveWindow(&comboBox);
    QVERIFY(QTest::qWaitForWindowActive(&comboBox));
    QSignalSpy spy(comboBox.completer(), QOverload<const QModelIndex &>::of(&QCompleter::activated));
    QTest::keyPress(&comboBox, 'a');
    QTest::keyPress(comboBox.completer()->popup(), Qt::Key_Down);
    QTest::keyPress(comboBox.completer()->popup(), Qt::Key_Down);
    QTest::keyPress(comboBox.completer()->popup(), Qt::Key_Enter); // don't crash!
    QCOMPARE(spy.size(), 1);
    const auto index = spy.at(0).constFirst().toModelIndex();
    QVERIFY(!index.isValid());
}

class task250064_TextEdit : public QTextEdit
{
public:
    QCompleter *completer;

    task250064_TextEdit()
    {
        completer = new QCompleter(this);
        completer->setWidget(this);
    }

    void keyPressEvent (QKeyEvent *e) override
    {
        completer->popup();
        QTextEdit::keyPressEvent(e);
    }
};

class task250064_Widget : public QWidget
{
    Q_OBJECT
public:
    task250064_Widget() : m_textEdit(new task250064_TextEdit)
    {
        auto tabWidget = new QTabWidget;
        tabWidget->setFocusPolicy(Qt::ClickFocus);
        tabWidget->addTab(m_textEdit, "untitled");

        auto layout = new QVBoxLayout(this);
        layout->addWidget(tabWidget);

        m_textEdit->setPlainText("bla bla bla");
        m_textEdit->setFocus();
    }

    void setCompletionModel()
    {
        m_textEdit->completer->setModel(nullptr);
    }

    QTextEdit *textEdit() const { return m_textEdit; }

private:
    task250064_TextEdit *m_textEdit;
};

void tst_QCompleter::task250064_lostFocus()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    task250064_Widget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    widget.show();
    QApplicationPrivate::setActiveWindow(&widget);
    QVERIFY(QTest::qWaitForWindowActive(&widget));
    QTest::keyPress(widget.textEdit(), 'a');
    Qt::FocusPolicy origPolicy = widget.textEdit()->focusPolicy();
    QVERIFY(origPolicy != Qt::NoFocus);
    widget.setCompletionModel();
    QCOMPARE(widget.textEdit()->focusPolicy(), origPolicy);
}

void tst_QCompleter::task253125_lineEditCompletion_data()
{
    QTest::addColumn<QStringList>("list");
    QTest::addColumn<QCompleter::CompletionMode>("completionMode");

    QStringList list = {"alpha", "beta", "gamma", "delta", "epsilon", "zeta",
        "eta", "theta", "iota", "kappa", "lambda", "mu",
        "nu", "xi", "omicron", "pi", "rho", "sigma",
        "tau", "upsilon", "phi", "chi", "psi", "omega"};

    QTest::newRow("Inline") << list << QCompleter::InlineCompletion;
    QTest::newRow("Filtered") << list << QCompleter::PopupCompletion;
    QTest::newRow("Unfiltered") << list << QCompleter::UnfilteredPopupCompletion;
}

void tst_QCompleter::task253125_lineEditCompletion()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QFETCH(QStringList, list);
    QFETCH(QCompleter::CompletionMode, completionMode);

    std::unique_ptr<QStringListModel> model(new QStringListModel(list));

    std::unique_ptr<QCompleter> completer(new QCompleter(list));
    completer->setModel(model.get());
    completer->setCompletionMode(completionMode);

    QLineEdit edit;
    edit.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1String("::")
                        + QLatin1String(QTest::currentDataTag()));
    edit.setCompleter(completer.get());
    edit.move(200, 200);
    edit.show();
    edit.setFocus();
    QApplicationPrivate::setActiveWindow(&edit);
    QVERIFY(QTest::qWaitForWindowActive(&edit));

    QTest::keyClick(&edit, 'i');
    QCOMPARE(edit.completer()->currentCompletion(), QString("iota"));
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("iota"));

    edit.clear();
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setFilterMode(Qt::MatchContains);

    QTest::keyClick(&edit, 't');
    QCOMPARE(edit.completer()->currentCompletion(), QString("beta"));
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("beta"));

    edit.clear();

    QTest::keyClick(&edit, 'p');
    QTest::keyClick(&edit, 'p');
    QCOMPARE(edit.completer()->currentCompletion(), QString("kappa"));
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("kappa"));

    edit.clear();
    completer->setFilterMode(Qt::MatchStartsWith);

    QTest::keyClick(&edit, 't');
    QCOMPARE(edit.completer()->currentCompletion(), QString("theta"));
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("theta"));

    edit.clear();

    QTest::keyClick(&edit, 'p');
    QTest::keyClick(&edit, 'p');
    QCOMPARE(edit.completer()->currentCompletion(), QString());
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("pp"));

    edit.clear();

    QTest::keyClick(&edit, 'u');
    QTest::keyClick(&edit, 'p');
    QTest::keyClick(&edit, 's');
    QCOMPARE(edit.completer()->currentCompletion(), QString("upsilon"));
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("upsilon"));

    edit.clear();
    completer->setFilterMode(Qt::MatchEndsWith);

    QTest::keyClick(&edit, 'm');
    QTest::keyClick(&edit, 'm');
    QTest::keyClick(&edit, 'a');
    QCOMPARE(edit.completer()->currentCompletion(), QString("gamma"));
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("gamma"));

    edit.clear();

    QTest::keyClick(&edit, 'g');
    QTest::keyClick(&edit, 'm');
    QTest::keyClick(&edit, 'a');
    QCOMPARE(edit.completer()->currentCompletion(), QString("sigma"));
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("sigma"));

    edit.clear();

    QTest::keyClick(&edit, 'm');
    QTest::keyClick(&edit, 'm');
    QCOMPARE(edit.completer()->currentCompletion(), QString());
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("mm"));

    edit.clear();
    completer->setFilterMode(Qt::MatchStartsWith);

    QTest::keyClick(&edit, 'z');
    QTest::keyClick(&edit, 'e');
    QCOMPARE(edit.completer()->currentCompletion(), QString("zeta"));
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("zeta"));

    edit.clear();
    completer->setFilterMode(Qt::MatchEndsWith);

    QTest::keyClick(&edit, 'e');
    QTest::keyClick(&edit, 'g');
    QTest::keyClick(&edit, 'a');
    QCOMPARE(edit.completer()->currentCompletion(), QString("omega"));
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("omega"));

    edit.clear();
    completer->setFilterMode(Qt::MatchContains);

    QTest::keyClick(&edit, 'c');
    QTest::keyClick(&edit, 'r');
    QCOMPARE(edit.completer()->currentCompletion(), QString("omicron"));
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("omicron"));

    edit.clear();

    QTest::keyClick(&edit, 'z');
    QTest::keyClick(&edit, 'z');
    QCOMPARE(edit.completer()->currentCompletion(), QString());
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("zz"));
}

void tst_QCompleter::task247560_keyboardNavigation()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QStandardItemModel model;

    for (int i = 0; i < 5; i++) {
        const QString prefix = QLatin1String("row ") + QString::number(i) + QLatin1String(" column ");
        for (int j = 0; j < 5; j++)
            model.setItem(i, j, new QStandardItem(prefix + QString::number(j)));
    }


    QCompleter completer(&model);
    completer.setCompletionColumn(1);

    QLineEdit edit;
    edit.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    edit.setCompleter(&completer);
    edit.move(200, 200);
    edit.show();
    edit.setFocus();
    QApplicationPrivate::setActiveWindow(&edit);
    QVERIFY(QTest::qWaitForWindowActive(&edit));

    QTest::keyClick(&edit, 'r');
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("row 1 column 1"));

    edit.clear();

    QTest::keyClick(&edit, 'r');
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Up);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Up);
    QTest::keyClick(edit.completer()->popup(), Qt::Key_Enter);

    QCOMPARE(edit.text(), QString("row 3 column 1"));
}

// Helpers for QTBUG_14292_filesystem: Recursion helper for below recurseTreeModel
// Function to recurse over a tree model applying a function
// taking index and depth, returning true to terminate recursion.
template <class Function>
bool recurseTreeModel(const QAbstractItemModel &m, const QModelIndex &idx, Function f, int depth = 0)
{
    if (idx.isValid() && f(idx, depth))
        return true;
    const int rowCount = m.rowCount(idx);
    for (int row = 0; row < rowCount; ++row)
        if (recurseTreeModel(m, m.index(row, 0, idx), f, depth + 1))
            return true;
    return false;
}

// Function applicable to the above recurseTreeModel() to search for a data item.
class SearchFunction
{
public:
    SearchFunction(const QString &needle, int role = Qt::DisplayRole) :
        m_needle(needle), m_role(role) {}

    bool operator()(const QModelIndex &idx, int /* depth */) const
        { return idx.data(m_role).toString() == m_needle; }

private:
    const QString m_needle;
    const int m_role;
};

// Function applicable to the above recurseTreeModel() for debug output
// of a model.
class DebugFunction
{
public:
    DebugFunction(QDebug d) : m_d(d) {}

    bool operator()(const QModelIndex &idx, int depth)
    {
        for (int i = 0; i < 4 * depth; ++i)
            m_d << ' ';
        m_d << idx.data(QFileSystemModel::FileNameRole).toString()
            << '\n';
        return false;
    }
private:
    QDebug m_d;
};

QDebug operator<<(QDebug d, const QAbstractItemModel &m)
{
    QDebug dns = d.nospace();
    dns << '\n';
    recurseTreeModel(m, QModelIndex(), DebugFunction(dns));
    return d;
}

static const char testDir1[] = "hello";
static const char testDir2[] = "holla";

// Helper for QTBUG_14292_filesystem, checking whether both
// test directories are seen by the file system model for usage
// with QTRY_VERIFY.

static inline bool testFileSystemReady(const QAbstractItemModel &model)
{
    return recurseTreeModel(model, QModelIndex(), SearchFunction(QLatin1String(testDir1), QFileSystemModel::FileNameRole))
           && recurseTreeModel(model, QModelIndex(), SearchFunction(QLatin1String(testDir2), QFileSystemModel::FileNameRole));
}

void tst_QCompleter::QTBUG_14292_filesystem()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    // This test tests whether the creation of subdirectories
    // does not cause completers based on file system models
    // to pop up the completion list due to file changed signals.
    FileSystem fs;
    QFileSystemModel model;
    QSignalSpy filesAddedSpy(&model, &QAbstractItemModel::rowsInserted);
    model.setRootPath(fs.path());

    QVERIFY(fs.createDirectory(QLatin1String(testDir1)));
    QVERIFY(fs.createDirectory(QLatin1String(testDir2)));

    QLineEdit edit;
    edit.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QCompleter comp;
    comp.setModel(&model);
    edit.setCompleter(&comp);

    edit.move(200, 200);
    edit.show();
    QApplicationPrivate::setActiveWindow(&edit);
    QVERIFY(QTest::qWaitForWindowActive(&edit));
    QCOMPARE(QApplication::activeWindow(), &edit);
    edit.setFocus();
    QTRY_VERIFY(edit.hasFocus());

    // Wait for all file system model slots/timers to trigger
    // until the model sees the subdirectories.
    QTRY_VERIFY(testFileSystemReady(model));
    // But this should not cause the combo to pop up.
    QVERIFY(!comp.popup()->isVisible());
    edit.setText(fs.path());
    QTest::keyClick(&edit, '/');
    QTRY_VERIFY(comp.popup()->isVisible());
    QCOMPARE(comp.popup()->model()->rowCount(), 2);
    QApplication::processEvents();
    QCOMPARE(qApp->focusObject(), &edit); // for QTBUG_108522
    QTest::keyClick(&edit, 'h');
    QCOMPARE(comp.popup()->model()->rowCount(), 2);
    QTest::keyClick(&edit, 'e');
    QCOMPARE(comp.popup()->model()->rowCount(), 1);
    QTest::keyClick(&edit, 'r');
    QTRY_VERIFY(!comp.popup()->isVisible());
    QVERIFY(fs.createDirectory(QStringLiteral("hero")));
    if (!filesAddedSpy.wait())
        QSKIP("File system model didn't notify about new directory, skipping tests");
    QTRY_VERIFY(comp.popup()->isVisible());
    QCOMPARE(comp.popup()->model()->rowCount(), 1);
    QTest::keyClick(comp.popup(), Qt::Key_Escape);
    QTRY_VERIFY(!comp.popup()->isVisible());
    QVERIFY(fs.createDirectory(QStringLiteral("nothingThere")));
    //there is no reason creating a file should open a popup, it did in Qt 4.7.0
    if (!filesAddedSpy.wait())
        QSKIP("File system model didn't notify about new file, skipping tests");
    QVERIFY(!comp.popup()->isVisible());

    QTest::keyClick(&edit, Qt::Key_Backspace);
    QTRY_VERIFY(comp.popup()->isVisible());
    QCOMPARE(comp.popup()->model()->rowCount(), 2);
    QTest::keyClick(&edit, 'm');
    QTRY_VERIFY(!comp.popup()->isVisible());

    QWidget w;
    w.move(400, 200);
    w.show();
    QApplicationPrivate::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));
    QVERIFY(!edit.hasFocus() && !comp.popup()->hasFocus());

    QVERIFY(fs.createDirectory(QStringLiteral("hemo")));
    //there is no reason creating a file should open a popup, it did in Qt 4.7.0
    if (!filesAddedSpy.wait())
        QSKIP("File system model didn't notify about new file, skipping tests");
    QVERIFY(!comp.popup()->isVisible());
}

void tst_QCompleter::QTBUG_52028_tabAutoCompletes()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.setLayout(new QVBoxLayout);

    QComboBox cbox;
    cbox.setEditable(true);
    cbox.setInsertPolicy(QComboBox::NoInsert);
    cbox.addItems({"foobar1", "foobar2", "hux"});

    cbox.completer()->setCaseSensitivity(Qt::CaseInsensitive);
    cbox.completer()->setCompletionMode(QCompleter::PopupCompletion);

    w.layout()->addWidget(&cbox);

    // Adding a line edit is a good reason for tab to do something unrelated
    auto le = new QLineEdit;
    w.layout()->addWidget(le);

    const auto pos = w.screen()->availableGeometry().topLeft() + QPoint(200,200);
    w.move(pos);
    w.show();
    QApplicationPrivate::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QSignalSpy activatedSpy(&cbox, &QComboBox::activated);

    // Tab key will complete but not activate
    cbox.lineEdit()->clear();
    QTest::keyClick(&cbox, Qt::Key_H);
    QVERIFY(cbox.completer()->popup());
    QTRY_VERIFY(cbox.completer()->popup()->isVisible());
    QTest::keyClick(cbox.completer()->popup(), Qt::Key_Tab);
    QCOMPARE(cbox.completer()->currentCompletion(), QLatin1String("hux"));
    QCOMPARE(activatedSpy.size(), 0);
    QEXPECT_FAIL("", "QTBUG-52028 will not be fixed today.", Abort);
    QCOMPARE(cbox.currentText(), QLatin1String("hux"));
    QCOMPARE(activatedSpy.size(), 0);
    QVERIFY(!le->hasFocus());
}

void tst_QCompleter::QTBUG_51889_activatedSentTwice()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.setLayout(new QVBoxLayout);

    QComboBox cbox;
    setFrameless(&cbox);
    cbox.setEditable(true);
    cbox.setInsertPolicy(QComboBox::NoInsert);
    cbox.addItems({"foobar1", "foobar2", "bar", "hux"});

    cbox.completer()->setCaseSensitivity(Qt::CaseInsensitive);
    cbox.completer()->setCompletionMode(QCompleter::PopupCompletion);

    w.layout()->addWidget(&cbox);

    w.layout()->addWidget(new QLineEdit);

    const auto pos = w.screen()->availableGeometry().topLeft() + QPoint(200,200);
    w.move(pos);
    w.show();
    QApplicationPrivate::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QSignalSpy activatedSpy(&cbox, &QComboBox::activated);

    // Navigate + enter activates only once (first item)
    cbox.lineEdit()->clear();
    QTest::keyClick(&cbox, Qt::Key_F);
    QVERIFY(cbox.completer()->popup());
    QTRY_VERIFY(cbox.completer()->popup()->isVisible());
    QTest::keyClick(cbox.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(cbox.completer()->popup(), Qt::Key_Return);
    QTRY_COMPARE(activatedSpy.size(), 1);

    // Navigate + enter activates only once (non-first item)
    cbox.lineEdit()->clear();
    activatedSpy.clear();
    QTest::keyClick(&cbox, Qt::Key_H);
    QVERIFY(cbox.completer()->popup());
    QTRY_VERIFY(cbox.completer()->popup()->isVisible());
    QTest::keyClick(cbox.completer()->popup(), Qt::Key_Down);
    QTest::keyClick(cbox.completer()->popup(), Qt::Key_Return);
    QTRY_COMPARE(activatedSpy.size(), 1);

    // Full text + enter activates only once
    cbox.lineEdit()->clear();
    activatedSpy.clear();
    QTest::keyClicks(&cbox, "foobar1");
    QVERIFY(cbox.completer()->popup());
    QTRY_VERIFY(cbox.completer()->popup()->isVisible());
    QTest::keyClick(&cbox, Qt::Key_Return);
    QTRY_COMPARE(activatedSpy.size(), 1);
}

void tst_QCompleter::showPopupInGraphicsView()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: Skip this test, see also QTBUG-107186");

    QGraphicsView view;
    QGraphicsScene scene;
    view.setScene(&scene);

    QLineEdit lineEdit;
    lineEdit.setCompleter(new QCompleter({"alpha", "omega", "omicron", "zeta"}));
    scene.addWidget(&lineEdit);

    view.move(view.screen()->availableGeometry().topLeft() + QPoint(10, 10));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // show popup under line edit
    QTest::keyClick(&lineEdit, Qt::Key_A);
    QVERIFY(lineEdit.completer()->popup());
    QVERIFY(lineEdit.completer()->popup()->isVisible());
    QCOMPARE(lineEdit.completer()->popup()->geometry().x(), lineEdit.mapToGlobal(QPoint(0, 0)).x());
    QVERIFY(lineEdit.completer()->popup()->geometry().top() >= (lineEdit.mapToGlobal(QPoint(0, lineEdit.height() - 1)).y() - 1));

    // move widget to the bottom of screen
    lineEdit.clear();
    int y = view.screen()->availableGeometry().height() - lineEdit.geometry().y();
    view.move(view.geometry().x(), y);

    // show popup above line edit
    QTest::keyClick(&lineEdit, Qt::Key_A);
    QVERIFY(lineEdit.completer()->popup()->geometry().bottom() < lineEdit.mapToGlobal(QPoint(0, 0)).y());
}

void tst_QCompleter::inheritedEventFilter()
{
    class Completer : public QCompleter
    {
    public:
        explicit Completer(QWidget *parent) : QCompleter(parent)
        {
            Q_ASSERT(parent);
            setPopup(new QListView());
            popup()->installEventFilter(this);
        }

        bool m_popupChildAdded = false;

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override
        {
            if (watched == popup() && event->type() == QEvent::ChildAdded)
                m_popupChildAdded = true;

            return QCompleter::eventFilter(watched, event);
        }
    };

    QComboBox comboBox;
    comboBox.setEditable(true);
    Completer *completer = new Completer(&comboBox);
    comboBox.setCompleter(completer);

    // comboBox.show() must not crash with an infinite loop in the event filter
    comboBox.show();
    QVERIFY(QTest::qWaitForWindowExposed(&comboBox));

    // Since event orders are platform dependent, only the the ChildAdded event is checked.
    QVERIFY(QTest::qWaitFor([completer](){return completer->m_popupChildAdded; }));
}

QTEST_MAIN(tst_QCompleter)
#include "tst_qcompleter.moc"
