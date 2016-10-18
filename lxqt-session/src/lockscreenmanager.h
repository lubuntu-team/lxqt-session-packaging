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

#ifndef LOCKSCREENMANAGER_H
#define LOCKSCREENMANAGER_H

#include <QObject>
#include <QEventLoop>
#include <QDBusInterface>
#include <QDBusUnixFileDescriptor>
#include <LXQt/ScreenSaver>

class LockScreenProvider : public QObject
{
    Q_OBJECT

public:
    virtual ~LockScreenProvider() {}

    virtual bool isValid() = 0;
    virtual bool inhibit() = 0;
    virtual void release() = 0;

signals:
    void aboutToSleep(bool beforeSleep);
    void lockRequested();
};

class LogindProvider : public LockScreenProvider
{
    Q_OBJECT

public:
    explicit LogindProvider();
    virtual ~LogindProvider();

    bool isValid() override;
    bool inhibit() override;
    void release() override;

private:
    QDBusInterface mInterface;
    QDBusUnixFileDescriptor mFileDescriptor;
};

class ConsoleKit2Provider : public LockScreenProvider
{
    Q_OBJECT

public:
    explicit ConsoleKit2Provider();
    virtual ~ConsoleKit2Provider();

    bool isValid() override;
    bool inhibit() override;
    void release() override;

private:
    QDBusInterface mInterface;
    bool mMethodInhibitPresent;
    QDBusUnixFileDescriptor mFileDescriptor;
};

class LockScreenManager : public QObject
{
    Q_OBJECT

public:
    explicit LockScreenManager(QObject *parent = nullptr);
    virtual ~LockScreenManager();

    bool startup(bool lockBeforeSleep);

private:
    LockScreenProvider *mProvider;

    // screensaver
    LXQt::ScreenSaver mScreenSaver;
    QEventLoop mLoop;
};

#endif
