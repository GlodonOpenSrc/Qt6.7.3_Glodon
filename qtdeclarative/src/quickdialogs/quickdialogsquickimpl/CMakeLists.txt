# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QuickDialogs2QuickImpl Module:
#####################################################################

set(qml_files
    "qml/ColorDialog.qml"
    "qml/ColorInputs.qml"
    "qml/FileDialog.qml"
    "qml/FileDialogDelegate.qml"
    "qml/FileDialogDelegateLabel.qml"
    "qml/FolderBreadcrumbBar.qml"
    "qml/FolderDialog.qml"
    "qml/FolderDialogDelegate.qml"
    "qml/FolderDialogDelegateLabel.qml"
    "qml/FontDialog.qml"
    "qml/FontDialogContent.qml"
    "qml/HueGradient.qml"
    "qml/MessageDialog.qml"
    "qml/PickerHandle.qml"
    "qml/SaturationLightnessPicker.qml"
    "qml/+Fusion/ColorDialog.qml"
    "qml/+Fusion/FileDialog.qml"
    "qml/+Fusion/FileDialogDelegate.qml"
    "qml/+Fusion/FolderBreadcrumbBar.qml"
    "qml/+Fusion/FolderDialog.qml"
    "qml/+Fusion/FolderDialogDelegate.qml"
    "qml/+Fusion/FontDialog.qml"
    "qml/+Fusion/MessageDialog.qml"
    "qml/+Imagine/ColorDialog.qml"
    "qml/+Imagine/FileDialog.qml"
    "qml/+Imagine/FileDialogDelegate.qml"
    "qml/+Imagine/FolderBreadcrumbBar.qml"
    "qml/+Imagine/FolderDialog.qml"
    "qml/+Imagine/FolderDialogDelegate.qml"
    "qml/+Imagine/FontDialog.qml"
    "qml/+Imagine/MessageDialog.qml"
    "qml/+Material/ColorDialog.qml"
    "qml/+Material/FileDialog.qml"
    "qml/+Material/FileDialogDelegate.qml"
    "qml/+Material/FolderBreadcrumbBar.qml"
    "qml/+Material/FolderDialog.qml"
    "qml/+Material/FolderDialogDelegate.qml"
    "qml/+Material/FontDialog.qml"
    "qml/+Material/MessageDialog.qml"
    "qml/+Universal/ColorDialog.qml"
    "qml/+Universal/FileDialog.qml"
    "qml/+Universal/FileDialogDelegate.qml"
    "qml/+Universal/FolderBreadcrumbBar.qml"
    "qml/+Universal/FolderDialog.qml"
    "qml/+Universal/FolderDialogDelegate.qml"
    "qml/+Universal/FontDialog.qml"
    "qml/+Universal/MessageDialog.qml"
)

