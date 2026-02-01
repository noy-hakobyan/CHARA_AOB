/*
 * Copyright (c) 2020 Teknic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
#ifndef __ETHERNETTCP_H__
#define __ETHERNETTCP_H__
 
#include "lwip/tcp.h"
#include <string.h>
#ifndef HIDE_FROM_DOXYGEN
namespace ClearCore {
 
#define CLIENT_MAX 8
#define TCP_DATA_BUFFER_SIZE 600
 
class EthernetTcp {
 
public:
    typedef struct {
        struct tcp_pcb *pcb;    
        uint16_t dataHead;      
        uint16_t dataTail;      
        tcp_state state;        
        uint8_t data[TCP_DATA_BUFFER_SIZE]; 
    } TcpData;
 
    EthernetTcp() : m_tcpData(nullptr) {};
 
    EthernetTcp(TcpData *tcpData);
 
    uint32_t Send(uint8_t charToSend);
 
    uint32_t Send(const char *nullTermStr) {
        return Send((const uint8_t *)nullTermStr, strlen(nullTermStr));
    }
 
    virtual uint32_t Send(const uint8_t *buff, uint32_t size) = 0;
 
    uint16_t LocalPort();
 
    volatile const TcpData *ConnectionState() {
        return m_tcpData;
    }
 
protected:
    // The TCP state.
    TcpData *m_tcpData;
 
};  // EthernetTcp
 
#ifndef HIDE_FROM_DOXYGEN
err_t TcpAccept(void *arg, struct tcp_pcb *newpcb, err_t err);
 
err_t TcpConnect(void *arg, struct tcp_pcb *tpcb, err_t err);
 
void TcpError(void *arg, err_t err);
 
err_t TcpReceive(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
                 err_t err);
 
err_t TcpSend(void *arg, struct tcp_pcb *tpcb, u16_t len);
 
void TcpClose(struct tcp_pcb *pcb, EthernetTcp::TcpData *data);
#endif // !HIDE_FROM_DOXYGEN
 
} // ClearCore namespace
#endif // !HIDE_FROM_DOXYGEN
#endif // !__ETHERNETTCP_H__
 