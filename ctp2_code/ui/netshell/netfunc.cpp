//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Net functionality (find something better if you don't like this description ;))
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// _DEBUG
// Generate debug information.
// USE_SDL
// Use SDL API calls
// WIN32
// Use MS Windows32 API calls
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added SDL support.
// - Replaced typename T in specialized template member function by the
//   the type for that the function is specialized, by Martin G�hmann.
// - Display the main thread function name in the debugger.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/netshell/netfunc.h"

#if defined(_DEBUG)
#include "ctp/debugtools/debug.h"      	// Os::SetThreadName
#endif

#ifdef USE_SDL
#include "ui/aui_sdl/aui_sdlcompat.h"
#endif

#ifdef WIN32
#include <ras.h>
#endif

#include <algorithm>

namespace
{
    char const  COUNT_MAX   = 255u;
}

/// Operating system dependent wrapper functions
namespace Os
{
    /// Stop the current thread
    /// \param a_Code Code to return to the environment (OS/debugger)
    inline int ExitThread(int a_Code)
    {
#if defined(WIN32)
        ::ExitThread(static_cast<DWORD>(a_Code));
        UNREACHABLE_RETURN(a_Code);
#else
        return a_Code;
#endif
    }
} // namespace Os

int adialup_autodial_enabled()
{
#ifdef WIN32
	HKEY hKey;
	unsigned long werr = RegOpenKeyEx(HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
			0, KEY_EXECUTE, &hKey);

	if (werr != ERROR_SUCCESS) {
		DPRINT(("autodial_enabled: RegOpenKeyEx(%s) fails, rCode:%d\n",
			"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
			werr));
		return FALSE;
	}

	int enableAutodial = FALSE;
	unsigned long len = sizeof(enableAutodial);
	werr = RegQueryValueEx(hKey,
			"EnableAutodial",
			NULL, NULL, (unsigned char *)&enableAutodial, &len);
	CloseHandle(hKey);
	if (werr != ERROR_SUCCESS) {
		DPRINT(("autodial_enabled: Can't find %s in subkey %s, error %d\n",
			"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
			"EnableAutodial",
			werr));
		return FALSE;
	}

	if (!enableAutodial) {
		DPRINT(("autodial_enabled: autodial is off.\n"));
		return FALSE;
	}

	DPRINT(("autodial_enabled: autodial is on.\n"));
	return TRUE;
#else
	return FALSE;
#endif
}

#define adialup_MAXCONNS 10

#ifdef WIN32
typedef DWORD (APIENTRY *pfnRasEnumConnections_t)(LPRASCONN, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *pfnRasGetConnectStatus_t)(HRASCONN, LPRASCONNSTATUS);
#endif

int adialup_is_active()
{
#ifdef WIN32
	HANDLE hlib = LoadLibrary("rasapi32.dll");
	if (NULL == hlib) {
		DPRINT(("adialup_is_active: can't load library rasapi32.dll\n"));
		return FALSE;
	}

	pfnRasEnumConnections_t pfnRasEnumConnections =
        (pfnRasEnumConnections_t)GetProcAddress((HINSTANCE)hlib, "RasEnumConnectionsA");
	pfnRasGetConnectStatus_t pfnRasGetConnectStatus =
        (pfnRasGetConnectStatus_t) GetProcAddress((HINSTANCE)hlib, "RasGetConnectStatusA");

	if (!pfnRasEnumConnections || !pfnRasGetConnectStatus) {
		DPRINT(("adialup_is_active: can't get fns from library rasapi32.dll\n"));
		FreeLibrary((HINSTANCE)hlib);
		return FALSE;
	}

	RASCONN rasconnArray[adialup_MAXCONNS];
	rasconnArray[0].dwSize = sizeof(rasconnArray[0]);
	unsigned long rasconnLen = sizeof(rasconnArray);
	DWORD cConnections = adialup_MAXCONNS;
	DWORD werr = pfnRasEnumConnections( rasconnArray, &rasconnLen,  &cConnections);
	if (werr) {
		DPRINT(("adialup_is_active: RasEnumConnections fails, err %d\n", werr));
		FreeLibrary((HINSTANCE)hlib);
		return FALSE;
	}
	DPRINT(("adialup_is_active: RasEnumConnections reports %d connections\n", cConnections));
	if (cConnections < 1) {
		FreeLibrary((HINSTANCE)hlib);
		return FALSE;
	}

	RASCONNSTATUS rasconnstatus;
	for (DWORD i = 0; i < cConnections; ++i)
	{
		rasconnstatus.dwSize = sizeof(rasconnstatus);
		werr = pfnRasGetConnectStatus( rasconnArray[i].hrasconn, &rasconnstatus);
		if (rasconnstatus.rasconnstate == RASCS_Connected) {
			DPRINT(("adialup_is_active: cnxn %d is active!\n", i));
			FreeLibrary((HINSTANCE)hlib);
			return TRUE;
		}
	}

	DPRINT(("adialup_is_active: none of the connections are active\n"));
	FreeLibrary((HINSTANCE)hlib);
#endif
	return FALSE;
}




int adialup_willdial()
{
	return adialup_autodial_enabled() && !adialup_is_active();
}











void NETFunc::StringMix(int c, char *mix, char *msg, ...) {
	va_list al;
	va_start(al, msg);
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	strcpy(mix, msg);

	char * arg = va_arg(al, char *);
	char str[] = "% ";

	for(int i = 1; i <= c; i++) {
		str[1] = static_cast<char>('0' + i);

		char * next = strstr(mix, str);
		if(next) {
			char *tmp = strdup(next + strlen(str));
			// TODO(phase-2): strcpy → strlcpy — dst is expression (`next + strlen(arg)`), capacity unknown at call site
			strcpy(next + strlen(arg), tmp);
			// TODO(phase-2): strncpy → strlcpy — non-standard length argument, requires manual review
			strncpy(next, arg, strlen(arg));
			free(tmp);
		}

		arg = va_arg(al, char *);
	}

	va_end(al);
}


char *NETFunc::StringDup(char *s) {
    if (!s) return nullptr;
    size_t const len = strlen(s) + 1;
    char *dup = new char[len];
    strlcpy(dup, s, len);
    return dup;
}


NETFunc::Timer::Timer()
:
    start   (0),
    finish  (0),
    done    (false)
{
}

void NETFunc::Timer::Start(int d) {
	start = GetTickCount();
	finish = start + d;
	done = false;
}

bool NETFunc::Timer::Finished()
{
    return done || (GetTickCount() >= finish);
}


NETFunc::MessageHandler::MessageHandler() {
	if(hCount < nf_MAX_HANDLERS)
		hList[hCount++] = this;
}

NETFunc::MessageHandler::~MessageHandler() {
	Unregister();
}

void NETFunc::MessageHandler::Unregister() {
	bool f = false;
	for(int i = 0; i < hCount; i++) {
		if(f)
			hList[i-1] = hList[i];
		else
			f = (hList[i] == this);
	}
	if(f)
		hCount--;
}

bool NETFunc::MessageHandler::HandleAll(Message *m) {
	bool f = false;
	for(int i = 0; i < hCount; i++)
		f = hList[i]->Handle(m) || f;
	return f;
}

NETFunc::MessageHandler *NETFunc::MessageHandler::hList[] = {nullptr};
int NETFunc::MessageHandler::hCount = 0;


NETFunc::Message::Message(CODE c, void *p, size_t s) {
	newbody = true;
	body = new char[s + sizeof(CODE)];
	*(CODE *)body = c;
	memcpy(body + sizeof(CODE), p, s);
	size = s + sizeof(CODE);
	sender = dp_ID_NAMESERVER;
}

NETFunc::Message::Message(CODE c) {
	newbody = true;
	body = new char[sizeof(CODE)];
	*(CODE *)body = c;
	size = sizeof(CODE);
	sender = dp_ID_NAMESERVER;
}

NETFunc::Message::Message(void *p, size_t s, dpid_t id, bool b) {
	newbody = b;
	if(newbody) {
		body = new char[s];
		memcpy(body, p, s);
	} else
		body = (char *)p;
	size = s;
	sender = id;
}

NETFunc::Message::Message() {
	newbody = false;
	size = 0;
}

NETFunc::Message::~Message() {
	if(newbody)
		delete [] body;
}

dp_packetType_t *NETFunc::Message::Get() {
	return (dp_packetType_t *)body;
}

void *NETFunc::Message::GetBody() {
	return body + sizeof(CODE);
}

dpid_t NETFunc::Message::GetSender() {
	return sender;
}

size_t NETFunc::Message::GetSize() {
	return size;
}

size_t NETFunc::Message::GetBodySize() {
	return size - sizeof(CODE);
}

NETFunc::Message::CODE NETFunc::Message::GetCode() {
	return *(CODE *)body;
}


NETFunc::Messages::Messages() = default;

NETFunc::Messages::~Messages() = default;

void NETFunc::Messages::Push(Message *m) {
	push_back(m);
}

NETFunc::Message *NETFunc::Messages::Pop() {
	Message *m = *begin();
	pop_front();
	return m;
}

NETFunc::Messages NETFunc::messages = Messages();


NETFunc::Keys::Keys() {
	memset(&curkey, 0, sizeof(KeyStruct));
	curkey.len = 1;
}

void NETFunc::Keys::NextKey() {
	if(curkey.buf[curkey.len-1] == COUNT_MAX)
		curkey.len++;
	curkey.buf[curkey.len-1]++;
}


NETFunc::Key::Key() {
	memset(&key, 0, sizeof(KeyStruct));
}

NETFunc::Key::Key(Key *k) {
	key = k->key;
}

NETFunc::Key::Key(KeyStruct *k) {
	key = *k;
}

bool NETFunc::Key::Equals(Key *k) {
	return memcmp(&key, &k->key, sizeof(key.len) + key.len) == 0;
}

bool NETFunc::Key::Equals(KeyStruct *k) {
	return memcmp(&key, k, sizeof(key.len) + key.len) == 0;
}

NETFunc::KeyStruct *NETFunc::Key::GetKey() {
	return &key;
}


NETFunc::STATUS NETFunc::EnumServers(bool b) {
	KeyStruct key;
	key.buf[0] = dp_KEY_SERVERPINGS;
	key.len = 1;
	if(!b)
		PushMessage(new Message(Message::RESET, &key, sizeof(KeyStruct)));

	key.buf[1] = (char) dpGETSHORT_FIRSTBYTE(GameType);
	key.buf[2] = (char) dpGETSHORT_SECONDBYTE(GameType);
	key.len = 3;
	return dpRequestObjectDeltas(dp, b, &key.buf[0], key.len) == dp_RES_OK ? OK : ERR;
}


NETFunc::STATUS NETFunc::EnumSessions(bool b) {
	KeyStruct key;
	key.buf[0] = dp_KEY_SESSIONS;
	key.len = 1;
	if(!b)
		PushMessage(new Message(Message::RESET, &key, sizeof(KeyStruct)));

	NETFunc::STATUS s = dpRequestObjectDeltas(dp, b, &key.buf[0], key.len) == dp_RES_OK ? OK : ERR;

	key.buf[0] = dp_KEY_PLAYER_LATENCIES;
	return dpRequestObjectDeltas(dp, 1, &key.buf[0], key.len) == dp_RES_OK ? s : ERR;
}


