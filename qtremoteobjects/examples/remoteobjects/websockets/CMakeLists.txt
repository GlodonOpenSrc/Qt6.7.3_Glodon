# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(TARGET Qt::WebSockets AND TARGET Qt::Widgets)
    qt_internal_add_example(wsclient)
    qt_internal_add_example(wsserver)
endif()
