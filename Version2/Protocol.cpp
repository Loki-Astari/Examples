
#include "Protocol.h"

using namespace ThorsAnvil::Socket;

Protocol::Protocol(DataSocket& socket)
    : socket(socket)
{}

Protocol::~Protocol()
{}

