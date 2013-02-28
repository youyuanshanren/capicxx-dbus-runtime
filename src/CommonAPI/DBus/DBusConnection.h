/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_CONNECTION_H_
#define COMMONAPI_DBUS_DBUS_CONNECTION_H_

#include "DBusProxyConnection.h"
#include "DBusDaemonProxy.h"
#include "DBusServiceRegistry.h"
#include "DBusObjectManager.h"

#include <dbus/dbus.h>


namespace CommonAPI {
namespace DBus {

class DBusObjectManager;

class DBusConnectionStatusEvent: public DBusProxyConnection::ConnectionStatusEvent {
    friend class DBusConnection;

 public:
    DBusConnectionStatusEvent(DBusConnection* dbusConnection);

 protected:
    virtual void onListenerAdded(const CancellableListener& listener);

    DBusConnection* dbusConnection_;
};


class DBusConnection: public DBusProxyConnection, public std::enable_shared_from_this<DBusConnection> {
 public:
	enum BusType {
		SESSION = DBUS_BUS_SESSION,
		SYSTEM = DBUS_BUS_SYSTEM,
		STARTER = DBUS_BUS_STARTER,
		WRAPPED
	};

	inline static std::shared_ptr<DBusConnection> getBus(const BusType& busType);
	inline static std::shared_ptr<DBusConnection> wrapLibDBus(::DBusConnection* libDbusConnection);
	inline static std::shared_ptr<DBusConnection> getSessionBus();
	inline static std::shared_ptr<DBusConnection> getSystemBus();
	inline static std::shared_ptr<DBusConnection> getStarterBus();

	DBusConnection(const DBusConnection&) = delete;
	DBusConnection(::DBusConnection* libDbusConnection);

	DBusConnection& operator=(const DBusConnection&) = delete;
	virtual ~DBusConnection();

	BusType getBusType() const;

	bool connect();
	bool connect(DBusError& dbusError);
	void disconnect();

	virtual bool isConnected() const;

	virtual ConnectionStatusEvent& getConnectionStatusEvent();

	virtual bool requestServiceNameAndBlock(const std::string& serviceName) const;
	virtual bool releaseServiceName(const std::string& serviceName) const;

	bool sendDBusMessage(const DBusMessage& dbusMessage, uint32_t* allocatedSerial = NULL) const;

	static const int kDefaultSendTimeoutMs = 100 * 1000;

	std::future<CallStatus> sendDBusMessageWithReplyAsync(
			const DBusMessage& dbusMessage,
			std::unique_ptr<DBusMessageReplyAsyncHandler> dbusMessageReplyAsyncHandler,
			int timeoutMilliseconds = kDefaultSendTimeoutMs) const;

	DBusMessage sendDBusMessageWithReplyAndBlock(const DBusMessage& dbusMessage,
	                                             DBusError& dbusError,
	                                             int timeoutMilliseconds = kDefaultSendTimeoutMs) const;

	DBusSignalHandlerToken addSignalMemberHandler(const std::string& objectPath,
	                                              const std::string& interfaceName,
	                                              const std::string& interfaceMemberName,
	                                              const std::string& interfaceMemberSignature,
	                                              DBusSignalHandler* dbusSignalHandler);

	void registerObjectPath(const std::string& objectPath);
	void unregisterObjectPath(const std::string& objectPath);

	void removeSignalMemberHandler(const DBusSignalHandlerToken& dbusSignalHandlerToken);

	bool readWriteDispatch(int timeoutMilliseconds = -1);

    virtual const std::shared_ptr<DBusServiceRegistry> getDBusServiceRegistry();
    virtual const std::shared_ptr<DBusObjectManager> getDBusObjectManager();

 private:
    void dispatch();

    std::thread dispatchThread_;
    bool stopDispatching_;

	DBusConnection(BusType busType);

	void addLibdbusSignalMatchRule(const std::string& objectPath,
	                               const std::string& interfaceName,
	                               const std::string& interfaceMemberName);

	void removeLibdbusSignalMatchRule(const std::string& objectPath,
	                                  const std::string& interfaceName,
	                                  const std::string& interfaceMemberName);

	void initLibdbusObjectPathHandlerAfterConnect();

	void initLibdbusSignalFilterAfterConnect();

	::DBusHandlerResult onLibdbusObjectPathMessage(::DBusMessage* libdbusMessage) const;

	::DBusHandlerResult onLibdbusSignalFilter(::DBusMessage* libdbusMessage);

	static void onLibdbusPendingCallNotifyThunk(::DBusPendingCall* libdbusPendingCall, void *userData);
	static void onLibdbusDataCleanup(void* userData);

	static ::DBusHandlerResult onLibdbusObjectPathMessageThunk(::DBusConnection* libdbusConnection,
	                                                           ::DBusMessage* libdbusMessage,
	                                                            void* userData);

	static ::DBusHandlerResult onLibdbusSignalFilterThunk(::DBusConnection* libdbusConnection,
	                                                      ::DBusMessage* libdbusMessage,
	                                                       void* userData);

	BusType busType_;

	::DBusConnection* libdbusConnection_;

	std::weak_ptr<DBusServiceRegistry> dbusServiceRegistry_;
    std::shared_ptr<DBusObjectManager> dbusObjectManager_;

	DBusConnectionStatusEvent dbusConnectionStatusEvent_;

	typedef std::tuple<std::string, std::string, std::string> DBusSignalMatchRuleTuple;
	typedef std::pair<uint32_t, std::string> DBusSignalMatchRuleMapping;
	typedef std::unordered_map<DBusSignalMatchRuleTuple, DBusSignalMatchRuleMapping> DBusSignalMatchRulesMap;
	DBusSignalMatchRulesMap dbusSignalMatchRulesMap_;

    bool isLibdbusSignalFilterAdded_;

    DBusSignalHandlerTable dbusSignalHandlerTable_;

    // referenceCount, objectPath
    typedef std::unordered_map<std::string, uint32_t> LibdbusRegisteredObjectPathHandlersTable;
    LibdbusRegisteredObjectPathHandlersTable libdbusRegisteredObjectPaths_;

    static DBusObjectPathVTable libdbusObjectPathVTable_;
};

std::shared_ptr<DBusConnection> DBusConnection::getBus(const BusType& busType) {
	return std::shared_ptr<DBusConnection>(new DBusConnection(busType));
}

std::shared_ptr<DBusConnection> DBusConnection::wrapLibDBus(::DBusConnection* libDbusConnection) {
    return std::shared_ptr<DBusConnection>(new DBusConnection(libDbusConnection));
}

std::shared_ptr<DBusConnection> DBusConnection::getSessionBus() {
	return getBus(BusType::SESSION);
}

std::shared_ptr<DBusConnection> DBusConnection::getSystemBus() {
	return getBus(BusType::SYSTEM);
}

std::shared_ptr<DBusConnection> DBusConnection::getStarterBus() {
	return getBus(BusType::STARTER);
}


} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_CONNECTION_H_
