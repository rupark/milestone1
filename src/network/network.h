//
// Created by kramt on 3/25/2020.
//

#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "serial.h"
#include "../string.h"

/**
 * NodeInfo: each node is identified
 * by its node id (also called index) and its socket address.
 */
class NodeInfo : public Object {
public:
    unsigned id;
    sockaddr_in address;
};


/**
 * IP based network communications layer. Each node has an index
 * between 0 and num_nodes-1. nodes directory is ordered by node
 * index. Each node has a socket and ip address.
*/
class NetworkIP {
public:
    NodeInfo* nodes_;
    size_t this_node_;
    int sock_;
    sockaddr_in ip_;

    ~NetworkIP() {
        close(sock_);
    }

    NetworkIP() {

    }

    /**
     * Returns this node's index.
     */
    size_t index() {
        return this_node_;
    }

    /**
     * Initialize node 0.
     */
   void server_init(unsigned idx, unsigned port) {
       this_node_ = idx;
       assert(idx==0 && "Server must be 0");
       init_sock_(port);
       nodes_ = new NodeInfo[2];

       for (size_t i =0; i < 2; i++) {
           Register* msg = dynamic_cast<Register*>(recv_m());
           nodes_[msg->sender_].id = msg->sender_;
           nodes_[msg->sender_].address.sin_family = AF_INET;
           nodes_[msg->sender_].address.sin_addr = msg->client.sin_addr;
           nodes_[msg->sender_].address.sin_port = htons(msg->port);
       }
       size_t* ports = new size_t[2-1];
       String** addresses = new String*[2 - 1];
       for (size_t i = 0; i < 2 = 1; i++) {
           ports[i] = ntohs(nodes_[i + 1].address.sin_port);
           addresses[i] = new String(inet_ntoa(nodes_[i + 1].address.sin_addr));
       }

       Directory ipd(ports, addresses);
       //ipd.log();
       for (size_t i = 1; i < 2; i++) {
           ipd.target_ = i;
           send_m(&ipd);
       }
   }

   /** Intializes a client node */
   void client_init(unsigned idx, unsigned port, char* server_adr,
           unsigned server_port) {
       this_node_ = idx;
       init_sock_(port);
       nodes_ = new NodeInfo[1];
       nodes_[0].id = 0;
       nodes_[0].address.sin_family = AF_INET;
       nodes_[0].address.sin_port = htons(server_port);
       if (inet_pton(AF_INET, server_adr, &nodes_[0].address.sin_addr) <= 0) {
           assert(false && "Imvalid server IP address format");
       }
       Register msg(idx, port);
       send_m(&msg);
       Directory* ipd = dynamic_cast<Directory*>(recv_m());
       NodeInfo* nodes = new NodeInfo[2];
       nodes[0] = nodes_[0];
       for (size_t i = 0; i < ipd->clients; i++) {
           nodes[i+1].id = i+1;
           nodes[i+1].address.sin_family = AF_INET;
           nodes[i+1].address.sin_port = htons(ipd->ports[i]);
           if (inet_pton(AF_INET, ipd->addresses[i]->c_str(),
                   &nodes[i+1].address.sin_addr) <= 0) {
               FATAL_ERROR("Invalid IP direcotry-addr. for node " << (i+1));
           }
       }
       delete[] nodes_;
       nodes_ = nodes;
       delete ipd;
   }

   /** Create a socket and bind it. */
   void init_sock_(unsigned port) {
       assert((sock_ = socket(AF_INET, SOCK_STREAM, 0)) >= 0);
       int opt = 1;
       assert(setsockopt(sock_,
               SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt)) == 0);
       ip_.sin_family = AF_INET;
       ip_.sin_addr.s_addr = INADDR_ANY;
       ip_.sin_port = htons(port);
       assert(bind(sock_, (sockaddr*) &ip, sizeof(ip_)) >= 0);
       assert(listen(sock_, 100) >= 0);
   }

   /** Based on the message target, creates new connection to the appropriate
    * server and then serializes the message on the connection fd. **/
   void send_m(Message* msg) {
       NodeInfo & tgt = nodes_[msg->target()];
       int conn = socket(AF_INET, SOCK_STREAM, 0);
       assert(conn >= 0 && "Unable to create client socket");
       if (connect(conn, (sockaddr*)&tgt.address, sizeof(tgt.address)) < 0) {
           FATAL_ERROR("Unable to connect to remote node");
       }
       String* msg_ser =  msg->serialize();
       char* buf = msg_ser->c_str();
       size_t size = msg_ser->size();
       send(conn, &size, sizeof(size_t), 0);
       send(conn, buf, size, 0);
   }

   /** Listens on the server socket. When a message becomes available, reads
    * its data, deserialize it and return object. */
   Message* recv_m() {
       sockaddr_in sender;
       socklen_t addrlen = sizeof(sender);
       int req = accept(sock_, (sockaddr*) &sender, &addrlen);
       size_t size = 0;
       if (read(req, &size, sizeof(size_t))  == 0) {
           FATAL_ERROR("failed to read");
       }
       char* buf = new char[size];
       int rd = 0;
       while(rd != size) {
           rd+= read(req, buf + rd, size - rd);
       }
       Message* msg;

       // SWITCH STATEMENT
       switch(buf[0]) {
           case 1: // Register
               msg = new Register(buf);
               break;
           case 2: // ACK
               msg = new Ack(buf);
               break;
           case 3: // Status
               msg = new Status(buf);
               break;
           case 4: // Directory
               msg = new Directory(buf);
               break;
       }

       return msg;
   }

};