NETFunc::STATUS NETFunc::EnumPlayers(bool b, KeyStruct *k) {
	KeyStruct key;
	key.buf[0] = dp_KEY_PLAYERS;
	memcpy(&key.buf[0] + 1, &k->buf[0], k->len);
	key.len = 1 + k->len;
	Receive();
	if(!b) {
		for(Messages::iterator i = messages.begin(); i != messages.end() && !messages.empty(); i++) {

			if((*i)->GetCode() == dp_OBJECTDELTA_PACKET_ID) {
				dp_objectDelta_packet_t *p = (dp_objectDelta_packet_t *)(*i)->GetBody();
				KeyStruct key = *(KeyStruct *)(&p->keylen);

				if(key.buf[0] == dp_KEY_PLAYERS && memcmp(&key.buf[key.len - 2], &k->buf[k->len - 2], 2) == 0) {

					messages.Del(*i);
					i = messages.begin();
				}
			}
		}
	}
	return dpRequestObjectDeltas(dp, b, &key.buf[0], key.len) == dp_RES_OK ? OK : ERR;
}


















NETFunc::Server::Server() {
	memset(&server, 0, sizeof(dp_serverInfo_t));
}

NETFunc::Server::Server(dp_object_t *o, KeyStruct *k, long f):Key(k) {
	server = o->serv;
}

NETFunc::Server::~Server() = default;

char *NETFunc::Server::GetName() {
	return server.hostname;
}

int NETFunc::Server::GetPlayers() {
	return server.cur_sessTypeUsers;
}

int NETFunc::Server::GetPing() {
	return server.rtt_ms_avg;
}

dp_serverInfo_t *NETFunc::Server::GetServer() {
	return &server;
}

template<>
bool NETFunc::ListHandler<NETFunc::Server>::Handle(Message *m) {
	if(m->GetCode() == Message::RESET && Equals((KeyStruct *)m->GetBody())) {
		Destroy();
		Clr();
	} else if(m->GetCode() == dp_OBJECTDELTA_PACKET_ID) {
		dp_objectDelta_packet_t *p = (dp_objectDelta_packet_t *)m->GetBody();
		if(Equals((KeyStruct *)&p->keylen)) {
			Server t(&p->data, (KeyStruct *)&p->subkeylen, p->flags);
			switch(p->status) {

			case dp_RES_CREATED:

			case dp_RES_CHANGED:
				if(p->data.serv.loss_percent != 100)
					if(Find(&t) != end())
						Change(Chg(&t));
					else
						Insert(Add(new Server(t)));

				return true;

			case dp_RES_DELETED:
				Delete(&t);
				Del(&t);
				return true;
			default:
				break;
			}
		}
	}
	return false;
}

template<>
void NETFunc::ListHandler<NETFunc::Server>::SetKey() {
	key.buf[0] = dp_KEY_SERVERPINGS;


	key.len = 1;
}


NETFunc::Contact::Contact(char *n, char *p) {
	name = NETFunc::StringDup(n);
	number = NETFunc::StringDup(p);
}

NETFunc::Contact::Contact() = default;

NETFunc::Contact::~Contact() {
	delete name;
	delete number;
}

char *NETFunc::Contact::GetName() {
	return name;
}

char *NETFunc::Contact::GetNumber() {
	return number;
}

void NETFunc::Contact::SetName(char *n) {
	delete name;
	name = NETFunc::StringDup(n);
}

void NETFunc::Contact::SetNumber(char *p) {
	delete number;
	number = NETFunc::StringDup(p);
}


NETFunc::ContactList::ContactList() = default;

NETFunc::ContactList::~ContactList() = default;









NETFunc::Port::Port(commPortName_t *p, int b, char *i) {
	port = *p;
	baud	= b;
	strlcpy(init, i, sizeof(init));
}

NETFunc::Port::Port() = default;

NETFunc::Port::~Port() = default;

commPortName_t *NETFunc::Port::GetPort() {
	return &port;
}

int NETFunc::Port::GetNumber() {
	return port.portnum;
}

char *NETFunc::Port::GetName() {
	return port.name;
}

int NETFunc::Port::GetBaud() {
	return baud;
}

char *NETFunc::Port::GetInit() {
	return init;
}


NETFunc::PortList::PortList(Transport *t)
{
    commPortName_t portName[10];
    int portCount;
    dp_result_t err = dpEnumPorts(&t->transport , portName, 10, &portCount);
    if (err == dp_RES_OK)
    {
        for (int i = 0; i < portCount; i++)
        {
            push_back(new Port(&portName[i], 0, ""));
        }
    }
}

NETFunc::PortList::PortList() = default;

NETFunc::PortList::~PortList() = default;

NETFUNC_CONNECT_RESULT NETFunc::ConnectThread(NETFUNC_CONNECT_PARAMETER t)
{
#if defined(_DEBUG)
    Os::SetThreadName("NETFunc::ConnectThread");
#endif
    result = dpCreate(&dp, ((TransportSetup *)t)->GetTransport(), ((TransportSetup *)t)->GetParams(), nullptr);
    return Os::ExitThread((dp_RES_OK == result) ? 0 : 1);
}

NETFUNC_CONNECT_RESULT NETFunc::ReConnectThread(NETFUNC_CONNECT_PARAMETER r)
{
#if defined(_DEBUG)
    Os::SetThreadName("NETFunc::ReconnectThread");
#endif
    *((bool *)r) = false;
    dpSetActiveThread(dp);
    while (!(*((bool *)r)))
    {
		Receive();
    }

    return Os::ExitThread(0);
}



































NETFunc::Transport::Transport(const comm_driverInfo_t *d, const dp_transport_t *t, KeyStruct *k)
:
	Key(k),
	transport(*t),
	description(*d)
{

	memset(&parameters, 0, sizeof(commInitReq_t));
	parameters.reqLen = sizeof(commInitReq_t);
	parameters.sessionId = GetTickCount();
	parameters.dialing_method = comm_INIT_DIALING_METHOD_TONE;
	status = NOSETUP;
}

NETFunc::Transport::Transport() = default;

NETFunc::Transport::~Transport() = default;

dp_transport_t *NETFunc::Transport::GetTransport() {
	return &transport;
}

commInitReq_t *NETFunc::Transport::GetParams() {
	return &parameters;
}




NETFunc::STATUS NETFunc::Transport::GetStatus() {
	return status;
}

NETFunc::Transport::TYPE NETFunc::Transport::GetType() {
	return Transport::UNKNOWN;
}

char *NETFunc::Transport::GetName() {
	return description.name;
}

char *NETFunc::Transport::GetFileName() {
	return transport.fname;
}

NETFunc::STATUS NETFunc::Transport::SetPort(long p) {
	return BADSETUP;
}

NETFunc::STATUS NETFunc::Transport::SetPort(Port *p) {
	return BADSETUP;
}

NETFunc::STATUS NETFunc::Transport::SetContact(Contact *c) {
	return BADSETUP;
}

NETFunc::TransportSetup::TransportSetup(Transport *t) {

	transport = *t->GetTransport();
	parameters = *t->GetParams();
	status = t->GetStatus();
	parameters.modeministr = StringDup(t->GetParams()->modeministr);
	parameters.phonenum = StringDup(t->GetParams()->phonenum);
	type = t->GetType();
}

NETFunc::TransportSetup::~TransportSetup() {

	
		delete [] parameters.modeministr;
	
		delete [] parameters.phonenum;
}

NETFunc::STATUS NETFunc::TransportSetup::GetStatus() {
	return status;
}

NETFunc::Transport::TYPE NETFunc::TransportSetup::GetType() {
	return type;
}

dp_transport_t *NETFunc::TransportSetup::GetTransport() {
	return &transport;
}

commInitReq_t *NETFunc::TransportSetup::GetParams() {
	return &parameters;
}

NETFunc::TransportSetup *NETFunc::transport = nullptr;


NETFunc::Transport::TYPE NETFunc::GetTransportType(const comm_driverInfo_t *c) {
	if((c->needs & (comm_INIT_NEEDS_PORTNUM | comm_INIT_NEEDS_PHONENUM)) == comm_INIT_NEEDS_PORTNUM)
		return Transport::NULLMODEM;
	else if((c->needs & (comm_INIT_NEEDS_PORTNUM | comm_INIT_NEEDS_PHONENUM)) == (comm_INIT_NEEDS_PORTNUM | comm_INIT_NEEDS_PHONENUM))
		return Transport::MODEM;
	else if(c->capabilities & comm_DRIVER_NO_BROADCAST)
		return Transport::INTERNET;
	else
		return Transport::IPX;
}


NETFunc::TransportList::TransportList() {
	dp_transport_t transport;
	strlcpy(transport.fname, DllPath, sizeof(transport.fname));
	memset(&key, 0, sizeof(KeyStruct));
	key.len = 1;
	result = dpEnumTransports(&transport, CallBack, this);
}

NETFunc::TransportList::~TransportList() = default;

NETFUNC_CALLBACK_RESULT(void)
NETFunc::TransportList::CallBack(const dp_transport_t *t, const comm_driverInfo_t *d, void *context) {
	if (comm_DRIVER_IS_VISIBLE & d->capabilities) {
		KeyStruct *k = (KeyStruct *)&((TransportList *)context)->key;
		if(k->buf[k->len-1] == COUNT_MAX)
			k->len++;
		else
			k->buf[k->len-1]++;

		switch(GetTransportType(d)) {
		case Transport::INTERNET:
			((TransportList *)context)->Add(new Internet(d, t, k));
		break;
		case Transport::IPX:
			((TransportList *)context)->Add(new IPX(d, t, k));
		break;
		case Transport::MODEM:
			((TransportList *)context)->Add(new Modem(d, t, k));
		break;
		case Transport::NULLMODEM:
			((TransportList *)context)->Add(new NullModem(d, t, k));
		break;
		default:
		break;
		}
	}
}


NETFunc::Internet::Internet(const comm_driverInfo_t *d, const dp_transport_t *t, KeyStruct *k):Transport(d, t, k) {
}

NETFunc::Internet::~Internet() = default;

NETFunc::STATUS NETFunc::Internet::SetPort(long p) {
	long o = parameters.portnum;
	parameters.portnum = p;
	return o != p ? RESET : OK;
}

NETFunc::Transport::TYPE NETFunc::Internet::GetType() {
	return Transport::INTERNET;
}


NETFunc::IPX::IPX(const comm_driverInfo_t *d, const dp_transport_t *t, KeyStruct *k):Transport(d, t, k) {
	status = READY;
}

NETFunc::IPX::~IPX() = default;

NETFunc::Transport::TYPE NETFunc::IPX::GetType() {
	return Transport::IPX;
}


NETFunc::Modem::Modem(const comm_driverInfo_t *d, const dp_transport_t *t, KeyStruct *k):Transport(d, t, k) {

	parameters.hwirq = 12345;
	parameters.swint = (long) &NETFunc::cancelDial;

}

NETFunc::Modem::~Modem() = default;

NETFunc::STATUS NETFunc::Modem::SetContact(Contact *c) {
	if(c)

		parameters.phonenum = c->GetNumber();
	else

		parameters.phonenum = "";
	status = status == NOSETUP ? NOPORT :READY;
	return OK;
}

NETFunc::STATUS NETFunc::Modem::SetPort(Port *p) {

	parameters.baud = p->GetBaud();
	parameters.portnum = p->GetNumber();
	parameters.modeministr = p->GetInit();
	status = status == NOSETUP ? NOCONTACT : READY;
	return OK;
}

NETFunc::Transport::TYPE NETFunc::Modem::GetType() {
	return Transport::MODEM;
}


NETFunc::NullModem::NullModem(const comm_driverInfo_t *d, const dp_transport_t *t, KeyStruct *k):Transport(d, t, k) {
}

NETFunc::NullModem::~NullModem() = default;

NETFunc::STATUS NETFunc::NullModem::SetPort(Port *p) {

	parameters.baud = p->GetBaud();
	parameters.portnum = p->GetNumber();
	parameters.modeministr = "";
	return OK;
}

NETFunc::Transport::TYPE NETFunc::NullModem::GetType() {
	return Transport::NULLMODEM;
}











NETFunc::AIPlayer::AIPlayer() = default;

NETFunc::AIPlayer::~AIPlayer() = default;

