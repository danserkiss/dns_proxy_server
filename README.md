DNS Proxy Server

This project implements a simple DNS proxy server in C with a domain blacklist feature to filter unwanted hostname resolutions. The server can forward DNS queries to an upstream DNS server or respond to blacklisted domains according to its configuration.
Table of Contents

    Requirements
    Building
    Configuration
    Running
    Setting up Localhost as DNS Server
    Testing
    Known Limitations

Requirements

    A C compiler (e.g., GCC).
    A POSIX-compliant operating system (Linux, macOS).

Building
    Clone the repository (or extract the archive) and navigate into the project directory:
    
    git clone https://github.com/danserkiss/dns_proxy_server
    cd dns_proxy_server

Run make:

    make

This will compile the source code and create an executable file named dns_proxy in the current directory.

Cleaning the project:
To remove all compiled object files (.o) and the executable, run:

    make clean

Configuration

    The server reads its parameters from a config.txt file, which must be located in the same directory as the dns_proxy executable.

Example config.txt file:

    upstream_dns_ip=8.8.8.8
    blacklist=doubleclick.net,tracking.com,ads.example.com
    response=nxdomain

Configuration Parameters:

    upstream_dns_ip=<IP_ADDRESS>: The IP address of the upstream DNS server to which the proxy will forward non-blacklisted queries.
        Example: 8.8.8.8 (Google Public DNS)
    blacklist=<DOMAIN1,DOMAIN2,...>: A comma-separated list of domain names to be blocked.
        Example: doubleclick.net,tracking.com
    response=<RESPONSE_TYPE>: The type of DNS response the proxy server will send for blacklisted domains.
        Possible values:
            noerror (RCODE 0): Query successfully processed, but no data (rarely used for blocking).
            formerror (RCODE 1): Format error in the query.
            servfail (RCODE 2): Server temporarily unavailable or unable to process the query.
            nxdomain (RCODE 3): Non-existent domain name (the most common choice for blocking).
            notimp (RCODE 4): Query type not implemented.
            refused (RCODE 5): Server refused to fulfill the query.

Running

After building and configuring config.txt, you can start the server:

    sudo ./dns_proxy

The server will listen on UDP port 53 at 127.0.0.1.

To test the server, you'll need to configure your system or DNS client (e.g., dig) to use 127.0.0.1 as its DNS server. See the Setting up Localhost as DNS Server section for instructions.
dig Usage Examples:
    Query for a non-blacklisted domain:
    
    dig @127.0.0.1 google.com

You should receive a response forwarded from the upstream DNS server.

Query for a blacklisted domain:
If tracking.com is in your blacklist and response=nxdomain:

    dig @127.0.0.1 tracking.com

You should receive an NXDOMAIN response. If response=refused, you'll see REFUSED.

Setting up Localhost as DNS Server

To test the DNS proxy server, you need to configure your operating system or DNS client to use 127.0.0.1 (localhost) as its primary DNS server.

Important: Remember to revert these changes after you are done testing to restore your normal internet connectivity.
For Linux (Ubuntu/Debian-based systems using systemd-resolved):

Many modern Linux distributions use systemd-resolved for DNS resolution.
    Backup the original resolv.conf:

    sudo cp /etc/resolv.conf /etc/resolv.conf.backup

Edit resolv.conf:
Open the resolv.conf file with a text editor. Note that resolv.conf is often managed by systemd-resolved and might be a symlink. We'll directly create a new one.

    sudo nano /etc/resolv.conf

Add 127.0.0.1 as the nameserver:
Delete existing content and add the following lines:
    
    nameserver 127.0.0.1
    options single-request-reopen

Save and exit the editor (Ctrl+O, Enter, Ctrl+X for nano).

Make resolv.conf immutable (optional, but recommended for testing):
This prevents systemd-resolved or other services from overwriting your changes while you're testing.

    sudo chattr +i /etc/resolv.conf

Test your DNS resolution:
After starting your DNS proxy server:

    ping google.com

If successful, your system is using your proxy.

Revert changes after testing:

    sudo chattr -i /etc/resolv.conf  # Make it mutable again
    sudo rm /etc/resolv.conf        # Remove the modified file
    sudo cp /etc/resolv.conf.backup /etc/resolv.conf # Restore original
    sudo systemctl restart systemd-resolved.service # Restart resolved

Or, simply reboot your system for systemd-resolved to regenerate it.

Testing

Testing was performed using the dig utility on a Linux system.
Test Scenarios:

    Successful Query Forwarding (non-blacklisted domain):
        Action: dig @127.0.0.1 example.com (where example.com is not in the blacklist).
        Expected Result: The server forwards the query to upstream_dns_ip and returns the correct response to the client.
    Domain Blocking with NXDOMAIN Response:
        Configuration: blacklist=blocked.test,example.com, response=nxdomain.
        Action: dig @127.0.0.1 blocked.test.
        Expected Result: The server immediately responds to the client with NXDOMAIN (RCODE 3).
    Domain Blocking with REFUSED Response:
        Configuration: blacklist=blocked.test, response=refused.
        Action: dig @127.0.0.1 blocked.test.
        Expected Result: The server immediately responds to the client with REFUSED (RCODE 5).
    Handling Non-existent Domains (via upstream):
        Action: dig @127.0.0.1 non-existent-domain-12345.com (a domain guaranteed not to exist and not in the blacklist).
        Expected Result: The server forwards the query to upstream_dns_ip and returns the NXDOMAIN (RCODE 3) received from the upstream.
    Timeout or Upstream DNS Server Unavailability:
        Configuration: Set upstream_dns_ip to a non-existent or blocked IP address (e.g., 192.0.2.1).
        Action: dig @127.0.0.1 anydomain.com.
        Expected Result: The server waits for 2 seconds (due to SO_RCVTIMEO) and returns a SERVFAIL (RCODE 2) response to the client.
    Memory Allocation Failure during DNS Name Parsing:
        Action: (Difficult to simulate directly, but logic is handled) Simulating a scenario where malloc in Read_Name returns NULL.
        Expected Result: The server returns a SERVFAIL (RCODE 2) to the client and continues operation.

Known Limitations

    IPv4 Only (A Records): The proxy server is primarily designed to handle queries for IPv4 addresses (QTYPE A). It does not explicitly support queries for IPv6 (AAAA), MX, NS, and other DNS record types. Queries for other types will be forwarded upstream, but result processing might not be optimal.
    Single-threaded: The server processes requests sequentially (one at a time). It is not optimized for high loads and does not support parallel query processing.
    No Caching: The server does not cache DNS responses. Every non-blacklisted query will be forwarded to the upstream_dns_ip, potentially increasing latency and upstream load.
    Basic Compression Handling: While parsing of compressed names is implemented, responses for blocked domains do not utilize compression for names in the Authority or Additional sections (though this was not a requirement for this task).
    No EDNS0: DNS extensions (EDNS0) are not supported.
    No DNSSEC/DNS over TLS/HTTPS: No security or encryption features are implemented.

