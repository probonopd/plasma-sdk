/*
Copyright 2009 Riccardo Iaconelli <riccardo@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QValidator>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

#include <KUser>
// #include <KLocalizedString>
#include <KDebug>
#include <KDesktopFile>
#include <KLineEdit>
#include <KMimeType>
#include <KPushButton>
#include <KSeparator>
#include <KUrlRequester>
#include <KShell>
#include <KStandardAction>
#include <KStandardDirs>
#include <KUser>
#include <KMessageBox>
#include <Plasma/PackageMetadata>
#include <knewstuff3/downloaddialog.h>

#include "packagemodel.h"
#include "startpage.h"
#include "mainwindow.h"
#include "projectmanager/projectmanager.h"

StartPage::StartPage(MainWindow *parent) // TODO set a palette so it will look identical with any color scheme.
        : QWidget(parent),
        m_parent(parent)
{
    setupWidgets();
    refreshRecentProjectsList();
}

StartPage::~StartPage()
{
}

void StartPage::setupWidgets()
{
    m_projectManager = new ProjectManager(this);
    m_ui.setupUi(this);

    // Set some default parameters, like username/email and preferred scripting language
    KConfigGroup cg = KGlobal::config()->group("NewProjectDefaultPreferences");
    KUser user = KUser(KUser::UseRealUserID);

    QString userName = cg.readEntry("Username", user.loginName());
    QString userEmail = cg.readEntry("Email", userName+"@none.org");

    // If username or email are empty string, i.e. in the previous project the
    // developer deleted it, restore the default values
    if (userName.isEmpty()) {
        userName = user.loginName();
    }

    if (userEmail.isEmpty()) {
        userEmail.append(user.loginName()+"@none.org");
    }

    m_ui.authorTextField->setText(userName);
    m_ui.emailTextField->setText(userEmail);

    m_ui.radioButtonJs->setChecked(cg.readEntry("radioButtonJsChecked", false));
    m_ui.radioButtonPy->setChecked(cg.readEntry("radioButtonPyChecked", false));
    m_ui.radioButtonRb->setChecked(cg.readEntry("radioButtonRbChecked", false));
    m_ui.radioButtonDe->setChecked(cg.readEntry("radioButtonDeChecked", true));

    m_ui.cancelNewProjectButton->setIcon(KIcon("draw-arrow-back"));
    m_ui.newProjectButton->setIcon(KIcon("dialog-ok"));
    m_ui.loadLocalProject->setEnabled(false);
    m_ui.importPackageButton->setEnabled(false);

    // Enforce the security restriction from package.cpp in the input field
    connect(m_ui.projectName, SIGNAL(textEdited(const QString&)),
            this, SLOT(checkProjectName(const QString&)));

    connect(m_ui.projectName, SIGNAL(returnPressed()),
            this, SLOT(createNewProject()));
    connect(m_ui.recentProjects, SIGNAL(clicked(const QModelIndex)),
            this, SLOT(recentProjectSelected(const QModelIndex)));

    // Enforce the security restriction from package.cpp in the input field
    connect(m_ui.localProject, SIGNAL(textChanged(const QString&)),
            this, SLOT(checkLocalProjectPath(const QString&)));
    connect(m_ui.loadLocalProject, SIGNAL(clicked()),
            this, SLOT(loadLocalProject()));

    connect(m_ui.importPackage, SIGNAL(textChanged(const QString&)),
            this, SLOT(checkPackagePath(const QString&)));
    connect(m_ui.importPackageButton, SIGNAL(clicked()),
            this, SLOT(importPackage()));

    // When there will be a good API for js and rb dataengines and runners, remove the
    // first connect() statement and uncomment the one below :)
    connect(m_ui.contentTypes, SIGNAL(clicked(const QModelIndex)),
            this, SLOT(validateProjectType(const QModelIndex)));
    /*connect(m_ui.contentTypes, SIGNAL(clicked(const QModelIndex)),
            m_ui.projectName, SLOT(setFocus()));*/


    connect(m_ui.newProjectButton, SIGNAL(clicked()),
            this, SLOT(createNewProject()));
    connect(m_ui.cancelNewProjectButton, SIGNAL(clicked()),
            this, SLOT(cancelNewProject()));
    connect(m_ui.importGHNSButton, SIGNAL(clicked()),
            this, SLOT(doGHNSImport()));

    connect(m_projectManager, SIGNAL(projectSelected(QString, QString)),
            this, SIGNAL(projectSelected(QString, QString)));
    connect(m_projectManager, SIGNAL(requestRefresh()),
            this, SLOT(refreshRecentProjectsList()));

    new QListWidgetItem(KIcon("application-x-plasma"), i18n("Plasma Widget"), m_ui.contentTypes);
    new QListWidgetItem(KIcon("kexi"), i18n("Data Engine"), m_ui.contentTypes);
    new QListWidgetItem(KIcon("system-run"), i18n("Runner"), m_ui.contentTypes);
    new QListWidgetItem(KIcon("inkscape"), i18n("Theme"), m_ui.contentTypes);

