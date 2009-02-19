
#include <packagemodel.h>

#include <KDirWatch>

#include <Plasma/Package>

PackageModel::PackageModel(QObject *parent)
    : QAbstractItemModel(parent),
      m_directory(0),
      m_structure(0),
      m_package(0)
{
}

PackageModel::~PackageModel()
{
    delete m_directory;
    delete m_package;
    m_structure = 0;
}

void PackageModel::setPackageType(const QString &type)
{
    m_structure = 0;
    m_structure = Plasma::PackageStructure::load(type);
}

QString PackageModel::packageType() const
{
    if (m_structure) {
        return m_structure->type();
    }

    return QString();
}

void PackageModel::setPackage(const QString &path)
{
    if (!m_structure) {
        kDebug() << "Must set the package type FIRST!";
        return;
    }

    m_structure->setPath(path);

    delete m_package;
    m_package = new Plasma::Package(path, m_structure);

    delete m_directory;
    m_directory = new KDirWatch(this);
    m_directory->addDir(path, KDirWatch::WatchSubDirs);

    loadPackage();

    connect(m_directory, SIGNAL(created(QString)), this, SLOT(fileAddedOnDisk(QString)));
    connect(m_directory, SIGNAL(deleted(QString)), this, SLOT(fileDeletedOnDisk(QString)));
}

QString PackageModel::package() const
{
    if (m_package) {
        return m_package->path();
    }

    return QString();
}

QVariant PackageModel::data(const QModelIndex &index, int role) const
{
    if (!m_package) {
        return QVariant();
    }

    //TODO: other display roles!
    const char *key = static_cast<const char *>(index.internalPointer());
    if (key) {
        QStringList l = m_files.value(key);
        if (index.row() < l.count()) {
            //kDebug() << "got" << l.at(index.row());
            if (role == Qt::DisplayRole) {
                return l.at(index.row());
            }
        }
    } else if (role == Qt::DisplayRole) {
        return m_structure->name(m_topEntries.at(index.row()));
    }

    return QVariant();
}

QModelIndex PackageModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        if (parent.row() >= m_topEntries.count()) {
            return QModelIndex();
        }

        const char *key = m_topEntries.at(parent.row());

        if (row < m_files[key].count()) {
            //kDebug() << "going to return" << row << column << key;
            return createIndex(row, column, (void*)key);
        } else {
            //kDebug() << "FAIL";
            return QModelIndex();
        }
    }

    if (row < m_topEntries.count()) {
        return createIndex(row, column);
    }

    return QModelIndex();
}

QModelIndex PackageModel::parent(const QModelIndex &index) const
{
    const char *key = static_cast<const char *>(index.internalPointer());

    if (m_topEntries.contains(key)) {
        return createIndex(m_topEntries.indexOf(key), 0);
    }

    return QModelIndex();
}

int PackageModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    //FIXME: need to accmodate more information
    return 1;
}

int PackageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        if (parent.row() < m_topEntries.count()) {
            const char *key = m_topEntries.at(parent.row());
            //kDebug() << "looking for" << key << m_files[key].count();
            return m_files[key].count();
        } else {
            return 0;
        }
    }

    return m_topEntries.count();
}

void PackageModel::loadPackage()
{
    reset();

    if (!m_package) {
        kDebug() << "No package to load.";
        return;
    }

    foreach (const char *key, m_structure->directories()) {
        m_topEntries.append(key);
    }

    foreach (const char *key, m_structure->files()) {
        m_topEntries.append(key);
    }

    beginInsertRows(QModelIndex(), 0, m_topEntries.count() - 1);
    endInsertRows();

    foreach (const char *key, m_structure->directories()) {
        QStringList files = m_package->entryList(key);
        m_files[key] = files;

        if (files.isEmpty()) {
            continue;
        }

        //kDebug() << m_topEntries.indexOf(key) << key << "has" << files.count() << "files" << files;
        beginInsertRows(createIndex(m_topEntries.indexOf(key), 0), 0, files.count());
        endInsertRows();
    }
}

void PackageModel::fileAddedOnDisk(const QString &path)
{
    kDebug() << path;

}

void PackageModel::fileDeletedOnDisk(const QString &path)
{
    kDebug() << path;
}
