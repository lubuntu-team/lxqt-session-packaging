/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * http://lxqt.org/
 *
 * Copyright: 2016 LXQt team
 * Authors:
 *   Paulo Lieuthier <paulolieuthier@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "lockscreenmanager.h"
#include "log.h"
#include <QTimer>
#include <QDBusReply>
#include <QDebug>
#include <unistd.h>

LockScreenManager::LockScreenManager(QObject *parent) :
    QObject(parent)
{
}

LockScreenManager::~LockScreenManager()
{
    delete mProvider;
}

bool LockScreenManager::startup(bool lockBeforeSleep)
{
    mProvider = new LogindProvider;
    if (!mProvider->isValid())
    {
        mProvider->deleteLater();
        mProvider = new ConsoleKit2Provider;
        if (!mProvider->isValid())
        {
            mProvider->deleteLater();
            mProvider = nullptr;
        }
    }

    if (!mProvider)
    {
        qCDebug(SESSION) << "LockScreenManager: no valid provider";
        return false;
    }

    qCDebug(SESSION) << "LockScreenManager:"
            << mProvider->metaObject()->className()
            << "will be used";

    connect(&mScreenSaver, &LXQt::ScreenSaver::done, &mLoop, &QEventLoop::quit);

    connect(mProvider, &LockScreenProvider::lockRequested, [this] {
        mScreenSaver.lockScreen();
        mLoop.exec();
    });

    if (lockBeforeSleep)
    {
        connect(mProvider,
                &LockScreenProvider::aboutToSleep,
                [this] (bool beforeSleep)
        {
            if (beforeSleep)
            {
                qCDebug(SESSION) << "LockScreenManager: system is about to sleep";

                mScreenSaver.lockScreen();
                mLoop.exec();
                mProvider->release();
            }
            else
                mProvider->inhibit();
        });

        if (!mProvider->inhibit())
            qCDebug(SESSION) << "LockScreenManager:"
                    << "could not inhibit session provider";
    }

    return true;
}

/*
 * Logind Provider
 */

LogindProvider::LogindProvider() :
    mInterface(QStringLiteral("org.freedesktop.login1"),
            QStringLiteral("/org/freedesktop/login1"),
            QStringLiteral("org.freedesktop.login1.Manager"),
            QDBusConnection::systemBus())
{
    if (mInterface.isValid())
    {
        connect(&mInterface, SIGNAL(PrepareForSleep(bool)),
                this, SIGNAL(aboutToSleep(bool)));

        QString sessionId = QDBusInterface(
                QStringLiteral("org.freedesktop.login1"),
                QStringLiteral("/org/freedesktop/login1/session/self"),
                QStringLiteral("org.freedesktop.login1.Session"),
                QDBusConnection::systemBus())
            .property("Id").toString();

        // listen to Lock signal as well
        mInterface.connection().connect(
                QStringLiteral("org.freedesktop.login1"),
                QStringLiteral("/org/freedesktop/login1/session/") + sessionId,
                QStringLiteral("org.freedesktop.login1.Session"),
                QStringLiteral("Lock"),
                this,
                SIGNAL(lockRequested()));
    }
}

LogindProvider::~LogindProvider()
{
    release();
}

bool LogindProvider::isValid()
{
    return mInterface.isValid();
}

bool LogindProvider::inhibit()
{
    if (mFileDescriptor.isValid())
        return false;

    QDBusReply<QDBusUnixFileDescriptor> reply = mInterface.call(
            QStringLiteral("Inhibit"),
            QStringLiteral("sleep"),
            QStringLiteral("LXQt Session"),
            QStringLiteral("Start screen locker before sleep."),
            QStringLiteral("delay"));

    if (!reply.isValid())
    {
        qCDebug(SESSION) << "LockScreenManager: " << reply.error();
        return false;
    }

    mFileDescriptor = reply.value();
    return true;
}

void LogindProvider::release()
{
    if (mFileDescriptor.isValid())
    {
        ::close(mFileDescriptor.fileDescriptor());
        mFileDescriptor.setFileDescriptor(-1);
    }
}

/*
 * ConsoleKit2 provider
 */

ConsoleKit2Provider::ConsoleKit2Provider() :
    mInterface(QStringLiteral("org.freedesktop.ConsoleKit"),
            QStringLiteral("/org/freedesktop/ConsoleKit/Manager"),
            QStringLiteral("org.freedesktop.ConsoleKit.Manager"),
            QDBusConnection::systemBus()),
    mMethodInhibitPresent(false)

{
    // validate interface, look for melhod Inhibit
    if (mInterface.isValid())
    {
        // this is needed to differentiate ConsoleKit2 from ConsoleKit
        QDBusReply<QString> reply = QDBusInterface(
                QStringLiteral("org.freedesktop.ConsoleKit"),
                QStringLiteral("/org/freedesktop/ConsoleKit/Manager"),
                QStringLiteral("org.freedesktop.DBus.Introspectable"),
                QDBusConnection::systemBus())
            .call(QStringLiteral("Introspect"));
        mMethodInhibitPresent = reply.value().contains("Inhibit");

        if (mMethodInhibitPresent)
        {
            connect(&mInterface, SIGNAL(PrepareForSleep(bool)),
                    this, SIGNAL(aboutToSleep(bool)));

            QDBusReply<QDBusObjectPath> sessionObjectPath = mInterface
                .call("GetCurrentSession");

            // listen to Lock signal as well
            mInterface.connection().connect(
                    QStringLiteral("org.freedesktop.ConsoleKit"),
                    sessionObjectPath.value().path(),
                    QStringLiteral("org.freedesktop.ConsoleKit.Session"),
                    QStringLiteral("Lock"),
                    this,
                    SIGNAL(lockRequested()));
        }
    }
}

ConsoleKit2Provider::~ConsoleKit2Provider()
{
    release();
}

bool ConsoleKit2Provider::isValid()
{
    return mInterface.isValid() && mMethodInhibitPresent;
}

bool ConsoleKit2Provider::inhibit()
{
    if (mFileDescriptor.isValid())
        return false;

    QDBusReply<QDBusUnixFileDescriptor> reply = mInterface.call(
            QStringLiteral("Inhibit"),
            QStringLiteral("sleep"),
            QStringLiteral("LXQt Power Management"),
            QStringLiteral("Start screen locker before sleep."),
            QStringLiteral("delay"));

    if (!reply.isValid())
    {
        qCDebug(SESSION) << "LockScreenWatcher: " << reply.error();
        return false;
    }

    mFileDescriptor = reply.value();
    return true;
}

void ConsoleKit2Provider::release()
{
    if (mFileDescriptor.isValid())
    {
        ::close(mFileDescriptor.fileDescriptor());
        mFileDescriptor.setFileDescriptor(-1);
    }
}