char *NETFunc::AIPlayer::GetName() {
	return name;
}

void NETFunc::AIPlayer::SetName(char *n) {
	strlcpy(name, n, sizeof(name));
}

unsigned char NETFunc::AIPlayer::GetGroup() {
	return group;
}

void NETFunc::AIPlayer::SetGroup(unsigned char g) {
	group = g;
}

void NETFunc::AIPlayer::SetKey(KeyStruct *k) {
	key = *k;
}

void NETFunc::AIPlayer::Pack() {
	Clear();
	Push(key.buf);
	Push(name);
	Push(group);
}

void NETFunc::AIPlayer::Unpack() {
	first = body;
	Pop(key.buf);
	key.len = static_cast<short>(strlen(key.buf));
	Pop(name);
	Pop(group);
}

NETFunc::STATUS NETFunc::AIPlayer::Load(FILE *f) {
	if(fread(&(Packet::size), sizeof(SizeT), 1, f) == 1)
		if(fread(body, Packet::size, 1, f) == 1) {
			Unpack();
			return OK;
		}
	return ERR;
}

NETFunc::STATUS NETFunc::AIPlayer::Save(FILE *f) {
	Pack();
	if(fwrite(&(Packet::size), sizeof(SizeT), 1, f) == 1)
		if(fwrite(body, Packet::size, 1, f) == 1)
			return OK;
	return ERR;
}


NETFunc::STATUS NETFunc::AIPlayers::Send(dp_t *p, dpid_t id, dpid_t from) {
	for(auto & i : *this)
	{
		Message message = Message(Message::ADDAIPLAYER, i->GetBody(), i->GetSize());
		if(NETFunc::Send(p, &message, id, from) != OK)
			return ERR;
	}
	return OK;
}


NETFunc::AIPlayers::AIPlayers(AIPlayers *l):NETFunc::List<NETFunc::AIPlayer>(l) {
}

bool NETFunc::AIPlayers::Handle(Message *m) {
	AIPlayer t;
	if(m->GetCode() == Message::ENTERGAME) {
		Clr();
		return true;
	} else if(m->GetCode() == Message::ADDAIPLAYER) {
		t.Set(m->GetBodySize(), m->GetBody());

		Add(new AIPlayer(t));
		return true;
	} else if(m->GetCode() == Message::DELAIPLAYER) {
		t.Set(m->GetBodySize(), m->GetBody());

		Del(&t);
		return true;
	} else if(m->GetCode() == Message::CHGAIPLAYER) {
		t.Set(m->GetBodySize(), m->GetBody());

		Chg(&t);
		return true;
	}
	return false;
}

bool NETFunc::AIPlayers::Handle(dp_t *p, Message *m, dpid_t from) {
	if(m->GetCode() == Message::PLAYERENTER) {
		Send(p, m->GetSender(), from);
		return true;
	}
	return false;
}

template<>
bool NETFunc::ListHandler<NETFunc::AIPlayer>::Handle(Message *m) {
	return false;
}

template<>
void NETFunc::ListHandler<NETFunc::AIPlayer>::SetKey() {
}
















void NETFunc::Player::SetGroupMaster(bool b) {
	if(player.bloblen == 0) {
		player.bloblen = 1;
		player.blob[0] = 0;
	}
	if(b)
		player.blob[0] |= nf_GROUPMASTER;
	else
		player.blob[0] &= ~nf_GROUPMASTER;
}

void NETFunc::Player::SetKey() {
	key.buf[0] = (char) dpGETSHORT_FIRSTBYTE(player.id);
	key.buf[1] = (char) dpGETSHORT_SECONDBYTE(player.id);
	key.len = 2;
}

NETFunc::Player::Player() {
	memset(&player, 0, sizeof(dp_playerId_t));
	player.bloblen = 1;
	SetKey();
	flags = 0;
	muted = false;
}

NETFunc::Player::Player(dp_object_t *o, KeyStruct *k, long f, short l):Key(k) {
	player = o->p;
	SetKey();
	flags = f;
	latency = l;
	muted = false;
}

void NETFunc::Player::Set(dp_playerId_t *p) {
	player = *p;
}

NETFunc::Player::~Player() = default;

dpid_t NETFunc::Player::GetId() {
	return player.id;
}

char *NETFunc::Player::GetName() {
	return player.name;
}

unsigned char *NETFunc::Player::GetBlob() {
	return &player.blob[1];
}

unsigned char NETFunc::Player::GetBlobLen() {
	if(player.bloblen == 0)
		return 0;
	return player.bloblen - 1;
}

unsigned char NETFunc::Player::GetGroup() {
	if(player.bloblen == 0)
		return 0;
	return (player.blob[0] & nf_GROUPNUMBER);
}

short NETFunc::Player::GetLatency() {
	return latency;
}

bool NETFunc::Player::IsMe() {
	return (flags & dp_OBJECTDELTA_FLAG_LOCAL) != 0;
}

bool NETFunc::Player::IsMuted() {
	return muted;
}

void NETFunc::Player::SetMuted(bool m) {
	muted = m;
}

bool NETFunc::Player::IsInCurrentSession() {
	return (flags & dp_OBJECTDELTA_FLAG_INOPENSESS) != 0;
}

bool NETFunc::Player::IsHost() {
	return (flags & dp_OBJECTDELTA_FLAG_ISHOST) != 0;
}

bool NETFunc::Player::IsGroupMaster() {
	return player.bloblen > 0 && GetGroup() && player.blob[0] & nf_GROUPMASTER;
}

bool NETFunc::Player::IsReadyToLaunch() {
	return player.bloblen > 0 && player.blob[0] & nf_READYLAUNCH;
}

NETFunc::Player NETFunc::player = Player();
dp_uid_t NETFunc::userId = dp_UID_NONE;

template<>
bool NETFunc::ListHandler<NETFunc::Player>::Handle(Message *m) {
	if(m->GetCode() == Message::RESET) {
		Destroy();
		Clr();
	} else if(m->GetCode() == Message::UPDATE && Equals((KeyStruct *)m->GetBody())) {
		KeyStruct *k = ((KeyStruct *)m->GetBody()) + 1;
		Key key = Key(k);
		for(iterator i=begin(); i!=end(); i++)
			if((*i)->Equals(k)) {
				(*i)->SetMuted(IsMuted(&key));
				Change(Chg(*i));
			}
	} else if(m->GetCode() == dp_OBJECTDELTA_PACKET_ID) {
		dp_objectDelta_packet_t *p = (dp_objectDelta_packet_t *)m->GetBody();
		if(Equals((KeyStruct *)&p->keylen)) {
			Player t(&p->data, (KeyStruct *)&p->subkeylen, p->flags, p->latency);
			t.SetMuted(IsMuted(&t));
			switch(p->status) {

			case dp_RES_CREATED:

			case dp_RES_CHANGED:
				if(Find(&t) != end())
					Change(Chg(&t));
				else
					Insert(Add(new Player(t)));
				return true;

			case dp_RES_DELETED:
				Delete(&t);
				Del(&t);
				return true;
			default:
				break;
			}
		}
	}
	return false;
}

template<>
void NETFunc::ListHandler<NETFunc::Player>::SetKey() {
	key.buf[0] = dp_KEY_PLAYERS;
	key.len = 1;
}




NETFunc::PlayerStat::PlayerStat() {
	key.len = sizeof(dpid_t);
	name[0] = 0;
	group = 0;
	isingame = true;
	hasleft = false;
}

NETFunc::PlayerStat::~PlayerStat() = default;

char *NETFunc::PlayerStat::GetName() {
	return name;
}

void NETFunc::PlayerStat::SetName(char *n) {
	strlcpy(name, n, sizeof(name));
}

unsigned char NETFunc::PlayerStat::GetGroup() {
	return group;
}

void NETFunc::PlayerStat::SetGroup(unsigned char g) {
	group = g;
}

void NETFunc::PlayerStat::SetId(dpid_t i) {
	*(dpid_t *)&key.buf = i;
}

dpid_t NETFunc::PlayerStat::GetId() {
	return 	*(dpid_t *)&key.buf;
}

void NETFunc::PlayerStat::SetInGame(bool b) {
	isingame = b;
	*((bool *)GetBody() + 1) = b;
}

bool NETFunc::PlayerStat::IsInGame() {
	return isingame;
}

void NETFunc::PlayerStat::SetLeft(bool b) {
	hasleft = b;
	*(bool *)GetBody() = b;
}

bool NETFunc::PlayerStat::HasLeft() {
	return hasleft;
}

void NETFunc::PlayerStat::Pack() {
	Clear();
	Push(hasleft);
	Push(isingame);
	Push(*(dpid_t *)&key.buf);
	Push(name);
	Push(group);
}

void NETFunc::PlayerStat::Unpack() {
	first = body;
	Pop(hasleft);
	Pop(isingame);
	Pop(*(dpid_t *)&key.buf);
	Pop(name);
	Pop(group);
}

NETFunc::STATUS NETFunc::PlayerStat::Update(dp_t *p, bool r) {
	hasleft = false;
	isingame = true;
	Pack();
	Message message = Message(Message::ADDPLAYERSTAT, GetBody(), GetSize());
	if(NETFunc::Send(p, &message, dp_ID_BROADCAST, dp_ID_BROADCAST, r) != OK)
		return ERR;
	return OK;
}


NETFunc::PlayerStats::PlayerStats(PlayerStats *l):NETFunc::List<NETFunc::PlayerStat>(l) {
}

NETFunc::STATUS NETFunc::PlayerStats::Send(dp_t *p, dpid_t id, dpid_t from) {
	for(auto & i : *this)
	{
		Message message = Message(Message::ADDPLAYERSTAT, i->GetBody(), i->GetSize());
		if(NETFunc::Send(p, &message, id) != OK)
			return ERR;
	}
	return OK;
}

void NETFunc::PlayerStats::Left(dpid_t id) {
	PlayerStat t;
	t.SetId(id);
	iterator i = Find(&t);
	if(i != end()) {
		t = **i;
		t.SetLeft(true);
		Chg(&t);
		if(status != RESET)
			PushMessage(new Message(Message::CHGPLAYERSTAT, t.GetBody(), t.GetSize()));
	}
}

bool NETFunc::PlayerStats::Handle(Message *m) {
	PlayerStat t;
	if((m->GetCode() == Message::ENTERGAME && IsHost()) || (m->GetCode() == Message::GAMESESSION && !GotGameSetup())) {
		Clr();
		return true;
	} else if(m->GetCode() == Message::ADDPLAYERSTAT) {
		t.Set(m->GetBodySize(), m->GetBody());
		t.SetInGame(false);

		if(Find(&t) == end())

			Add(new PlayerStat(t));
		else {

			Chg(&t);
			*(Message::CODE *)(m->Get()) = Message::CHGPLAYERSTAT;
		}
		return true;
	} else if(m->GetCode() == Message::DELPLAYERSTAT) {
		t.Set(m->GetBodySize(), m->GetBody());

		Del(&t);
		return true;
	} else if(m->GetCode() == dp_USER_DELPLAYER_PACKET_ID) {
		Left(((dp_playerId_t *)m->GetBody())->id);
		return true;
	} else if(m->GetCode() == dp_OBJECTDELTA_PACKET_ID) {
		dp_objectDelta_packet_t *p = (dp_objectDelta_packet_t *)m->GetBody();
		if((p->key[0] == dp_KEY_PLAYERS) && (p->flags & dp_OBJECTDELTA_FLAG_INOPENSESS) && (p->status == dp_RES_DELETED)) {
			Left(p->data.p.id);
			return true;
		}
	}
	return false;
}

bool NETFunc::PlayerStats::Handle(dp_t *p, Message *m, dpid_t from) {
	if(m->GetCode() == Message::PLAYERENTER) {
		Send(p, m->GetSender(), from);
		return true;
	}

	return false;
}




