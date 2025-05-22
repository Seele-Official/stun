## Introduce
A tool designed to assist in Network Address Translation (NAT) traversal by enabling clients to discover their NAT type, determine NAT binding lifetime, and facilitate peer-to-peer (P2P) connections.

## Usage

### Client

The client application allows users to discover their public IP address, NAT type, and binding lifetime, etc.

**Example:**
```
./stun-client 195.208.107.138:3478 -b 11451 -t 
```
Options:
- `<server_addr>`: Specify the STUN server `195.208.107.138:3478` to connect.
- `-b <bind_port>`: Build a binding by port `11451`.
- `-t`: Identify the NAT type your client is behind to understand its behavior in network communications.

## NAT discover
![](NAT%20discover.png)



## rfc
- [RFC5389](./doc/RFC_5389.md)
- [RFC5780](./doc/RFC_5780.md)
- [RFC8489](./doc/RFC_8489.md)