//     connect(m_ui.newProjectButton, SIGNAL(clicked()), this, SLOT(launchNewProjectWizard()));
}

// Convert FooBar to foo_bar
QString StartPage::camelToSnakeCase(const QString& name)
{
    QString result(name);
    return result.replace(QRegExp("([A-Z])"), "_\\1").toLower().replace(QRegExp("^_"), "");
}

void StartPage::checkProjectName(const QString& name)
{
    QRegExp validatePluginName("[a-zA-Z0-9_.]*");
    if (!validatePluginName.exactMatch(name)) {
        int pos = 0;
        for (int i = 0; i < name.size(); i++) {
            if (validatePluginName.indexIn(name, pos, QRegExp::CaretAtZero) == -1)
                break;
            pos += validatePluginName.matchedLength();
        }
        m_ui.projectName->setText(QString(name).remove(pos, 1));
    }

    m_ui.newProjectButton->setEnabled(!m_ui.projectName->text().isEmpty());
}

void StartPage::validateProjectType(const QModelIndex &sender)
{
    m_ui.languageLabel->show();
    m_ui.radioButtonJs->setEnabled(true);
    m_ui.radioButtonPy->setEnabled(true);

    if (sender.row() == DataEngineRow) {
        m_ui.radioButtonDe->setEnabled(false);
        m_ui.radioButtonJs->setChecked(true);
        m_ui.radioButtonRb->setEnabled(true);
    } else if (sender.row() == RunnerRow) {
        m_ui.radioButtonDe->setEnabled(false);
        m_ui.radioButtonJs->setChecked(true);
        m_ui.radioButtonRb->setEnabled(false);
    } else if (sender.row() == ThemeRow) {
        m_ui.languageLabel->hide();
        m_ui.frame->hide();
    } else /* if (sender.row() == PlasmaRow) */ {
        m_ui.radioButtonDe->setEnabled(true);
        m_ui.radioButtonDe->setChecked(true);
        m_ui.radioButtonRb->setEnabled(true);
    }

    m_ui.newProjectButton->setEnabled(!m_ui.projectName->text().isEmpty());
    m_ui.layoutHackStackedWidget->setCurrentIndex(1);
    m_ui.projectName->setFocus();
}

QString StartPage::userName()
{
    return m_ui.authorTextField->text();
}

QString StartPage::userEmail()
{
    return m_ui.emailTextField->text();
}

bool StartPage::selectedJsRadioButton()
{
    return m_ui.radioButtonJs->isChecked();
}

bool StartPage::selectedRbRadioButton()
{
    return m_ui.radioButtonRb->isChecked();
}

bool StartPage::selectedPyRadioButton()
{
    return m_ui.radioButtonPy->isChecked();
}

bool StartPage::selectedDeRadioButton()
{
    return m_ui.radioButtonDe->isChecked();
}

void StartPage::resetStatus()
{
    kDebug() << "Reset status!";
    m_ui.layoutHackStackedWidget->setCurrentIndex(0);
    refreshRecentProjectsList();
}