NETFunc::PlayerSetup::PlayerSetup(Player *p):Player(*p) {
	description[0] = 0;
}

NETFunc::PlayerSetup::PlayerSetup() {
	description[0] = 0;
}

NETFunc::PlayerSetup::~PlayerSetup() = default;

void NETFunc::PlayerSetup::SetName(char *n) {
	strlcpy(player.name, n, sizeof(player.name));
}

void NETFunc::PlayerSetup::SetBlob(char *b) {
	SetBlob((void *)b, strlen(b) + 1);
}

void NETFunc::PlayerSetup::SetBlob(void *b, size_t s) {
    s = std::min<size_t>(s, dp_MAX_PLAYERBLOB_LEN - 1);
	memcpy(&player.blob[1], b, s);
	player.bloblen = static_cast<unsigned char>(s + 1);
}

void NETFunc::PlayerSetup::SetBlobLen(unsigned char l) {
	if(l < dp_MAX_PLAYERBLOB_LEN)
		player.bloblen = l + 1;
}

char *NETFunc::PlayerSetup::GetDescription() {
	return description;
}

void NETFunc::PlayerSetup::SetDescription(char *d) {
	strlcpy(description, d, sizeof(description));
}

void NETFunc::PlayerSetup::SetGroup(char group) {
	if(player.bloblen == 0) {
		player.bloblen = 1;
		player.blob[0] = 0;
	}
	player.blob[0] &= ~nf_GROUPNUMBER;
	player.blob[0] |= (group & nf_GROUPNUMBER);
}

void NETFunc::PlayerSetup::SetReadyToLaunch(bool b) {
	if(player.bloblen == 0) {
		player.bloblen = 1;
		player.blob[0] = 0;
	}
	if(b)
		player.blob[0] |= nf_READYLAUNCH;
	else
		player.blob[0] &= ~nf_READYLAUNCH;
}

void NETFunc::PlayerSetup::Pack() {
	Clear();
	Push(description);
}

void NETFunc::PlayerSetup::Unpack() {
	first = body;
	Pop(description);
}

NETFunc::STATUS NETFunc::PlayerSetup::Load(FILE *f) {
	if(fread(&player, sizeof(dp_playerId_t), 1, f) == 1)
		if(fread(&(Packet::size), sizeof(SizeT), 1, f) == 1)
			if(fread(body, Packet::size, 1, f) == 1) {
				Unpack();
				return OK;
			}
	return ERR;
}

NETFunc::STATUS NETFunc::PlayerSetup::Save(FILE *f) {
	if(fwrite(&player, sizeof(dp_playerId_t), 1, f) == 1) {
		Pack();
		if(fwrite(&(Packet::size), sizeof(SizeT), 1, f) == 1)
			if(fwrite(body, Packet::size, 1, f) == 1)
				return OK;
	}
	return ERR;
}

NETFunc::STATUS NETFunc::PlayerSetup::Update() {
	Pack();
	if(SetPlayerSetupPlayer(this) != OK)
		return ERR;
	if(SetPlayerSetupPacket(this) != OK)
		return ERR;
	return OK;
}



































void NETFunc::Session::SetKey() {
	int i;
	dpGetSessionId(dp, &session, &key.buf[0], &i);

	key.len = static_cast<short>(i);
}

NETFunc::Session::Session() {
	memset(&session, 0, sizeof(dp_session_t));
	session.dwSize = sizeof(dp_session_t);
	session.maxPlayers = dp_MAXREALPLAYERS;
	session.flags = dp_SESSION_FLAGS_CREATESESSION | dp_SESSION_FLAGS_ENABLE_PLAYERVARS;
	session.guidApplication = GameType;
}

NETFunc::Session::Session(dp_object_t *o, KeyStruct *k, long f):Key(k) {
	session = o->sess;
	flags = f;
}

char *NETFunc::Session::GetName() {
	return session.sessionName;
}

char *NETFunc::Session::GetPassword() {
	return session.szPassword;
}

short NETFunc::Session::GetPlayers() {
	return session.currentPlayers;
}

short NETFunc::Session::GetMaxPlayers() {
	return session.maxPlayers;
}

short NETFunc::Session::GetFree() {
	return GetMaxPlayers() - GetPlayers();
}

char *NETFunc::Session::GetUserField() {
	return session.szUserField;
}

dp_karma_t NETFunc::Session::GetKarma() {
	return session.karma;
}

bool NETFunc::Session::IsLobby() {
	return (session.flags & dp_SESSION_FLAGS_ISLOBBY) != 0;
}

bool NETFunc::Session::IsServer() {
	return (session.flags & dp_SESSION_FLAGS_ISSERVER) != 0;
}

bool NETFunc::Session::IsClosed() {
	return (session.flags & dp_SESSION_FLAGS_ENABLE_NEWPLAYERS) == 0;
}

bool NETFunc::Session::IsMine() {
	return (flags & dp_OBJECTDELTA_FLAG_LOCAL) != 0;
}

bool NETFunc::Session::IsCurrentSession() {
	return (flags & dp_OBJECTDELTA_FLAG_INOPENSESS) != 0;
}

NETFunc::Session NETFunc::session = Session();


template<>
bool NETFunc::ListHandler<NETFunc::Session>::Handle(Message *m) {
	if(m->GetCode() == Message::RESET && Equals((KeyStruct *)m->GetBody())) {
		Destroy();
		Clr();
	} else if(m->GetCode() == dp_OBJECTDELTA_PACKET_ID) {
		dp_objectDelta_packet_t *p = (dp_objectDelta_packet_t *)m->GetBody();
		if(Equals((KeyStruct *)&p->keylen)) {
			Session t(&p->data, (KeyStruct *)&p->subkeylen, p->flags);
			if(t.session.sessionType != GameType)
				return false;
			switch(p->status) {

			case dp_RES_CREATED:

			case dp_RES_CHANGED:
				if(Find(&t) != end())
					Change(Chg(&t));
				else
					Insert(Add(new Session(t)));
				return true;

			case dp_RES_DELETED:
				Delete(&t);
				Del(&t);
				return true;
			default:
				break;
			}
		}
	}
	return false;
}

template<>
void NETFunc::ListHandler<NETFunc::Session>::SetKey() {
	key.buf[0] = dp_KEY_SESSIONS;
	key.len = 1;
}


NETFunc::Game::Game(): Session() {
	hostile = false;
}

NETFunc::Game::Game(dp_object_t *o, KeyStruct *k, long f): Session(o, k, f) {
	hostile = false;
}

NETFunc::Game::Game(Session *s): Session(*s) {
	hostile = false;
}

char NETFunc::Game::GetGroups() {
	return static_cast<char>(session.dwUser1 & nf_GROUPNUMBER);
}

void NETFunc::Game::Set(dp_session_t *s) {
	session = *s;
}

bool NETFunc::Game::IsLaunched() {
	return (session.dwUser1 & nf_LAUNCHED) != 0;
}

bool NETFunc::Game::IsSyncLaunch() {
	return (session.dwUser1 & nf_SYNCLAUNCH) != 0;
}

bool NETFunc::Game::IsHostile() {
	return hostile;
}

void NETFunc::Game::SetHostile(bool h) {
	hostile = h;
}


NETFunc::Lobby::Lobby(): Session(), bad(false) {
	strlcpy(session.sessionName, LobbyName, sizeof(session.sessionName));
	session.flags |= dp_SESSION_FLAGS_ISLOBBY;
}

NETFunc::Lobby::Lobby(dp_object_t *o, KeyStruct *k, long f): Session(o, k, f), bad(false) {
}

void NETFunc::Lobby::SetBad(bool b) {
	bad = b;
}

bool NETFunc::Lobby::IsBad() {
	return bad;
}

NETFunc::Lobby NETFunc::lobby = Lobby();


template<>
bool NETFunc::ListHandler<NETFunc::Lobby>::Handle(Message *m) {
	if(m->GetCode() == Message::RESET && Equals((KeyStruct *)m->GetBody())) {
		Destroy();
		Clr();
	} else if(m->GetCode() == Message::LOBBYDELTAPACKET) {
		dp_objectDelta_packet_t *p = (dp_objectDelta_packet_t *)m->GetBody();
		if(Equals((KeyStruct *)&p->keylen)) {
			Lobby t(&p->data, (KeyStruct *)&p->subkeylen, p->flags);
			if(t.session.sessionType != GameType)
				return false;
			switch(p->status) {

			case dp_RES_CREATED:

			case dp_RES_CHANGED:
				if(Find(&t) != end())
					Change(Chg(&t));
				else
					Insert(Add(new Lobby(t)));
				return true;

			case dp_RES_DELETED:
				Delete(&t);
				Del(&t);
				return true;
			default:
				break;
			}
		}
	}
	return false;
}

template<>
void NETFunc::ListHandler<NETFunc::Lobby>::SetKey() {
	key.buf[0] = dp_KEY_SESSIONS;
	key.len = 1;
}


template<>
bool NETFunc::ListHandler<NETFunc::Game>::Handle(Message *m) {
	if(m->GetCode() == Message::RESET && Equals((KeyStruct *)m->GetBody())) {
		Destroy();
		Clr();
	} else if(m->GetCode() == Message::UPDATE && Equals((KeyStruct *)m->GetBody())) {
		KeyStruct *k = ((KeyStruct *)m->GetBody()) + 1;
		Key key = Key(k);
		for(iterator i=begin(); i!=end(); i++)
			if((*i)->Equals(k)) {
				(*i)->SetHostile(IsHostile(&key));
				Change(Chg(*i));
			}
	} else if(m->GetCode() == Message::GAMEDELTAPACKET) {
		dp_objectDelta_packet_t *p = (dp_objectDelta_packet_t *)m->GetBody();
		if(Equals((KeyStruct *)&p->keylen)) {
			Game t(&p->data, (KeyStruct *)&p->subkeylen, p->flags);
			if(t.session.sessionType != GameType)
				return false;
			t.SetHostile(IsHostile(&t));
			switch(p->status) {

			case dp_RES_CREATED:

			case dp_RES_CHANGED:
				if(Find(&t) != end())
					Change(Chg(&t));
				else
					Insert(Add(new Game(t)));
				return true;

			case dp_RES_DELETED:
				Delete(&t);
				Del(&t);
				return true;
			default:
				break;
			}
		}
	}
	return false;
}

template<>
void NETFunc::ListHandler<NETFunc::Game>::SetKey() {
	key.buf[0] = dp_KEY_SESSIONS;
	key.len = 1;
}


NETFunc::PlayerList::PlayerList() {
	count++;
	if(!players)
		players = new Players();
}

NETFunc::PlayerList::~PlayerList() {
	count--;
	if(!count) {
		delete players;
		players = nullptr;
	}
}

void NETFunc::PlayerList::SetKey(KeyStruct *k) {
	if(players)
		players->SetKey(k);
}

NETFunc::Player *NETFunc::PlayerList::FindPlayer(dpid_t id) {
	Players::iterator i;
	for(i = players->begin(); i!=players->end(); i++)
		if((*i)->GetId() == id)
			return *i;
	return nullptr;
};

NETFunc::PlayerList::Players *NETFunc::PlayerList::players = nullptr;
int NETFunc::PlayerList::count = 0;




NETFunc::GameSetup::GameSetup() {
	description[0] = 0;
}

NETFunc::GameSetup::GameSetup(Game *g):Game(*g), Packet() {
	description[0] = 0;
}

NETFunc::GameSetup::~GameSetup() = default;

char *NETFunc::GameSetup::GetDescription() {
	return description;
}

void NETFunc::GameSetup::SetDescription(char *d) {
	strlcpy(description, d, sizeof(description));
}

