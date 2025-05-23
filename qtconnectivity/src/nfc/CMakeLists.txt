# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## Nfc Module:
#####################################################################

qt_internal_add_module(Nfc
    SOURCES
        qndeffilter.cpp qndeffilter.h
        qndefmessage.cpp qndefmessage.h
        qndefnfcsmartposterrecord.cpp qndefnfcsmartposterrecord.h qndefnfcsmartposterrecord_p.h
        qndefnfctextrecord.cpp qndefnfctextrecord.h
        qndefnfcurirecord.cpp qndefnfcurirecord.h
        qndefrecord.cpp qndefrecord.h qndefrecord_p.h
        qnearfieldmanager.cpp qnearfieldmanager.h qnearfieldmanager_p.h
        qnearfieldtarget.cpp qnearfieldtarget.h qnearfieldtarget_p.cpp qnearfieldtarget_p.h
        qtnfcglobal.h qtnfcglobal_p.h
    DEFINES
        QT_NO_CONTEXTLESS_CONNECT
    PUBLIC_LIBRARIES
        Qt::Core
    GENERATE_CPP_EXPORTS
)

#### Keys ignored in scope 1:.:.:nfc.pro:<TRUE>:
# OTHER_FILES = "doc/src/*.qdoc"

## Scopes:
#####################################################################

if(ANDROID AND (ANDROID AND NOT ANDROID_EMBEDDED))
    set_property(TARGET Nfc APPEND PROPERTY QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
        jar/Qt${QtConnectivity_VERSION_MAJOR}AndroidNfc.jar:org.qtproject.qt.android.nfc.QtNfc
    )
    set_property(TARGET Nfc APPEND PROPERTY QT_ANDROID_PERMISSIONS
        android.permission.NFC
    )
    set(NFC_BACKEND_AVAILABLE ON)
endif()

qt_internal_extend_target(Nfc CONDITION ANDROID AND NOT ANDROID_EMBEDDED
    SOURCES
        android/androidjninfc.cpp android/androidjninfc_p.h
        android/androidmainnewintentlistener.cpp android/androidmainnewintentlistener_p.h
        qnearfieldmanager_android.cpp qnearfieldmanager_android_p.h
        qnearfieldtarget_android.cpp qnearfieldtarget_android_p.h
    DEFINES
        ANDROID_NFC
        QT_ANDROID_NFC
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
)

if(IOS)
    # special case begin
    set(NFC_BACKEND_AVAILABLE ON)
    qt_disable_apple_app_extension_api_only(Nfc)
    qt_internal_find_apple_system_framework(FWCoreNFC CoreNFC)
    # special case end
endif()

qt_internal_extend_target(Nfc CONDITION IOS
    SOURCES
        ios/qiostagreaderdelegate.mm ios/qiostagreaderdelegate_p.h
        ios/qiosnfcndefsessiondelegate.mm ios/qiosnfcndefsessiondelegate_p.h
        ios/qiosndefnotifier.cpp ios/qiosndefnotifier_p.h
        qnearfieldmanager_ios.mm qnearfieldmanager_ios_p.h
        qnearfieldtarget_ios.mm qnearfieldtarget_ios_p.h
    DEFINES
        IOS_NFC
    LIBRARIES
        ${FWCoreNFC}
        Qt::CorePrivate
)

if(QT_FEATURE_pcsclite)
    set(NFC_BACKEND_AVAILABLE ON)
endif()

qt_internal_extend_target(Nfc CONDITION QT_FEATURE_pcsclite
    SOURCES
        qnearfieldmanager_pcsc.cpp qnearfieldmanager_pcsc_p.h
        qnearfieldtarget_pcsc.cpp qnearfieldtarget_pcsc_p.h
        qapduutils.cpp qapduutils_p.h
        pcsc/qpcsc.cpp pcsc/qpcsc_p.h
        pcsc/qpcscmanager.cpp pcsc/qpcscmanager_p.h
        pcsc/qpcscslot.cpp pcsc/qpcscslot_p.h
        pcsc/qpcsccard.cpp pcsc/qpcsccard_p.h
        ndef/qndefaccessfsm_p.h
        ndef/qnfctagtype4ndeffsm.cpp ndef/qnfctagtype4ndeffsm_p.h
    DEFINES
        PCSC_NFC
    LIBRARIES
        PkgConfig::PCSCLITE
)

#### Keys ignored in scope 2:.:.:nfc.pro:ANDROID AND NOT ANDROID_EMBEDDED:
# NFC_BACKEND_AVAILABLE = "yes"

qt_internal_extend_target(Nfc CONDITION NOT NFC_BACKEND_AVAILABLE
    SOURCES
        qnearfieldmanager_generic.cpp qnearfieldmanager_generic_p.h
)
qt_internal_add_docs(Nfc
    doc/qtnfc.qdocconf
)

