// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "custominputhandler.h"

CustomInputHandler::CustomInputHandler(QAbstract3DGraph *graph, QObject *parent)
    : QAbstract3DInputHandler(parent)
    , m_graph(graph)
{
    QObject::connect(m_graph,
                     &QAbstract3DGraph::queriedGraphPositionChanged,
                     this,
                     &CustomInputHandler::onPositionQueryChanged);
}

//! [0]
void CustomInputHandler::mouseMoveEvent(QMouseEvent *event, const QPoint &mousePos)
{
    Q_UNUSED(event);
    setInputPosition(mousePos, true);
}
//! [0]

//! [1]
void CustomInputHandler::wheelEvent(QWheelEvent *event)
{
    // Adjust zoom level based on what zoom range we're in.
    int zoomLevel = this->cameraZoomLevel();
    if (zoomLevel > 100)
        zoomLevel += event->angleDelta().y() / 12;
    else if (zoomLevel > 50)
        zoomLevel += event->angleDelta().y() / 60;
    else
        zoomLevel += event->angleDelta().y() / 120;
    if (zoomLevel > 500)
        zoomLevel = 500;
    else if (zoomLevel < 10)
        zoomLevel = 10;

    this->setCameraZoomLevel(zoomLevel);
}

void CustomInputHandler::mousePressEvent(QMouseEvent *event, const QPoint &mousePos)
{
    Q_UNUSED(event);
    m_graph->scene()->setGraphPositionQuery(mousePos);
}

void CustomInputHandler::onPositionQueryChanged(const QVector3D &position)
{
    qDebug() << "Queried Position from signal:" << position;
}
//! [1]
