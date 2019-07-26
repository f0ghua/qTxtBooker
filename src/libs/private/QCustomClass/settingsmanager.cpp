#include "settingsmanager.h"

#include <QSettings>

using namespace QCC;

class SettingsManagerPrivate : public QObject
{
public:
    SettingsManagerPrivate(SettingsManager *smgr);
    virtual ~SettingsManagerPrivate();

    void setSettings(QSettings *s);

    QSettings *m_settings;

private:
    SettingsManager *q;
};

SettingsManagerPrivate::SettingsManagerPrivate(SettingsManager *smgr)
    : m_settings(nullptr)
    , q(smgr)
{
}

SettingsManagerPrivate::~SettingsManagerPrivate()
{

}

void SettingsManagerPrivate::setSettings(QSettings *s)
{
    if (m_settings)
        delete m_settings;
    m_settings = s;
    if (m_settings)
        m_settings->setParent(this);
}

static SettingsManagerPrivate *d = nullptr;
static SettingsManager *m_instance = nullptr;

/*!
    Gets the unique settings manager instance.
*/
SettingsManager *SettingsManager::instance()
{
    return m_instance;
}

/*!
    Creates a settings manager. Should be done only once per application.
*/
SettingsManager::SettingsManager()
{
    m_instance = this;
    d = new SettingsManagerPrivate(this);
}

SettingsManager::~SettingsManager()
{
    delete d;
    d = nullptr;
}

void SettingsManager::setSettings(QSettings *settings)
{
    d->setSettings(settings);
}

QSettings *SettingsManager::settings()
{
    return d->m_settings;
}
