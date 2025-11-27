#include "../../program.h"

typedef struct
{
    uint16_t id;
    uint16_t flags;
    uint16_t qdCount;
    uint16_t anCount;
    uint16_t nsCount;
    uint16_t arCount;
} __attribute__((packed)) DnsLayer;
Connection* connection = NULL;
uint16_t size = 0;
BOOLEAN got = FALSE;
IpInfo info = {};

void _start()
{
    getIpInfo(&info);
    connection = createConnection(49152);
    char* name = "pool.ntp.org";
    uint16_t length = strlena(name);
    size = sizeof(DnsLayer) + 6 + length;
    Packet* send = addItem(&connection->send, sizeof(Packet) + size);
    for (uint8_t i = 0; i < 4; i++)
    {
        send->ip[i] = info.dns[i];
    }
    send->port = 53;
    send->length = size;
    DnsLayer* dns = (DnsLayer*)send->data;
    dns->id = 0x1234;
    dns->flags = 0x0001;
    dns->qdCount = 0x0100;
    dns->anCount = 0;
    dns->nsCount = 0;
    dns->arCount = 0;
    uint8_t* info = (uint8_t*)((uint64_t)send->data + sizeof(DnsLayer));
    uint8_t* number = info;
    *number = 0;
    info++;
    while (*name)
    {
        if (*name != '.')
        {
            *info = *name;
            *number += 1;
        }
        else
        {
            number = info;
            *number = 0;
        }
        info++;
        name++;
    }
    *info = 0;
    info++;
    *(uint32_t*)info = 0x01000100;
}

void update(uint64_t ticks)
{
    Packet* packet = (Packet*)&connection->received;
    while (iterateList(&packet))
    {
        if (!got)
        {
            got = TRUE;
            uint8_t* ip = (uint8_t*)((uint64_t)packet->data + size + 12);
            Packet* send = addItem(&connection->send, sizeof(Packet) + 48);
            for (uint8_t i = 0; i < 4; i++)
            {
                send->ip[i] = ip[i];
            }
            send->port = 123;
            send->length = 48;
            for (uint8_t i = 0; i < 48; i++)
            {
                send->data[i] = 0;
            }
            send->data[0] = 0x23;
            unallocateList(&connection->received);
            connection->received = NULL;
        }
        else
        {
            CHAR16 characters[10];
            uint32_t time = *(uint32_t*)(packet->data + 40);
            ValueToString(characters, FALSE, (((time >> 24) & 0xFF) | ((time >> 8)  & 0xFF00) | ((time << 8)  & 0xFF0000) | ((time << 24) & 0xFF000000)) % 60);
            print(characters);
            print(L"\n");
            closeConnection(connection);
            quit();
        }
        break;
    }
}
