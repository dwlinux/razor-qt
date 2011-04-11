 #include "upower.h"

UPower::UPower() : m_interfaceProps(0), m_interface(0) {
    m_watcher = new QDBusServiceWatcher;
    m_watcher->addWatchedService("org.freedesktop.UPower");
    m_watcher->setConnection(QDBusConnection::systemBus());
    m_watcher->setWatchMode( QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration );

    connect(m_watcher, SIGNAL(serviceUnregistered(QString)), this, SIGNAL(upowerUnavailable()));
    connect(m_watcher, SIGNAL(serviceRegistered(QString)), this, SLOT(connectInterfaces()));
    connect(this, SIGNAL(upowerAvailable()), this, SLOT(probeDevices()));

    connectInterfaces();
}

UPower::~UPower() {
    delete m_interfaceProps;
    delete m_interface;
}

bool UPower::connectInterfaces() {
    if(m_interface==0 || !m_interface->isValid()) {
        m_interface = new QDBusInterface("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower",
                                         QDBusConnection::systemBus());
        if(!m_interface->isValid()) {
            m_interface = 0;
            m_interfaceProps = 0;
            return false;
        }

        connect(m_interface, SIGNAL(DeviceAdded(QString)), this, SLOT(deviceAdded(QString)));
        connect(m_interface, SIGNAL(DeviceRemoved(QString)), this, SIGNAL(batteryDisconnected(QString)));
        connect(m_interface, SIGNAL(Changed()), this, SLOT(changed()));
    }

    if(m_interfaceProps==0 || !m_interfaceProps->isValid()) {
        m_interfaceProps = new QDBusInterface("org.freedesktop.UPower", "/org/freedesktop/UPower",
                                              "org.freedesktop.DBus.Properties", QDBusConnection::systemBus());
        if(!m_interfaceProps->isValid()) {
            m_interface = 0;
            m_interfaceProps = 0;
            return false;
        }
    }

    emit upowerAvailable();

    return true;
}

const QString UPower::version() const {
    return m_interfaceProps->call( "Get", m_interfaceProps->interface(), "DaemonVersion" ).arguments().at(0).value<QDBusVariant>().variant().toString();
}

void UPower::probeDevices() {
    QDBusArgument argument = m_interface->call( "EnumerateDevices" ).arguments().at(0).value<QDBusArgument>();

    if( m_interface->lastError().type() == QDBusError::NoError ) {
        argument.beginArray();
        while( !argument.atEnd() ) {
            QDBusObjectPath dbusPath;
            argument >> dbusPath;
            deviceAdded(dbusPath.path());
        }
    }
}

void UPower::deviceAdded(QString path) {
    Battery *bat = new Battery(path);
    if( bat->isValid() ) {
        emit batteryDetected(bat);
    } else {
        delete bat;
    }
}

void UPower::changed() {
    QSettings settings;
    settings.beginGroup("Alert");
        bool alertOnLow = settings.value( "Low", false ).toBool();
    settings.endGroup();

    if(alertOnLow) {
        bool onLowBatteryVal = m_interfaceProps->call( "Get", m_interface->interface(), "OnLowBattery" ).arguments().at(0).value<QDBusVariant>().variant().toBool();
        if( onLowBatteryVal ) {
            emit onLowBattery();
        }
    }

    bool _onBattery = m_interfaceProps->call( "Get", m_interface->interface(), "OnBattery" ).arguments().at(0).value<QDBusVariant>().variant().toBool();
    if(_onBattery) {
        emit onBattery();
    } else {
        emit onPowerSupply();
    }
}

void UPower::suspend() {
    if( canSuspend() ) {
        m_interface->callWithCallback("Suspend", QList<QVariant>(), this, SIGNAL(suspendSucceed()));
    } else {
        emit suspendFail();
    }
}

void UPower::hibernate() {
    if( canHibernate() ) {
        m_interface->callWithCallback("Hibernate", QList<QVariant>(), this, SIGNAL(hibernateSucceed()));
    } else {
        emit hibernateFail();
    }
}

void UPower::reboot() {
    QDBusInterface interface( "org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", QDBusConnection::systemBus() );
    interface.call( "Restart" );
}

void UPower::halt() {
    QDBusInterface interface( "org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", QDBusConnection::systemBus() );
    interface.call( "Stop" );
}

bool UPower::canHibernate() {
    return m_interfaceProps->call( "Get", m_interface->interface(), "CanHibernate" ).arguments().at(0).value<QDBusVariant>().variant().toBool();
}

bool UPower::canSuspend() {
    return m_interfaceProps->call( "Get", m_interface->interface(), "CanSuspend" ).arguments().at(0).value<QDBusVariant>().variant().toBool();
}