void NETFunc::GameSetup::SetGroups(char groups) {
	session.dwUser1 &= ~nf_GROUPNUMBER;
	session.dwUser1 |= (groups & nf_GROUPNUMBER);
}

void NETFunc::GameSetup::SetName(char *n) {
	strlcpy(session.sessionName, n, sizeof(session.sessionName));
}

void NETFunc::GameSetup::SetPassword(char *p) {
	strlcpy(session.szPassword, p, sizeof(session.szPassword));
}

void NETFunc::GameSetup::SetSize(short s) {
	session.maxPlayers = s;
}

void NETFunc::GameSetup::SetUserField(char *u) {
	SetUserField((void *)u, strlen(u) + 1);
}

void NETFunc::GameSetup::SetUserField(void *u, size_t s) {
	memcpy(session.szUserField, u, std::min<size_t>(s, dp_USERFIELDLEN));
}

void NETFunc::GameSetup::SetLaunched(bool b) {
	if(b)
		session.dwUser1 |= nf_LAUNCHED;
	else
		session.dwUser1 &= ~nf_LAUNCHED;
}

void NETFunc::GameSetup::SetSyncLaunch(bool s) {
	if(s)
		session.dwUser1 |= nf_SYNCLAUNCH;
	else
		session.dwUser1 &= ~nf_SYNCLAUNCH;
}

void NETFunc::GameSetup::SetClosed(bool c) {
	if(c)
		session.flags &= ~dp_SESSION_FLAGS_ENABLE_NEWPLAYERS;
	else
		session.flags |= dp_SESSION_FLAGS_ENABLE_NEWPLAYERS;
}

void NETFunc::GameSetup::Pack() {
	Clear();
	Push(description);
}

void NETFunc::GameSetup::Unpack() {
	first = body;
	Pop(description);
}

NETFunc::STATUS NETFunc::GameSetup::Send(dp_t *p, dpid_t id, dpid_t from) {
	Message message = Message(Message::GAMESESSION, &session, sizeof(dp_session_t));
	if(NETFunc::Send(p, &message, id, from) != OK)
		return ERR;
	message = Message(Message::GAMEPACKET, GetBody(), GetSize());
	if(NETFunc::Send(p, &message, id, from) != OK)
		return ERR;
	return OK;
}

bool NETFunc::GameSetup::Handle(Message *m) {
	if(m->GetCode() == Message::GAMESESSION) {
		Game::Set((dp_session_t *)m->GetBody());
		gotgame = true;
		return true;
	} else if(m->GetCode() == Message::GAMEPACKET) {
		Packet::Set(m->GetBodySize(), m->GetBody());
		gotgamepacket = true;
		return true;
	}
	return false;
}

bool NETFunc::GameSetup::Handle(dp_t *p, Message *m, dpid_t from) {
	if(m->GetCode() == Message::PLAYERENTER) {
		Send(p, m->GetSender(), from);
		return true;
	}
	return false;
}

NETFunc::STATUS NETFunc::GameSetup::Load(FILE *f) {
	if(fread(&session, sizeof(dp_session_t), 1, f) == 1)
		if(fread(&(Packet::size), sizeof(SizeT), 1, f) == 1)
			if(fread(body, Packet::size, 1, f) == 1) {
				Unpack();
				return OK;
			}
	return ERR;
}

NETFunc::STATUS NETFunc::GameSetup::Save(FILE *f) {
	if(fwrite(&session, sizeof(dp_session_t), 1, f) == 1) {
		Pack();
		if(fwrite(&(Packet::size), sizeof(SizeT), 1, f) == 1)
			if(fwrite(body, Packet::size, 1, f) == 1)
				return OK;
	}
	return ERR;
}

NETFunc::STATUS NETFunc::GameSetup::Update(bool b) {
	Pack();
	if(b)
		NETFunc::UnLaunchAll();
	if(SetGameSetupSession(this) != OK)
		return ERR;
	if(SetGameSetupPacket(this) != OK)
		return ERR;
	return OK;
}




NETFunc::STATUS	NETFunc::Chat::Send(Player *p, char *m) {
	size_t size = strlen(m) + 1;
	char buffer[dpio_MAXLEN_UNRELIABLE];
	*(TYPE *)buffer = PUBLIC;
	memcpy(buffer + sizeof(TYPE), m , size);
	size += sizeof(TYPE);
	dpid_t id = dp_ID_BROADCAST;
	if(p) {
		id = p->GetId();
		*(TYPE *)buffer = PRIVATE;
	}
	if(GetStatus() != OK)
		return BUSSY;
	else {
		Receive(&player, *(TYPE *)buffer, m);
		Message message = Message(Message::CHAT, buffer, size);
		return NETFunc::Send(netf->GetDP(), &message, id);
	}
}

NETFunc::STATUS	NETFunc::Chat::SendGroup(char *m) {
	size_t size = strlen(m) + 1;
	char buffer[dpio_MAXLEN_UNRELIABLE];
	*(TYPE *)buffer = GROUP;
	memcpy(buffer + sizeof(TYPE), m , size);
	size += sizeof(TYPE);
	if(GetStatus() != OK)
		return BUSSY;
	else {
		Receive(&player, *(TYPE *)buffer, m);
		Message message = Message(Message::CHAT, buffer, size);
		return NETFunc::Send(netf->GetDP(), &message, dp_ID_BROADCAST);
	}
}

bool NETFunc::Chat::Handle(Message *m) {
	if(m->GetCode() == Message::CHAT) {
		Player *p = FindPlayer(m->GetSender());
		if(!IsMuted(p) && ((*(TYPE *)m->GetBody() == GROUP && p->GetGroup() == player.GetGroup()) || *(TYPE *)m->GetBody() != GROUP))
			Receive(p, *(TYPE *)m->GetBody(), (char *)m->GetBody() + sizeof(TYPE));
		return true;
	}
	return false;
}

NETFunc::Lobby *NETFunc::Lobbies::FindBest() {
	iterator i;
	Lobby *l;
	Lobby *lobby = nullptr;

	for(i = begin(); i != end(); i++) {
		l = *i;
		if
		(!l->IsBad() && l->GetFree() &&

			(!lobby ||

				(l->GetFree() < lobby->GetFree() ||

				l->GetKarma() < lobby->GetKarma()
		)	)	)
		lobby = l;
	}
	return lobby;
}

void NETFunc::Lobbies::Reset() {
	iterator i;

	for(i = begin(); i != end(); i++)
		(*i)->SetBad(false);
}

NETFunc::Players::Players(Players *l):NETFunc::List<NETFunc::Player>(l) {
}

bool NETFunc::Players::ReadyToLaunch() {
	iterator i;




	for(i = begin(); i!=end(); i++)
		if(!((*i)->IsReadyToLaunch()))
			return false;
	return true;
};

NETFunc::Player *NETFunc::Players::FindGroupMate(char g) {
	iterator i;
	for(i = begin(); i!=end(); i++)
		if((*i)->GetGroup() == g)
			return (*i);
	return nullptr;
};

NETFunc::Player *NETFunc::Players::FindGroupMaster(char g) {
	iterator i;
	for(i = begin(); i!=end(); i++)
		if((*i)->GetGroup() == g && (*i)->IsGroupMaster())
			return (*i);
	return nullptr;
};

char NETFunc::Players::FindSmallestGroup() {
	if(!gameSetup.GetGroups())
		return 0;
	iterator i;
	char gc[nf_GROUPNUMBER];
	for(char & c : gc)
		c = 0;
	for(i = begin(); i!=end(); i++)
		gc[(*i)->GetGroup()]++;
	char sg = 0;
	int sn = 255;
	for(char g = 1; g <= gameSetup.GetGroups(); g++) {
		if(gc[g] < sn) {
			sn = gc[g];
			sg = g;
		}
	}
	return sg;
};

bool NETFunc::Players::Handle(Message *m) {
	if(m->GetCode() == dp_OBJECTDELTA_PACKET_ID) {
		dp_objectDelta_packet_t *p = (dp_objectDelta_packet_t *)m->GetBody();
		if(p->key[0] == dp_KEY_PLAYERS) {
			Player t = Player(&p->data, (KeyStruct *)&p->subkeylen, p->flags);
			if(p->flags & dp_OBJECTDELTA_FLAG_INOPENSESS) {
				switch(p->status) {

				case dp_RES_CREATED:
					Add(new Player(t));




























					return true;
					break;

				case dp_RES_DELETED: {
					for(auto & i : *this)
						if(i->Equals(&t) && i->IsHost()) {
							for(auto & j : *this)
								((PlayerSetup *)j)->SetReadyToLaunch(false);
							PushMessage(new Message(Message::UNLAUNCH));
							break;
						}
					Del(&t);












					return true;
					break;
					}

				case dp_RES_CHANGED:
					Chg(&t);




















					return true;
					break;
				default:
					break;
				}
			}
		}
	} else if(m->GetCode() == Message::RESET) {
		Clr();
		return true;
	} else if(m->GetCode() == Message::UNLAUNCH) {
		for(auto & i : *this)
			((PlayerSetup *)i)->SetReadyToLaunch(false);
		return true;
	}
	return false;
}

bool NETFunc::Hostiles::Check(Key *s) {
	iterator i;
	if(s)
	for(i = begin(); i != end(); i++)
		if(s->Equals(*i))
			return true;
	return false;
}

NETFunc::Hostiles NETFunc::hostiles = Hostiles();

bool NETFunc::Mutes::Check(Key *p) {
	iterator i;
	if(p)
	for(i = begin(); i != end(); i++)
		if(p->Equals(*i))
			return true;
	return false;
}

NETFunc::Mutes NETFunc::mutes = Mutes();


NETFunc::NETFunc() {
	transport = nullptr;
	playerStats = new PlayerStats();
	aiPlayers = new AIPlayers();
	connected = false;
	reconnected = true;
	playerset = false;
	game = false;
	host = false;
	launch = false;
	launched = false;
	lobby = Lobby();
	memset(&appParam, 0, sizeof(dp_appParam_t));

	FILE *f = fopen("netf.def", "rb");
	if(f) {
		fread(&servername, sizeof(servername) + sizeof(playername) + sizeof(sessionname), 1, f);
		fclose(f);
	}

	status = START;
}

NETFunc::STATUS NETFunc::Connect(char *file) {

	result = dpCreate(&dp, nullptr, nullptr, file);
	if(result != dp_RES_OK)
		return ERR;

	connected = true;
	dp_transport_t t;
	comm_driverInfo_t i;

	dpGetCurrentTransportInfo(dp, &t, &i);

	KeyStruct k;
	k.len = 0;

	Transport trans = Transport(&i, &t, &k);
	transport = new TransportSetup(&trans);

	connected = true;

	size = sizeof(dp_session_t);

	if(dpGetSessionDesc(dp, &session.session, &size) != dp_RES_OK)
		return ERR;

	session.SetKey();

	dp_caps_t info;
	result = dpGetCaps(dp, &info, 0);
	if(result != dp_RES_OK)
		return ERR;

	host = info.dwFlags & dp_CAPS_FLAGS_ISHOST;

	EnumPlayers(true, session.GetKey());

	status = READY;
	game = true;
	launch = false;
	launched = false;
	canlaunch = false;


	return OK;
}

NETFunc::~NETFunc() {

	FILE *f = fopen("netf.def", "wb");
	if(f) {
		fwrite(&servername, sizeof(servername) + sizeof(playername) + sizeof(sessionname), 1, f);
		fclose(f);
	}

	Quit();

	while(GetStatus() != NETFunc::START) {
		Message *m = GetMessage();
		
			delete m;
	}

	messages.clear();
	
		delete transport;
	
		delete playerStats;
	
		delete aiPlayers;
}

NETFunc::STATUS NETFunc::GetStatus() {
	return status;
}

bool NETFunc::Connected() {
	return connected;
}

dp_t *NETFunc::GetDP() {
	return dp;
}

