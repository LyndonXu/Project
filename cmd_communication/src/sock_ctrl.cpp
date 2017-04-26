/*
 * sock_ctrl.cpp
 *
 *  Created on: 2017年4月26日
 *      Author: lyndon
 */

#include "cmd_communication.h"
#include <list>

using namespace std;


class CSockCtrl
{
private:
	pthread_mutex_t m_stMutex;
	bool m_boHasInit;

	void Destory();
public:
	list<int32_t> m_csSockList;

	CSockCtrl();
	~CSockCtrl()
	{
		Destory();
	};
};

CSockCtrl::CSockCtrl()
	: m_boHasInit(false)
{
	memset(&m_stMutex, 0, sizeof(pthread_mutex_t));
}

void CSockCtrl::Destory()
{
	if (!m_boHasInit)
	{
		return;
	}

	pthread_mutex_lock(&m_stMutex);

	for (list<int32_t>::iterator iter = m_csSockList.begin(); iter != m_csSockList.end(); iter++)
	{
		int32_t s32Socket = *(iter);
		if (s32Socket >= 0)
		{
			close(s32Socket);
		}
	}

	pthread_mutex_unlock(&m_stMutex);

	pthread_mutex_destroy(&m_stMutex);

}
