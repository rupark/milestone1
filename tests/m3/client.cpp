#pragma once
#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include "../../src/network/network.h"
using namespace std;
//Client side
int main(int argc, char *argv[])
{
    NetworkIP* client = new NetworkIP();
    client->client_init(1, 8080, "127.0.0.4", 8080, "127.0.0.5");
    String* exp_s0 = new String("127.0.0.1");
    String* exp_c1 = new String("127.0.0.2");
    String* exp_c2 = new String("127.0.0.3");

    String* s0 = new String(inet_ntoa(client->nodes_[0].address.sin_addr));
    String* c1 = new String(inet_ntoa(client->nodes_[1].address.sin_addr));
    String* c2 = new String(inet_ntoa(client->nodes_[2].address.sin_addr));

/**	
    assert(s0->equals(exp_s0));
    cout << "Client: ASSERT PASSED nodes_[0]: " << s0->c_str() << " = " << exp_s0->c_str() << endl;
    assert(c1->equals(exp_c1));
    cout << "Client: ASSERT PASSED nodes_[1]: " << c1->c_str() << " = " << exp_c1->c_str() << endl;
    assert(c2->equals(exp_c2));
    cout << "Client: ASSERT PASSED nodes_[2]: " << c2->c_str() << " = " << exp_c2->c_str() << endl;
**/
    cout << "Client DONE" << endl;

    delete exp_s0;
    delete exp_c1;
    delete exp_c2;
    delete s0;
    delete c1;
    delete c2;
}