NETFunc::Session *NETFunc::GetSession() {
	return &session;
}

NETFunc::Player *NETFunc::GetPlayer() {
	return &player;
}

NETFunc::TransportSetup *NETFunc::GetTransport() {
	return transport;
}

NETFunc::STATUS NETFunc::SetTransport(Transport *t) {
	if(status == START) {
		
			delete transport;
		if(t->GetType() == Transport::INTERNET && adialup_willdial())
			return BUSSY;
		transport = new TransportSetup(t);
		if(transport->GetType() == Transport::INTERNET) {
			userId = dp_UID_NONE;
			nextStatus = PRECONNECT;
		} else {
			servername[0] = 0;
			nextStatus = READY;
		}
		cancelDial = 0;
#ifdef USE_SDL
		threadHandle = SDL_CreateThread(ConnectThread, "ConnectThread", (void *)transport);
#else
		threadHandle = CreateThread(0, 0, ConnectThread, (void *)transport, 0, &threadId);
#endif
		if(threadHandle) {
			status = CONNECT;
			return OK;
		}
	}
	return ERR;
}


NETFunc::STATUS NETFunc::SetPlayerSetup(PlayerSetup *p) {
	p->SetReadyToLaunch(false);
	p->SetGroup(0);
	p->SetGroupMaster(false);
	playerSetup = *p;
	playerset = true;
	setplayer = true;
	setplayerpacket = true;
	return OK;
}

NETFunc::STATUS NETFunc::SetPlayerSetupPacket(PlayerSetup *p) {
	if(!playerset)
		return ERR;
	playerSetup = *p;
	setplayerpacket = true;
	if(status != OK)
		return OK;

	if(dpSetPlayerData(dp, player.GetId(), PlayerSetupPacketKey, p->GetBody(), p->GetSize(), 0) != dp_RES_OK)
		return ERR;
	setplayerpacket = false;
	return OK;
}

NETFunc::STATUS NETFunc::SetPlayerSetupPlayer(PlayerSetup *p) {
	if(!playerset)
		return ERR;
	playerSetup = *p;
	setplayer = true;
	if(status != OK)
		return OK;
	if(dpSetPlayerName(dp, player.GetId(), p->GetName()) != dp_RES_OK)
		return ERR;
	p->player.id = player.player.id;
	player.player = p->player;
	if(dpSetPlayerBlob(dp, player.GetId(), p->player.blob, p->player.bloblen) != dp_RES_OK)
		return ERR;
	setplayer = false;
	return OK;
}

NETFunc::STATUS NETFunc::GetPlayerSetupPacket(Player *p) {

	if(p->IsInCurrentSession()) {
		size = dpio_MAXLEN_UNRELIABLE;


		if(dpGetPlayerData(dp, p->GetId(), PlayerSetupPacketKey, buffer, &size, 0) == dp_RES_OK) {
			PushMessage(new Message(Message::PLAYERPACKET, buffer, size));
			return OK;
		}
	}
	return ERR;
}

NETFunc::STATUS NETFunc::SetReadyToLaunch(bool b) {
	if(!playerset)
		return ERR;
	playerSetup.SetReadyToLaunch(b);
	SetPlayerSetupPlayer(&playerSetup);
	return OK;
}

NETFunc::STATUS NETFunc::SetGroupMaster(Player *p) {
	if(!host)
		return ERR;
	Player *m = players.FindGroupMaster(p->GetGroup());
	PlayerSetup s;
	if(m) {
		if(m->Equals(p))
			return OK;
		m->SetGroupMaster(false);
		s.Player::Set(&m->player);
		SetRemotePlayerRecord(&s);
	}
	p->SetGroupMaster(true);
	s.Player::Set(&p->player);
	SetRemotePlayerRecord(&s);
	return OK;
}

NETFunc::STATUS NETFunc::SetRemotePlayerRecord(PlayerSetup *p) {
	if(status != OK)
		return ERR;

	if(!host && (!playerset || !playerSetup.GetGroup() || playerSetup.GetGroup() != p->GetGroup() || player.IsGroupMaster()))
		return ERR;
	if(p->GetId() == player.GetId()) {
		PushMessage(new Message(Message::SETPLAYERRECORD, &p->player, sizeof(dp_playerId_t)));
	} else {
		Message message = Message(Message::SETPLAYERRECORD, &p->player, sizeof(dp_playerId_t));
		if(Send(dp, &message, p->GetId()) != OK)
			return ERR;
	}
	return OK;
}

NETFunc::STATUS NETFunc::SetRemotePlayerPacket(PlayerSetup *p) {
	if(status != OK)
		return ERR;

	if(!host && (!playerset || !playerSetup.GetGroup() || playerSetup.GetGroup() != p->GetGroup() || player.IsGroupMaster()))
		return ERR;
	if(p->GetId() == player.GetId())
		SetPlayerSetupPacket(p);
	else {
		Message message = Message(Message::SETPLAYERPACKET, p->GetBody(), p->GetSize());
		if(Send(dp, &message, p->GetId()) != OK)
			return ERR;
	}
	return OK;
}


NETFunc::STATUS NETFunc::SetGameSetup(GameSetup *g) {
	if(!host)
		return ERR;
	g->SetLaunched(false);
	gameSetup = *g;
	game = true;
	launch = false;
	launched = false;
	canlaunch = !gameSetup.IsSyncLaunch();
	setgame = true;
	setgamepacket = true;

	return OK;
}

NETFunc::STATUS NETFunc::SetGameSetupPacket(GameSetup *g) {
	gameSetup = *g;
	setgamepacket = true;
	if(status != OK)
		return OK;
	Message message = Message(Message::GAMEPACKET, g->GetBody(), g->GetSize());
	if(Send(dp, &message, dp_ID_BROADCAST) != OK)
		return ERR;
	setgamepacket = false;
	return OK;
}

NETFunc::STATUS NETFunc::SetGameSetupSession(GameSetup *g) {
	gameSetup = *g;
	setgame = true;
	if(!gameSetup.IsSyncLaunch())
		canlaunch = true;
	if(status != OK)
		return OK;
	if(dpSetSessionDesc(dp, &g->session, 0) != dp_RES_OK)
		return ERR;
	Message *m = new Message(Message::GAMESESSION, &g->session, sizeof(dp_session_t));

	PushMessage(m);
	if(Send(dp, m, dp_ID_BROADCAST) != OK)
		return ERR;
	setgame = false;
	return OK;
}

NETFunc::STATUS NETFunc::InsertAIPlayer(AIPlayer *p) {
	if(!aiPlayers)
		return ERR;
	AIPlayer aip(*p);
	aiPlayers->NextKey();
	aip.key = aiPlayers->curkey;
	Message *m = new Message(Message::ADDAIPLAYER, aip.GetBody(), aip.GetSize());
	PushMessage(m);
	if(Send(dp, m, dp_ID_BROADCAST) != OK)
		return ERR;
	return OK;
}

NETFunc::STATUS NETFunc::DeleteAIPlayer(AIPlayer *p) {
	if(!aiPlayers)
		return ERR;
	Message *m = new Message(Message::DELAIPLAYER, p->GetBody(), p->GetSize());
	PushMessage(m);
	if(Send(dp, m, dp_ID_BROADCAST) != OK)
		return ERR;
	return OK;
}

NETFunc::STATUS NETFunc::ChangeAIPlayer(AIPlayer *p) {
	if(!aiPlayers)
		return ERR;
	Message *m = new Message(Message::CHGAIPLAYER, p->GetBody(), p->GetSize());
	PushMessage(m);
	if(Send(dp, m, dp_ID_BROADCAST) != OK)
		return ERR;
	return OK;
}

NETFunc::STATUS NETFunc::Launch() {
	if(!launch)
		SetReadyToLaunch(true);


	launch = true;
	if(status == OK && canlaunch && !launched) {
		if(host) {
			gameSetup.SetLaunched(true);
			if(gameSetup.IsSyncLaunch())
				gameSetup.SetClosed(true);
			SetGameSetupSession(&gameSetup);
		}
		launched = true;
		PushMessage(new Message(Message::GAMELAUNCH));
		return OK;
	}
	return ERR;
}

NETFunc::STATUS NETFunc::UnLaunchAll() {
	Message *m = new Message(Message::UNLAUNCH);
	PushMessage(m);
	if(NETFunc::Send(dp, m, dp_ID_BROADCAST) != OK)
		return ERR;
	return OK;
}

NETFunc::STATUS NETFunc::Login(char *username, char *password) {
	if(status == LOGIN) {
		if(userId != dp_UID_NONE || !strlen(username)) {
			PushMessage(new Message(Message::LOGINOK));
		} else if(dpAccountLogin(dp, username, password) != dp_RES_OK)
			return ERR;
		return OK;
	}
	return ERR;
}

NETFunc::STATUS NETFunc::SetServer(Server *s) {

	if(status == PRECONNECT || status == LOGIN) {
		if(dpSetGameServerEx(dp, s->GetName(), GameType) != dp_RES_OK)
			return ERR;
		status = LOGIN;

		strlcpy(servername, s->GetName(), sizeof(servername));
		return OK;
	}
	return ERR;
}

bool NETFunc::NeedUpdate() {

	if(status == LOGIN) {
		result = dpGetAppVersion(dp, &appParam);
		if(result == dp_RES_OK)
			return true;
	}
	return false;
}

NETFunc::STATUS NETFunc::DoUpdate() {

	if(status == LOGIN) {

		if(appParam.name) {
			Quit();

			while(GetStatus() != NETFunc::START) {
				Message *m = GetMessage();
				
					delete m;
			}

			result = dpDownloadUpdate(dp, &appParam);
			if(result == dp_RES_OK)
				return BUSSY;
			else if(result == dp_RES_EMPTY)
				return OK;
		}
	}
	return ERR;
}

void NETFunc::SetHost(bool h) {
	host = h;
}

bool NETFunc::GotGameSetup() {
	return gotgame && gotgamepacket;
}

bool NETFunc::IsHost() {
	return host;








}

bool NETFunc::IsHostile(Key *s) {
	return hostiles.Check(s);
}

bool NETFunc::IsMuted(Key *p) {
	return mutes.Check(p);
}

void NETFunc::Mute(Key *p, bool m) {
	KeyStruct keys[2];
	keys[0].buf[0] = dp_KEY_PLAYERS;
	memcpy(&keys[0].buf[1], &session.key.buf[0], session.key.len);
	keys[0].len = session.key.len + 1;
	memcpy(&keys[1], &p->key, sizeof(KeyStruct));
	if(m) {
		if(mutes.Find(p) == mutes.end())
			mutes.Add(new Key(*p));
	} else
		mutes.Del(p);

	PushMessage(new Message(Message::UPDATE, &keys[0], (sizeof(KeyStruct)<<1)));
}

NETFunc::STATUS NETFunc::Connect() {

	if(!playerset)
		return ERR;

	if(status == READY) {

		if(transport && transport->GetType() == Transport::INTERNET) {
			timer.Start(1000);
			status = WAITDELAY;
			nextStatus = OPENLOBBY;

		} else {
			EnumSessions(false);
			EnumSessions(true);
			timer.Start(2000);
			status = WAITDELAY;
			nextStatus = OPENLOBBY;
		}
		return OK;
	}
	return ERR;
}

