1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

The client uses a special end-of-message marker (ASCII 0x04 in this assignment) appended by the server to signal the end of the command’s output. Because TCP is a stream protocol, the client may receive data in chunks, so it keeps calling recv() until that marker appears. This prevents partial reads from cutting off the command’s output, ensuring the entire message arrives before the client treats the command as complete.

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

A shell protocol must specify explicit boundaries—such as null-termination for commands sent to the server and an EOF marker for responses—so each side knows exactly where messages start and finish. Without these boundaries, incoming data can blur together (e.g., parts of one command mixing with another), causing malformed parsing or missed commands.

3. Describe the general differences between stateful and stateless protocols.

A stateful protocol maintains session data across multiple requests, so later interactions depend on earlier exchanges (like an authenticated session). A stateless protocol treats each request as independent, lacking stored context on the server side. This means each request must carry all necessary information, simplifying server design but requiring more data in each message.

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

UDP’s lack of built-in reliability—no guaranteed delivery, ordering, or retransmission—makes it lightweight and fast. Applications that can tolerate or handle occasional packet loss (e.g., real-time streaming, online gaming) benefit from lower latency and overhead, so UDP is an optimal choice when speed and simplicity outweigh strict reliability.

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

The operating system exposes sockets as the key abstraction for network I/O. With sockets, applications can create endpoints, bind to addresses, connect or accept connections, and read/write data, while the OS manages the underlying details of IP, TCP, or UDP.