void StartPage::refreshRecentProjectsList()
{
    m_ui.recentProjects->clear();
    m_projectManager->clearProjects();
    const QStringList recentProjects = m_parent->recentProjects();

    if (recentProjects.isEmpty()) {
        m_ui.recentProjectsLabel->hide();
        m_ui.recentProjects->hide();
        return;
    }
    int counter = 0;
    foreach (const QString file, recentProjects) {
        // Specify path + filename as well to avoid mistaking .gitignore
        // as being the metadata file.
        QString f = file;
        QDir pDir(file);
        QString pPath =  file + "/metadata.desktop";
        kDebug() << "RECENT FILE::: " << file;
        if (pDir.isRelative()) {
            kDebug() << "NOT LOCAL";
            pPath = KStandardDirs::locateLocal("appdata", file + "/metadata.desktop");
        }

        if (!QFile::exists(pPath)) {
            continue;
        }

        Plasma::PackageMetadata metadata(pPath);

        // Changed this back on second thought. It's better that we
        // do not expose the idea of a 'folder name' to the user -
        // we keep the folder hidden and name it whatever we want as
        // long as it's unique. Only the plasmoid name, which the user
        // sets in the metadata editor, will ever be visible in the UI.
        QString projectName = metadata.name();

        kDebug() << "adding" << projectName << "to the list of recent projects...";
        QListWidgetItem *item = new QListWidgetItem(projectName); // show the user-set plasmoid name in the UI
	counter++;
        // the loading code uses this to find the project to load.
        // since folder name and plasmoid name can be different, this
        // should be set to the folder name, which the loading code expects.
        item->setData(FullPathRole, file);

        // set a tooltip for extra info and to help differentiating similar projects
        QString tooltip = "Project : " + projectName + "\n\n";
        tooltip += "\"" + metadata.description() + "\"\n---\n";
        tooltip += i18n("Author") + " : " + metadata.author() + " <" + metadata.email() + ">\n";
        tooltip += i18n("Version") + " : " + metadata.version() + "\n";
        tooltip += i18n("API") + " : " + metadata.implementationApi() + "\n";

        QString serviceType = metadata.serviceType();

        if (serviceType == "Plasma/Applet" ||
            serviceType == "Plasma/Applet,Plasma/PopupApplet") {
            if (!metadata.icon().isEmpty()) {
                item->setIcon(KIcon(metadata.icon()));
                kDebug() << "Setting ICON:" << metadata.icon();
            } else {
                item->setIcon(KIcon("application-x-plasma"));
            }
            tooltip += i18n("Project type") + " : " + i18n("Plasmoid");
        } else if (serviceType == "Plasma/DataEngine") {
            item->setIcon(KIcon("kexi"));
            tooltip += i18n("Project type") + " : " + i18n("Data Engine");
        } else if (serviceType == "Plasma/Theme") {
            item->setIcon(KIcon("inkscape"));
            tooltip += i18n("Project type") + " : " + i18n("Theme");
        } else if (serviceType == "Plasma/Runner") {
            item->setIcon(KIcon("system-run"));
            tooltip += i18n("Project type") + " : " + i18n("Runner");
        } else {
            kWarning() << "Unknown service type" << serviceType;
        }
        item->setToolTip(tooltip);

        m_projectManager->addProject(item);
        // limit to 5 projects to display up front
        if (m_ui.recentProjects->count() < 5) {
            m_ui.recentProjects->addItem(new QListWidgetItem(*item));
        }
    }

    QListWidgetItem *more;
    //if (recentProjects.count() >=0 && recentProjects.count() <=4) {
    if (counter >=0 && counter <=4) {
      more = new QListWidgetItem(i18n("More projects..."));
    } else {
	more = new QListWidgetItem(i18n("Manage and More Projects..."));
    }
    more->setIcon(KIcon("window-new"));
    m_ui.recentProjects->addItem(more);
}