NETFunc::STATUS NETFunc::Connect(dp_t *d, PlayerStats *stats, bool h) {

	dp = d;

	connected = true;

	host = h;

	strlcpy(player.player.name, playername, sizeof(player.player.name));
	strlcpy(session.session.sessionName, sessionname, sizeof(session.session.sessionName));

	if(stats) {
		playerStats = new PlayerStats();

		PlayerStats::iterator i;
		for(i=stats->begin(); i!=stats->end(); i++)
			PushMessage(new Message(Message::ADDPLAYERSTAT, (*i)->GetBody(), (*i)->GetSize()));
	}

	status = READY;
	game = true;
	launch = false;
	launched = false;
	canlaunch = false;
	reconnected = true;

	dp_transport_t t;
	comm_driverInfo_t i;

	dpGetCurrentTransportInfo(dp, &t, &i);

	KeyStruct k;
	k.len = 0;

	switch(GetTransportType(&i)) {
	case Transport::INTERNET:
		{
			Internet internet = Internet(&i, &t, &k);
			transport = new TransportSetup(&internet);
		}
		if(dpSetGameServerEx(dp, servername, GameType) != dp_RES_OK)
			return ERR;
	break;
	case Transport::IPX:
		{
			IPX ipx = IPX(&i, &t, &k);
			transport = new TransportSetup(&ipx);
		}
	break;
	case Transport::MODEM:
		{
			Modem modem = Modem(&i, &t, &k);
			transport = new TransportSetup(&modem);
		}
	break;
	case Transport::NULLMODEM:
		{
			NullModem nullModem = NullModem(&i, &t, &k);
			transport = new TransportSetup(&nullModem);
		}
	break;
	default:
		{
			Transport trans = Transport(&i, &t, &k);
			transport = new TransportSetup(&trans);
		}
		break;
	}

	size = sizeof(dp_session_t);

	if(dpGetSessionDesc(dp, &session.session, &size) != dp_RES_OK)
		return ERR;

	session.SetKey();

	EnumPlayers(false, session.GetKey());
	EnumPlayers(true, session.GetKey());

	dp_caps_t info;
	result = dpGetCaps(dp, &info, 0);
	if(result != dp_RES_OK)
		return ERR;

#ifdef USE_SDL
	threadHandle = SDL_CreateThread(ReConnectThread, "ReConnectThread", (void *) &reconnected);
#else
	threadHandle = CreateThread(0, 0, ReConnectThread, (void *)&reconnected, 0, &threadId);
#endif

	return (threadHandle) ? OK : ERR;
}

void NETFunc::ReConnect() {
	if(!reconnected) {
		reconnected = true;
#ifdef USE_SDL
		SDL_WaitThread(threadHandle, nullptr);
#else
		DWORD dw;

		do
			GetExitCodeThread(threadHandle, &dw);
		while(dw == STILL_ACTIVE);

		CloseHandle(threadHandle);
#endif
		threadHandle = nullptr;
		dpSetActiveThread(dp);
	}
}

NETFunc::STATUS NETFunc::Reset() {

	if(!playerset)
		return ERR;
	status = READY;
	messages.clear();


	return OK;
}

NETFunc::STATUS NETFunc::Join(Game *g, char *password) {

	if(g->IsCurrentSession())
		return ERR;

	if(status == OK) {

		if(strlen(g->GetPassword()) != strlen(password) || strncmp(g->GetPassword(), password, dp_SNAMELEN)) {

			return BADPASSWORD;

		} else if(g->GetFree() && !g->IsClosed() && !g->IsHostile()) {
			Close();
			*(Game *)&gameSetup = *g;
			host = false;
			game = true;
			launch = false;
			launched = false;
			canlaunch = false;
			gotgame = false;
			gotgamepacket = false;
			session= *(Session *)g;
			nextStatus = OPENSESSION;
			return OK;
		} else
			return ERR;
	}
	return BUSSY;
}

NETFunc::STATUS NETFunc::Join(Lobby *l) {

	if(l->IsCurrentSession())
		return ERR;

	if(status == OK) {

		if(l->GetFree()) {
			Close();
			game = false;
			session = *(Session *)l;
			nextStatus = OPENSESSION;
			return OK;
		} else
			return ERR;
	}
	return BUSSY;
}

NETFunc::STATUS NETFunc::Create(GameSetup *g) {
	if(!playerset)
		return ERR;

	if(status == OK) {
		Close();
		g->SetLaunched(false);
		gameSetup = *g;
		host = true;
		game = true;
		launch = false;
		launched = false;
		canlaunch = !gameSetup.IsSyncLaunch();
		gotgame = true;
		gotgamepacket = true;
		setgame = false;
		setgamepacket = false;

		session = *(Session *)g;
		session.session.flags = dp_SESSION_FLAGS_CREATESESSION | dp_SESSION_FLAGS_ENABLE_PLAYERVARS;
		nextStatus = OPENSESSION;
		return OK;
	}
	return BUSSY;
}

NETFunc::STATUS NETFunc::Leave() {
	game = false;
	host = false;
	launch = false;
	launched = false;
	canlaunch = false;
	SetPlayerSetup(&playerSetup);
	Close();
	nextStatus = OPENLOBBY;
	return OK;
}

NETFunc::STATUS NETFunc::Disconnect() {
	if(status == WAITCLOSE)
		return ERR;
	if(status != START)
	if((status == READY && transport && transport->GetType() == Transport::INTERNET)) {
		userId = dp_UID_NONE;
		EnumServers(true);

		timer.Start(1000);
		status = WAITDELAY;
		nextStatus = PRECONNECT;
	} else if(status == PRECONNECT || status == LOGIN) {
		EnumServers(false);
		dpSetGameServerEx(dp, nullptr, GameType);
		status = QUIT;
	} else if(status == READY) {
		status = QUIT;
	} else if(status == WAITDELAY && nextStatus == PRECONNECT) {
		return BUSSY;
	} else {
		EnumSessions(false);
		Close();
		nextStatus = READY;
	}
	return OK;
}

NETFunc::STATUS NETFunc::Quit() {
	if(status == START)
		return OK;
	if(status == PRECONNECT || status == READY || status == LOGIN)
		if(transport && transport->GetType() == Transport::INTERNET) {
			dpSetGameServerEx(dp, nullptr, GameType);
			timer.Start(1500);
			status = WAITDELAY;
			nextStatus = QUIT;
		} else
			status = QUIT;
	else {
		Close();
		nextStatus = QUIT;
	}
	return OK;
}

NETFunc::STATUS NETFunc::Send(dp_t *p, Message *m, dpid_t id, dpid_t from, bool r) {
	char buf[dpio_MAXLEN_UNRELIABLE];

	if(m->GetSize() + 6 > dpio_MAXLEN_UNRELIABLE)
		return ERR;
	memcpy(buf, m->Get(), m->GetSize());
	if(from == dp_ID_BROADCAST)
		from = player.GetId();
	return dpSend(p, from, id, r ? dp_SEND_RELIABLE : 0, buf, m->GetSize()) == dp_RES_OK ? OK : ERR;
}

bool NETFunc::Handle(Message *m) {
	if(aiPlayers)
		aiPlayers->Handle(m);
	if(playerStats)
		playerStats->Handle(m);
	gameSetup.Handle(m);
	players.Handle(m);

	if(host && game) {
		gameSetup.Handle(dp, m);
		if(aiPlayers)
			aiPlayers->Handle(dp, m);
		if(playerStats)
			playerStats->Handle(dp, m);

		if(m->GetCode() == dp_OBJECTDELTA_PACKET_ID) {
			dp_objectDelta_packet_t *p = (dp_objectDelta_packet_t *)m->GetBody();
			if(p->key[0] == dp_KEY_PLAYERS) {
				Player t = Player(&p->data, (KeyStruct *)&p->subkeylen, p->flags);
				if(p->flags & dp_OBJECTDELTA_FLAG_INOPENSESS) {
					if(p->status == dp_RES_DELETED || p->status == dp_RES_CREATED) {
						gameSetup.SetGroups(players.size() + (aiPlayers ? aiPlayers->size() : 0));
						SetGameSetupSession(&gameSetup);
					}
				}
			}
		} else if(m->GetCode() == Message::ADDAIPLAYER || m->GetCode() == Message::CHGAIPLAYER || m->GetCode() == Message::DELAIPLAYER) {
			gameSetup.SetGroups(players.size() + (aiPlayers ? aiPlayers->size() : 0));
			SetGameSetupSession(&gameSetup);
		}

	}

	if(m->GetCode() == dp_ACCOUNT_PACKET_ID) {
		dp_account_packet_t *p = (dp_account_packet_t *)m->GetBody();
		userId = p->uid;
		if(p->reason == dp_RES_OK && status == LOGIN) {
			PushMessage(new Message(Message::LOGINOK));
		} else {
			PushMessage(new Message(Message::LOGINERR));
		}
		return true;
	} else if(m->GetCode() == Message::LOGINOK) {
		EnumServers(false);
		status = READY;
		return true;
	} else if(m->GetCode() == Message::PLAYERENTER) {
		dp_playerId_t *p = (dp_playerId_t *)m->GetBody();
		Message message = Message(Message::PINGBACK);
		Send(dp, &message, p->id);
		return true;
	}
	else if(m->GetCode() == Message::RESET) {
		if(((KeyStruct *)m->GetBody())->buf[0] == dp_KEY_SESSIONS)
			lobbies.Clr();
		return true;
	} else if(m->GetCode() == dp_USER_HOST_PACKET_ID) {
		host = true;
		if(!gameSetup.IsSyncLaunch())
			canlaunch = true;
		PushMessage(new Message(Message::GAMEHOST));
		return true;
	} else if(m->GetCode() == Message::UNLAUNCH) {
		launch = false;
		launched = false;
		playerSetup.SetReadyToLaunch(false);
		SetPlayerSetupPlayer(&playerSetup);
		return true;
	} else if(m->GetCode() == dp_SESSIONLOST_PACKET_ID) {
		status = CLOSE;
		nextStatus = OPENLOBBY;

		PushMessage(new Message(Message::SESSIONERR));
		return true;
	} else if(m->GetCode() == Message::GAMESESSION || m->GetCode() == Message::GAMEPACKET) {
		if(gameSetup.IsLaunched() && GotGameSetup())
			canlaunch = true;
		return true;
	} else if(m->GetCode() == Message::SETPLAYERRECORD) {
		playerSetup.Player::Set((dp_playerId_t *)m->GetBody());
		player.Set((dp_playerId_t *)m->GetBody());
		SetPlayerSetupPlayer(&playerSetup);
		return true;
	} else if(m->GetCode() == Message::SETPLAYERPACKET) {
		playerSetup.Packet::Set(m->GetBodySize(), m->GetBody());
		SetPlayerSetupPacket(&playerSetup);
		return true;
	} else if(m->GetCode() == Message::KICKED) {






		PushMessage(new Message(Message::PLAYERKICK));

		Leave();
	} else if(m->GetCode() == dp_OBJECTDELTA_PACKET_ID) {
		dp_objectDelta_packet_t *p = (dp_objectDelta_packet_t *)m->GetBody();
		if(p->key[0] == dp_KEY_SESSIONS) {
			Session s = Session(&p->data, (KeyStruct *)&p->subkeylen, p->flags);

			if(s.session.sessionType == GameType)
			if(s.IsLobby()) {
				*(Message::CODE *)(m->body) = Message::LOBBYDELTAPACKET;
				Lobby l(&p->data, (KeyStruct *)&p->subkeylen, p->flags);
				switch(p->status) {

				case dp_RES_CREATED:
					lobbies.Add(new Lobby(l));
					break;

				case dp_RES_DELETED:
					lobbies.Del(&l);
					break;

				case dp_RES_CHANGED:
					lobbies.Chg(&l);
					break;
				default:
					break;
				}
			} else
				*(Message::CODE *)(m->body) = Message::GAMEDELTAPACKET;

			if( ( s.Equals(&session) || s.IsCurrentSession() )
				&& (p->status == dp_RES_CREATED || p->status == dp_RES_CHANGED)) {
				if((session.session.flags & dp_OBJECTDELTA_FLAG_INOPENSESS) == 0) {
					p->flags |= dp_OBJECTDELTA_FLAG_INOPENSESS;
					mutes.Clr();
					players.Clr();
				}
				s.flags |= dp_OBJECTDELTA_FLAG_INOPENSESS;
				session = s;
			}
			return true;
		} else if(p->key[0] == dp_KEY_PLAYERS) {
			Player t = Player(&p->data, (KeyStruct *)&p->subkeylen, p->flags);
			if(p->flags & dp_OBJECTDELTA_FLAG_LOCAL) {
				if(p->status == dp_RES_DELETED) {











				} else if(p->status == dp_RES_CREATED || p->status == dp_RES_CHANGED) {

					player = t;
					if(status == READY) {
						status = OK;

						PushMessage(new Message(Message::ENTERGAME, session.GetKey(), sizeof(KeyStruct)));
					}
					if(p->status == dp_RES_CREATED) {
						Message message = Message(Message::PLAYERENTER, &player.player, sizeof(dp_playerId_t));
						Send(dp, &message, dp_ID_BROADCAST);
					}
				}
			}

			if(p->flags & dp_OBJECTDELTA_FLAG_INOPENSESS) {
				if(p->status == dp_RES_DELETED)
					mutes.Del(&t);

				if(host && gameSetup.IsSyncLaunch()) {

					canlaunch = players.ReadyToLaunch() && GotGameSetup() &&
						!(t.IsHost() && p->status == dp_RES_DELETED);
				}
				return true;
			}
		}
	}
	return false;
}

