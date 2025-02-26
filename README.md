## Introduce
A tool designed to assist in Network Address Translation (NAT) traversal by enabling clients to discover their NAT type, determine NAT binding lifetime, and facilitate peer-to-peer (P2P) connections.

## Usage

### Client

The client application allows users to discover their public IP address, NAT type, and binding lifetime, etc.

**Example:**
```
./stun-client -b -t <server_addr>
```
Options:
- `-b`: Build a binding.
- `-t`: Identify the NAT type your client is behind to understand its behavior in network communications.
- `<server_addr>`: Specify the STUN server to connect to, for example `195.208.107.138:3478`.
## NAT discover
![](NAT%20discover.png)



## rfc
- [RFC5389](RFC_5389.md)
- [RFC5780](RFC_5780.md)