#include "RemoteInstallPage.hpp"
#include "../input.hpp"
#include "../ViewFunctions.hpp"
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

RemoteInstallPage::~RemoteInstallPage()
{
	StopSocketing();
}

RemoteInstallPage::RemoteInstallPage() : 
BtnStart("Start remote install"),
lblInfo("You can install a theme directly from your pc with the theme injector, go to the 'Nxtheme builder' tab and click on 'Remote install...'", WHITE, 890, font30),
lblConfirm("Press A to install, B to cancel.",WHITE, 890, font30)
{
	Name = "Remote Install";
	BtnStart.selected = false;
}

void RemoteInstallPage::Render(int X, int Y)
{
	if (entry)
	{
		entry->Render(X + 10, Y + 10,true);
		lblConfirm.Render(X + 10, Y + 100);
	}
	else 
	{
		lblInfo.Render(X + 10, Y + 20);
		BtnStart.Render(X + 10, Y + 20 + lblInfo.GetSize().h + 10);
	}
}

void RemoteInstallPage::StartSocketing()
{
	if (sock != -1)
		return;
	
	int err;
	struct sockaddr_in temp;
	
	sock=socket(AF_INET,SOCK_STREAM,0);
	if (sock < 0)
	{
		Dialog("Couldn't start socketing (socket error)");
		sock = -1;
		return;
	}
	temp.sin_family=AF_INET;
	temp.sin_addr.s_addr=INADDR_ANY;
	temp.sin_port=htons(5000);
	
	err=fcntl(sock,F_SETFL,O_NONBLOCK);
	if (err)
	{
		Dialog("Couldn't start socketing (fcntl error)");
		StopSocketing();
		return;
	}
	
	err=bind(sock,(struct sockaddr*) &temp,sizeof(temp));
	if (err)
	{
		Dialog("Couldn't start socketing (bind error)");
		StopSocketing();
		return;
	}
	
	err=listen(sock,1);
	if (err)
	{
		Dialog("Couldn't start socketing (listen error)");
		StopSocketing();
		return;
	}
	
	char hostname[128];
	err = gethostname(hostname, sizeof(hostname));
	if(err != 0)
	{
		Dialog("Couldn't start socketing (gethostname error)");
		StopSocketing();
		return;
	}
	
	BtnStart.SetString("IP: " + string(hostname) + " - Press to stop");
}

void RemoteInstallPage::StopSocketing()
{
	if (sock == -1) 
		return;
	close(sock);
	sock = -1;
	BtnStart.SetString("Start remote install");
}

void RemoteInstallPage::SocketUpdate()
{	
	if (sock < 0) 
	{
		return;
	}	
	
	u8 buf[12]; 
	int curSock = -1;
	if ((curSock=accept(sock,0,0))!=-1)
	{
		int size = -1;
		if ((size=recv(curSock,buf,sizeof(buf),0)) <0)
		{
			Dialog("Couldn't read any data.");
			close(curSock);
			StopSocketing();
			return;
		}
		else
		{			
			if (strncmp(buf, "theme", 5) != 0)
			{
				Dialog("Unexpected data received.");
				close(curSock);
				StopSocketing();
				return;
			}
			int *ThemeSize = reinterpret_cast<int*>(buf + 8);
			if (*ThemeSize < 50 || *ThemeSize > 2000000)
			{
				Dialog("Invalid size (" + to_string(*ThemeSize) + ")");
				close(curSock);
				StopSocketing();
				return;				
			}
			vector<u8> data;
			DisplayLoading("Loading...");
			data.reserve(*ThemeSize);
			u8 tmp[100];
			while ((size = recv(curSock,tmp,100,0)) > 0)
			{
				for (int i = 0; i < size; i++)
					data.push_back(tmp[i]);
			}
			if (data.size() != *ThemeSize)
				Dialog("Unexpected data count. (" + to_string(size) + ")");
			else
			{
				write(curSock,"ok",2);
				entry = new ThemeEntry(data);
			}
			close(curSock);
			StopSocketing();
			return;
		}
		close(curSock);
	}
}

void RemoteInstallPage::Update()
{
	if (entry)
	{
		if (kDown & KEY_A)
		{
			string overrideStr = "";
			if (kHeld & KEY_R)
				overrideStr = MakeThemeShuffleDir();
			entry->InstallTheme(true,overrideStr);
			delete entry;
			entry = 0;
			return;
		}
		else if (kDown & KEY_B)
		{
			delete entry;
			entry = 0;			
			return;			
		}
	}
	
	if (kDown & KEY_B || kDown & KEY_LEFT){
		BtnStart.selected = false;
		Parent->PageLeaveFocus(this);
		return;
	}
	
	BtnStart.selected = true;
	
	if (sock >= 0)
	{
		SocketUpdate();
		if (kDown & KEY_A)
		{
			StopSocketing();
			return;
		}
	}
	else 
	{
		if (kDown & KEY_A)
		{
			StartSocketing();
			return;
		}
	}	
}