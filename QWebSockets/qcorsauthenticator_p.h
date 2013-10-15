#ifndef QCORSAUTHENTICATOR_P_H
#define QCORSAUTHENTICATOR_P_H

#include <qglobal.h>    //for QT_BEGIN_NAMESPACE
#include <QString>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
QT_BEGIN_NAMESPACE

class QCorsAuthenticatorPrivate
{
public:
    QCorsAuthenticatorPrivate(const QString &origin, bool allowed);
    ~QCorsAuthenticatorPrivate();

    QString m_origin;
    bool m_isAllowed;
};

#endif // QCORSAUTHENTICATOR_P_H