void StartPage::createNewProject()
{
    // packagePath -> projectPath
    const QString projectName = m_ui.projectName->text();
    if (projectName.isEmpty()) {
        return;
    }

    kDebug() << "Creating simple folder structure for the project " << projectName;
    QString projectNameLowerCase = projectName.toLower();
    QString projectNameSnakeCase = camelToSnakeCase(projectName);
    QString projectFileExtension;

    QString templateFilePath = KStandardDirs::locate("appdata", "templates/");

    Plasma::PackageMetadata metadata;

    // type -> serviceTypes
    QString serviceTypes;
    if (m_ui.contentTypes->currentRow() == 1) {
        serviceTypes = "Plasma/DataEngine";
        templateFilePath.append("mainDataEngine");
    } else if (m_ui.contentTypes->currentRow() == 2) {
        serviceTypes = "Plasma/Runner";
        templateFilePath.append("mainRunner");
    } else if (m_ui.contentTypes->currentRow() == 3) {
        serviceTypes = "Plasma/Theme";
    } else {
        serviceTypes = "Plasma/Applet";
        templateFilePath.append("mainPlasmoid");
    }

    QString projectFolderName;
    QString mainScriptName;

    // Append the desired extension
    if (m_ui.radioButtonPy->isChecked()) {
        metadata.setImplementationApi("python");
        projectFolderName = generateProjectFolderName(projectNameLowerCase);
        projectFileExtension = ".py";
        mainScriptName = projectNameLowerCase + projectFileExtension;
    } else if (m_ui.radioButtonRb->isChecked()) {
        metadata.setImplementationApi("ruby-script");
        projectFolderName = generateProjectFolderName(projectNameSnakeCase);
        projectFileExtension = ".rb";
        mainScriptName = QString("main_") + projectNameSnakeCase + projectFileExtension;
    } else if (m_ui.radioButtonDe->isChecked()) {
        metadata.setImplementationApi("declarativeappletscript");
        projectFolderName = generateProjectFolderName(projectNameLowerCase);
        projectFileExtension = ".qml";
        mainScriptName = projectNameLowerCase + projectFileExtension;
    } else {
        metadata.setImplementationApi("javascript");
        projectFolderName = generateProjectFolderName(projectNameLowerCase);
        projectFileExtension = ".js";
        mainScriptName = projectNameLowerCase + projectFileExtension;
    }

    // Creating the corresponding folder

    //  From this commit, the directory is changed int the following way. Old directory structure:
    //  <plasmateProjDir>/projectname/metadata.desktop
    //                               /NOTES
    //                               /.git/
    //                               /.gitignore
    //                               /contents/...
    //  New directory structure:
    //  <plasmateProjDir>/projectname/NOTES
    //                               /.git/..
    //                               /.gitignore
    //                               /projectname/metadata.desktop
    //                                           /contents/..
    //  The reason of this change is simple: with the old directory structure, packaging the project
    //  implied also packaging the whole git history, and the NOTES file as well (which are plasmate-only
    //  related files). With the new structure, this won't happen again :)
    //  Besides, this change will allow us to start implementing the per-project config file :)
    //
    QString projectPath = KStandardDirs::locateLocal("appdata",projectFolderName + '/');
    QDir packageSubDirs(projectPath);
    packageSubDirs.mkpath(projectFolderName + "/contents/code/");

    // Create a QFile object that points to the template we need to copy
    QFile sourceFile(templateFilePath + projectFileExtension);
    QFile destinationFile(projectPath + projectFolderName +  "/contents/code/" + mainScriptName);

    // Now open these files, and substitute the main class, author, email and date fields
    sourceFile.open(QIODevice::ReadOnly);
    destinationFile.open(QIODevice::ReadWrite);

    QByteArray rawData = sourceFile.readAll();

    QByteArray replacedString("$PLASMOID_NAME");
    if (rawData.contains(replacedString)) {
        rawData.replace(replacedString, projectName.toAscii());
    }
    replacedString.clear();


    // @schmidt-domine : thanks for committing the lines I forgot adding,
    // BUT please comply with the TAGS already assigned :)
    replacedString.append("$PLASMOID_NAME");
    if (rawData.contains(replacedString)) {
        rawData.replace(replacedString, projectName.toAscii());
    }
    replacedString.clear();

    replacedString.append("$DATAENGINE_NAME");
    if (rawData.contains(replacedString)) {
        rawData.replace(replacedString, projectName.toAscii());
    }
    replacedString.clear();

    replacedString.append("$RUNNER_NAME");
    if (rawData.contains(replacedString)) {
        rawData.replace(replacedString, projectName.toAscii());
    }
    replacedString.clear();

    replacedString.append("$AUTHOR");
    if (rawData.contains(replacedString)) {
        rawData.replace(replacedString, m_ui.authorTextField->text().toAscii());
    }
    replacedString.clear();

    replacedString.append("$EMAIL");
    if (rawData.contains(replacedString)) {
        rawData.replace(replacedString, m_ui.emailTextField->text().toAscii());
    }
    replacedString.clear();

    replacedString.append("$DATE");
    QDate date = QDate::currentDate();
    QByteArray datetime(date.toString().toUtf8());
    QTime time = QTime::currentTime();
    datetime.append(", " + time.toString().toUtf8());
    if (rawData.contains(replacedString)) {
        rawData.replace(replacedString, datetime);
    }

    destinationFile.write(rawData);
    destinationFile.close();

    // Write the .desktop file
    metadata.setName(projectName);
    //FIXME: the plugin name needs to be globally unique, so should use more than just the project
    //       name
    metadata.setPluginName(projectNameLowerCase);
    metadata.setServiceType(serviceTypes);
    metadata.setAuthor(m_ui.authorTextField->text());
    metadata.setEmail(m_ui.emailTextField->text());
    metadata.setLicense("GPL");
    metadata.write(projectPath + projectFolderName + "/metadata.desktop");

    // Note: since PackageMetadata lacks of a good api to add X-Plasma-* entries,
    // we add it manually until a patch is released.
    KDesktopFile metaFile(projectPath + projectFolderName + "/metadata.desktop");
    KConfigGroup metaDataGroup = metaFile.desktopGroup();
    metaDataGroup.writeEntry("X-Plasma-MainScript", "code/" + mainScriptName);
    metaDataGroup.writeEntry("X-Plasma-DefaultSize", QSize(200, 100));
    metaFile.sync();

    // Create the notes file
    QFile notesFile(projectPath + "/NOTES");
    notesFile.open(QIODevice::ReadWrite);
    notesFile.close();

    // the loading code expects the FOLDER NAME
    emit projectSelected(projectFolderName + "/" + projectFolderName, serviceTypes);

    // need to clear the project name field here too because startpage is still
    // accessible after project loads.
    m_ui.projectName->clear();
}

