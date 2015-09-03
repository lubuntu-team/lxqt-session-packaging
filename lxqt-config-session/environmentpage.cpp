/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org, http://lxde.org/
 *
 * Copyright: 2010-2012 LXQt team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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

#include "environmentpage.h"
#include "ui_environmentpage.h"

EnvironmentPage::EnvironmentPage(LXQt::Settings *settings, QWidget *parent) :
    QWidget(parent),
    m_settings(settings),
    ui(new Ui::EnvironmentPage)
{
    ui->setupUi(this);

    connect(ui->addButton, SIGNAL(clicked()), SLOT(addButton_clicked()));
    connect(ui->deleteButton, SIGNAL(clicked()), SLOT(deleteButton_clicked()));
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            SLOT(itemChanged(QTreeWidgetItem*,int)));

    /* restoreSettings() is called from SessionConfigWindow
       after connections with DefaultApps have been set up */
}

EnvironmentPage::~EnvironmentPage()
{
    delete ui;
}

void EnvironmentPage::restoreSettings()
{
    m_settings->beginGroup("Environment");
    QString value;
    ui->treeWidget->clear();
    foreach (const QString& i, m_settings->childKeys())
    {
        value = m_settings->value(i).toString();
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget, QStringList() << i << value);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->treeWidget->addTopLevelItem(item);
        emit envVarChanged(i, value);
    }

    if (m_settings->value("BROWSER").isNull())
        emit envVarChanged("BROWSER", "");
    if (m_settings->value("TERM").isNull())
        emit envVarChanged("TERM", "");

    m_settings->endGroup();
}

void EnvironmentPage::save()
{
    m_settings->beginGroup("Environment");
    m_settings->remove("");
    for(int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = ui->treeWidget->topLevelItem(i);
        m_settings->setValue(item->text(0), item->text(1));
    }
    m_settings->endGroup();
}

void EnvironmentPage::addButton_clicked()
{
    QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget, QStringList() << "" << "");
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    ui->treeWidget->addTopLevelItem(item);
    ui->treeWidget->setCurrentItem(item);
    emit needRestart();
}

void EnvironmentPage::deleteButton_clicked()
{
    foreach (QTreeWidgetItem* item, ui->treeWidget->selectedItems())
    {
        emit envVarChanged(item->text(0), "");
        delete item;
    }
    emit needRestart();
}

void EnvironmentPage::itemChanged(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    emit envVarChanged(item->text(0), item->text(1));
    emit needRestart();
}

void EnvironmentPage::updateItem(const QString& var, const QString& val)
{
    QList<QTreeWidgetItem*> itemList = ui->treeWidget->findItems(var, Qt::MatchExactly);
    if (itemList.isEmpty())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget, QStringList() << var << val);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->treeWidget->addTopLevelItem(item);
        return;
    }

    foreach (QTreeWidgetItem* item, itemList)
    {
        if (!val.isEmpty())
            item->setText(1, val);
        else
            delete item;
    }
    emit needRestart();
}
