// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_GUI_BUILD_REMOVED_API

#include "qtguiglobal.h"

QT_USE_NAMESPACE

#if QT_GUI_REMOVED_SINCE(6, 4)

#include "qpagesize.h" // removed duplicate declaration of op==
                       // (still caused an symbol on some platforms)

// #include "qotherheader.h"
// // implement removed functions from qotherheader.h
// order sections alphabetically

#endif // QT_GUI_REMOVED_SINCE(6, 4)

#if QT_GUI_REMOVED_SINCE(6, 6)

#include "qpixmapcache.h" // inlined API

// #include "qotherheader.h"
// // implement removed functions from qotherheader.h
// order sections alphabetically

#endif // QT_GUI_REMOVED_SINCE(6, 6)

#if QT_GUI_REMOVED_SINCE(6, 7)

#include "qtextdocument.h"

bool Qt::mightBeRichText(const QString& text)
{
    return Qt::mightBeRichText(qToStringViewIgnoringNull(text));
}

// #include "qotherheader.h"
// // implement removed functions from qotherheader.h
// order sections alphabetically

#endif // QT_GUI_REMOVED_SINCE(6, 7)
