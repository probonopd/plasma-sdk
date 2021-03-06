/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "serviceviewer.h"

#include <QDebug>
#include <QDialogButtonBox>
#include <KMessageBox>
#include <KStringHandler>
#include <KLocalizedString>

#include <Plasma/DataEngine>
#include <Plasma/Service>
#include <Plasma/ServiceJob>

#include "engineexplorer.h"

ServiceViewer::ServiceViewer(Plasma::DataEngine *engine, const QString &source, QWidget *parent)
    : QDialog(parent),
      m_engine(engine),
      m_service(0),
      m_source(source),
      m_operationCount(0),
      m_operationButton(new QPushButton(i18n("Start Operation"), this))
{
    setAttribute(Qt::WA_DeleteOnClose);
    QWidget* mainWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->addButton(m_operationButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Close);

    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    layout->addWidget(mainWidget);
    layout->addWidget(buttonBox);
    setLayout(layout);

    setupUi(mainWidget);
    m_operationStatus->hide();

    connect(m_operationButton, SIGNAL(clicked()), this, SLOT(startOperation()));
    m_operationButton->setEnabled(false);

    connect(m_operations, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(operationSelected(QString)));

    QString engineName = i18nc("Plasma engine with unknown name", "Unknown");
    QString serviceName = i18nc("Plasma service with unknown name", "Unknown");

    if (m_engine) {
        if (m_engine->pluginInfo().isValid()) {
            engineName = KStringHandler::capwords(m_engine->pluginInfo().name());
        }
        qDebug() << "########### CALLING SERVICE FOR SOURCE: " << m_source;
        m_service = m_engine->serviceForSource(m_source);

        if (m_service != 0) {
            serviceName = m_service->name();
            updateOperations();
            connect(m_service, SIGNAL(operationsChanged()), this, SLOT(updateOperations()));
            connect(m_engine, SIGNAL(destroyed(QObject*)), this, SLOT(engineDestroyed()));
        } else {
            KMessageBox::sorry(this, i18n("No valid service was returned. Verify that a service is available for this source."));
            close();
        }
    }

    setWindowTitle(i18nc("%1 is a Plasma service name", "%1 Service Explorer", serviceName));

    QString title = i18nc("Source: name of the data, Service: writes data instead of fetching", "DataEngine: <b>%1</b>; Source: <b>%2</b>; Service: <b>%3</b>", engineName, m_source, serviceName);
    m_title->setText(title);
    m_operations->setFocus();
}

ServiceViewer::~ServiceViewer()
{
    delete m_service;
    m_engine = 0;
}

void ServiceViewer::updateOperations()
{
    if (!m_engine) {
        return;
    }

    bool enable = false;

    m_operations->clear();
    m_operationDescription->clear();

    if (m_service) {
        const QStringList operations = m_service->operationNames();

        if (!operations.isEmpty()) {
            enable = true;

            foreach (const QString& operation, operations) {
                m_operations->addItem(operation);
            }
        }
    }

    m_operations->setEnabled(enable);
    m_operationsLabel->setEnabled(enable);
    m_operationDescription->setEnabled(enable);
}

void ServiceViewer::startOperation()
{
    if (!m_service) {
        return;
    }

    QString operation = m_operations->currentText();
    QVariantMap desc = m_service->operationDescription(operation);
    for (int i = 0; i < m_operationDescription->rowCount(); ++i) {
        QTableWidgetItem *item = m_operationDescription->item(i, 1);
        QString value = item->text();

        if (value.isEmpty()) {
            continue;
        }

        item = m_operationDescription->item(i, 0);
        QString key = item->text();
        desc[key] = value;
    }

    updateJobCount(1);
    Plasma::ServiceJob *job = m_service->startOperationCall(desc);
    connect(job, SIGNAL(finished(KJob*)), this, SLOT(operationResult(KJob*)));
}

void ServiceViewer::operationSelected(const QString &operation)
{
    if (!m_service) {
        return;
    }

    m_operationButton->setEnabled(true);
    QStringList headers;
    headers << i18n("Key") << i18n("Value");
    m_operationDescription->setHorizontalHeaderLabels(headers);

    QVariantMap desc = m_service->operationDescription(operation);
    int i = 0;
    const QStringList keys = desc.keys();
    m_operationDescription->setRowCount(keys.count());
    foreach (const QString &key, keys) {
        QTableWidgetItem *item = new QTableWidgetItem(key);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_operationDescription->setItem(i, 0, item);

        item = new QTableWidgetItem(desc[key].toString());
        m_operationDescription->setItem(i, 1, item);

        ++i;
    }
}

void ServiceViewer::operationResult(KJob *j)
{
    if (!m_service) {
        return;
    }

    Plasma::ServiceJob *job = qobject_cast<Plasma::ServiceJob *>(j);
    if (!job) {
        return;
    }

    updateJobCount(-1);

    if (job->error()) {
        KMessageBox::information(this,
                                 i18n("<b>'%1'</b> operation with destination <b>'%2'</b> failed. "
                                      "<p>The error was: <b>'%3: %4'</b></p>", job->operationName(), job->destination(),
                                      job->error(), job->errorString()),
                                 i18n("Operation Result"));
    } else {
        QString result = EngineExplorer::convertToString(job->result());
        if (result.isEmpty()) {
            result = i18n("No response from job.");
        }

        KMessageBox::information(this,
                                 i18n("<b>'%1'</b> operation with destination <b>'%2'</b> returned successfully. "
                                      "<p>The result was: <b>'%3'</b></p>", job->operationName(),
                                      job->destination(), result),
                                 i18n("Operation Result"));
    }

    qDebug() << "operation results are in!";
}

void ServiceViewer::engineDestroyed()
{
    m_service = 0;
    m_engine = 0;
    hide();
    deleteLater();
}

void ServiceViewer::updateJobCount(int numberOfJobs)
{
    m_operationCount += numberOfJobs;

    if (m_operationCount < 1) {
        m_operationCount = 0;
        m_operationStatus->hide();
    } else {
        m_operationStatus->setText(i18np("One active operation...", "%1 operations active...", m_operationCount));
        m_operationStatus->show();
    }
}