void StartPage::cancelNewProject()
{
    m_ui.projectName->clear();
    resetStatus();
}

void StartPage::checkLocalProjectPath(const QString& name)
{
    QDir dir(KShell::tildeExpand(name));
    kDebug() << "checking: " << name << dir.exists();
    m_ui.loadLocalProject->setEnabled(!name.isEmpty() && dir.exists());
}

void StartPage::loadLocalProject()
{
    QString path = KShell::tildeExpand(m_ui.localProject->text());
    kDebug() << "loading local project from" << path;
    emit projectSelected(path, QString());
}

void StartPage::recentProjectSelected(const QModelIndex &index)
{
    QAbstractItemModel *m = m_ui.recentProjects->model();
    QString url = m->data(index, FullPathRole).value<QString>();
    if (url.isEmpty()) {
        m_projectManager->exec();
        return;
    }
    kDebug() << "Loading project file:" << m->data(index, FullPathRole);

    emit projectSelected(url, QString());
}

void StartPage::checkPackagePath(const QString& name)
{
    const QString fullName = KShell::tildeExpand(name);
    bool valid = QFile::exists(fullName) &&
                 KMimeType::findByUrl(fullName)->is("application/x-plasma");
    m_ui.importPackageButton->setEnabled(valid);
}

void StartPage::importPackage()
{
    const KUrl target = m_ui.importPackage->url();
    selectProject(target);
}

void StartPage::doGHNSImport()
{
    KNS3::DownloadDialog *mNewStuffDialog = new KNS3::DownloadDialog("plasmate.knsrc", this);
    if (mNewStuffDialog->exec() == QDialog::Accepted)
    {
        KNS3::Entry::List installed = mNewStuffDialog->installedEntries();

        if (!installed.empty())
        {
            KNS3::Entry entry = installed.at(0);
            QStringList installedFiles = entry.installedFiles();

            if (!installedFiles.empty())
            {
                QString file = installedFiles.at(0);
                KUrl target(file);

                selectProject(target);
            }
        }
    }
}

void StartPage::selectProject(const KUrl &target)
{
    if (!target.isLocalFile() ||
          !QFile::exists(target.path()) ||
          QDir(target.path()).exists()) {
        KMessageBox::error(this, i18n("The file you entered is invalid."));
        return;
    }

    // we can do this because this is 'under the hood'
    // the user should never be aware of any 'folder names'
    // This nicely eliminates the need for a 'conflict resolution' dialog :)
    QString suggested = QFileInfo(target.path()).completeBaseName();
    QString projectFolder = generateProjectFolderName(suggested);

    QString projectPath = KStandardDirs::locateLocal("appdata", projectFolder + '/');

    if (!ProjectManager::importPackage(target, projectPath)) {
        KMessageBox::information(this, i18n("A problem has occurred during import."));
    }
    emit projectSelected(projectFolder, QString());

}

/**
 * Brain-dead way of generating a unique folder name.
 */
const QString StartPage::generateProjectFolderName(const QString& suggestion)
{
    QString projectFolder = suggestion;
    int suffix = 1;
    while (!KStandardDirs::locate("appdata", projectFolder + '/').isEmpty()) {
        projectFolder = suggestion + QString::number(suffix);
        suffix++;
    }
    return projectFolder;
}