void NETFunc::Execute() {
	switch(status) {
	case CONNECT:
#ifdef USE_SDL
		int dw;
		// This is ugly, but there is no other way in SDL
		// to determine if a thread is running
		SDL_WaitThread(threadHandle, &dw);
#else
		DWORD dw;

		GetExitCodeThread(threadHandle, &dw);
		if(dw != STILL_ACTIVE) {
			CloseHandle(threadHandle);
#endif
			threadHandle = nullptr;
			if(dw) {
				status = START;

				PushMessage(new Message(Message::NETWORKERR));
			} else {

				dpSetActiveThread(dp);
				connected = true;
				status = nextStatus;
				if(status == PRECONNECT)
					EnumServers(true);
			}
#ifndef USE_SDL
		}
#endif
		break;
	case OK:
		if(!session.IsServer() && session.IsLobby() && session.GetPlayers() == 1) {

			Lobby *l = lobbies.FindBest();
			if(l) {

				if(!session.Equals(l)) {

					status = CLOSE;
					nextStatus = OPENSESSION;
					session.session = l->session;
				}
			}
		}
		if(game) {
			if(host) {

				if(setgame)
					SetGameSetupSession(&gameSetup);

				if(setgamepacket)
					SetGameSetupPacket(&gameSetup);
			}

			if(!launched && launch && canlaunch) {
				Launch();
			}
		}
		if(playerset) {
			if(setplayer)
				SetPlayerSetupPlayer(&playerSetup);
			if(setplayerpacket)
				SetPlayerSetupPacket(&playerSetup);
		}
		break;
	case OPENLOBBY:
		if(timeout.Finished() || retries == 0) {
			PushMessage(new Message(Message::NETWORKERR));
			status = READY;
			break;
		} else
			retries--;
		status = JOINLOBBY;

		if(transport && transport->GetType() == Transport::INTERNET) {
			dpSetGameServerEx(dp, servername, GameType);
			if(dpOpen(dp, nullptr, SessionCallBack, this) != dp_RES_OK)
				status = OPENLOBBY;
		} else {

			Lobby *l = lobbies.FindBest();
			if(l) {

				if(dpOpen(dp, &l->session, SessionCallBack, this) != dp_RES_OK) {
					l->SetBad();
					lobbies.Chg(l);
					status = OPENLOBBY;
				} else
					session.session = l->session;
			} else

				if(dpOpen(dp, &lobby.session, SessionCallBack, this) != dp_RES_OK)
					status = OPENLOBBY;
		}
		break;
	case OPENSESSION:
		status = JOINSESSION;
		if(dpOpen(dp, &session.session, SessionCallBack, this) != dp_RES_OK) {
			status = OPENLOBBY;

			PushMessage(new Message(Message::SESSIONERR));
		}
		break;
	case CREATEPLAYER:

		setplayerpacket = true;
		setplayer = true;
		if(dpCreatePlayer(dp, PlayerCallBack, this, playerSetup.GetName()) != dp_RES_OK) {
			status = CLOSE;
			nextStatus = OPENLOBBY;

			PushMessage(new Message(Message::SESSIONERR));
		} else if(status == CREATEPLAYER)
			status = BUSSY;
		break;
	case CLOSE:
		dpClose(dp);
		timer.Start(8000);
		status = WAITCLOSE;
	case WAITCLOSE:
		if(dpReadyToFreeze(dp) == dp_RES_OK || timer.Finished()) {
			if(nextStatus != READY && nextStatus != OPENLOBBY && nextStatus != OPENSESSION && transport && transport->GetType() == Transport::INTERNET)
				dpSetGameServerEx(dp, nullptr, GameType);
			timer.Start(2000);
			status = WAITDELAY;
		}
		break;
	case WAITDELAY:
		if(timer.Finished()) {
			status = nextStatus;
			if(status == OPENLOBBY) {
				lobbies.Reset();
				timeout.Start(60000);
				retries = 6;
			}
		}
		if(status != START)
			break;
	case QUIT:
		dpDestroy(dp, 1);
		connected = false;
		status = START;
		break;
	}
}

NETFUNC_CALLBACK_RESULT(int)
NETFunc::SessionCallBack(dp_session_t *s, long *pTimeout, long flags, void *context) {

	if(s) {

		NETFunc::session.session = *s;
		NETFunc::session.SetKey();
		NETFunc::session.flags = 0;

		strlcpy(sessionname, s->sessionName, sizeof(sessionname));
		EnumSessions(false);
		if(NETFunc::session.session.flags & dp_SESSION_FLAGS_ISLOBBY)
			EnumSessions(true);
		dpSetPingIntervals( ((NETFunc *)context)->GetDP(), 5000, 5000 );
		NETFunc::status = CREATEPLAYER;
	} else {
		if((NETFunc::session.session.flags & dp_SESSION_FLAGS_ISLOBBY)
			&& NETFunc::transport
			&& NETFunc::transport->GetType() != Transport::INTERNET) {

			Lobby l;
			session.session = l.session;
			l.SetBad();
			((NETFunc *)context)->lobbies.Chg(&l);
		}
		NETFunc::status = OPENLOBBY;

		PushMessage(new Message(Message::SESSIONERR));
	}
	return 0;
}

NETFUNC_CALLBACK_RESULT(void)
NETFunc::PlayerCallBack(dpid_t id, dp_char_t *n, long flags, void *context) {

	if(n) {

		strlcpy(NETFunc::player.player.name, n, sizeof(NETFunc::player.player.name));
		NETFunc::player.player.id = id;
		NETFunc::player.SetKey();
		NETFunc::status = OK;

		strlcpy(playername, n, sizeof(playername));

		if(NETFunc::session.IsLobby())
			PushMessage(new Message(Message::ENTERLOBBY, NETFunc::session.GetKey(), sizeof(KeyStruct)));
		else
			PushMessage(new Message(Message::ENTERGAME, NETFunc::session.GetKey(), sizeof(KeyStruct)));
	} else {
		NETFunc::status = CLOSE;
		((NETFunc *)context)->nextStatus = OPENLOBBY;

		PushMessage(new Message(Message::SESSIONERR));
	}
}

void NETFunc::CancelDial() {
	cancelDial = 1;
}

NETFunc::STATUS NETFunc::Kick(Player *p) {
	if(status == OK && p->IsInCurrentSession() && !p->IsMe())
		if(player.IsHost()){
			Message message = Message(Message::KICKED);
			return Send(dp, &message, p->GetId(), player.GetId());
		}

		else if(player.IsGroupMaster() && player.GetGroup() == p->GetGroup())
		{
			Message message = Message(Message::KICKED);
			return Send(dp, &message, p->GetId(), player.GetId());
		}
	return ERR;
}

NETFunc::STATUS NETFunc::Close() {




		session = Session();
		setplayer = true;
		setplayerpacket = true;


		mutes.Clr();
		players.Clr();
		status = CLOSE;
		return OK;

}































bool NETFunc::HandleMessage(Message *m) {
	return MessageHandler::HandleAll(m);
}

void NETFunc::Receive() {

	size = 0;
	source = dp_ID_NONE;
	destination = dp_ID_NONE;
	Message *m;
	if(connected) do {

		size = dpio_MAXLEN_UNRELIABLE;
		result = dpReceive(dp, &source, &destination, 0, buffer, &size);
		switch (result) {
		case dp_RES_EMPTY:
		break;
		case dp_RES_OK:
			m = nullptr;
			m = new Message(buffer, size, source);
			if(m && ((char)m->GetCode() == dp_PACKET_INITIALBYTE || (char)m->GetCode() == nf_PACKET_INITIALBYTE || (char)m->GetCode() == ns_PACKET_INITIALBYTE) || m->GetCode() == Message::CHAT)
				PushMessage(m);
		break;

		case dp_RES_BUG:
		case dp_RES_BAD:
			;
			break;
		case dp_RES_MODEM_NORESPONSE:
		case dp_RES_NETWORK_NOT_RESPONDING:
			;

		case dp_RES_HOST_NOT_RESPONDING:
			;
			PushMessage(new Message(Message::NETWORKERR));
			dpDestroy(dp, 1);
			connected = false;
			status = START;
			break;
			default:
			break;
		}
	} while(result == dp_RES_OK);
}

NETFunc::Message *NETFunc::GetMessage() {

	Execute();

	Receive();

	Message *m = nullptr;

	if(!messages.empty()) {
		m = messages.Pop();
		Handle(m);
	}
	return m;
}

void NETFunc::PushChatMessage(char *m) {
	size_t size = strlen(m) + 1;
	char buffer[dpio_MAXLEN_UNRELIABLE];
	*(Chat::TYPE *)buffer = Chat::SYSTEM;
	memcpy(buffer + sizeof(Chat::TYPE), m , size);
	size += sizeof(Chat::TYPE);
	PushMessage(new Message(Message::CHAT, buffer, size));
}

void NETFunc::PushMessage(Message *m) {
	messages.Push(m);
}

dp_t *NETFunc::dp = nullptr;

dp_result_t	NETFunc::result = 0;

char NETFunc::buffer[dpio_MAXLEN_UNRELIABLE] = "";

size_t NETFunc::size = 0;

dpid_t NETFunc::source = 0;

NETFunc::STATUS NETFunc::status = RESET;

dpid_t NETFunc::destination = 0;

dp_species_t NETFunc::GameType = 0;

char *NETFunc::LobbyName = "Lobby";

char *NETFunc::DllPath = "anet";

int NETFunc::PlayerSetupPacketKey = 0;

long NETFunc::cancelDial = 0;

bool NETFunc::connected = false;

bool NETFunc::reconnected = true;

NETFunc::PlayerSetup NETFunc::playerSetup = NETFunc::PlayerSetup();

bool NETFunc::playerset = false;

bool NETFunc::setplayerpacket = false;

bool NETFunc::setplayer = false;

NETFunc::GameSetup NETFunc::gameSetup = NETFunc::GameSetup();

bool NETFunc::game = false;

bool NETFunc::host = false;

bool NETFunc::launch = false;

bool NETFunc::launched = false;

bool NETFunc::canlaunch = false;

bool NETFunc::setgamepacket = false;

bool NETFunc::setgame = false;

bool NETFunc::gotgamepacket = false;

bool NETFunc::gotgame = false;

char NETFunc::servername[64] = "";
char NETFunc::playername[dp_PNAMELEN] = "";
char NETFunc::sessionname[dp_SNAMELEN] = "";