qt_internal_add_qml_module(QuickDialogs2QuickImpl
    URI "QtQuick.Dialogs.quickimpl"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickDialogs2QuickImplPlugin
    PLUGIN_TARGET qtquickdialogs2quickimplplugin
    DEPENDENCIES
        QtQuick/auto
        QtQuick.Templates/auto
        QtQuick.Layouts/auto
    SOURCES
        qquickabstractcolorpicker.cpp
        qquickabstractcolorpicker_p.h
        qquickabstractcolorpicker_p_p.h
        qquickcolordialogimpl.cpp
        qquickcolordialogimpl_p.h
        qquickcolordialogimpl_p_p.h
        qquickcolordialogutils_p.h
        qquickcolordialogutils.cpp
        qquickcolorinputs.cpp
        qquickcolorinputs_p.h
        qquickdialogimplfactory.cpp
        qquickdialogimplfactory_p.h
        qquickmessagedialogimpl.cpp
        qquickmessagedialogimpl_p.h
        qquickmessagedialogimpl_p_p.h
        qquickplatformcolordialog.cpp
        qquickplatformcolordialog_p.h
        qquickplatformmessagedialog.cpp
        qquickplatformmessagedialog_p.h
        qquicksaturationlightnesspicker.cpp
        qquicksaturationlightnesspicker_p.h
        qtquickdialogs2quickimplforeign.cpp
        qtquickdialogs2quickimplforeign_p.h
        qtquickdialogs2quickimplglobal_p.h
    QML_FILES
        ${qml_files}
    DEFINES
        QT_BUILD_QUICKDIALOGS2QUICKIMPL_LIB
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2ImplPrivate
        Qt::QuickDialogs2Utils
        Qt::QuickDialogs2UtilsPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

add_dependencies(QuickDialogs2QuickImpl Qt::QuickControls2Basic)

if(QT_FEATURE_quickcontrols2_fusion)
    add_dependencies(QuickDialogs2QuickImpl QuickControls2Fusion)
endif()

if(QT_FEATURE_quickcontrols2_imagine)
    add_dependencies(QuickDialogs2QuickImpl QuickControls2Imagine)
endif()

if(QT_FEATURE_quickcontrols2_material)
    add_dependencies(QuickDialogs2QuickImpl QuickControls2Material)
endif()

if(QT_FEATURE_quickcontrols2_universal)
    add_dependencies(QuickDialogs2QuickImpl QuickControls2Universal)
endif()

qt_internal_extend_target(QuickDialogs2QuickImpl CONDITION QT_FEATURE_quick_listview
    SOURCES
        qquickfiledialogdelegate.cpp
        qquickfiledialogdelegate_p.h
        qquickfiledialogimpl.cpp
        qquickfiledialogimpl_p.h
        qquickfiledialogimpl_p_p.h
        qquickfolderbreadcrumbbar.cpp
        qquickfolderbreadcrumbbar_p.h
        qquickfolderbreadcrumbbar_p_p.h
        qquickfolderdialogimpl.cpp
        qquickfolderdialogimpl_p.h
        qquickfolderdialogimpl_p_p.h
        qquickfontdialogimpl.cpp
        qquickfontdialogimpl_p.h
        qquickfontdialogimpl_p_p.h
        qquickplatformfiledialog.cpp
        qquickplatformfiledialog_p.h
        qquickplatformfolderdialog.cpp
        qquickplatformfolderdialog_p.h
        qquickplatformfontdialog.cpp
        qquickplatformfontdialog_p.h
)

# Resources:
set(qtquickdialogs2quickimpl_resource_files
    "images/checkers.png"
    "images/crumb-separator-icon-round.png"
    "images/crumb-separator-icon-round@2x.png"
    "images/crumb-separator-icon-round@3x.png"
    "images/crumb-separator-icon-round@4x.png"
    "images/crumb-separator-icon-square.png"
    "images/crumb-separator-icon-square@2x.png"
    "images/crumb-separator-icon-square@3x.png"
    "images/crumb-separator-icon-square@4x.png"
    "images/eye-dropper.png"
    "images/eye-dropper@2x.png"
    "images/eye-dropper@3x.png"
    "images/eye-dropper@4x.png"
    "images/file-icon-round.png"
    "images/file-icon-round@2x.png"
    "images/file-icon-round@3x.png"
    "images/file-icon-round@4x.png"
    "images/file-icon-square.png"
    "images/file-icon-square@2x.png"
    "images/file-icon-square@3x.png"
    "images/file-icon-square@4x.png"
    "images/folder-icon-round.png"
    "images/folder-icon-round@2x.png"
    "images/folder-icon-round@3x.png"
    "images/folder-icon-round@4x.png"
    "images/folder-icon-square.png"
    "images/folder-icon-square@2x.png"
    "images/folder-icon-square@3x.png"
    "images/folder-icon-square@4x.png"
    "images/imagine/filedialogdelegate-background-disabled.9.png"
    "images/imagine/filedialogdelegate-background-disabled@2x.9.png"
    "images/imagine/filedialogdelegate-background-disabled@3x.9.png"
    "images/imagine/filedialogdelegate-background-disabled@4x.9.png"
    "images/imagine/filedialogdelegate-background-focused.9.png"
    "images/imagine/filedialogdelegate-background-focused@2x.9.png"
    "images/imagine/filedialogdelegate-background-focused@3x.9.png"
    "images/imagine/filedialogdelegate-background-focused@4x.9.png"
    "images/imagine/filedialogdelegate-background-highlighted.9.png"
    "images/imagine/filedialogdelegate-background-highlighted@2x.9.png"
    "images/imagine/filedialogdelegate-background-highlighted@3x.9.png"
    "images/imagine/filedialogdelegate-background-highlighted@4x.9.png"
    "images/imagine/filedialogdelegate-background-pressed.9.png"
    "images/imagine/filedialogdelegate-background-pressed@2x.9.png"
    "images/imagine/filedialogdelegate-background-pressed@3x.9.png"
    "images/imagine/filedialogdelegate-background-pressed@4x.9.png"
    "images/imagine/filedialogdelegate-background.9.png"
    "images/imagine/filedialogdelegate-background@2x.9.png"
    "images/imagine/filedialogdelegate-background@3x.9.png"
    "images/imagine/filedialogdelegate-background@4x.9.png"
    "images/up-icon-round.png"
    "images/up-icon-round@2x.png"
    "images/up-icon-round@3x.png"
    "images/up-icon-round@4x.png"
    "images/up-icon-square.png"
    "images/up-icon-square@2x.png"
    "images/up-icon-square@3x.png"
    "images/up-icon-square@4x.png"
    "images/up-icon-thick-square.png"
    "images/up-icon-thick-square@2x.png"
    "images/up-icon-thick-square@3x.png"
    "images/up-icon-thick-square@4x.png"
)

qt_internal_add_resource(QuickDialogs2QuickImpl "QuickDialogs2QuickImpl"
    PREFIX
        "/qt-project.org/imports/QtQuick/Dialogs/quickimpl"
    FILES
        ${qtquickdialogs2quickimpl_resource_files}
)

qt_internal_add_shaders(QuickDialogs2QuickImpl "QuickDialogs2QuickImplShaders"
    PREFIX
        "/qt-project.org/imports/QtQuick/Dialogs/quickimpl"
    FILES
        "shaders/SaturationLightness.frag"
    SILENT
)
